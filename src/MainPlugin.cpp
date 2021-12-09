#include "MainPlugin.hpp"

#include "Version.hpp"

#include <cstring>
#include <ctime>

#ifdef _WIN32
#	include <filesystem>
#endif

#include "Cheats.hpp"
#include "Command.hpp"
#include "CrashHandler.hpp"
#include "Event.hpp"
#include "Features.hpp"
#include "Game.hpp"
#include "Hook.hpp"
#include "Interface.hpp"
#include "Modules.hpp"
#include "Variable.hpp"

MainPlugin mainPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(MainPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, mainPlugin);

Plugin::Plugin()
	: ptr(nullptr)
	, index(0) {
}

MainPlugin::MainPlugin()
	: modules(new Modules())
	, features(new Features())
	, cheats(new Cheats())
	, plugin(new Plugin())
	, game(Game::CreateNew()) {
}

bool MainPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
	console = new Console();
	if (!console->Init())
		return false;

	if (this->game) {
		this->game->LoadOffsets();

		CrashHandler::Init();

		SarInitHandler::RunAll();

		tier1 = new Tier1();
		if (tier1->Init()) {
			this->features->AddFeature<Cvars>(&cvars);
			this->features->AddFeature<Session>(&session);
			this->features->AddFeature<Summary>(&summary);
			this->features->AddFeature<ClassDumper>(&classDumper);
			this->features->AddFeature<EntityList>(&entityList);
			this->features->AddFeature<OffsetFinder>(&offsetFinder);
			this->features->AddFeature<DataMapDumper>(&dataMapDumper);

			this->modules->AddModule<InputSystem>(&inputSystem);
			this->modules->AddModule<Scheme>(&scheme);
			this->modules->AddModule<Surface>(&surface);
			this->modules->AddModule<VGui>(&vgui);
			this->modules->AddModule<Engine>(&engine);
			this->modules->AddModule<Client>(&client);
			this->modules->AddModule<Server>(&server);
			this->modules->AddModule<MaterialSystem>(&materialSystem);
			this->modules->AddModule<VScript>(&vscript);
			this->modules->InitAll();

			if (engine && engine->hasLoaded) {
				engine->demoplayer->Init();
				engine->demorecorder->Init();

				this->cheats->Init();

				this->SearchPlugin();

				console->PrintActive("Loaded " PLUGIN_NAME ", Version %s\n", PLUGIN_VERSION);
				return true;
			} else {
				console->Warning(PLUGIN_NAME ": Failed to load engine module!\n");
			}
		} else {
			console->Warning(PLUGIN_NAME ": Failed to load tier1 module!\n");
		}
	} else {
		console->Warning(PLUGIN_NAME ": Game not supported!\n");
	}

	console->Warning(PLUGIN_NAME ": Failed to load the plugin!\n");

	if (mainPlugin.cheats) {
		mainPlugin.cheats->Shutdown();
	}
	if (mainPlugin.features) {
		mainPlugin.features->DeleteAll();
	}

	if (mainPlugin.modules) {
		mainPlugin.modules->ShutdownAll();
	}

	Variable::ClearAllCallbacks();
	SAFE_DELETE(mainPlugin.features)
	SAFE_DELETE(mainPlugin.cheats)
	SAFE_DELETE(mainPlugin.modules)
	SAFE_DELETE(mainPlugin.plugin)
	SAFE_DELETE(mainPlugin.game)
	SAFE_DELETE(tier1)
	SAFE_DELETE(console)
	CrashHandler::Cleanup();
	return false;
}

// plugin has to disable itself in the plugin list or the game might crash because of missing callbacks
// This is a race condition though
bool MainPlugin::GetPlugin() {
	auto s_ServerPlugin = reinterpret_cast<uintptr_t>(engine->s_ServerPlugin->ThisPtr());
	auto m_Size = *reinterpret_cast<int *>(s_ServerPlugin + CServerPlugin_m_Size);
	if (m_Size > 0) {
		auto m_Plugins = *reinterpret_cast<uintptr_t *>(s_ServerPlugin + CServerPlugin_m_Plugins);
		for (auto i = 0; i < m_Size; ++i) {
			auto ptr = *reinterpret_cast<CPlugin **>(m_Plugins + sizeof(uintptr_t) * i);
			if (!std::strcmp(ptr->m_szName, PLUGIN_SIGNATURE)) {
				this->plugin->ptr = ptr;
				this->plugin->index = i;
				return true;
			}
		}
	}
	return false;
}
void MainPlugin::SearchPlugin() {
	this->findPluginThread = std::thread([this]() {
		GO_THE_FUCK_TO_SLEEP(1000);
		if (this->GetPlugin()) {
			this->plugin->ptr->m_bDisable = true;
		} else {
			console->DevWarning(PLUGIN_NAME ": Failed to find KrzyMod in the plugin list!\nTry again with \"plugin_load\".\n");
		}
	});
	this->findPluginThread.detach();
}

CON_COMMAND(PLUGIN_VAR_NAME(about), PLUGIN_VAR "_about - prints info about " PLUGIN_NAME " plugin\n") {
	console->Print(PLUGIN_DESC "\n");
	console->Print("More information at: " PLUGIN_WEB);
	console->Print("Game: %s\n", mainPlugin.game->Version());
	console->Print("Version: " PLUGIN_VERSION "\n");
	console->Print("Built: " PLUGIN_BUILT "\n");
}
CON_COMMAND(PLUGIN_VAR_NAME(exit), "sar_exit - removes all function hooks, registered commands and unloads the module\n") {

	Variable::ClearAllCallbacks();

	Hook::DisableAll();

	if (mainPlugin.cheats) {
		mainPlugin.cheats->Shutdown();
	}
	if (mainPlugin.features) {
		mainPlugin.features->DeleteAll();
	}

	if (mainPlugin.GetPlugin()) {
		// SAR has to unhook CEngine some ticks before unloading the module
		auto unload = std::string("plugin_unload ") + std::to_string(mainPlugin.plugin->index);
		engine->SendToCommandBuffer(unload.c_str(), SAFE_UNLOAD_TICK_DELAY);
	}

	if (mainPlugin.modules) {
		mainPlugin.modules->ShutdownAll();
	}

	SAFE_DELETE(mainPlugin.features)
	SAFE_DELETE(mainPlugin.cheats)
	SAFE_DELETE(mainPlugin.modules)
	SAFE_DELETE(mainPlugin.plugin)
	SAFE_DELETE(mainPlugin.game)

	console->Print(PLUGIN_NAME " has been disabled.\n");

	SAFE_DELETE(tier1)
	SAFE_DELETE(console)
	CrashHandler::Cleanup();
}

#pragma region Unused callbacks
void MainPlugin::Unload() {
}
void MainPlugin::Pause() {
}
void MainPlugin::UnPause() {
}
const char *MainPlugin::GetPluginDescription() {
	return PLUGIN_SIGNATURE;
}
void MainPlugin::LevelInit(char const *pMapName) {
}
void MainPlugin::ServerActivate(void *pEdictList, int edictCount, int clientMax) {
}
void MainPlugin::GameFrame(bool simulating) {
}
void MainPlugin::LevelShutdown() {
}
void MainPlugin::ClientFullyConnect(void *pEdict) {
}
void MainPlugin::ClientActive(void *pEntity) {
}
void MainPlugin::ClientDisconnect(void *pEntity) {
}
void MainPlugin::ClientPutInServer(void *pEntity, char const *playername) {
}
void MainPlugin::SetCommandClient(int index) {
}
void MainPlugin::ClientSettingsChanged(void *pEdict) {
}
int MainPlugin::ClientConnect(bool *bAllowConnect, void *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) {
	return 0;
}
int MainPlugin::ClientCommand(void *pEntity, const void *&args) {
	return 0;
}
int MainPlugin::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) {
	return 0;
}
void MainPlugin::OnQueryCvarValueFinished(int iCookie, void *pPlayerEntity, int eStatus, const char *pCvarName, const char *pCvarValue) {
}
void MainPlugin::OnEdictAllocated(void *edict) {
}
void MainPlugin::OnEdictFreed(const void *edict) {
}
#pragma endregion
