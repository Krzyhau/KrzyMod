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

KrzyMod::KrzyMod()
	: Hud(HudType_InGame | HudType_Paused, true) {
}

bool KrzyMod::ShouldDraw() {
	return sar_krzymod_enabled.GetBool() && Hud::ShouldDraw() && sv_cheats.GetBool();
}

bool KrzyMod::GetCurrentSize(int &w, int &h) {
	return false;
}

ON_EVENT(PRE_TICK) {krzyMod.Update();}
void KrzyMod::Update() {
	if (!sar_krzymod_enabled.GetBool() || !sv_cheats.GetBool() || !engine->isRunning() || engine->IsGamePaused()) return;

	IncreaseTimer(1);

	// execute engine tick dependent modifiers
	for (ActiveKrzyModifier &mod : activeModifiers) {
		mod.Execute(ENGINE_TICK, true);
		mod.time += 1.0f / 60.0f;
	}

	//clear modifiers that expired
	activeModifiers.remove_if([](const ActiveKrzyModifier &mod) -> bool {
		return mod.time > mod.endTime;
	});

	// adding a new modifier
	if (timer >= 1) {
		auto mod = GetNextModifier();
		ActivateModifier(mod);
		timer = 0;
	}
}


void KrzyMod::IncreaseTimer(float multiplier) {
	timer += (1.0f / (60.0f * sar_krzymod_delaytime.GetFloat())) * multiplier;
}

void KrzyMod::ActivateModifier(KrzyModifier *mod) {
	float endTime = mod->executeTime == 0 ? 2.0f : mod->executeTime; 
	ActiveKrzyModifier activeMod = {mod, 0, endTime * sar_krzymod_delaytime.GetFloat()};
	activeModifiers.push_back(activeMod);
	if (mod->executeTime == 0) {
		activeMod.Execute(INSTANT,true);
	}
}

void KrzyMod::DisableModifier(KrzyModifier *modifier) {
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

void KrzyMod::InvokeProcessMovementEvents(CMoveData *moveData, bool preCall) {
	for (ActiveKrzyModifier &mod : activeModifiers) {
		mod.Execute(PROCESS_MOVEMENT, preCall, moveData);
	}
}

void KrzyMod::InvokeOverrideCameraEvents(CViewSetup *view) {
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










CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, wStuck, "Help my W is stuck", 4.0f) {
	if (!info.preCall) return;
	auto moveData = (CMoveData *)info.data;
	moveData->m_flForwardMove += 175.0f;
}

CREATE_KRZYMOD_SIMPLE(OVERRIDE_CAMERA, quakefov, "Quake FOV", 4.0f) {
	auto viewSetup = (CViewSetup *)info.data;
	viewSetup->fov = 160;
	viewSetup->fovViewmodel = 120;
}

CREATE_KRZYMOD_SIMPLE(OVERRIDE_CAMERA, upsideDownView, "Upside down view", 2.0f) {
	auto viewSetup = (CViewSetup *)info.data;
	viewSetup->angles.z += 180;
}

CREATE_KRZYMOD_SIMPLE(ENGINE_TICK, fasterDelay, "Mod timer x4 faster", 1.0f) {
	krzyMod.IncreaseTimer(3);
}

CREATE_KRZYMOD_SIMPLE(HUD_PAINT, snapchatMode, "Snapchat mode", 4.0f) {
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

CREATE_KRZYMOD_INSTANT(kill, "Kill") {
	
}

CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, stickyGround, "Sticky ground", 4.0f) {
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

CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, inverseControl, "Inverse controls", 4.0f) {
	auto moveData = (CMoveData *)info.data;

	moveData->m_flForwardMove *= -1;
	moveData->m_flSideMove *= -1;
}

CREATE_KRZYMOD_SIMPLE(HUD_PAINT, dvdLogo, "DVD Logo", 5.0f) {
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

		const int texWidth = 400;
		const int texHeight = 200;

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