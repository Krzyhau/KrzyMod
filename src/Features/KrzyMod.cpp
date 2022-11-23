#include "KrzyMod.hpp"

#include "Features/EntityList.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Scheme.hpp"
#include "Modules/Server.hpp"
#include "Modules/Client.hpp"
#include "Modules/Surface.hpp"
#include "Modules/VScript.hpp"
#include "Modules/Console.hpp"
#include "Event.hpp"

#include <random>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <vector>
#include <set>


KrzyMod krzyMod;

Variable krzymod_enabled("krzymod_enabled", "0", "Enables KrzyMod (TM).\n");
Variable krzymod_time_base("krzymod_time_base", "30", 5.0f, 3600.0f, "The base time for all timers in KrzyMod.\n");
Variable krzymod_timer_multiplier("krzymod_timer_multiplier", "1", 0, "Multiplier for the main KrzyMod timer.\n");
Variable krzymod_primary_font("krzymod_primary_font", "92", 0, "Change font of KrzyMod.\n");
Variable krzymod_secondary_font("krzymod_secondary_font", "97", 0, "Change font of KrzyMod.\n");
Variable krzymod_debug("krzymod_debug", "0", "Debugs KrzyMod.\n");
Variable krzymod_vote_enabled("krzymod_vote_enabled", "0", 0, 2, "Enables Twitch chat voting for KrzyMod effects.\n");
Variable krzymod_vote_double_numbering("krzymod_vote_double_numbering", "0", "Uses different numbers for every voting in KrzyMod\n");
Variable krzymod_vote_channel("krzymod_vote_channel", "krzyhau", "Sets a twitch channel from which votes should be read.\n", 0);
Variable krzymod_vote_proportional("krzymod_vote_proportional", "1", 0, 1, "Should KrzyMod use proportional voting? (x% means effect has x% to be activated).\n", 0);


KrzyMod::KrzyMod()
	: Hud(HudType_InGame | HudType_Paused, true) {
}
KrzyMod::~KrzyMod() {
	// Making sure the commands are reset to their original values before the plugin is disabled
	Stop();
}
bool KrzyMod::ShouldDraw() {
	return krzymod_enabled.GetBool() && Hud::ShouldDraw() && sv_cheats.GetBool();
}
bool KrzyMod::GetCurrentSize(int &w, int &h) {
	return false;
}


bool KrzyMod::IsEnabled() {
	return krzymod_enabled.GetBool() && sv_cheats.GetBool() && engine->isRunning() && !engine->IsGamePaused();
}

// literally every single bit of logic related to KrzyMod.
ON_EVENT(PRE_TICK) {krzyMod.Update();}
void KrzyMod::Update() {
	// managing time stuff
	auto timeNow = std::chrono::high_resolution_clock::now();
	float deltaTime = ((std::chrono::duration<float>)(timeNow - lastUpdate)).count();
	lastUpdate = timeNow;

	// stop everything if not enabled, and update the start timer because
	// i'm too lazy to detect when the mod is being enabled.
	if (!krzymod_enabled.GetBool()) {
		ResetTimer();
		Stop();
	}
	
	else {
		std::string channel = krzymod_vote_channel.GetString();
		if (krzymod_vote_enabled.GetBool() && channel.length() > 0) {
			//update twitch connection accordingly
			if (twitchCon.GetChannel().compare(krzymod_vote_channel.GetString()) != 0) {
				twitchCon.JoinChannel(krzymod_vote_channel.GetString());
			}
			if (!twitchCon.IsConnected()) {
				twitchCon.JoinChannel(krzymod_vote_channel.GetString());
			}
		} else if (twitchCon.IsConnected()) {
			twitchCon.Disconnect();	
		}
		// we always want sv_cheats to be enabled when krzymod is enabled. no questions.
		if (!sv_cheats.GetBool()) sv_cheats.SetValue(true);
	}

	// janky chrono time - don't increase duration if it's not enabled or deltaTime is unreasonably huge
	static Variable host_timescale("host_timescale");
	if (!IsEnabled() || deltaTime > 0.1 / host_timescale.GetFloat()) {
		auto advanceTimespan = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::duration<float>(deltaTime));
		startTimeModified += advanceTimespan;
		startTime += advanceTimespan;
	}

	if (!IsEnabled()) return;

	deltaTime = fminf(deltaTime, 0.1f);

	// manipulate second starting time point depending on how values changed
	if (duration != krzymod_time_base.GetFloat()) {
		float newTime = (GetTime(true) / duration) * krzymod_time_base.GetFloat();
		
		auto newTimespan = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::duration<float>(newTime));
		startTimeModified = timeNow - newTimespan;

		duration = krzymod_time_base.GetFloat();
	}
	if (krzymod_timer_multiplier.GetFloat() != 1) {
		float advanceTime = fabs(krzymod_timer_multiplier.GetFloat() - 1.0f) * deltaTime;

		auto advanceTimespan = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::duration<float>(advanceTime));
		if (krzymod_timer_multiplier.GetFloat() > 0) {
			startTimeModified -= advanceTimespan;
		} else {
			startTimeModified += advanceTimespan;
		}
		
	}


	// execute engine tick dependent modifiers and update their timers
	for (KrzyModActiveEffect &effect : activeEffects) {
		effect.Execute(ENGINE_TICK, true);
		effect.Update(deltaTime);
	}

	//clear modifiers that expired
	activeEffects.remove_if([](const KrzyModActiveEffect &effect) -> bool {
		return effect.time > effect.duration;
	});

	// update cvar controllers
	for (ConvarController &control : convarControllers) {
		control.Control();

		bool hasActiveMod = false;
		for (KrzyModActiveEffect &eff : activeEffects) {
			if (control.IsActivator(eff.effect)) {
				hasActiveMod = true;
				break;
			}
		}
		if (!hasActiveMod) control.Disable();
	}
	convarControllers.remove_if([](const ConvarController &con) -> bool {
		return !con.IsEnabled();
	});

	// resetting timer and activating most voted effect
	if (GetTime(true) > krzymod_time_base.GetFloat()) {
		if (selectedEffect != nullptr && evaluatedVoting) {
			ActivateEffect(selectedEffect);
			evaluatedVoting = false;
			votingPeople.clear();
		}
		//moving votes to the old votes buffer
		for (int i = 0; i < 4; i++) {
			oldVotes[i].effect = votes[i].effect;
			oldVotes[i].voteNumber = votes[i].voteNumber;
			oldVotes[i].votes = votes[i].votes;

			votes[i].effect = nullptr;
		}
		ResetTimer();
	}

	// adding new effects for the voting
	if (votes[0].effect == nullptr) {
		int beginNumber = (oldVotes[0].voteNumber == 1 && krzymod_vote_double_numbering.GetBool()) ? 5 : 1;
		for (int i = 0; i < 4; i++) {
			votes[i].voteNumber = beginNumber + i;
			votes[i].votes = 0;

			//make sure that new effects are not interfering already active ones
			int safetyCounter = 0;
			bool groupMemberUsed;
			do {
				safetyCounter++;
				groupMemberUsed = false;
				votes[i].effect = GetNextEffect();
				int groupID = votes[i].effect->groupID;
				if (groupID != 0) {
					for (auto& activeEffect : activeEffects) {
						if (activeEffect.effect->groupID == groupID) {
							groupMemberUsed = true;
							break;
						}
					}
				}
			} while (groupMemberUsed && safetyCounter < effects.size() - 1);
			
			
		}
	}

	// evaluating the voting
	if (!evaluatedVoting && krzymod_time_base.GetFloat() - GetTime(true) < 1.0) {
		
		KrzyModVote *vote = &votes[0];
		if (krzymod_vote_proportional.GetBool()) {
			// proportional. evaluate chances for each effect and randomly assign one
			int totalVoteCount = 0;
			for (int i = 0; i < 4; i++) totalVoteCount += votes[i].votes;

			float random = Math::RandomNumber(0.0f, 1.0f);
			float totalPerc = 0;
			for (int i = 0; i < 4; i++) {
				totalPerc += (totalVoteCount == 0) ? 0.25 : ((votes[i].votes) / (float)(totalVoteCount));
				if (totalPerc > random) {
					vote = &votes[i];
					break;
				}
			}
		} else {
			//not proportional, get most voted effect
			for (int i = 1; i < 4; i++) {
				if (votes[i].votes > vote->votes) {
					vote = &votes[i];
				} else if (votes[i].votes == vote->votes && Math::RandomNumber(0.0f, 1.0f) > 0.5) {
					vote = &votes[i];
				}
			}
		}
		
		evaluatedVoting = true;
		selectedEffect = vote->effect;
	}

	// getting new votes from Twitch chat
	auto twitchMsgs = twitchCon.FetchNewMessages();
	for (auto msg : twitchMsgs) {
		console->Print("[TWITCH] %s: %s\n", msg.username.c_str(), msg.message.c_str());
		if (msg.message.length() > 2) continue;
		if (std::find(votingPeople.begin(), votingPeople.end(), msg.username) == votingPeople.end()) {
			int voteNum = std::atoi(msg.message.c_str());
			if (krzyMod.Vote(voteNum)) {
				votingPeople.push_back(msg.username);
			}
		}
	}
}

void KrzyMod::Stop() {
	if (convarControllers.size() > 0) {
		for (ConvarController &control : convarControllers) {
			control.Disable();
		}
		convarControllers.clear();
	}
	if (activeEffects.size() > 0) {
		for (KrzyModEffect *eff : effects) {
			DisableEffect(eff);
		}
		activeEffects.clear();
	}
	if (twitchCon.IsConnected()) twitchCon.Disconnect();
}

// returns time value of the KrzyMod
// if modified flag is set, returns value manipulated by multiplier and base time change.
// if false is given, returns true timespan from the last applied effect
// if scaled, returns a value from 0 to 1 based on current base time value
float KrzyMod::GetTime(bool modified, bool scaled) {
	auto timeNow = std::chrono::high_resolution_clock::now();
	if (!IsEnabled()) timeNow = lastUpdate; // prevents wacky interface by keeping time interval static when paused.
	float time = ((std::chrono::duration<float>)(timeNow - (modified ? startTimeModified : startTime))).count();
	return scaled ? (time / duration) : time;
}

// resets everything time-related
void KrzyMod::ResetTimer() {
	auto timeNow = std::chrono::high_resolution_clock::now();
	startTime = timeNow;
	startTimeModified = timeNow;
	duration = krzymod_time_base.GetFloat();
}

// Activating modifier by adding a new ActiveKrzyModifier into the list
void KrzyMod::ActivateEffect(KrzyModEffect *effect) {
	if (krzymod_debug.GetBool()) {
		//console->Print("Activating KrzyMod effect %s. Next effect: %s\n", mod->displayName.c_str(), GetNextModifier(false)->displayName.c_str());
	}
	// the time is calculated based on own multiplier and base time specified by cvar
	float durMult = effect->durationMultiplier == 0 ? 2.5f : effect->durationMultiplier; 
	float duration = durMult * krzymod_time_base.GetFloat();
	KrzyModActiveEffect activeEffect = {effect, 0, duration};
	activeEffects.push_back(activeEffect);
	activeEffect.Execute(INITIAL, true, effect);
}

// disables modifier and executes it with LAST execute type
void KrzyMod::DisableEffect(KrzyModEffect *effect) {
	for (KrzyModActiveEffect &eff : activeEffects) {
		if (eff.effect != effect) continue;
		eff.Execute(LAST, false, effect);
		break;
	}
	activeEffects.remove_if([effect](const KrzyModActiveEffect &eff) -> bool {
		return eff.effect == effect;
	});
}


// adds modifier to the list of available modifiers. used only when initialized.
void KrzyMod::AddEffect(KrzyModEffect* effect) {
	effects.push_back(effect);
}

// gets next modifier in a queue and handles queue shuffling if needed.
KrzyModEffect *KrzyMod::GetNextEffect(bool increaseCounter) {
	if (nextEffectID == -1) {
		RandomizeEffectOrder();
	}
	auto mod = effects[nextEffectID];
	if (increaseCounter) {
		nextEffectID++;
		if (nextEffectID >= effects.size()) {
			RandomizeEffectOrder();
		}
	}
	return mod;
}

//shuffles modifiers queue
void KrzyMod::RandomizeEffectOrder() {
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(effects.begin(), effects.end(), g);
	nextEffectID = 0;
}

bool KrzyMod::Vote(int num) {
	for (KrzyModVote &vote : votes) {
		if (vote.voteNumber == num) {
			vote.votes++;
			return true;
		}
	}
	return false;
}

// adds convar controller
void KrzyMod::AddConvarController(Variable convar, std::string newValue, float time, KrzyModEffect* parent) {
	for (ConvarController &control : convarControllers) {
		if (control.Convar().ThisPtr() == convar.ThisPtr() && !control.IsActivator(parent)) {
			// there's already an active controller contrilling this cvar, update it.
			control.AddActivator(parent);
			return;
		}
	}
	convarControllers.push_back(ConvarController(convar, newValue, time, parent));
	//console->Print("Convar controller for cvar %s=%f added for %fs.\n", convar.ThisPtr()->m_pszName, newValue, time);
}



// Executed by Server::ProcessMovement
void KrzyMod::InvokeProcessMovementEvents(CMoveData *moveData, bool preCall) {
	if (!IsEnabled()) return;
	for (KrzyModActiveEffect &eff : activeEffects) {
		eff.Execute(PROCESS_MOVEMENT, preCall, moveData);
	}
}

// Executed by Client::OverrideCamera
void KrzyMod::InvokeOverrideCameraEvents(CViewSetup *view) {
	if (!IsEnabled()) return;
	for (KrzyModActiveEffect &eff : activeEffects) {
		eff.Execute(OVERRIDE_CAMERA, true, view);
	}
}

void KrzyMod::InvokeTraceRayEvents(CGameTrace *tr) {
	if (!IsEnabled()) return;
	for (KrzyModActiveEffect &eff : activeEffects) {
		eff.Execute(TRACERAY, true, tr);
	}
}


// Paints not only KrzyMod hud but also all of the active effects
void KrzyMod::Paint(int slot) {
	if (!krzymod_enabled.GetBool() || !sv_cheats.GetBool()) return;

	auto font = scheme->GetDefaultFont() + krzymod_primary_font.GetInt();
	auto font2 = scheme->GetDefaultFont() + krzymod_secondary_font.GetInt();

	int lineHeight = surface->GetFontHeight(font);
	int lineHeight2 = surface->GetFontHeight(font2);

	void *player = server->GetPlayer(slot + 1);
	if (!player) return;

	// draw modifiers
	for (KrzyModActiveEffect &eff : activeEffects) {
		((void (*)(KrzyModExecInfo))eff.effect->function)({HUD_PAINT, true, eff.time, eff.duration});
	}

	int xScreen, yScreen;
	engine->GetScreenSize(nullptr, xScreen, yScreen);

	//drawing progress bar
	surface->DrawRect(Color(0, 0, 0, 192), 0, 0, xScreen, 30);
	surface->DrawRect(Color(0, 111, 222, 255), 2, 2, (xScreen - 4) * GetTime(true, true) + 2, 28);


	auto DrawTextWithShadow = [](int font, float x, float y, Color color, Color shadowColor, const char* text) {
		surface->DrawTxt(font, x + 2, y + 2, shadowColor, text);
		surface->DrawTxt(font, x, y, color, text);
	};

	// drawing voting info
	if (krzymod_vote_enabled.GetInt()==1) {
		KrzyModVote *drawVotes = (GetTime(false) < 0.5) ? oldVotes : votes;
		if (drawVotes->effect != nullptr) {
			// calculating voting info
			int totalVoteCount = 0;
			for (int i = 0; i < 4; i++) totalVoteCount += drawVotes[i].votes;

			float votePercentages[4];
			for (int i = 0; i < 4; i++) {
				votePercentages[i] = (totalVoteCount == 0) ? 0 : ((drawVotes[i].votes) / (float)(totalVoteCount));
			}


			int voteWidth = 400;
			std::string voteCountText = std::string("Vote count: ") + std::to_string(totalVoteCount);
			DrawTextWithShadow(font, xScreen - voteWidth - 30, 40, {255, 255, 255}, {0, 0, 0, 220}, voteCountText.c_str());

			// draw actual voting screen
			for (int i = 0; i < 4; i++) {
				int yPos = 80 + i * (lineHeight2 + 30);

				// animate x offset for nice slide in and slide out anim
				float t = fmaxf(0.0, GetTime(false) - i * 0.033);
				float t1 = fminf(fmaxf(t / 0.4, 0.0), 1.0);
				float t2 = fminf(fmaxf((t - 0.5) / 0.4, 0.0), 1.0);
				float xOffset = fminf(pow(t1, 2.0), pow(1.0 - t2, 2.0));

				int xPos = xScreen - (voteWidth + 30) * (1.0 - xOffset);

				bool isSelected = selectedEffect == drawVotes[i].effect;
				bool shouldHighlight = !evaluatedVoting || isSelected;
				surface->DrawRect({64, 64, 64}, xPos, yPos, xPos + voteWidth, yPos + lineHeight2 + 14);
				surface->DrawRect(
					shouldHighlight ? Color(0, 111, 222) : Color(111, 111, 111),
					xPos,
					yPos,
					xPos + voteWidth * (isSelected ? 1.0 : votePercentages[i]),
					yPos + lineHeight2 + 14);

				std::string strNum = std::to_string(drawVotes[i].voteNumber) + ".";
				std::string strName = (i == 3) ? "Random Effect" : drawVotes[i].effect->displayName;
				std::string strPercentage = std::to_string((int)roundf(votePercentages[i] * 100.0f)) + std::string("%%");

				int nameWidth = surface->GetFontLength(font2, strName.c_str());
				int percWidth = surface->GetFontLength(font2, strPercentage.c_str());

				strPercentage += std::string("%%");  // DrawTxt uses double printf but GetFontLength doesn't

				DrawTextWithShadow(font2, xPos + 10, yPos + 7, {255, 255, 255}, {0, 0, 0, 220}, strNum.c_str());
				DrawTextWithShadow(font2, xPos + (voteWidth - nameWidth) * 0.5, yPos + 7, {255, 255, 255}, {0, 0, 0, 220}, strName.c_str());
				DrawTextWithShadow(font2, xPos + voteWidth - percWidth - 10, yPos + 7, {255, 255, 255}, {0, 0, 0, 220}, strPercentage.c_str());
			}
		}
	}
	


	// effects are using their own timer for animation, which isn't really smooth.
	// calculating delta time based on last time they were updated (which is last main update call)
	float dt = !IsEnabled() ? 0 : ((std::chrono::duration<float>)(std::chrono::high_resolution_clock::now() - lastUpdate)).count();

	// drawing active effects
	float fontPos = 0;
	for (KrzyModActiveEffect &eff : activeEffects) {
		const char *displayName = eff.effect->displayName.c_str();
		int textWidth = surface->GetFontLength(font, displayName);
		//x pos is adjusted at the beginning for slide-in anim
		int textX = xScreen - (30 + textWidth) * sin(fminf(eff.time + dt, 1.0) * M_PI * 0.5);
		int textY = 100 + fontPos * (lineHeight + 30);
		if (krzymod_vote_enabled.GetInt() == 1) textY += 4 * (lineHeight2 + 30);

		// for slowly disappearing near the end
		float alpha = fmaxf(fminf(eff.duration - (eff.time + dt), 1.0), 0.0f);
		int iAlpha = (int)(alpha * 255);

		DrawTextWithShadow(font, textX, textY, {255, 255, 255, iAlpha}, {0, 0, 0, (int)(alpha * 220)}, displayName);

		// effect progress bar
		if (eff.effect->durationMultiplier > 0) {
			surface->DrawRect({50, 50, 50, iAlpha}, textX, textY + lineHeight, textX + textWidth, textY + lineHeight + 10);
			float barVal = (1.0f - (eff.time+dt) / eff.duration);
			surface->DrawRect({0, 111, 222, iAlpha}, textX, textY + lineHeight, textX + textWidth * barVal, textY + lineHeight + 10);
		}

		// adjust position of elements below if current one is disappearing
		if (alpha < 0.5f) {
			float t = alpha / 0.5f;
			float val = t < 0.5 ? 4.0 * t * t * t : 1.0 - pow(-2.0 * t + 2.0, 3.0) * 0.5;
			fontPos += t;
		} else {
			fontPos++;
		}
	}
}

DECL_COMMAND_COMPLETION(krzymod_activate) {
	std::set<std::string> nameList;
	for (KrzyModEffect *effect : krzyMod.effects) {
		if (nameList.size() == COMMAND_COMPLETION_MAXITEMS) {
			break;
		}
		if (std::strstr(effect->name.c_str(), match)) {
			nameList.insert(effect->name);
		}
	}
	for (std::string name : nameList) {items.push_back(name);}
	FINISH_COMMAND_COMPLETION();
}
CON_COMMAND_F_COMPLETION(krzymod_activate, 
	"krzymod_activate [effect] - activate effect with given name\n", 
	0, AUTOCOMPLETION_FUNCTION(krzymod_activate)
) {
	if (args.ArgC() != 2) {
		return console->Print(krzymod_activate.ThisPtr()->m_pszHelpString);
	}
	for (KrzyModEffect *effect : krzyMod.effects) {
		if (effect->name.compare(args[1]) == 0) {
			krzyMod.ActivateEffect(effect);
			console->Print("Activated effect \"%s\".\n", args[1]);
			return;
		}
	}
	console->Print("Cannot find effect \"%s\".\n", args[1]);
}



DECL_COMMAND_COMPLETION(krzymod_deactivate) {
	std::set<std::string> nameList;
	for (KrzyModActiveEffect &eff : krzyMod.activeEffects) {
		if (nameList.size() == COMMAND_COMPLETION_MAXITEMS) {
			break;
		}
		if (std::strstr(eff.effect->name.c_str(), match)) {
			nameList.insert(eff.effect->name);
		}
	}
	for (std::string name : nameList) {
		items.push_back(name);
	}
	FINISH_COMMAND_COMPLETION();
}
CON_COMMAND_F_COMPLETION(krzymod_deactivate, 
	"krzymod_deactivate [effect] - stops all instances of an effect with given name\n", 
	0, AUTOCOMPLETION_FUNCTION(krzymod_deactivate)
){
	if (args.ArgC() != 2) {
		return console->Print(krzymod_deactivate.ThisPtr()->m_pszHelpString);
	}
	for (KrzyModEffect *effect : krzyMod.effects) {
		if (effect->name.compare(args[1]) == 0) {
			krzyMod.DisableEffect(effect);
			console->Print("Stopped effect \"%s\".\n", args[1]);
			return;
		}
	}
	console->Print("Cannot find effect \"%s\".\n", args[1]);
}

CON_COMMAND(krzymod_list, "krzymod_list - shows a list of all effects") {
	std::set<std::string> nameList;
	for (KrzyModEffect *effect : krzyMod.effects) {
		// putting it into the set first to have it ordered alphabetically
		nameList.insert(effect->name); 
		// console->Print("|%s|%s||%f|\n", effect->name.c_str(), effect->displayName.c_str(), effect->durationMultiplier);
	}
	console->Print("KrzyMod currently has %d effects:\n", nameList.size());
	for (std::string name : nameList) {
		console->Print(" - %s\n", name.c_str());
	}
}

CON_COMMAND(krzymod_vote, "krzymod_vote [number] - votes for an effect with given number") {
	if (args.ArgC() != 2) {
		return console->Print(krzymod_vote.ThisPtr()->m_pszHelpString);
	}
	int voteNum = std::atoi(args[1]);
	krzyMod.Vote(voteNum);
}
