#include "Engine.hpp"

#include "Client.hpp"
#include "Console.hpp"
#include "Event.hpp"
#include "Features/KrzyMod.hpp"
#include "Game.hpp"
#include "Hook.hpp"
#include "Interface.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include "Variable.hpp"
#include "PluginMain.hpp"

#include <cstring>

#ifdef _WIN32
// clang-format off
#	include <Windows.h>
#	include <Memoryapi.h>
#	define strcasecmp _stricmp
// clang-format on
#else
#	include <sys/mman.h>
#endif


REDECL(Engine::Frame);
REDECL(Engine::TraceRay);
REDECL(Engine::OnGameOverlayActivated);
REDECL(Engine::OnGameOverlayActivatedBase);

void Engine::ExecuteCommand(const char *cmd, bool immediately) {
	if (immediately) {
		this->ExecuteClientCmd(this->engineClient->ThisPtr(), cmd);
	} else {
		this->ClientCmd(this->engineClient->ThisPtr(), cmd);
	}
}
int Engine::GetTick() {
	return (this->GetMaxClients() < 2) ? *this->tickcount : TIME_TO_TICKS(*this->net_time);
}
float Engine::ToTime(int tick) {
	return tick * *this->interval_per_tick;
}
int Engine::GetLocalPlayerIndex() {
	return this->GetLocalPlayer(this->engineClient->ThisPtr());
}
edict_t *Engine::PEntityOfEntIndex(int iEntIndex) {
	if (iEntIndex >= 0 && iEntIndex < server->gpGlobals->maxEntities) {
		auto pEdict = reinterpret_cast<edict_t *>((uintptr_t)server->gpGlobals->pEdicts + iEntIndex * sizeof(edict_t));
		if (!pEdict->IsFree()) {
			return pEdict;
		}
	}

	return nullptr;
}
QAngle Engine::GetAngles(int nSlot) {
	auto va = QAngle();
	if (this->GetLocalClient) {
		auto client = this->GetLocalClient(nSlot);
		if (client) {
			va = *reinterpret_cast<QAngle *>((uintptr_t)client + Offsets::viewangles);
		}
	} else {
		this->GetViewAngles(this->engineClient->ThisPtr(), va);
	}
	return va;
}
void Engine::SetAngles(int nSlot, QAngle va) {
	if (this->GetLocalClient) {
		auto client = this->GetLocalClient(nSlot);
		if (client) {
			auto viewangles = reinterpret_cast<QAngle *>((uintptr_t)client + Offsets::viewangles);
			viewangles->x = Math::AngleNormalize(va.x);
			viewangles->y = Math::AngleNormalize(va.y);
			viewangles->z = Math::AngleNormalize(va.z);
		}
	} else {
		this->SetViewAngles(this->engineClient->ThisPtr(), va);
	}
}
void Engine::SendToCommandBuffer(const char *text, int delay) {
	auto slot = this->GetActiveSplitScreenPlayerSlot(nullptr);
	this->Cbuf_AddText(slot, text, delay);
}
int Engine::PointToScreen(const Vector &point, Vector &screen) {
	return this->ScreenPosition(nullptr, point, screen);
}
void Engine::SafeUnload(const char *postCommand) {
	Event::Trigger<Event::PLUGIN_UNLOAD>({});

	// The exit command will handle everything
	this->ExecuteCommand("krzymod_exit");

	if (postCommand) {
		this->SendToCommandBuffer(postCommand, SAFE_UNLOAD_TICK_DELAY);
	}
}
bool Engine::isRunning() {
	return engine->hoststate->m_activeGame && engine->hoststate->m_currentState == HOSTSTATES::HS_RUN;
}
bool Engine::IsGamePaused() {
	return this->IsPaused(this->engineClient->ThisPtr());
}

int Engine::GetMapIndex(const std::string map) {
	auto it = std::find(Game::mapNames.begin(), Game::mapNames.end(), map);
	if (it != Game::mapNames.end()) {
		return std::distance(Game::mapNames.begin(), it);
	} else {
		return -1;
	}
}

std::string Engine::GetCurrentMapName() {
	static std::string last_map;

	std::string map = this->GetLevelNameShort(this->engineClient->ThisPtr());

	if (map != "") {
		// Forward-ify all the slashes
		std::replace(map.begin(), map.end(), '\\', '/');

		last_map = map;
	}

	return last_map;
}

bool Engine::Trace(Vector &pos, QAngle &angle, float distMax, CTraceFilterSimple &filter, CGameTrace &tr) {
	float X = DEG2RAD(angle.x), Y = DEG2RAD(angle.y);
	auto cosX = std::cos(X), cosY = std::cos(Y);
	auto sinX = std::sin(X), sinY = std::sin(Y);

	Vector dir(cosY * cosX, sinY * cosX, -sinX);

	Vector finalDir = Vector(dir.x, dir.y, dir.z).Normalize() * distMax;

	Ray_t ray;
	ray.m_IsRay = true;
	ray.m_IsSwept = true;
	ray.m_Start = VectorAligned(pos.x, pos.y, pos.z);
	ray.m_Delta = VectorAligned(finalDir.x, finalDir.y, finalDir.z);
	ray.m_StartOffset = VectorAligned();
	ray.m_Extents = VectorAligned();

	engine->TraceRay(this->engineTrace->ThisPtr(), ray, MASK_SHOT_PORTAL, &filter, &tr);

	if (tr.fraction >= 1) {
		return false;
	}
	return true;
}

bool Engine::TraceFromCamera(float distMax, CGameTrace &tr) {
	void *player = server->GetPlayer(GET_SLOT() + 1);

	if (player == nullptr || (int)player == -1)
		return false;

	Vector camPos = server->GetAbsOrigin(player) + server->GetViewOffset(player);
	QAngle angle = engine->GetAngles(GET_SLOT());

	CTraceFilterSimple filter;
	filter.SetPassEntity(server->GetPlayer(GET_SLOT() + 1));

	return this->Trace(camPos, angle, distMax, filter, tr);
}

bool Engine::ConsoleVisible() {
	return this->Con_IsVisible(this->engineClient->ThisPtr());
}

float Engine::GetHostFrameTime() {
	return this->HostFrameTime(this->engineTool->ThisPtr());
}

float Engine::GetClientTime() {
	return this->ClientTime(this->engineTool->ThisPtr());
}



void Engine::GetTicks(int &host, int &server, int &client) {
	auto &et = this->engineTool;
	using _Fn = int(__rescall *)(void *thisptr);
	host = et->Original<_Fn>(Offsets::HostTick)(et->ThisPtr());
	server = et->Original<_Fn>(Offsets::ServerTick)(et->ThisPtr());
	client = et->Original<_Fn>(Offsets::ClientTick)(et->ThisPtr());
}

// CEngine::Frame
DETOUR(Engine::Frame) {
	Event::Trigger<Event::PRE_TICK>({false, engine->lastTick});
	
	auto result = Engine::Frame(thisptr);

	Event::Trigger<Event::POST_TICK>({false, engine->lastTick});

	return result;
}

DETOUR(Engine::TraceRay, const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, CGameTrace *pTrace) {
	auto result = Engine::TraceRay(thisptr, ray, fMask, pTraceFilter, pTrace);

	krzyMod.InvokeTraceRayEvents(pTrace);

	return result;
}



// CSteam3Client::OnGameOverlayActivated
DETOUR_B(Engine::OnGameOverlayActivated, GameOverlayActivated_t *pGameOverlayActivated) {
	engine->overlayActivated = pGameOverlayActivated->m_bActive;
	return Engine::OnGameOverlayActivatedBase(thisptr, pGameOverlayActivated);
}

bool Engine::Init() {
	this->engineClient = Interface::Create(this->Name(), "VEngineClient015", false);
	this->s_ServerPlugin = Interface::Create(this->Name(), "ISERVERPLUGINHELPERS001", false);

	if (this->engineClient) {
		this->GetScreenSize = this->engineClient->Original<_GetScreenSize>(Offsets::GetScreenSize);
		this->ClientCmd = this->engineClient->Original<_ClientCmd>(Offsets::ClientCmd);
		this->ExecuteClientCmd = this->engineClient->Original<_ExecuteClientCmd>(Offsets::ExecuteClientCmd);
		this->GetLocalPlayer = this->engineClient->Original<_GetLocalPlayer>(Offsets::GetLocalPlayer);
		this->GetViewAngles = this->engineClient->Original<_GetViewAngles>(Offsets::GetViewAngles);
		this->SetViewAngles = this->engineClient->Original<_SetViewAngles>(Offsets::SetViewAngles);
		this->GetMaxClients = this->engineClient->Original<_GetMaxClients>(Offsets::GetMaxClients);
		this->GetGameDirectory = this->engineClient->Original<_GetGameDirectory>(Offsets::GetGameDirectory);
		this->GetSaveDirName = this->engineClient->Original<_GetSaveDirName>(Offsets::GetSaveDirName);
		this->IsPaused = this->engineClient->Original<_IsPaused>(Offsets::IsPaused);
		this->Con_IsVisible = this->engineClient->Original<_Con_IsVisible>(Offsets::Con_IsVisible);
		this->GetLevelNameShort = this->engineClient->Original<_GetLevelNameShort>(Offsets::GetLevelNameShort);

		Memory::Read<_Cbuf_AddText>((uintptr_t)this->ClientCmd + Offsets::Cbuf_AddText, &this->Cbuf_AddText);
#ifndef _WIN32
		if (pluginMain.game->Is(SourceGame_EIPRelPIC)) {
			this->s_CommandBuffer = (void *)((uintptr_t)this->Cbuf_AddText + 9 + *(uint32_t *)((uintptr_t)this->Cbuf_AddText + 11) + *(uint32_t *)((uintptr_t)this->Cbuf_AddText + 71));
		} else
#endif
			Memory::Deref<void *>((uintptr_t)this->Cbuf_AddText + Offsets::s_CommandBuffer, &this->s_CommandBuffer);

		Memory::Read((uintptr_t)this->SetViewAngles + Offsets::GetLocalClient, &this->GetLocalClient);

		this->m_bWaitEnabled = reinterpret_cast<bool *>((uintptr_t)s_CommandBuffer + Offsets::m_bWaitEnabled);
		this->m_bWaitEnabled2 = reinterpret_cast<bool *>((uintptr_t)this->m_bWaitEnabled + Offsets::CCommandBufferSize);

		auto GetSteamAPIContext = this->engineClient->Original<uintptr_t (*)()>(Offsets::GetSteamAPIContext);
		auto OnGameOverlayActivated = reinterpret_cast<_OnGameOverlayActivated *>(GetSteamAPIContext() + Offsets::OnGameOverlayActivated);

		Engine::OnGameOverlayActivatedBase = *OnGameOverlayActivated;
		*OnGameOverlayActivated = reinterpret_cast<_OnGameOverlayActivated>(Engine::OnGameOverlayActivated_Hook);

		if (this->g_VEngineServer = Interface::Create(this->Name(), "VEngineServer022", false)) {
			this->ClientCommand = this->g_VEngineServer->Original<_ClientCommand>(Offsets::ClientCommand);
			this->IsServerPaused = this->g_VEngineServer->Original<_IsServerPaused>(Offsets::IsServerPaused);
			this->ServerPause = this->g_VEngineServer->Original<_ServerPause>(Offsets::ServerPause);
		}

		typedef void *(*_GetClientState)();
		auto GetClientState = Memory::Read<_GetClientState>((uintptr_t)this->ClientCmd + Offsets::GetClientStateFunction);
		void *clPtr = GetClientState();

		this->GetActiveSplitScreenPlayerSlot = this->engineClient->Original<_GetActiveSplitScreenPlayerSlot>(Offsets::GetActiveSplitScreenPlayerSlot);

		if (this->cl = Interface::Create(clPtr)) {
#if _WIN32
			auto IServerMessageHandler_VMT = Memory::Deref<uintptr_t>((uintptr_t)this->cl->ThisPtr() + IServerMessageHandler_VMT_Offset);
			auto ProcessTick = Memory::Deref<uintptr_t>(IServerMessageHandler_VMT + sizeof(uintptr_t) * Offsets::ProcessTick);
#else
			auto ProcessTick = this->cl->Original(Offsets::ProcessTick);
#endif

#ifndef _WIN32
			if (pluginMain.game->Is(SourceGame_EIPRelPIC)) {
				tickcount = (int *)(ProcessTick + 12 + *(uint32_t *)(ProcessTick + 14) + *(uint32_t *)(ProcessTick + 86) + 0x18);
			} else
#endif
				tickcount = Memory::Deref<int *>(ProcessTick + Offsets::tickcount);

#ifndef _WIN32
			if (pluginMain.game->Is(SourceGame_EIPRelPIC)) {
				interval_per_tick = (float *)(ProcessTick + 12 + *(uint32_t *)(ProcessTick + 14) + *(uint32_t *)(ProcessTick + 72) + 8);
			} else
#endif
				interval_per_tick = Memory::Deref<float *>(ProcessTick + Offsets::interval_per_tick);

			auto SetSignonState = this->cl->Original(Offsets::Disconnect - 1);
			auto HostState_OnClientConnected = Memory::Read(SetSignonState + Offsets::HostState_OnClientConnected);
#ifndef _WIN32
			if (pluginMain.game->Is(SourceGame_EIPRelPIC)) {
				hoststate = (CHostState *)(HostState_OnClientConnected + 5 + *(uint32_t *)(HostState_OnClientConnected + 6) + *(uint32_t *)(HostState_OnClientConnected + 15));
			} else
#endif
				Memory::Deref<CHostState *>(HostState_OnClientConnected + Offsets::hoststate, &hoststate);
		}

		if (this->engineTrace = Interface::Create(this->Name(), "EngineTraceServer004")) {
			this->engineTrace->Hook(TraceRay_Hook, TraceRay, Offsets::TraceRay);
		}
	}

	if (this->engineTool = Interface::Create(this->Name(), "VENGINETOOL003", false)) {
		auto GetCurrentMap = this->engineTool->Original(Offsets::GetCurrentMap);
#ifndef _WIN32
		if (pluginMain.game->Is(SourceGame_EIPRelPIC)) {
			this->m_szLevelName = (char *)(GetCurrentMap + 7 + *(uint32_t *)(GetCurrentMap + 9) + *(uint32_t *)(GetCurrentMap + 18) + 16);
		} else
#endif
			this->m_szLevelName = Memory::Deref<char *>(GetCurrentMap + Offsets::m_szLevelName);

		this->m_bLoadgame = reinterpret_cast<bool *>((uintptr_t)this->m_szLevelName + Offsets::m_bLoadGame);

		this->HostFrameTime = this->engineTool->Original<_HostFrameTime>(Offsets::HostFrameTime);
		this->ClientTime = this->engineTool->Original<_ClientTime>(Offsets::ClientTime);

		this->PrecacheModel = this->engineTool->Original<_PrecacheModel>(Offsets::PrecacheModel);
	}

	if (auto s_EngineAPI = Interface::Create(this->Name(), "VENGINE_LAUNCHER_API_VERSION004", false)) {
		auto IsRunningSimulation = s_EngineAPI->Original(Offsets::IsRunningSimulation);
		void *engAddr;
#ifndef _WIN32
		if (pluginMain.game->Is(SourceGame_EIPRelPIC)) {
			engAddr = *(void **)(IsRunningSimulation + 5 + *(uint32_t *)(IsRunningSimulation + 6) + *(uint32_t *)(IsRunningSimulation + 15));
		} else
#endif
			engAddr = Memory::DerefDeref<void *>(IsRunningSimulation + Offsets::eng);

		if (this->eng = Interface::Create(engAddr)) {
			if (this->tickcount && this->hoststate && this->m_szLevelName) {
				this->eng->Hook(Engine::Frame_Hook, Engine::Frame, Offsets::Frame);
			}
		}

		uintptr_t Init = s_EngineAPI->Original(Offsets::Init);
		uintptr_t VideoMode_Create = Memory::Read(Init + Offsets::VideoMode_Create);
		void **videomode;
#ifndef _WIN32
		if (pluginMain.game->Is(SourceGame_EIPRelPIC)) {
			videomode = (void **)(VideoMode_Create + 6 + *(uint32_t *)(VideoMode_Create + 8) + *(uint32_t *)(VideoMode_Create + 193));
		} else
#endif
			videomode = *(void ***)(VideoMode_Create + Offsets::videomode);

		Interface::Delete(s_EngineAPI);
	}

	this->s_GameEventManager = Interface::Create(this->Name(), "GAMEEVENTSMANAGER002", false);
	if (this->s_GameEventManager) {
		this->AddListener = this->s_GameEventManager->Original<_AddListener>(Offsets::AddListener);
		this->RemoveListener = this->s_GameEventManager->Original<_RemoveListener>(Offsets::RemoveListener);

		auto FireEventClientSide = s_GameEventManager->Original(Offsets::FireEventClientSide);
		auto FireEventIntern = Memory::Read(FireEventClientSide + Offsets::FireEventIntern);
		Memory::Read<_ConPrintEvent>(FireEventIntern + Offsets::ConPrintEvent, &this->ConPrintEvent);
	}


	// This is the address of the one interesting call to ReadCustomData - the E8 byte indicates the start of the call instruction
#ifdef _WIN32
	this->readCustomDataInjectAddr = Memory::Scan(this->Name(), "8D 45 E8 50 8D 4D BC 51 8D 4F 04 E8 ? ? ? ? 8B 4D BC 83 F9 FF", 12);
	this->readConsoleCommandInjectAddr = Memory::Scan(this->Name(), "8B 45 F4 50 68 FE 04 00 00 68 ? ? ? ? 8D 4D 90 E8 ? ? ? ? 8D 4F 04 E8", 26);
#else
	if (pluginMain.game->Is(SourceGame_EIPRelPIC)) {
		this->readCustomDataInjectAddr = Memory::Scan(this->Name(), "8B 9D 94 FE FF FF 8D 85 C0 FE FF FF 83 EC 04 50 8D 85 B0 FE FF FF 50 FF B5 8C FE FF FF E8", 30);
		this->readConsoleCommandInjectAddr = Memory::Scan(this->Name(), "68 FE 04 00 00 FF B5 68 FE FF FF 50 89 85 90 FE FF FF E8 ? ? ? ? 58 FF B5 8C FE FF FF E8", 31);
	} else {
		this->readCustomDataInjectAddr = Memory::Scan(this->Name(), "89 44 24 08 8D 85 B0 FE FF FF 89 44 24 04 8B 85 8C FE FF FF 89 04 24 E8", 24);
		this->readConsoleCommandInjectAddr = Memory::Scan(this->Name(), "89 44 24 0C 8D 85 C0 FE FF FF 89 04 24 E8 ? ? ? ? 8B 85 8C FE FF FF 89 04 24 E8", 28);
	}
#endif

	// Pesky memory protection doesn't want us overwriting code - we
	// get around it with a call to mprotect or VirtualProtect
	Memory::UnProtect((void *)this->readCustomDataInjectAddr, 4);
	Memory::UnProtect((void *)this->readConsoleCommandInjectAddr, 4);


	if (auto debugoverlay = Interface::Create(this->Name(), "VDebugOverlay004", false)) {
		ScreenPosition = debugoverlay->Original<_ScreenPosition>(Offsets::ScreenPosition);
		AddBoxOverlay = debugoverlay->Original<_AddBoxOverlay>(Offsets::AddBoxOverlay);
		AddSphereOverlay = debugoverlay->Original<_AddSphereOverlay>(Offsets::AddSphereOverlay);
		AddTriangleOverlay = debugoverlay->Original<_AddTriangleOverlay>(Offsets::AddTriangleOverlay);
		AddLineOverlay = debugoverlay->Original<_AddLineOverlay>(Offsets::AddLineOverlay);
		AddScreenTextOverlay = debugoverlay->Original<_AddScreenTextOverlay>(Offsets::AddScreenTextOverlay);
		ClearAllOverlays = debugoverlay->Original<_ClearAllOverlays>(Offsets::ClearAllOverlays);
		Interface::Delete(debugoverlay);
	}

	return this->hasLoaded = this->engineClient && this->s_ServerPlugin && this->engineTrace;
}
void Engine::Shutdown() {
	if (this->engineClient) {
		auto GetSteamAPIContext = this->engineClient->Original<uintptr_t (*)()>(Offsets::GetSteamAPIContext);
		auto OnGameOverlayActivated = reinterpret_cast<_OnGameOverlayActivated *>(GetSteamAPIContext() + Offsets::OnGameOverlayActivated);
		*OnGameOverlayActivated = Engine::OnGameOverlayActivatedBase;
	}

	Interface::Delete(this->engineClient);
	Interface::Delete(this->s_ServerPlugin);
	Interface::Delete(this->cl);
	Interface::Delete(this->eng);
	Interface::Delete(this->s_GameEventManager);
	Interface::Delete(this->engineTool);
	Interface::Delete(this->engineTrace);
	Interface::Delete(this->g_VEngineServer);

	// Reset to the offsets that were originally in the code
#ifdef _WIN32
	*(uint32_t *)this->readCustomDataInjectAddr = 0x50E8458D;
	*(uint32_t *)this->readConsoleCommandInjectAddr = 0x000491E3;
#else
	*(uint32_t *)this->readCustomDataInjectAddr = 0x08244489;
	*(uint32_t *)this->readConsoleCommandInjectAddr = 0x0008155A;
#endif
}

Engine *engine;
