#include "PluginMain.hpp"

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
#include "Features/EntityList.hpp"
#include "Features/OffsetFinder.hpp"
#include "Game.hpp"
#include "Hook.hpp"
#include "Interface.hpp"
#include "Modules.hpp"
#include "Variable.hpp"

Plugin::Plugin()
	: ptr(nullptr)
	, index(0) {
}

PluginMain pluginMain;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(PluginMain, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, pluginMain);



PluginMain::PluginMain()
	: modules(new Modules())
	, cheats(new Cheats())
	, plugin(new Plugin())
	, game(Game::CreateNew()) {
}

bool PluginMain::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
	console = new Console();
	if (!console->Init())
		return false;

	if (this->game) {
		this->game->LoadOffsets();

		CrashHandler::Init();

		PluginInitHandler::RunAll();

		tier1 = new Tier1();
		if (tier1->Init()) {
			this->modules->AddModule<InputSystem>(&inputSystem);
			this->modules->AddModule<Scheme>(&scheme);
			this->modules->AddModule<Surface>(&surface);
			this->modules->AddModule<VGui>(&vgui);
			this->modules->AddModule<Engine>(&engine);
			this->modules->AddModule<Client>(&client);
			this->modules->AddModule<Server>(&server);
			this->modules->AddModule<VScript>(&vscript);
			this->modules->InitAll();

			if (engine && engine->hasLoaded) {

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

	if (pluginMain.cheats) {
		pluginMain.cheats->Shutdown();
	}

	if (pluginMain.modules) {
		pluginMain.modules->ShutdownAll();
	}

	Variable::ClearAllCallbacks();
	SAFE_DELETE(pluginMain.cheats)
	SAFE_DELETE(pluginMain.modules)
	SAFE_DELETE(pluginMain.plugin)
	SAFE_DELETE(pluginMain.game)
	SAFE_DELETE(tier1)
	SAFE_DELETE(console)
	CrashHandler::Cleanup();
	return false;
}

// Plugin has to disable itself in the plugin list or the game might crash because of missing callbacks
// This is a race condition though
bool PluginMain::GetPlugin() {
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
void PluginMain::SearchPlugin() {
	this->findPluginThread = std::thread([this]() {
		GO_THE_FUCK_TO_SLEEP(1000);
		if (this->GetPlugin()) {
			this->plugin->ptr->m_bDisable = true;
		} else {
			console->DevWarning(PLUGIN_NAME": Failed to find the plugin in the plugin list!\nTry again with \"plugin_load\".\n");
		}
	});
	this->findPluginThread.detach();
}

//TODO: create a macro for plugin-named command creation

CON_COMMAND(krzymod_about, "krzymod_about - prints info about KrzyMod plugin\n") {
	console->Print(PLUGIN_DESC "\n");
	console->Print("More information at: " PLUGIN_WEB "\n");
	console->Print("Game: %s\n", pluginMain.game->Version());
	console->Print("Version: " PLUGIN_VERSION "\n");
	console->Print("Built: " PLUGIN_BUILT "\n");
}

CON_COMMAND(krzymod_exit, "krzymod_exit - removes all function hooks, registered commands and unloads the module\n") {
	Variable::ClearAllCallbacks();

	Hook::DisableAll();

	if (pluginMain.cheats) {
		pluginMain.cheats->Shutdown();
	}

	if (pluginMain.GetPlugin()) {
		// Plugin has to unhook CEngine some ticks before unloading the module
		auto unload = std::string("plugin_unload ") + std::to_string(pluginMain.plugin->index);
		engine->SendToCommandBuffer(unload.c_str(), SAFE_UNLOAD_TICK_DELAY);
	}

	if (pluginMain.modules) {
		pluginMain.modules->ShutdownAll();
	}

	SAFE_DELETE(pluginMain.cheats)
	SAFE_DELETE(pluginMain.modules)
	SAFE_DELETE(pluginMain.plugin)
	SAFE_DELETE(pluginMain.game)

	console->Print(PLUGIN_NAME ": Disabling...\n");

	SAFE_DELETE(tier1)
	SAFE_DELETE(console)
	CrashHandler::Cleanup();
}

#pragma region Unused callbacks
void PluginMain::Unload() {
}
void PluginMain::Pause() {
}
void PluginMain::UnPause() {
}
const char *PluginMain::GetPluginDescription() {
	return PLUGIN_SIGNATURE;
}
void PluginMain::LevelInit(char const *pMapName) {
}
void PluginMain::ServerActivate(void *pEdictList, int edictCount, int clientMax) {
}
void PluginMain::GameFrame(bool simulating) {
}
void PluginMain::LevelShutdown() {
}
void PluginMain::ClientFullyConnect(void *pEdict) {
}
void PluginMain::ClientActive(void *pEntity) {
}
void PluginMain::ClientDisconnect(void *pEntity) {
}
void PluginMain::ClientPutInServer(void *pEntity, char const *playername) {
}
void PluginMain::SetCommandClient(int index) {
}
void PluginMain::ClientSettingsChanged(void *pEdict) {
}
int PluginMain::ClientConnect(bool *bAllowConnect, void *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) {
	return 0;
}
int PluginMain::ClientCommand(void *pEntity, const void *&args) {
	return 0;
}
int PluginMain::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) {
	return 0;
}
void PluginMain::OnQueryCvarValueFinished(int iCookie, void *pPlayerEntity, int eStatus, const char *pCvarName, const char *pCvarValue) {
}
void PluginMain::OnEdictAllocated(void *edict) {
}
void PluginMain::OnEdictFreed(const void *edict) {
}
#pragma endregion
