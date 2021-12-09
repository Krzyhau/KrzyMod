#include "Client.hpp"

#include "Command.hpp"
#include "Console.hpp"
#include "Engine.hpp"
#include "Event.hpp"
#include "Features/OffsetFinder.hpp"
#include "Features/KrzyMod.hpp"
#include "Game.hpp"
#include "Interface.hpp"
#include "Offsets/Offsets.hpp"
#include "Server.hpp"
#include "Utils.hpp"

#include <cstdarg>
#include <cstdint>
#include <cstring>

Variable cl_showpos;
Variable cl_sidespeed;
Variable cl_backspeed;
Variable cl_forwardspeed;
Variable in_forceuser;
Variable crosshairVariable;
Variable cl_fov;
Variable prevent_crouch_jump;
Variable r_portaltestents;

REDECL(Client::LevelInitPreEntity);
REDECL(Client::CInput_CreateMove);
REDECL(Client::GetButtonBits);
REDECL(Client::OverrideView);

MDECL(Client::GetAbsOrigin, Vector, C_m_vecAbsOrigin);
MDECL(Client::GetAbsAngles, QAngle, C_m_angAbsRotation);
MDECL(Client::GetLocalVelocity, Vector, C_m_vecVelocity);
MDECL(Client::GetViewOffset, Vector, C_m_vecViewOffset);


void *Client::GetPlayer(int index) {
	return this->GetClientEntity(this->s_EntityList->ThisPtr(), index);
}
void Client::CalcButtonBits(int nSlot, int &bits, int in_button, int in_ignore, kbutton_t *button, bool reset) {
	auto pButtonState = &button->m_PerUser[nSlot];
	if (pButtonState->state & 3) {
		bits |= in_button;
	}

	int clearmask = ~2;
	if (in_ignore & in_button) {
		clearmask = ~3;
	}

	if (reset) {
		pButtonState->state &= clearmask;
	}
}

bool Client::ShouldDrawCrosshair() {
	if (!crosshairVariable.GetBool()) {
		crosshairVariable.SetValue(1);
		auto value = this->ShouldDraw(this->g_HUDQuickInfo->ThisPtr());
		crosshairVariable.SetValue(0);
		return value;
	}

	return this->ShouldDraw(this->g_HUDQuickInfo->ThisPtr());
}

void Client::Chat(TextColor color, const char *fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	char data[1024];
	vsnprintf(data, sizeof(data), fmt, argptr);
	va_end(argptr);
	client->ChatPrintf(client->g_HudChat->ThisPtr(), 0, 0, "%c%s", color, data);
}

void Client::QueueChat(TextColor color, const char *fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	char data[1024];
	vsnprintf(data, sizeof data, fmt, argptr);
	va_end(argptr);
	this->chatQueue.push_back(std::pair(color, std::string(data)));
}

void Client::FlushChatQueue() {
	for (auto &s : this->chatQueue) {
		this->Chat(s.first, "%s", s.second.c_str());
	}
	this->chatQueue.clear();
}

void Client::SetMouseActivated(bool state) {
	if (state) {
		this->IN_ActivateMouse(g_Input->ThisPtr());
	} else {
		this->IN_DeactivateMouse(g_Input->ThisPtr());
	}
}

CMStatus Client::GetChallengeStatus() {
	auto player = client->GetPlayer(1);
	if (!player) {
		return CMStatus::NONE;
	}

	int bonusChallenge = *(int *)((uintptr_t)player + Offsets::m_iBonusChallenge);

	if (bonusChallenge) {
		return CMStatus::CHALLENGE;
	} else if (sv_bonus_challenge.GetBool()) {
		return CMStatus::WRONG_WARP;
	} else {
		return CMStatus::NONE;
	}
}

int Client::GetSplitScreenPlayerSlot(void *entity) {
	for (auto i = 0; i < Offsets::MAX_SPLITSCREEN_PLAYERS; ++i) {
		if (client->GetPlayer(i + 1) == entity) {
			return i;
		}
	}
	return 0;
}

// CHLClient::LevelInitPreEntity
DETOUR(Client::LevelInitPreEntity, const char *levelName) {
	client->lastLevelName = std::string(levelName);
	return Client::LevelInitPreEntity(thisptr, levelName);
}

// CInput::CreateMove
DETOUR(Client::CInput_CreateMove, int sequence_number, float input_sample_frametime, bool active) {
	auto result = Client::CInput_CreateMove(thisptr, sequence_number, input_sample_frametime, active);

	return result;
}

// CInput::GetButtonBits
DETOUR(Client::GetButtonBits, bool bResetState) {
	auto bits = Client::GetButtonBits(thisptr, bResetState);

	return bits;
}

DETOUR(Client::OverrideView, CViewSetup *m_View) {
	krzyMod.InvokeOverrideCameraEvents(m_View);
	return Client::OverrideView(thisptr, m_View);
}

bool Client::Init() {
	bool readJmp = false;

	this->g_ClientDLL = Interface::Create(this->Name(), "VClient016");
	this->s_EntityList = Interface::Create(this->Name(), "VClientEntityList003", false);

	if (this->g_ClientDLL) {
		this->GetAllClasses = this->g_ClientDLL->Original<_GetAllClasses>(Offsets::GetAllClasses, readJmp);

		this->g_ClientDLL->Hook(Client::LevelInitPreEntity_Hook, Client::LevelInitPreEntity, Offsets::LevelInitPreEntity);

		auto leaderboard = Command("+leaderboard");
		if (!!leaderboard) {
			using _GetHud = void *(__cdecl *)(int unk);
			using _FindElement = void *(__rescall *)(void *thisptr, const char *pName);

			auto cc_leaderboard_enable = (uintptr_t)leaderboard.ThisPtr()->m_pCommandCallback;
			auto GetHud = Memory::Read<_GetHud>(cc_leaderboard_enable + Offsets::GetHud);
			auto FindElement = Memory::Read<_FindElement>(cc_leaderboard_enable + Offsets::FindElement);
			auto CHUDChallengeStats = FindElement(GetHud(-1), "CHUDChallengeStats");

			auto CHUDQuickInfo = FindElement(GetHud(-1), "CHUDQuickInfo");

			if (this->g_HUDQuickInfo = Interface::Create(CHUDQuickInfo)) {
				this->ShouldDraw = this->g_HUDQuickInfo->Original<_ShouldDraw>(Offsets::ShouldDraw, readJmp);
			}

			auto CHudChat = FindElement(GetHud(-1), "CHudChat");
			if (this->g_HudChat = Interface::Create(CHudChat)) {
				this->ChatPrintf = g_HudChat->Original<_ChatPrintf>(Offsets::ChatPrintf);
			}
		}

		this->IN_ActivateMouse = this->g_ClientDLL->Original<_IN_ActivateMouse>(Offsets::IN_ActivateMouse, readJmp);
		this->IN_DeactivateMouse = this->g_ClientDLL->Original<_IN_DeactivateMouse>(Offsets::IN_DeactivateMouse, readJmp);

		auto IN_ActivateMouse = this->g_ClientDLL->Original(Offsets::IN_ActivateMouse, readJmp);
		void *g_InputAddr;
#ifndef _WIN32
		if (pluginMain.game->Is(SourceGame_EIPRelPIC)) {
			g_InputAddr = *(void **)(IN_ActivateMouse + 5 + *(uint32_t *)(IN_ActivateMouse + 6) + *(uint32_t *)(IN_ActivateMouse + 12));
		} else
#endif
			g_InputAddr = Memory::DerefDeref<void *>(IN_ActivateMouse + Offsets::g_Input);

		if (g_Input = Interface::Create(g_InputAddr)) {
			g_Input->Hook(Client::GetButtonBits_Hook, Client::GetButtonBits, Offsets::GetButtonBits);

			auto JoyStickApplyMovement = g_Input->Original(Offsets::JoyStickApplyMovement, readJmp);
			Memory::Read(JoyStickApplyMovement + Offsets::KeyDown, &this->KeyDown);
			Memory::Read(JoyStickApplyMovement + Offsets::KeyUp, &this->KeyUp);

			in_forceuser = Variable("in_forceuser");
			if (!!in_forceuser && this->g_Input) {
				this->g_Input->Hook(CInput_CreateMove_Hook, CInput_CreateMove, Offsets::GetButtonBits + 1);
			}
		}

		auto HudProcessInput = this->g_ClientDLL->Original(Offsets::HudProcessInput, readJmp);
		auto GetClientMode = Memory::Read<uintptr_t>(HudProcessInput + Offsets::GetClientMode);
		uintptr_t g_pClientMode;
#ifndef _WIN32
		if (pluginMain.game->Is(SourceGame_EIPRelPIC)) {
			g_pClientMode = GetClientMode + 6 + *(uint32_t *)(GetClientMode + 8) + *(uint32_t *)(GetClientMode + 35);
		} else
#endif
			g_pClientMode = Memory::Deref<uintptr_t>(GetClientMode + Offsets::g_pClientMode);
		void *clientMode = Memory::Deref<void *>(g_pClientMode);

		if (this->g_pClientMode = Interface::Create(clientMode)) {
			this->g_pClientMode->Hook(Client::OverrideView_Hook, Client::OverrideView, Offsets::OverrideView);
		}
	}

	Variable("r_PortalTestEnts").RemoveFlag(FCVAR_CHEAT);

	if (this->s_EntityList) {
		this->GetClientEntity = this->s_EntityList->Original<_GetClientEntity>(Offsets::GetClientEntity, readJmp);
	}

	offsetFinder->ClientSide("CBasePlayer", "m_vecVelocity[0]", &Offsets::C_m_vecVelocity);
	offsetFinder->ClientSide("CBasePlayer", "m_vecViewOffset[0]", &Offsets::C_m_vecViewOffset);
	offsetFinder->ClientSide("CBasePlayer", "m_hGroundEntity", &Offsets::C_m_hGroundEntity);
	offsetFinder->ClientSide("CBasePlayer", "m_iBonusChallenge", &Offsets::m_iBonusChallenge);
	offsetFinder->ClientSide("CPortal_Player", "m_StatsThisLevel", &Offsets::C_m_StatsThisLevel);

	cl_showpos = Variable("cl_showpos");
	cl_sidespeed = Variable("cl_sidespeed");
	cl_forwardspeed = Variable("cl_forwardspeed");
	cl_backspeed = Variable("cl_backspeed");
	prevent_crouch_jump = Variable("prevent_crouch_jump");
	crosshairVariable = Variable("crosshair");
	r_portaltestents = Variable("r_portaltestents");

	// Useful for fixing rendering bugs
	r_portaltestents.RemoveFlag(FCVAR_CHEAT);


	return this->hasLoaded = this->g_ClientDLL && this->s_EntityList;
}
void Client::Shutdown() {
	r_portaltestents.AddFlag(FCVAR_CHEAT);
	Interface::Delete(this->g_ClientDLL);
	Interface::Delete(this->g_pClientMode);
	Interface::Delete(this->s_EntityList);
	Interface::Delete(this->g_Input);
	Interface::Delete(this->g_HUDQuickInfo);
}

Client *client;
