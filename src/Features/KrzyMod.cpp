#include "KrzyMod.hpp"

#include "Features/EntityList.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Scheme.hpp"
#include "Modules/Server.hpp"
#include "Modules/Surface.hpp"
#include "Event.hpp"

#include <random>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <vector>

KrzyMod krzyMod;

Variable sar_krzymod_enabled("sar_krzymod_enabled", "0", "Enables KrzyMod (TM).\n");
Variable sar_krzymod_delaytime("sar_krzymod_delaytime", "30", 1.0f, 3600.0f, "The delay between KrzyMod modifiers, in seconds.\n");
Variable sar_krzymod_font("sar_krzymod_font", "92", 0, "Change font of KrzyMod.\n");

KrzyModifier::KrzyModifier(std::string name, std::string displayName, float executeTime, void *function)
	: name(name), displayName(displayName), executeTime(executeTime), function(function) {
	krzyMod.AddModifier(this);
}

void ActiveKrzyModifier::Execute(KrzyModExecType type, bool preCall, void* data = nullptr) {
	bool lastCall = time + 1.0f / 60.0f > endTime;
	((void (*)(KrzyModExecInfo))modifier->function)({type, preCall, lastCall, time, endTime, data});
}

KrzyModConvarControl::KrzyModConvarControl(Variable var, std::string value, float time, KrzyModifier* parent)
	: convar(var), value(value), remainingTime(time), parentModifier(parent){
	originalValue = var.GetString();
}

void KrzyModConvarControl::Update() {
	remainingTime -= 1.0f / 60.0f;

	if (remainingTime <= 0) {
		remainingTime = 0;
		convar.SetValue(originalValue.c_str());
	} else {
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

bool KrzyMod::IsEnabled() {
	return sar_krzymod_enabled.GetBool() && sv_cheats.GetBool() && engine->isRunning() && !engine->IsGamePaused();
}

bool KrzyMod::GetCurrentSize(int &w, int &h) {
	return false;
}

ON_EVENT(PRE_TICK) {krzyMod.Update();}
void KrzyMod::Update() {
	if (!IsEnabled()) {
		timer = 0; 
		Stop();
		return;
	}

	IncreaseTimer(1);

	// execute engine tick dependent modifiers and update their timers
	for (ActiveKrzyModifier &mod : activeModifiers) {
		mod.Execute(ENGINE_TICK, true);
		mod.time += 1.0f / 60.0f;
	}

	//clear modifiers that expired
	activeModifiers.remove_if([](const ActiveKrzyModifier &mod) -> bool {
		return mod.time > mod.endTime;
	});

	// update cvar controllers
	for (KrzyModConvarControl &control : convarControllers) {
		bool hasActiveMod = false;
		for (ActiveKrzyModifier &mod : activeModifiers) {
			if (mod.modifier == control.parentModifier) {
				hasActiveMod = true;
				break;
			}
		}
		if (!hasActiveMod) control.remainingTime = 0;
		control.Update();
	}
	convarControllers.remove_if([](const KrzyModConvarControl &con) -> bool {
		return con.remainingTime <= 0;
	});

	// adding a new modifier
	if (timer >= 1) {
		auto mod = GetNextModifier();
		ActivateModifier(mod);
		timer = 0;
	}
}

void KrzyMod::Stop() {
	if (convarControllers.size() > 0) {
		for (KrzyModConvarControl &control : convarControllers) {
			control.remainingTime = 0;
			control.Update();
		}
		convarControllers.clear();
	}
	if (activeModifiers.size() > 0) {
		for (KrzyModifier *mod : modifiers) {
			DisableModifier(mod);
		}
		activeModifiers.clear();
	}
	
}


void KrzyMod::IncreaseTimer(float multiplier) {
	timer += (1.0f / (60.0f * sar_krzymod_delaytime.GetFloat())) * multiplier;
}

void KrzyMod::ActivateModifier(KrzyModifier *mod) {
	float endTime = mod->executeTime == 0 ? 2.5f : mod->executeTime; 
	ActiveKrzyModifier activeMod = {mod, 0, endTime * sar_krzymod_delaytime.GetFloat()};
	activeModifiers.push_back(activeMod);
	activeMod.Execute(INITIAL, true, mod);
}

void KrzyMod::DisableModifier(KrzyModifier *modifier) {
	for (ActiveKrzyModifier &mod : activeModifiers) {
		if (mod.modifier != modifier) continue;
		mod.time = mod.endTime;
		mod.Execute(ENGINE_TICK, false);
	}
	activeModifiers.remove_if([modifier](const ActiveKrzyModifier &mod) -> bool {
		return mod.modifier == modifier;
	});
}

void KrzyMod::AddModifier(KrzyModifier* modifier) {
	modifiers.push_back(modifier);
}

KrzyModifier* KrzyMod::GetNextModifier() {
	nextModifierID++;
	if (nextModifierID >= modifiers.size()) {
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(modifiers.begin(), modifiers.end(), g);
		nextModifierID = 0;
	}
	return modifiers[nextModifierID];
}

void KrzyMod::AddConvarController(Variable convar, std::string newValue, float time, KrzyModifier* parent) {
	for (KrzyModConvarControl &control : convarControllers) {
		if (control.convar.ThisPtr() == convar.ThisPtr()) {
			control.value = newValue;
			control.remainingTime = time;
			control.parentModifier = parent;
			return;
		}
	}
	convarControllers.push_back(KrzyModConvarControl(convar, newValue, time, parent));
	//console->Print("Convar controller for cvar %s=%f added for %fs.\n", convar.ThisPtr()->m_pszName, newValue, time);
}


void KrzyMod::InvokeProcessMovementEvents(CMoveData *moveData, bool preCall) {
	if (!IsEnabled()) return;
	for (ActiveKrzyModifier &mod : activeModifiers) {
		mod.Execute(PROCESS_MOVEMENT, preCall, moveData);
	}
}

void KrzyMod::InvokeOverrideCameraEvents(CViewSetup *view) {
	if (!IsEnabled()) return;
	for (ActiveKrzyModifier &mod : activeModifiers) {
		mod.Execute(OVERRIDE_CAMERA, true, view);
	}
}



void KrzyMod::Paint(int slot) {
	if (!sar_krzymod_enabled.GetBool() || !sv_cheats.GetBool()) return;

	auto font = scheme->GetDefaultFont() + sar_krzymod_font.GetInt();

	int lineHeight = surface->GetFontHeight(font);

	void *player = server->GetPlayer(slot + 1);
	if (!player) return;

	// draw modifiers
	for (ActiveKrzyModifier &mod : activeModifiers) {
		bool lastCall = mod.time + 1.0f / 60.0f > mod.endTime;
		((void (*)(KrzyModExecInfo))mod.modifier->function)({HUD_PAINT, true, lastCall, mod.time, mod.endTime});
	}

	int xScreen, yScreen;
#if _WIN32
	engine->GetScreenSize(xScreen, yScreen);
#else
	engine->GetScreenSize(nullptr, xScreen, yScreen);
#endif

	surface->DrawRect(Color(0, 0, 0, 192), 0, 0, xScreen, 30);

	surface->DrawRect(Color(0, 111, 222, 255), 0, 0, xScreen * timer, 30);

	int fontPos = 0;
	for (ActiveKrzyModifier &mod : activeModifiers) {
		const char *displayName = mod.modifier->displayName.c_str();
		int textWidth = surface->GetFontLength(font, displayName);
		int textX = xScreen - 30 - textWidth;
		int textY = 150 + fontPos * (lineHeight + 30);
		surface->DrawTxt(font, textX, textY, Color(255, 255, 255, 255), displayName);

		if (mod.modifier->executeTime > 0) {
			surface->DrawRect(Color(50, 50, 50, 255), textX, textY + lineHeight, textX + textWidth, textY + lineHeight + 10);
			surface->DrawRect(Color(0, 111, 222, 255), textX, textY + lineHeight, textX + textWidth * (1.0f - mod.time / mod.endTime), textY + lineHeight + 10);
		}

		fontPos++;
	}
}

CON_COMMAND(sar_krzymod_addmod, "sar_krzymod_activate [modname] - activate mod with given name\n") {
	if (args.ArgC() != 2) {
		return console->Print(sar_krzymod_addmod.ThisPtr()->m_pszHelpString);
	}
	for (KrzyModifier* mod : krzyMod.modifiers) {
		if (mod->name.compare(args[1]) == 0) {
			krzyMod.ActivateModifier(mod);
			console->Print("Activated krzymod \"%s\".\n", args[1]);
			return;
		}
	}
	console->Print("Cannot find krzymod \"%s\".\n", args[1]);
}

CON_COMMAND(sar_krzymod_stopmod, "sar_krzymod_stopmod [modname] - stops all instances of a mod with given name\n") {
	if (args.ArgC() != 2) {
		return console->Print(sar_krzymod_addmod.ThisPtr()->m_pszHelpString);
	}
	for (KrzyModifier *mod : krzyMod.modifiers) {
		if (mod->name.compare(args[1]) == 0) {
			krzyMod.DisableModifier(mod);
			console->Print("Stopped krzymod \"%s\".\n", args[1]);
			return;
		}
	}
	console->Print("Cannot find krzymod \"%s\".\n", args[1]);
}










CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, moveWStuck, "Help My W Is Stuck", 2.5f) {
	if (!info.preCall) return;
	auto moveData = (CMoveData *)info.data;
	moveData->m_flForwardMove += 175.0f;
}

CREATE_KRZYMOD_SIMPLE(OVERRIDE_CAMERA, viewQuakeFov, "Quake FOV", 3.5f) {
	auto viewSetup = (CViewSetup *)info.data;
	viewSetup->fov = 160;
	viewSetup->fovViewmodel = 120;
}

CREATE_KRZYMOD_SIMPLE(OVERRIDE_CAMERA, viewUpsideDown, "Upside Down View", 2.5f) {
	auto viewSetup = (CViewSetup *)info.data;
	viewSetup->angles.z += 180;
}

CREATE_KRZYMOD_SIMPLE(ENGINE_TICK, metaFasterDelay, "x4 KrzyMod Speed", 1.0f) {
	krzyMod.IncreaseTimer(3);
}

CREATE_KRZYMOD_SIMPLE(HUD_PAINT, funSnapchatMode, "Snapchat Mode", 2.5f) {
	int xScreen, yScreen;
#if _WIN32
	engine->GetScreenSize(xScreen, yScreen);
#else
	engine->GetScreenSize(nullptr, xScreen, yScreen);
#endif

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

CREATE_KRZYMOD_INSTANT(playerKill, "Kill Player") {
	engine->ExecuteCommand("kill");
}

CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, moveStickyGround, "Sticky Ground", 3.5f) {
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

CREATE_KRZYMOD(moveInverseControl, "Inverse Controls", 3.5f) {
	if (info.execType == INITIAL) {
		Variable yaw = Variable("m_yaw");
		krzyMod.AddConvarController(yaw, std::to_string(yaw.GetFloat() * -1), info.endTime, (KrzyModifier *)info.data);
		Variable pitch = Variable("m_pitch");
		krzyMod.AddConvarController(pitch, std::to_string(pitch.GetFloat() * -1), info.endTime, (KrzyModifier *)info.data);
	}
	if (info.execType == PROCESS_MOVEMENT) {
		auto moveData = (CMoveData *)info.data;

		moveData->m_flForwardMove *= -1;
		moveData->m_flSideMove *= -1;
	}
}

CREATE_KRZYMOD_SIMPLE(HUD_PAINT, funDvdLogo, "DVD Logo", 4.5f) {
	int xScreen, yScreen;
#if _WIN32
	engine->GetScreenSize(xScreen, yScreen);
#else
	engine->GetScreenSize(nullptr, xScreen, yScreen);
#endif

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
				bool v = abs((pos.y - fmax(0, 0.5 - abs(pow(pos.x, 0.45) - 0.695) * 4.6))) < 0.155 && pos.y > 0.056 && pos.x < 0.7 && pos.x > 0;
				// using modulo to repeat D's
				bool xNot0 = pos.x > 0; // idfk, that first d is glitching, probably because of modulo being annoying
				pos.x = fmod(pos.x, 0.615f);
				// D's
				bool d1 = ((pos - Vector(0.09, 0.3)) * Vector(0.89, 0.82)).Length() < 0.2;
				bool d2 = ((pos - Vector(0.09, 0.3)) * Vector(0.89, 0.82)).Length() < 0.12;
				bool d = xNot0 && ((d1 && !d2 && pos.x > 0.07) || (pos.x < 0.09 && pos.y > 0.2 && pos.y < 0.544));
				
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

CREATE_KRZYMOD_SIMPLE(INITIAL, uiHideCrosshair, "Hide Crosshair", 3.5f) {
	KRZYMOD_CONTROL_CVAR(cl_drawhud, 0);
}

CREATE_KRZYMOD(playerLaunchRandom, "Launch Player Randomly", 0.0f) {
	static bool executed = false;
	if (info.execType == INITIAL) executed = false;
	if (info.execType == PROCESS_MOVEMENT && !info.preCall && !executed) {
		auto moveData = (CMoveData *)info.data;
		moveData->m_vecVelocity += Vector(
			Math::RandomNumber(-1.0f, 1.0f), 
			Math::RandomNumber(-1.0f, 1.0f), 
			Math::RandomNumber(-1.0f, 1.0f)
		).Normalize() * Math::RandomNumber(500.0f,1500.0f);
		executed = true;
	}
}

CREATE_KRZYMOD(playerLaunchUp, "Launch Player Up", 0.0f) {
	static bool executed = false;
	if (info.execType == INITIAL) executed = false;
	if (info.execType == PROCESS_MOVEMENT && !info.preCall && !executed) {
		auto moveData = (CMoveData *)info.data;
		moveData->m_vecVelocity += Vector(0,0,1000.0f);
		executed = true;
	}
}

CREATE_KRZYMOD(playerReverseVel, "Invert Velocity", 0.0f) {
	static bool executed = false;
	if (info.execType == INITIAL) executed = false;
	if (info.execType == PROCESS_MOVEMENT && !info.preCall && !executed) {
		auto moveData = (CMoveData *)info.data;
		moveData->m_vecVelocity = moveData->m_vecVelocity * - 1.0f;
		executed = true;
	}
}

CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, moveAirlock, "No Air Control", 3.5f) {
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

CREATE_KRZYMOD_SIMPLE(INITIAL, moveAbh, "ABH", 3.5f) {
	KRZYMOD_CONTROL_CVAR(sar_duckjump, 1);
	KRZYMOD_CONTROL_CVAR(sar_jumpboost, 1);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, moveNoAirlock, "Better Air Control", 3.5f) {
	KRZYMOD_CONTROL_CVAR(sar_aircontrol, 1);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gameSmallTimescale, "0.5x Timescale", 1.5f) {
	KRZYMOD_CONTROL_CVAR(host_timescale, 0.5);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gameLargeTimescale, "2x Timescale", 3.5f) {
	KRZYMOD_CONTROL_CVAR(host_timescale, 2);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gameSmallPhysscale, "0.5x Physics Timescale", 3.5f) {
	KRZYMOD_CONTROL_CVAR(phys_timescale, 0.5);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gameLargePhysscale, "2x Physics Timescale", 3.5f) {
	KRZYMOD_CONTROL_CVAR(phys_timescale, 2);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, gameMaxBounciness, "Maximum Repulsiveness", 3.5f) {
	KRZYMOD_CONTROL_CVAR(bounce_paint_min_speed, 3600);
}

CREATE_KRZYMOD(moveAutojump, "Autojump", 3.5f) {
	if (info.execType == INITIAL) {
		KRZYMOD_CONTROL_CVAR(sar_autojump, 1);
	}
	if(info.execType == ENGINE_TICK) {
		engine->ExecuteCommand(info.lastCall ? "-jump" : "+jump");
	}
}

CREATE_KRZYMOD_INSTANT(gameRemovePaint, "Remove All Paint") {
	engine->ExecuteCommand("removeallpaint");
}

CREATE_KRZYMOD(moveDrunk, "Drunk", 3.5f) {

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

CREATE_KRZYMOD_SIMPLE(INITIAL, moveMarioJump, "Mario Jump", 3.5f) {
	KRZYMOD_CONTROL_CVAR(sar_jump_height, 150);
}

CREATE_KRZYMOD_SIMPLE(INITIAL, moveStanleyParable, "The Stanley Parable", 3.5f) {
	KRZYMOD_CONTROL_CVAR(sar_jump_height, 0);
}