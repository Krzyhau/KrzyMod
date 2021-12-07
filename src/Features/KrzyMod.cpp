#include "KrzyMod.hpp"

#include "Features/EntityList.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Scheme.hpp"
#include "Modules/Server.hpp"
#include "Modules/Client.hpp"
#include "Modules/Surface.hpp"
#include "Modules/VScript.hpp"
#include "Event.hpp"

#include <random>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <vector>
#include <set>


KrzyMod krzyMod;

Variable sar_krzymod_enabled("sar_krzymod_enabled", "0", "Enables KrzyMod (TM).\n");
Variable sar_krzymod_time_base("sar_krzymod_time_base", "30", 5.0f, 3600.0f, "The base time for all timers in KrzyMod.\n");
Variable sar_krzymod_timer_multiplier("sar_krzymod_timer_multiplier", "1", 0, "Multiplier for the main KrzyMod timer.\n");
Variable sar_krzymod_primary_font("sar_krzymod_primary_font", "92", 0, "Change font of KrzyMod.\n");
Variable sar_krzymod_secondary_font("sar_krzymod_secondary_font", "97", 0, "Change font of KrzyMod.\n");
Variable sar_krzymod_debug("sar_krzymod_debug", "0", "Debugs KrzyMod.\n");
Variable sar_krzymod_double_numbering("sar_krzymod_double_numbering", "0", "Uses different numbers for every voting in KrzyMod\n");
Variable sar_krzymod_vote_channel("sar_krzymod_vote_channel", "krzyhau", "Sets a twitch channel from which votes should be read.\n", 0);
Variable sar_krzymod_vote_proportional("sar_krzymod_vote_proportional", "1", 0, 1, "Should KrzyMod use proportional voting? (x% means effect has x% to be activated).\n", 0);

KrzyModEffect::KrzyModEffect(std::string name, std::string displayName, float durationMultiplier, int groupID, void (*function)(KrzyModExecInfo info))
	: name(name)
	, displayName(displayName)
	, durationMultiplier(durationMultiplier)
	, groupID(groupID)
	, function(function) {
	krzyMod.AddEffect(this);
}

void KrzyModActiveEffect::Update(float dt) {
	time += dt;
	if (time > duration) Execute(LAST, true, effect);
}

void KrzyModActiveEffect::Execute(KrzyModExecType type, bool preCall, void *data = nullptr) {
	((void (*)(KrzyModExecInfo))effect->function)({type, preCall, time, duration, data});
}

KrzyModConvarControl::KrzyModConvarControl(Variable var, std::string value, float time, KrzyModEffect *parent)
	: convar(var), value(value), remainingTime(time), parentEffect(parent){
	originalValue = var.GetString();
}

void KrzyModConvarControl::Update(float dt) {
	remainingTime -= dt;

	if (remainingTime <= 0) {
		remainingTime = 0;
		convar.SetValue(originalValue.c_str());
	} else if (strcmp(convar.GetString(),value.c_str()) != 0) {
		convar.SetValue(value.c_str());
	}
}




KrzyMod::KrzyMod()
	: Hud(HudType_InGame | HudType_Paused, true) {
}
KrzyMod::~KrzyMod() {
	// Making sure the commands are reset to their original values before the plugin is disabled
	Stop();
}
bool KrzyMod::ShouldDraw() {
	return sar_krzymod_enabled.GetBool() && Hud::ShouldDraw() && sv_cheats.GetBool();
}
bool KrzyMod::GetCurrentSize(int &w, int &h) {
	return false;
}


bool KrzyMod::IsEnabled() {
	return sar_krzymod_enabled.GetBool() && sv_cheats.GetBool() && engine->isRunning() && !engine->IsGamePaused();
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
	if (!sar_krzymod_enabled.GetBool()) {
		ResetTimer();
		Stop();
	}
	
	else {
		//update twitch connection accordingly
		if (twitchCon.GetChannel().compare(sar_krzymod_vote_channel.GetString()) != 0) {
			twitchCon.SetChannel(sar_krzymod_vote_channel.GetString());
		}
		if (!twitchCon.IsActive()) {
			twitchCon.SetChannel(sar_krzymod_vote_channel.GetString());
			twitchCon.Connect();
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
	if (duration != sar_krzymod_time_base.GetFloat()) {
		float newTime = (GetTime(true) / duration) * sar_krzymod_time_base.GetFloat();
		
		auto newTimespan = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::duration<float>(newTime));
		startTimeModified = timeNow - newTimespan;

		duration = sar_krzymod_time_base.GetFloat();
	}
	if (sar_krzymod_timer_multiplier.GetFloat() != 1) {
		float advanceTime = fabs(sar_krzymod_timer_multiplier.GetFloat() - 1.0f) * deltaTime;

		auto advanceTimespan = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::duration<float>(advanceTime));
		if (sar_krzymod_timer_multiplier.GetFloat() > 0) {
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
	for (KrzyModConvarControl &control : convarControllers) {
		bool hasActiveMod = false;
		for (KrzyModActiveEffect &eff : activeEffects) {
			if (eff.effect == control.parentEffect) {
				hasActiveMod = true;
				break;
			}
		}
		if (!hasActiveMod) control.remainingTime = 0;
		control.Update(deltaTime);
	}
	convarControllers.remove_if([](const KrzyModConvarControl &con) -> bool {
		return con.remainingTime <= 0;
	});

	// resetting timer and activating most voted effect
	if (GetTime(true) > sar_krzymod_time_base.GetFloat()) {
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
		int beginNumber = (oldVotes[0].voteNumber == 1 && sar_krzymod_double_numbering.GetBool()) ? 5 : 1;
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
	if (!evaluatedVoting && sar_krzymod_time_base.GetFloat() - GetTime(true) < 1.0) {
		
		KrzyModVote *vote = &votes[0];
		if (sar_krzymod_vote_proportional.GetBool()) {
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
	auto twitchMsgs = twitchCon.GetNewMessages();
	for (auto msg : twitchMsgs) {
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
		for (KrzyModConvarControl &control : convarControllers) {
			control.remainingTime = 0;
			control.Update(0);
		}
		convarControllers.clear();
	}
	if (activeEffects.size() > 0) {
		for (KrzyModEffect *eff : effects) {
			DisableEffect(eff);
		}
		activeEffects.clear();
	}
	if (twitchCon.IsActive()) twitchCon.Disconnect();
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
	duration = sar_krzymod_time_base.GetFloat();
}

// Activating modifier by adding a new ActiveKrzyModifier into the list
void KrzyMod::ActivateEffect(KrzyModEffect *effect) {
	if (sar_krzymod_debug.GetBool()) {
		//console->Print("Activating KrzyMod effect %s. Next effect: %s\n", mod->displayName.c_str(), GetNextModifier(false)->displayName.c_str());
	}
	// the time is calculated based on own multiplier and base time specified by cvar
	float durMult = effect->durationMultiplier == 0 ? 2.5f : effect->durationMultiplier; 
	float duration = durMult * sar_krzymod_time_base.GetFloat();
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
	for (KrzyModConvarControl &control : convarControllers) {
		if (control.convar.ThisPtr() == convar.ThisPtr()) {
			control.value = newValue;
			control.remainingTime = time;
			control.parentEffect = parent;
			return;
		}
	}
	convarControllers.push_back(KrzyModConvarControl(convar, newValue, time, parent));
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
	if (!sar_krzymod_enabled.GetBool() || !sv_cheats.GetBool()) return;

	auto font = scheme->GetDefaultFont() + sar_krzymod_primary_font.GetInt();
	auto font2 = scheme->GetDefaultFont() + sar_krzymod_secondary_font.GetInt();

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
	KrzyModVote *drawVotes = (GetTime(false) < 0.5) ? oldVotes : votes;
	if (drawVotes->effect != nullptr) {
		// calculating voting info
		int totalVoteCount = 0;
		for (int i = 0; i < 4; i++) totalVoteCount += drawVotes[i].votes;

		float votePercentages[4];
		for (int i = 0; i < 4;i++) {
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
			float t2 = fminf(fmaxf((t-0.5) / 0.4, 0.0), 1.0);
			float xOffset = fminf(pow(t1, 2.0), pow(1.0-t2, 2.0));

			int xPos = xScreen - (voteWidth + 30) * (1.0 - xOffset);

			bool isSelected = selectedEffect == drawVotes[i].effect;
			bool shouldHighlight = !evaluatedVoting || isSelected;
			surface->DrawRect({64, 64, 64}, xPos, yPos, xPos + voteWidth, yPos + lineHeight2 + 14);
			surface->DrawRect(
				shouldHighlight ? Color(0, 111, 222) : Color(111, 111, 111), 
				xPos, yPos, xPos + voteWidth * (isSelected ? 1.0 : votePercentages[i]), yPos + lineHeight2 + 14
			);

			std::string strNum = std::to_string(drawVotes[i].voteNumber) + ".";
			std::string strName = (i == 3) ? "Random Effect" : drawVotes[i].effect->displayName;
			std::string strPercentage = std::to_string((int)roundf(votePercentages[i] * 100.0f)) + std::string("%%");

			int nameWidth = surface->GetFontLength(font2, strName.c_str());
			int percWidth = surface->GetFontLength(font2, strPercentage.c_str());

			strPercentage += std::string("%%"); // DrawTxt uses double printf but GetFontLength doesn't

			DrawTextWithShadow(font2, xPos + 10, yPos+7, {255, 255, 255}, {0, 0, 0, 220}, strNum.c_str());
			DrawTextWithShadow(font2, xPos + (voteWidth - nameWidth) * 0.5, yPos+7, {255, 255, 255}, {0, 0, 0, 220}, strName.c_str());
			DrawTextWithShadow(font2, xPos + voteWidth - percWidth - 10, yPos+7, {255, 255, 255}, {0, 0, 0, 220}, strPercentage.c_str());
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
		int textY = 100 + 4*(lineHeight2 + 30) + fontPos * (lineHeight + 30);

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

DECL_COMMAND_COMPLETION(sar_krzymod_activate) {
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
CON_COMMAND_F_COMPLETION(sar_krzymod_activate, 
	"sar_krzymod_activate [effect] - activate effect with given name\n", 
	0, AUTOCOMPLETION_FUNCTION(sar_krzymod_activate)
) {
	if (args.ArgC() != 2) {
		return console->Print(sar_krzymod_activate.ThisPtr()->m_pszHelpString);
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



DECL_COMMAND_COMPLETION(sar_krzymod_deactivate) {
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
CON_COMMAND_F_COMPLETION(sar_krzymod_deactivate, 
	"sar_krzymod_deactivate [effect] - stops all instances of an effect with given name\n", 
	0, AUTOCOMPLETION_FUNCTION(sar_krzymod_deactivate)
){
	if (args.ArgC() != 2) {
		return console->Print(sar_krzymod_deactivate.ThisPtr()->m_pszHelpString);
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

CON_COMMAND(sar_krzymod_list, "sar_krzymod_list - shows a list of all effects") {
	std::set<std::string> nameList;
	for (KrzyModEffect *effect : krzyMod.effects) {
		// putting it into the set first to have it ordered alphabetically
		nameList.insert(effect->name); 
	}
	console->Print("KrzyMod currently has %d effects:\n", nameList.size());
	for (std::string name : nameList) {
		console->Print(" - %s\n", name.c_str());
	}
}

CON_COMMAND(sar_krzymod_vote, "sar_krzymod_vote [number] - votes for an effect with given number") {
	if (args.ArgC() != 2) {
		return console->Print(sar_krzymod_vote.ThisPtr()->m_pszHelpString);
	}
	int voteNum = std::atoi(args[1]);
	krzyMod.Vote(voteNum);
}









CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, moveWStuck, "Help My W Is Stuck", 2.5f, 0) {
	if (!info.preCall) return;
	auto moveData = (CMoveData *)info.data;
	moveData->m_flForwardMove += 175.0f;
}

CREATE_KRZYMOD_SIMPLE(OVERRIDE_CAMERA, viewQuakeFov, "Quake FOV", 3.5f, 5) {
	auto viewSetup = (CViewSetup *)info.data;
	viewSetup->fov *= 1.6;
	viewSetup->fovViewmodel *= 1.5;
}

CREATE_KRZYMOD_SIMPLE(OVERRIDE_CAMERA, viewUpsideDown, "Upside Down View", 2.5f, 0) {
	auto viewSetup = (CViewSetup *)info.data;
	viewSetup->angles.z += 180;
}

CREATE_KRZYMOD_SIMPLE(INITIAL, metaFasterDelay, "x4 KrzyMod Speed", 1.0f, 1) {
	KRZYMOD_CONTROL_CVAR(sar_krzymod_timer_multiplier, 4);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, metaPause, "Pause KrzyMod", 1.0f, 1) {
	KRZYMOD_CONTROL_CVAR(sar_krzymod_timer_multiplier, 0);
}

CREATE_KRZYMOD_SIMPLE(HUD_PAINT, visualSnapchatMode, "Snapchat Mode", 2.5f, 0) {
	int xScreen, yScreen;
	engine->GetScreenSize(nullptr, xScreen, yScreen);

	float ratio = (float)yScreen / (float)xScreen;
	if (ratio > 1.0f) return;

	float newWidth = yScreen * ratio;
	float barWidth = (xScreen - newWidth) * 0.5f;

	surface->DrawRect(Color(0, 0, 0, 255), 0, 0, barWidth, yScreen);
	surface->DrawRect(Color(0, 0, 0, 255), barWidth+newWidth, 0, xScreen, yScreen);


	auto font = scheme->GetDefaultFont() + 1;
	int fontHeight = surface->GetFontHeight(font);

	float textYPos = yScreen * 0.7f;

	surface->DrawRect(Color(0, 0, 0, 200), 0, textYPos, xScreen, textYPos + fontHeight + 20.0f);

	const char *lmaoText = "YOOO HOW DOES HE DO THAT?!";
	int fontWidth = surface->GetFontLength(font, lmaoText);
	surface->DrawTxt(font, (xScreen - fontWidth) * 0.5f, textYPos + 10.0f, Color(255, 255, 255, 255), lmaoText);
}

CREATE_KRZYMOD_INSTANT(playerKill, "kill.", 0) {
	engine->ExecuteCommand("kill");
}

CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, moveStickyGround, "Sticky Ground", 3.5f, 0) {
	void *player = server->GetPlayer(1);
	if (!player) return;
	unsigned int groundEntity = *reinterpret_cast<unsigned int *>((uintptr_t)player + Offsets::S_m_hGroundEntity);
	bool grounded = groundEntity != 0xFFFFFFFF;

	auto moveData = (CMoveData *)info.data;

	if (grounded) {
		moveData->m_flForwardMove = 0;
		moveData->m_flSideMove = 0;
		moveData->m_vecVelocity.x = 0;
		moveData->m_vecVelocity.y = 0;
	}
}

CREATE_KRZYMOD(moveInverseControl, "Inverse Controls", 3.5f, 0) {
	if (info.execType == INITIAL) {
		//reverse angles
		Variable yaw = Variable("m_yaw");
		krzyMod.AddConvarController(yaw, std::to_string(yaw.GetFloat() * -1), info.endTime, (KrzyModEffect *)info.data);
		Variable pitch = Variable("m_pitch");
		krzyMod.AddConvarController(pitch, std::to_string(pitch.GetFloat() * -1), info.endTime, (KrzyModEffect *)info.data);
	}
	if (info.execType == PROCESS_MOVEMENT) {
		auto moveData = (CMoveData *)info.data;

		//reverse movement
		moveData->m_flForwardMove *= -1;
		moveData->m_flSideMove *= -1;

		//reverse portals
		bool bluePortal = (moveData->m_nButtons & IN_ATTACK) > 0;
		bool orangePortal = (moveData->m_nButtons & IN_ATTACK2) > 0;
		moveData->m_nButtons &= ~(IN_ATTACK | IN_ATTACK2);
		if (bluePortal) moveData->m_nButtons &= IN_ATTACK2;
		if (orangePortal) moveData->m_nButtons &= IN_ATTACK;
	}
}

CREATE_KRZYMOD_SIMPLE(HUD_PAINT, visualDvdLogo, "DVD Logo", 4.5f, 0) {
	int xScreen, yScreen;
	engine->GetScreenSize(nullptr, xScreen, yScreen);

	// procedurally generating DVD logo.
	// yes, I'm a madman
	static int dvdLogoTexID = 0;
	if (dvdLogoTexID == 0) {
		dvdLogoTexID = surface->CreateNewTextureID(surface->matsurface->ThisPtr(), true);

		const int texWidth = 248;
		const int texHeight = 140;

		unsigned char texDataRGBA[texWidth * texHeight * 4];

		for (int y = 0; y < texHeight; y++) {
			for (int x = 0; x < texWidth; x++) {
				Vector pos(x / (float)texWidth, y / (float)texHeight);

				//disk at the bottom
				bool diskCircle1 = ((pos - Vector(0.5, 0.8)) * Vector(1.0, 3.3)).Length() < 0.485;
				bool diskCircle2 = ((pos - Vector(0.503, 0.793)) * Vector(1.0, 2.3)).Length() < 0.115;
				bool disk = diskCircle1 && !diskCircle2;
				pos.x -= (1.0 - pos.y) * 0.1 - 0.02; //ofsetting for tilted look
				// V and parts of D's
				bool v = ((fabsf((pos.y - fmaxf(0, 0.5 - fabsf(powf(pos.x, 0.45) - 0.695) * 4.6)))) < 0.155) && pos.y > 0.056 && pos.x < 0.7 && pos.x > 0;
				// using """modulo""" to repeat D's
				bool xNot0 = pos.x > 0;
				Vector pos2 = pos;
				if (pos2.x >= 0.615f) pos2.x -= 0.615f;
				// D's
				bool d1 = ((pos2 - Vector(0.09, 0.3)) * Vector(0.89, 0.82)).Length() < 0.2;
				bool d2 = ((pos2 - Vector(0.09, 0.3)) * Vector(0.89, 0.82)).Length() < 0.12;
				bool d = xNot0 && ((d1 && !d2 && pos2.x > 0.07) || (pos2.x < 0.09 && pos2.y > 0.2 && pos2.y < 0.544));
				
				int pixelColor = (disk || v || d) ? 255 : 0;

				int pixelPos = (y * texWidth + x) * 4;
				texDataRGBA[pixelPos + 0] = pixelColor;
				texDataRGBA[pixelPos + 1] = pixelColor;
				texDataRGBA[pixelPos + 2] = pixelColor;
				texDataRGBA[pixelPos + 3] = pixelColor;
			}
		}
		surface->DrawSetTextureRGBA(surface->matsurface->ThisPtr(), dvdLogoTexID, texDataRGBA, texWidth, texHeight);
	}

	float dvdWidth = xScreen * 0.2;
	float dvdHeight = dvdWidth * 0.5;
	float maxX = xScreen - dvdWidth;
	float maxY = yScreen - dvdHeight;
	float speed = dvdWidth; //move at speed of your own width per second

	float time = engine->GetClientTime() * speed;

	float posX = fmod(time, maxX);
	float posY = fmod(time, maxY);
	int bouncesX = (int)(time / maxX);
	int bouncesY = (int)(time / maxY);
	if (bouncesX % 2 == 1) posX = maxX - posX;
	if (bouncesY % 2 == 1) posY = maxY - posY;

	static int previousBounceCounter = 0;
	static Color dvdColor = Color(255,255,255,255);
	if (bouncesX + bouncesY != previousBounceCounter) {
		previousBounceCounter = bouncesX + bouncesY;
		dvdColor.SetColor(Math::RandomNumber(100, 255), Math::RandomNumber(100, 255), Math::RandomNumber(100, 255));
	}

	surface->DrawSetColor(surface->matsurface->ThisPtr(), dvdColor.r(), dvdColor.g(), dvdColor.b(), dvdColor.a());
	surface->DrawSetTexture(surface->matsurface->ThisPtr(), dvdLogoTexID);
	surface->DrawTexturedRect(surface->matsurface->ThisPtr(), posX,posY,posX+dvdWidth,posY+dvdHeight);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, visualHideCrosshair, "Hide Crosshair", 3.5f, 0) {
	KRZYMOD_CONTROL_CVAR(cl_drawhud, 0);
}

CREATE_KRZYMOD(playerLaunchRandom, "Yeet Player", 0.0f, 0) {
	static bool executed = false;
	if (info.execType == INITIAL) executed = false;
	if (info.execType == PROCESS_MOVEMENT && !info.preCall && !executed) {
		auto moveData = (CMoveData *)info.data;
		Vector newVec = moveData->m_vecVelocity + Vector(
			Math::RandomNumber(-1.0f, 1.0f), 
			Math::RandomNumber(-1.0f, 1.0f), 
			Math::RandomNumber(0.0f, 1.0f) //different range here to give more interesting yeets
		).Normalize() * Math::RandomNumber(500.0f,1500.0f);
		moveData->m_vecVelocity = newVec;
		executed = true;
	}
}

CREATE_KRZYMOD(playerLaunchUp, "Polish Space Program", 0.0f, 0) {
	static bool executed = false;
	if (info.execType == INITIAL) executed = false;
	if (info.execType == PROCESS_MOVEMENT && !info.preCall && !executed) {
		auto moveData = (CMoveData *)info.data;
		Vector newVec = moveData->m_vecVelocity + Vector(0, 0, 3600.0f);
		moveData->m_vecVelocity = newVec;
		executed = true;
	}
}

CREATE_KRZYMOD(playerUTurn, "U-Turn", 0.0f, 0) {
	static bool executed = false;
	if (info.execType == INITIAL) executed = false;
	if (info.execType == PROCESS_MOVEMENT && !info.preCall && !executed) {
		auto moveData = (CMoveData *)info.data;
		moveData->m_vecVelocity = moveData->m_vecVelocity * - 1.0f;

		QAngle viewangles = engine->GetAngles(GET_SLOT());
		viewangles.y += 180;
		viewangles.x *= -1;
		engine->SetAngles(GET_SLOT(), viewangles);

		executed = true;
	}
}

CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, moveAirlock, "No Air Control", 3.5f, 0) {
	void *player = server->GetPlayer(1);
	if (!player) return;
	unsigned int groundEntity = *reinterpret_cast<unsigned int *>((uintptr_t)player + Offsets::S_m_hGroundEntity);
	bool grounded = groundEntity != 0xFFFFFFFF;

	auto moveData = (CMoveData *)info.data;

	if (!grounded) {
		moveData->m_flForwardMove = 0;
		moveData->m_flSideMove = 0;
	}
}

CREATE_KRZYMOD_SIMPLE(INITIAL, moveAbh, "ABH", 3.5f, 0) {
	KRZYMOD_CONTROL_CVAR(sar_duckjump, 1);
	KRZYMOD_CONTROL_CVAR(sar_jumpboost, 1);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, moveNoAirlock, "Better Air Control", 3.5f, 0) {
	KRZYMOD_CONTROL_CVAR(sar_aircontrol, 1);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gameSmallTimescale, "Timescale 0.2", 1.5f, 2) {
	KRZYMOD_CONTROL_CVAR(host_timescale, 0.5);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gameLargeTimescale, "Timescale x2", 2.5f, 2) {
	KRZYMOD_CONTROL_CVAR(host_timescale, 2);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gameSmallPhysscale, "Slowy Props", 3.5f, 3) {
	KRZYMOD_CONTROL_CVAR(phys_timescale, 0.5);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gameLargePhysscale, "Speedy Props", 3.5f, 3) {
	KRZYMOD_CONTROL_CVAR(phys_timescale, 2);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gameMaxBounciness, "Maximum Repulsiveness", 3.5f, 0) {
	KRZYMOD_CONTROL_CVAR(bounce_paint_min_speed, 3600);
}

CREATE_KRZYMOD(moveAutojump, "Jump Script", 3.5f, 0) {
	if (info.execType == INITIAL) {
		KRZYMOD_CONTROL_CVAR(sar_autojump, 1);
	}
	if(info.execType == ENGINE_TICK) {
		engine->ExecuteCommand("+jump");
	}
	if (info.execType == LAST) {
		engine->ExecuteCommand("-jump");
	}
}

CREATE_KRZYMOD_INSTANT(gameRemovePaint, "Remove All Paint", 0) {
	engine->ExecuteCommand("removeallpaint");
}

CREATE_KRZYMOD_INSTANT(gameChangePortalgunLinkage, "These aren't my portals!", 0) {
	engine->ExecuteCommand("change_portalgun_linkage_id 0 100 1");
}

CREATE_KRZYMOD(moveDrunk, "Drunk", 3.5f, 5) {

	float time = engine->GetClientTime() * 0.3f;
	float td = fmin((info.endTime-info.time)*0.1,fmin(info.time * 0.2, 1.0));

	if (info.execType == PROCESS_MOVEMENT && info.preCall) {
		auto moveData = (CMoveData *)info.data;

		Vector wishDir(moveData->m_flForwardMove, moveData->m_flSideMove);
		float moveAng = (sinf(time * 1.9) + cosf(time * 1.8)) * (sinf(time * 1.7) + cosf(time * 1.6)) * 0.2 * td;
		moveData->m_flForwardMove = wishDir.x * cosf(moveAng) + wishDir.y * sinf(moveAng);
		moveData->m_flSideMove = wishDir.y * cosf(moveAng) + wishDir.x * sinf(moveAng);

		moveData->m_flForwardMove += (sinf(time * 1.5) + cosf(time * 1.4)) * (sinf(time * 1.3) + cosf(time * 1.2)) * 10.0f * td;
		moveData->m_flSideMove += (sinf(time * 1.3) + cosf(time * 1.2)) * (sinf(time * 1.1) + cosf(time * 1.0)) * 10.0f * td;
	}
	if (info.execType == OVERRIDE_CAMERA) {
		auto viewSetup = (CViewSetup *)info.data;
		viewSetup->angles.x += (sinf(time * 1.1) + cosf(time * 1.4)) * (sinf(time * 1.7) + cosf(time * 1.2)) * 5 * td;
		viewSetup->angles.y += (sinf(time * 1.5) + cosf(time * 1.6)) * (sinf(time * 1.5) + cosf(time * 1.1)) * 15 * td;
		viewSetup->angles.z += (sinf(time * 1.4) + cosf(time * 1.7)) * (sinf(time * 1.1) + cosf(time * 1.0)) * 10 * td;

		float fovMod = (sinf(time * 1.2) + cosf(time * 1.3)) * (sinf(time * 1.4) + cosf(time * 1.5)) * 30 * td;
		viewSetup->fov += fovMod;
		viewSetup->fovViewmodel += fovMod;
	}
}

CREATE_KRZYMOD_SIMPLE(INITIAL, moveMarioJump, "Mario Jump", 3.5f, 0) {
	KRZYMOD_CONTROL_CVAR(sar_jump_height, 150);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, moveStanleyParable, "The Stanley Parable", 3.5f, 0) {
	KRZYMOD_CONTROL_CVAR(sar_jump_height, 0);
}


CREATE_KRZYMOD(visualRainbowwPropss, "RainbowwPropss", 4.5f, 0) {
	if (info.execType == INITIAL) {
		KRZYMOD_CONTROL_CVAR(r_colorstaticprops, 1);
	}
	if (info.execType == ENGINE_TICK && info.preCall) {
		float time = engine->GetClientTime();

		for (int i = 0; i < Offsets::NUM_ENT_ENTRIES; ++i) {
			void *ent = server->m_EntPtrArray[i].m_pEntity;
			if (!ent) continue;

			const char *entClass = server->GetEntityClassName(ent);
			if (!std::strstr(entClass, "prop") && !std::strstr(entClass, "npc")) continue;

			float colorVal = fmodf((time + i) * 0.3f, 1.0f);

			Vector color = {
				fminf(fmaxf(fabs(colorVal * 6.0 - 3.0) - 1.0, 0), 1) * 255,
				fminf(fmaxf(2.0 - fabs(colorVal * 6.0 - 2.0), 0), 1) * 255,
				fminf(fmaxf(2.0 - fabs(colorVal * 6.0 - 4.0), 0), 1) * 255};

			server->SetKeyValueVector(nullptr, ent, "rendercolor", color);
		}
	}
}

CREATE_KRZYMOD_SIMPLE(INITIAL, visualBlackout, "Blackout", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(fog_override, 1);
	KRZYMOD_CONTROL_CVAR(fog_maxdensity, 1);
	KRZYMOD_CONTROL_CVAR(fog_start, -5000);
	KRZYMOD_CONTROL_CVAR(fog_end, 200);
	KRZYMOD_CONTROL_CVAR(fog_color, 0 0 0);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, visualRtxOn, "RTX On", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(mat_vignette_enable, 1);
	KRZYMOD_CONTROL_CVAR(mat_bloom_scalefactor_scalar, 10);
	KRZYMOD_CONTROL_CVAR(mat_motion_blur_strength, 10);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, visualPS2Graphics, "PS2 Graphics", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(mat_picmip, 4);
	KRZYMOD_CONTROL_CVAR(r_lod, 3);

}

CREATE_KRZYMOD_SIMPLE(INITIAL, visualGrid, "The Grid", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(mat_luxels, 1);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, visualHideStatic, "Hide Static World", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(r_portalsopenall, 1);
	KRZYMOD_CONTROL_CVAR(r_drawworld, 0);
	KRZYMOD_CONTROL_CVAR(r_drawstaticprops, 0);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, visualHideDynamic, "Hide Dynamic World", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(r_shadows, 0);
	KRZYMOD_CONTROL_CVAR(r_drawentities, 0);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, visualPaintItWhite, "Paint It, White", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(mat_fullbright, 2);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, visualOrtho, "Orthographic View", 1.5f, 0) {
	KRZYMOD_CONTROL_CVAR(r_portalsopenall, 1);
	KRZYMOD_CONTROL_CVAR(sar_cam_ortho, 1);
	KRZYMOD_CONTROL_CVAR(sar_cam_ortho_nearz, -10000);
	KRZYMOD_CONTROL_CVAR(cl_skip_player_render_in_main_view, 0);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gamePortalMachineGun, "Portal MacHINE GUN!", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(portalgun_fire_delay, 0);
	KRZYMOD_CONTROL_CVAR(portalgun_held_button_fire_fire_delay, 0);
}

CREATE_KRZYMOD_INSTANT(gamePressButtons, "Press All Map Buttons", 0) {
	engine->ExecuteCommand("ent_fire prop_floor_button pressin");
	engine->ExecuteCommand("ent_fire prop_under_floor_button pressin");
	engine->ExecuteCommand("ent_fire prop_under_button press");
	engine->ExecuteCommand("ent_fire prop_button press");
	engine->ExecuteCommand("ent_fire func_button press");
}

CREATE_KRZYMOD_INSTANT(gameReleaseButtons, "Release All Map Buttons", 0) {
	engine->ExecuteCommand("ent_fire prop_floor_button pressout");
	engine->ExecuteCommand("ent_fire prop_under_floor_button pressout");
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gameNoFriction, "Caution! Wet Floor!", 3.5f, 0) {
	KRZYMOD_CONTROL_CVAR(sv_friction, 0.2);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gameNegativeFriction, "Weeeeeeeeee!!!", 3.5f, 0) {
	KRZYMOD_CONTROL_CVAR(sv_friction, -1);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gameMoonGravity, "Moon Gravity", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(sv_gravity, 200);
	KRZYMOD_CONTROL_CVAR(sar_jump_height, 135);
}

CREATE_KRZYMOD(viewBarrelRoll, "Do A Barrel Roll!", 1.5f, 0) {
	static int mult = -1;
	if (info.execType == INITIAL) {
		mult = Math::RandomNumber(0.0f, 1.0f) > 0.5f ? 1 : -1;
	}
	if (info.execType == OVERRIDE_CAMERA) {
		auto viewSetup = (CViewSetup *)info.data;
		float t = fmodf(info.time*0.5, 1.0f);
		float angle = t < 0.5 ? 4.0 * t * t * t : 1.0 - pow(-2.0 * t + 2.0, 3.0) * 0.5;

		viewSetup->angles.z += mult * angle * 360.0f;
	}
}

CREATE_KRZYMOD_INSTANT(gameRestartLevel, "restart_level", 2) {
	engine->ExecuteCommand("restart_level");
}

CREATE_KRZYMOD_INSTANT(gameLoadQuick, "Load Quicksave", 2) {
	engine->ExecuteCommand("load quick");
}

CREATE_KRZYMOD_INSTANT(gameLoadAutosave, "Load Autosave", 2) {
	engine->ExecuteCommand("load autosave");
}

CREATE_KRZYMOD_SIMPLE(INITIAL, visualDeepFried, "Deep Fried", 1.5f, 0) {
	KRZYMOD_CONTROL_CVAR(mat_ambient_light_r, 10);
	KRZYMOD_CONTROL_CVAR(mat_ambient_light_g, 10);
	KRZYMOD_CONTROL_CVAR(mat_ambient_light_b, 10);
}

CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, moveFallDamage, "Short Fall Boots", 3.5f, 0) {

	if (info.preCall) return;

	static float velLastFrame = 0;
	static float lastGroundedState = true;

	void *player = server->GetPlayer(1);
	if (!player) return;
	unsigned int groundEntity = *reinterpret_cast<unsigned int *>((uintptr_t)player + Offsets::S_m_hGroundEntity);
	bool grounded = groundEntity != 0xFFFFFFFF;

	

	if (!lastGroundedState && grounded) {
		float damage = fmaxf((velLastFrame - 300.0f) * 0.25f, 0);
		std::string command("hurtme ");
		command += std::to_string(damage*2);
		engine->ExecuteCommand(command.c_str());
	}

	auto moveData = (CMoveData *)info.data;
	velLastFrame = fabs(moveData->m_vecVelocity.z);
	lastGroundedState = grounded;
}

CREATE_KRZYMOD_SIMPLE(INITIAL, visualClaustrophobia, "Claustrophobia", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(r_aspectratio, 6);
}

CREATE_KRZYMOD(moveSuperhot, "SUPER HOT", 3.5f, 2) {
	static Vector prevAngles;
	static float superHotValue;
	static Variable host_timescale("host_timescale");
	if (info.execType == INITIAL) {
		superHotValue = 1;
	}
	if (info.execType == PROCESS_MOVEMENT) {
		if (!info.preCall) return;
		auto moveData = (CMoveData *)info.data;

		bool shouldIncrease = moveData->m_flForwardMove != 0 || moveData->m_flSideMove != 0;

		Vector newAngles = QAngleToVector(engine->GetAngles(GET_SLOT()));
		if ((prevAngles - newAngles).Length() > 1) shouldIncrease = true;

		superHotValue = fminf(fmaxf(superHotValue + 0.04 * (shouldIncrease ? 1 : -1), 0.1), 1.0);

		host_timescale.SetValue(superHotValue);

		prevAngles = newAngles;
	}
	if (info.execType == LAST) {
		host_timescale.SetValue(1);
	}
}

CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, moveAlwaysDuck, "I'm A Duck!", 2.5f, 0) {
	auto moveData = (CMoveData *)info.data;
	moveData->m_nButtons -= moveData->m_nButtons & IN_JUMP;
	moveData->m_nButtons |= IN_DUCK;
}

CREATE_KRZYMOD_INSTANT(gameOpenSesame, "Open Sesame!", 0) {
	for (int i = 0; i < Offsets::NUM_ENT_ENTRIES; ++i) {
		void *ent = server->m_EntPtrArray[i].m_pEntity;
		if (!ent) continue;

		const char *entName = server->GetEntityName(ent);
		if (entName == nullptr) continue;
		if (!std::strstr(entName, "door_open_relay") && !std::strstr(entName, "open_door")) continue;
		std::string command = "ent_fire " + std::string(entName) + " trigger";
		engine->ExecuteCommand(command.c_str());
	}
}

CREATE_KRZYMOD_INSTANT(gameCloseSesame, "Close Sesame!", 0) {
	for (int i = 0; i < Offsets::NUM_ENT_ENTRIES; ++i) {
		void *ent = server->m_EntPtrArray[i].m_pEntity;
		if (!ent) continue;

		const char *entName = server->GetEntityName(ent);
		if (entName == nullptr) continue;
		if (!std::strstr(entName, "door_close_relay") && !std::strstr(entName, "close_door")) continue;
		std::string command = "ent_fire " + std::string(entName) + " trigger";
		engine->ExecuteCommand(command.c_str());
	}
}

CREATE_KRZYMOD_SIMPLE(TRACERAY, gameOnlyWallsPortalable, "Portals Only On Walls", 2.5f, 4) {
	auto tr = (CGameTrace *)info.data;

	if (fabsf(tr->plane.normal.z) > 0.2) {
		tr->surface.flags |= SURF_NOPORTAL;
	}
}

CREATE_KRZYMOD_SIMPLE(TRACERAY, gameReversePortalSurfaces, "Reverse Surface Portalability", 2.5f, 4) {
	auto tr = (CGameTrace *)info.data;

	tr->surface.flags ^= SURF_NOPORTAL;
}

CREATE_KRZYMOD_SIMPLE(TRACERAY, gameLeastPortals, "Least Portals", 1.5f, 4) {
	auto tr = (CGameTrace *)info.data;

	tr->surface.flags |= SURF_NOPORTAL;
}

CREATE_KRZYMOD_SIMPLE(OVERRIDE_CAMERA, viewZoomFov, "Magnifying Glass", 2.5f, 5) {
	auto viewSetup = (CViewSetup *)info.data;
	viewSetup->fov /= 1.6;
	viewSetup->fovViewmodel /= 1.5;
}

CREATE_KRZYMOD(viewGTA2, "GTA II", 3.5f, 5) {
	if (info.execType == INITIAL) {
		KRZYMOD_CONTROL_CVAR(cl_skip_player_render_in_main_view, 0);
	}
	if (info.execType == PROCESS_MOVEMENT && info.preCall) {
		auto moveData = (CMoveData *)info.data;
		
		auto wishDir = Vector(moveData->m_flSideMove, moveData->m_flForwardMove);
		float moveForce = wishDir.Length();
		float moveAng = RAD2DEG(atan2f(wishDir.y, wishDir.x));

		moveData->m_flForwardMove = moveForce;
		moveData->m_flSideMove = 0;

		if (moveForce > 0) {
			QAngle finalAngles;
			finalAngles.y = moveAng;
			finalAngles.x = 0;
			if (moveData->m_nButtons & IN_DUCK) finalAngles.x = 89;
			if (moveData->m_vecVelocity.z > 20) finalAngles.x = -89;

			engine->SetAngles(GET_SLOT(), finalAngles);
			moveData->m_vecAngles = finalAngles;
			moveData->m_vecViewAngles = finalAngles;
			moveData->m_vecAbsViewAngles = finalAngles;
		}
	}
	if (info.execType == OVERRIDE_CAMERA) {
		auto viewSetup = (CViewSetup *)info.data;
		void *player = client->GetPlayer(1);
		if (!player) return;

		auto pPos = client->GetAbsOrigin(player);

		viewSetup->origin = pPos + Vector(0, 0, 400);
		viewSetup->angles = {90.0f, 90.0f, 0};
	}
}

CREATE_KRZYMOD(gameLaserEyes, "Laser Eyes", 2.5f, 0) {

	if (info.execType == INITIAL) {
		KRZYMOD_CONTROL_CVAR(sv_player_collide_with_laser, 0);
	}

	if (info.execType == OVERRIDE_CAMERA) {
		void *player = client->GetPlayer(1);
		if (!player) return;

		Vector pos = client->GetAbsOrigin(player) + client->GetViewOffset(player);
		QAngle angles = engine->GetAngles(0);

		Vector left = {-sinf(DEG2RAD(angles.y)), cosf(DEG2RAD(angles.y)), 0};

		Vector leftEye = pos + left * 8.0;
		Vector rightEye = pos - left * 8.0;

		char buff[256];
		snprintf(buff, sizeof(buff), "local pos = [Vector(%.03f,%.03f,%.03f), Vector(%.03f,%.03f,%.03f)];local ang=Vector(%.03f,%.03f,%.03f);", leftEye.x, leftEye.y, leftEye.z, rightEye.x, rightEye.y, rightEye.z, angles.x, angles.y, angles.z);
		std::string strPos = buff;

		std::string script = R"NUT(
			local name = "__krzymod_laser_eye_";
			local relay = null;
			if(!(relay = Entities.FindByName(null, name+"relay"))){
				for(local i = 0; i < 2; i++){
					local e = Entities.CreateByClassname("env_portal_laser");
					e.__KeyValueFromString("targetname", name+i);
					EntFireByHandle(e, "TurnOn", "", 0, null, null);
				}
				relay = Entities.CreateByClassname("logic_relay");
				relay.__KeyValueFromString("targetname", name+"relay");
				EntFireByHandle(relay, "AddOutput", "OnTrigger __krzymod_laser_eye_*,Kill,,0.2,-1", 0, null, null);
			}
			for(local i = 0; i < 2; i++){
				local e = Entities.FindByName(null, name+i);
				e.SetOrigin(pos[i]);
				e.SetAngles(ang.x,ang.y,ang.z);
			}
			EntFireByHandle(relay, "CancelPending", "", 0, null, null);
			EntFireByHandle(relay, "Trigger", "", 0, null, null);
		)NUT";

		script = strPos + script;
		vscript->RunScript(script.c_str());
	}
}

CREATE_KRZYMOD_INSTANT(visualLoudNoise, "Loud Music (!!!)", 0) {
	engine->ExecuteCommand("snd_musicvolume 1");
	engine->ExecuteCommand("playvol music/sp_a2_bts2_x1.wav 1");
}

CREATE_KRZYMOD_INSTANT(gameSpawnGelWater, "Spawn Water Blob", 0) {
	engine->ExecuteCommand("ent_create_paint_bomb_erase");
}

CREATE_KRZYMOD_INSTANT(gameSpawnGelJump, "Spawn Repulsion Blob", 0) {
	engine->ExecuteCommand("ent_create_paint_bomb_jump");
}

CREATE_KRZYMOD_INSTANT(gameSpawnGelSpeed, "Spawn Propulsion Blob", 0) {
	engine->ExecuteCommand("ent_create_paint_bomb_speed");
}

CREATE_KRZYMOD_INSTANT(gameSpawnGelPortal, "Spawn Conversion Blob", 0) {
	engine->ExecuteCommand("ent_create_paint_bomb_portal");
}

CREATE_KRZYMOD_INSTANT(gameSpawnTurret, "Spawn Turret", 0) {
	engine->ExecuteCommand("ent_create npc_portal_turret_floor");
}

CREATE_KRZYMOD_INSTANT(gameSpawnCompanionCube, "Spawn Companion Cube", 0) {
	engine->ExecuteCommand("ent_create_portal_companion_cube");
}

CREATE_KRZYMOD_INSTANT(gameGivePercent, "Give%%%%", 0) {
	for (int i=0;i<30;i++) engine->ExecuteCommand("give prop_weighted_cube");
}

CREATE_KRZYMOD(moveDelayInput, "Delayed Inputs", 1.5f, 5) {
	const int LAG_COUNT = 30;
	static int oldButtons[LAG_COUNT];
	static Vector oldWishDirs[LAG_COUNT];
	static QAngle oldAngles[LAG_COUNT];
	QAngle angles = engine->GetAngles(GET_SLOT());
	if (info.execType == INITIAL) {
		for (int i = 0; i < LAG_COUNT; i++) {
			oldWishDirs[i] = {0,0,0};
			oldAngles[i] = angles;
			oldButtons[i] = 0;
		}
	}
	if (info.execType == PROCESS_MOVEMENT && info.preCall) {
		auto moveData = (CMoveData *)info.data;

		for (int i = 0; i < LAG_COUNT-1; i++) {
			oldWishDirs[i] = oldWishDirs[i + 1];
			oldButtons[i] = oldButtons[i + 1];
		}
		oldWishDirs[LAG_COUNT - 1] = {moveData->m_flSideMove, moveData->m_flForwardMove};
		oldButtons[LAG_COUNT - 1] = moveData->m_nButtons;

		moveData->m_flSideMove = oldWishDirs[0].x;
		moveData->m_flForwardMove = oldWishDirs[0].y;
		moveData->m_nButtons = oldButtons[0];

		moveData->m_vecViewAngles = moveData->m_vecAbsViewAngles = moveData->m_vecAngles = oldAngles[0];
	}
	if (info.execType == OVERRIDE_CAMERA) {
		for (int i = 0; i < LAG_COUNT - 1; i++) {
			oldAngles[i] = oldAngles[i + 1];
		}
		oldAngles[LAG_COUNT - 1] = engine->GetAngles(GET_SLOT());

		auto viewSetup = (CViewSetup *)info.data;

		viewSetup->angles = oldAngles[0];
	}
}

CREATE_KRZYMOD(gameSpaceCoreOrbit, "Space Core Orbit", 1.6f, 0) {
	static bool precachedLastFrame = false;
	static int lastSound = 0;
	if (info.execType == ENGINE_TICK && info.preCall) {
		void *player = server->GetPlayer(1);
		if (!player) return;

		// the pain i have to get through to make these models function properly

		if (!precachedLastFrame) {
			bool spaceCoreExists = false;

			for (int i = 0; i < Offsets::NUM_ENT_ENTRIES; ++i) {
				void *ent = server->m_EntPtrArray[i].m_pEntity;
				if (!ent) continue;

				const char *entName = server->GetEntityName(ent);
				if (entName == nullptr) continue;
				if (std::strstr(entName, "__krzymod_space_core")) {
					spaceCoreExists = true;
					break;
				}
			}

			if (!spaceCoreExists) {
				engine->ExecuteCommand("prop_dynamic_create npcs/personality_sphere/personality_sphere_skins");
				engine->ExecuteCommand("ent_fire dynamic_prop kill");
				precachedLastFrame = true;
			}
		}
		else {
			precachedLastFrame = false;
		}

		if (!precachedLastFrame) {
			vscript->RunScript(R"NUT(
				local name = "__krzymod_space_core_";
				local relay = null;
				if(!(relay = Entities.FindByName(null, name+"relay"))){
					for(local i = 0; i < 20; i++){
						local e = Entities.CreateByClassname("prop_dynamic");
						e.SetModel("models/npcs/personality_sphere/personality_sphere_skins.mdl");
						e.__KeyValueFromString("targetname", name+i);
						e.__KeyValueFromString("DefaultAnim", "sphere_tuberide_long_twirl");
						e.__KeyValueFromString("skin", "2");
						EntFireByHandle(e, "TurnOn", "", 0, null, null);
						EntFireByHandle(e, "SetAnimation", "sphere_tuberide_long_twirl", i*0.5, null, null);
					}
					relay = Entities.CreateByClassname("logic_relay");
					relay.__KeyValueFromString("targetname", name+"relay");
					EntFireByHandle(relay, "AddOutput", "OnTrigger __krzymod_space_core_*,Kill,,0.2,-1", 0, null, null);
				}
				for(local i = 0; i < 20; i++){
					local e = Entities.FindByName(null, name+i);
					local ppos = GetPlayer().GetOrigin();
					local xang = i*14.35 + Time();
					local yang = i*16.0 + Time();
					if(i%2==0)xang *= -1.0;
					e.SetOrigin(ppos + Vector(cos(xang)*69,sin(xang)*69,64+64*sin(yang)));
				}
				EntFireByHandle(relay, "CancelPending", "", 0, null, null);
				EntFireByHandle(relay, "Trigger", "", 0, null, null);
			)NUT");
		}


		// playing random space core sound every 0.25 second
		if (lastSound <= 0) {
			std::string sound = "playvol vo/core01/";
			if (Math::RandomNumber(0.0f, 1.0f) > 0.5f) {
				sound += Utils::ssprintf("space%02d.wav 0.5", Math::RandomNumber(1, 24));
			} else {
				sound += Utils::ssprintf("babbleb%02d.wav 0.5", Math::RandomNumber(1, 35));
			}
			engine->ExecuteCommand(sound.c_str());
			lastSound += 15;
		} else {
			lastSound--;
		}
	}
}

CREATE_KRZYMOD_INSTANT(gameIgniteAll, "Ignite Everything", 0) {
	engine->ExecuteCommand("ent_fire * ignite");
}

CREATE_KRZYMOD_SIMPLE(ENGINE_TICK, gameButterfingers, "Butterfingers!", 2.5f, 0) {
	static bool pressedUseLastTick = false;
	if (pressedUseLastTick) {
		engine->ExecuteCommand("-use");
	}

	void *player = server->GetPlayer(1);
	if (!player) return;

	auto m_hUseEntity = *reinterpret_cast<int *>((uintptr_t)player + Offsets::m_hUseEntity);
	bool isHoldingSth = m_hUseEntity != 0xFFFFFFFF;

	if (isHoldingSth && Math::RandomNumber(0.0f,1.0f) > 0.95f) {
		engine->ExecuteCommand("+use");
		pressedUseLastTick = true;
	}
}
