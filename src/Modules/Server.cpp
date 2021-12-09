#include "Server.hpp"

#include "Client.hpp"
#include "Engine.hpp"
#include "Event.hpp"
#include "Features/EntityList.hpp"
#include "Features/OffsetFinder.hpp"
#include "Features/KrzyMod.hpp"
#include "Game.hpp"
#include "Hook.hpp"
#include "Interface.hpp"
#include "Offsets/Offsets.hpp"
#include "Utils.hpp"
#include "Variable.hpp"
#include "PluginMain.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <cfloat>

#define RESET_COOP_PROGRESS_MESSAGE_TYPE "coop-reset"
#define CM_FLAGS_MESSAGE_TYPE "cm-flags"

Variable sv_cheats;
Variable sv_footsteps;
Variable sv_alternateticks;
Variable sv_bonus_challenge;
Variable sv_accelerate;
Variable sv_airaccelerate;
Variable sv_paintairacceleration;
Variable sv_friction;
Variable sv_maxspeed;
Variable sv_stopspeed;
Variable sv_maxvelocity;
Variable sv_gravity;

REDECL(Server::CheckJumpButton);
REDECL(Server::GameFrame);
REDECL(Server::ProcessMovement);


MDECL(Server::GetPortals, int, iNumPortalsPlaced);
MDECL(Server::GetAbsOrigin, Vector, S_m_vecAbsOrigin);
MDECL(Server::GetAbsAngles, QAngle, S_m_angAbsRotation);
MDECL(Server::GetLocalVelocity, Vector, S_m_vecVelocity);
MDECL(Server::GetFlags, int, m_fFlags);
MDECL(Server::GetEFlags, int, m_iEFlags);
MDECL(Server::GetMaxSpeed, float, m_flMaxspeed);
MDECL(Server::GetGravity, float, m_flGravity);
MDECL(Server::GetViewOffset, Vector, S_m_vecViewOffset);
MDECL(Server::GetEntityName, char *, m_iName);
MDECL(Server::GetEntityClassName, char *, m_iClassName);
 

Variable sv_krzymod_jump_height("sv_krzymod_jump_height", "45", 0,99999, "Changes jump height of a player");
Variable sv_krzymod_autojump("sv_krzymod_autojump", "0", "Enables autojump");


void *Server::GetPlayer(int index) {
	return this->UTIL_PlayerByIndex(index);
}
bool Server::IsPlayer(void *entity) {
	return Memory::VMT<bool (*)(void *)>(entity, Offsets::IsPlayer)(entity);
}
bool Server::AllowsMovementChanges() {
	return sv_cheats.GetBool();
}
int Server::GetSplitScreenPlayerSlot(void *entity) {
	// Simplified version of CBasePlayer::GetSplitScreenPlayerSlot
	for (auto i = 0; i < Offsets::MAX_SPLITSCREEN_PLAYERS; ++i) {
		if (server->UTIL_PlayerByIndex(i + 1) == entity) {
			return i;
		}
	}

	return 0;
}
void Server::KillEntity(void *entity) {
	variant_t val = {0};
	val.fieldType = FIELD_VOID;
	void *player = this->GetPlayer(1);
	server->AcceptInput(entity, "Kill", player, player, val, 0);
}

float Server::GetCMTimer() {
	void *player = this->GetPlayer(1);
	if (!player) {
		void *clPlayer = client->GetPlayer(1);
		if (!clPlayer) return 0.0f;
		return *(float *)((uintptr_t)clPlayer + Offsets::C_m_StatsThisLevel + 12);
	}
	return *(float *)((uintptr_t)player + Offsets::S_m_StatsThisLevel + 12);
}

// CGameMovement::CheckJumpButton
DETOUR_T(bool, Server::CheckJumpButton) {
	auto jumped = false;

	if (server->AllowsMovementChanges()) {
		auto mv = *reinterpret_cast<CHLMoveData **>((uintptr_t)thisptr + Offsets::mv);

		float oldGravity = sv_gravity.GetFloat();

		sv_gravity.ThisPtr()->m_fValue += (oldGravity * (sv_krzymod_jump_height.GetFloat() - 45.0f)) / 45.0f;

		if (sv_krzymod_autojump.GetBool() && !server->jumpedLastTime) {
			mv->m_nOldButtons &= ~IN_JUMP;
		}

		server->jumpedLastTime = false;
		server->savedVerticalVelocity = mv->m_vecVelocity[2];

		jumped = Server::CheckJumpButton(thisptr);

		sv_gravity.ThisPtr()->m_fValue = oldGravity;


		if (jumped) {
			server->jumpedLastTime = true;
		}
	} else {
		jumped = Server::CheckJumpButton(thisptr);
	}

	return jumped;
}

// CGameMovement::ProcessMovement
DETOUR(Server::ProcessMovement, void *player, CMoveData *move) {

	unsigned int groundHandle = *(unsigned int *)((uintptr_t)player + Offsets::S_m_hGroundEntity);
	bool grounded = groundHandle != 0xFFFFFFFF;
	int slot = server->GetSplitScreenPlayerSlot(player);
	Event::Trigger<Event::PROCESS_MOVEMENT>({ slot, true });

	krzyMod.InvokeProcessMovementEvents(move, true);
	auto result = Server::ProcessMovement(thisptr, player, move);
	krzyMod.InvokeProcessMovementEvents(move, false);
	return result;
}

// CServerGameDLL::GameFrame
DETOUR(Server::GameFrame, bool simulating)
{
	int host, server, client;
	engine->GetTicks(host, server, client);

	//Event::Trigger<Event::PRE_TICK>({simulating, server});

	auto result = Server::GameFrame(thisptr, simulating);

	//Event::Trigger<Event::POST_TICK>({simulating, server});

	return result;
}

bool Server::Init() {
	this->g_GameMovement = Interface::Create(this->Name(), "GameMovement001");
	this->g_ServerGameDLL = Interface::Create(this->Name(), "ServerGameDLL005");

	if (this->g_GameMovement) {
		this->g_GameMovement->Hook(Server::CheckJumpButton_Hook, Server::CheckJumpButton, Offsets::CheckJumpButton);
		this->g_GameMovement->Hook(Server::ProcessMovement_Hook, Server::ProcessMovement, Offsets::ProcessMovement);
	}

	if (auto g_ServerTools = Interface::Create(this->Name(), "VSERVERTOOLS001")) {
		auto GetIServerEntity = g_ServerTools->Original(Offsets::GetIServerEntity);
#ifndef _WIN32
		if (pluginMain.game->Is(SourceGame_EIPRelPIC)) {
			this->m_EntPtrArray = (CEntInfo *)(GetIServerEntity + 12 + *(uint32_t *)(GetIServerEntity + 14) + *(uint32_t *)(GetIServerEntity + 54) + 4);
		} else
#endif
			Memory::Deref(GetIServerEntity + Offsets::m_EntPtrArray, &this->m_EntPtrArray);

		this->CreateEntityByName = g_ServerTools->Original<_CreateEntityByName>(Offsets::CreateEntityByName);
		this->DispatchSpawn = g_ServerTools->Original<_DispatchSpawn>(Offsets::DispatchSpawn);
		this->SetKeyValueChar = g_ServerTools->Original<_SetKeyValueChar>(Offsets::SetKeyValueChar);
		this->SetKeyValueFloat = g_ServerTools->Original<_SetKeyValueFloat>(Offsets::SetKeyValueFloat);
		this->SetKeyValueVector = g_ServerTools->Original<_SetKeyValueVector>(Offsets::SetKeyValueVector);

		Interface::Delete(g_ServerTools);
	}

	if (this->g_ServerGameDLL) {
		auto Think = this->g_ServerGameDLL->Original(Offsets::Think);
		Memory::Read<_UTIL_PlayerByIndex>(Think + Offsets::UTIL_PlayerByIndex, &this->UTIL_PlayerByIndex);
#ifndef _WIN32
		if (pluginMain.game->Is(SourceGame_EIPRelPIC)) {
			this->gpGlobals = *(CGlobalVars **)((uintptr_t)this->UTIL_PlayerByIndex + 5 + *(uint32_t *)((uintptr_t)UTIL_PlayerByIndex + 7) + *(uint32_t *)((uintptr_t)UTIL_PlayerByIndex + 21));
		} else
#endif
			Memory::DerefDeref<CGlobalVars *>((uintptr_t)this->UTIL_PlayerByIndex + Offsets::gpGlobals, &this->gpGlobals);

		this->GetAllServerClasses = this->g_ServerGameDLL->Original<_GetAllServerClasses>(Offsets::GetAllServerClasses);
		this->IsRestoring = this->g_ServerGameDLL->Original<_IsRestoring>(Offsets::IsRestoring);

		this->g_ServerGameDLL->Hook(Server::GameFrame_Hook, Server::GameFrame, Offsets::GameFrame);
	}

	offsetFinder->ServerSide("CBasePlayer", "m_nWaterLevel", &Offsets::m_nWaterLevel);
	offsetFinder->ServerSide("CBasePlayer", "m_iName", &Offsets::m_iName);
	offsetFinder->ServerSide("CBasePlayer", "m_vecVelocity[0]", &Offsets::S_m_vecVelocity);
	offsetFinder->ServerSide("CBasePlayer", "m_fFlags", &Offsets::m_fFlags);
	offsetFinder->ServerSide("CBasePlayer", "m_flMaxspeed", &Offsets::m_flMaxspeed);
	offsetFinder->ServerSide("CBasePlayer", "m_vecViewOffset[0]", &Offsets::S_m_vecViewOffset);
	offsetFinder->ServerSide("CBasePlayer", "m_hGroundEntity", &Offsets::S_m_hGroundEntity);
	offsetFinder->ServerSide("CBasePlayer", "m_bDucked", &Offsets::m_bDucked);
	offsetFinder->ServerSide("CBasePlayer", "m_flFriction", &Offsets::m_flFriction);
	offsetFinder->ServerSide("CBasePlayer", "m_nTickBase", &Offsets::m_nTickBase);
	offsetFinder->ServerSide("CPortal_Player", "m_hUseEntity", &Offsets::m_hUseEntity);
	offsetFinder->ServerSide("CPortal_Player", "m_InAirState", &Offsets::m_InAirState);
	offsetFinder->ServerSide("CPortal_Player", "m_StatsThisLevel", &Offsets::S_m_StatsThisLevel);

	offsetFinder->ServerSide("CPortal_Player", "iNumPortalsPlaced", &Offsets::iNumPortalsPlaced);
	offsetFinder->ServerSide("CPortal_Player", "m_hActiveWeapon", &Offsets::m_hActiveWeapon);
	offsetFinder->ServerSide("CProp_Portal", "m_bActivated", &Offsets::m_bActivated);
	offsetFinder->ServerSide("CProp_Portal", "m_bIsPortal2", &Offsets::m_bIsPortal2);
	offsetFinder->ServerSide("CWeaponPortalgun", "m_bCanFirePortal1", &Offsets::m_bCanFirePortal1);
	offsetFinder->ServerSide("CWeaponPortalgun", "m_bCanFirePortal2", &Offsets::m_bCanFirePortal2);
	offsetFinder->ServerSide("CWeaponPortalgun", "m_hPrimaryPortal", &Offsets::m_hPrimaryPortal);
	offsetFinder->ServerSide("CWeaponPortalgun", "m_hSecondaryPortal", &Offsets::m_hSecondaryPortal);
	offsetFinder->ServerSide("CWeaponPortalgun", "m_iPortalLinkageGroupID", &Offsets::m_iPortalLinkageGroupID);

	sv_cheats = Variable("sv_cheats");
	sv_footsteps = Variable("sv_footsteps");
	sv_alternateticks = Variable("sv_alternateticks");
	sv_bonus_challenge = Variable("sv_bonus_challenge");
	sv_accelerate = Variable("sv_accelerate");
	sv_airaccelerate = Variable("sv_airaccelerate");
	sv_paintairacceleration = Variable("sv_paintairacceleration");
	sv_friction = Variable("sv_friction");
	sv_maxspeed = Variable("sv_maxspeed");
	sv_stopspeed = Variable("sv_stopspeed");
	sv_maxvelocity = Variable("sv_maxvelocity");
	sv_gravity = Variable("sv_gravity");

	return this->hasLoaded = this->g_GameMovement && this->g_ServerGameDLL;
}

void Server::Shutdown() {
	Interface::Delete(this->g_GameMovement);
	Interface::Delete(this->g_ServerGameDLL);
}

Server *server;
