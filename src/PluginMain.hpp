#pragma once
#include "Cheats.hpp"
#include "Command.hpp"
#include "Features/Feature.hpp"
#include "Game.hpp"
#include "Interface.hpp"
#include "Modules/Console.hpp"
#include "Modules/Module.hpp"
#include "Variable.hpp"

#include <thread>

#define PLUGIN_NAME "KrzyMod"
#define PLUGIN_VAR_NAME krzymod
#define PLUGIN_DESC "KrzyMod is a plugin adding a bunch of chaotic effects being executed on a timer."
#define PLUGIN_BUILT __TIME__ " " __DATE__
#define PLUGIN_WEB "https://nekzor.github.io/SourceAutoRecord or https://wiki.portal2.sr/SAR"

#define PLUGIN_SIGNATURE \
	new char[26] { 75, 114, 122, 121, 77, 111, 100, 00 }

#define SAFE_UNLOAD_TICK_DELAY 33

// CServerPlugin
#define CServerPlugin_m_Size 16
#define CServerPlugin_m_Plugins 4

class Plugin {
public:
	CPlugin *ptr;
	int index;

public:
	Plugin();
};


class PluginMain : public IServerPluginCallbacks {
public:
	Modules *modules;
	Features *features;
	Cheats *cheats;
	Plugin *plugin;
	Game *game;

private:
	std::thread findPluginThread;

public:
	PluginMain();

	virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
	virtual void Unload();
	virtual void Pause();
	virtual void UnPause();
	virtual const char *GetPluginDescription();
	virtual void LevelInit(char const *pMapName);
	virtual void ServerActivate(void *pEdictList, int edictCount, int clientMax);
	virtual void GameFrame(bool simulating);
	virtual void LevelShutdown();
	virtual void ClientFullyConnect(void *pEdict);
	virtual void ClientActive(void *pEntity);
	virtual void ClientDisconnect(void *pEntity);
	virtual void ClientPutInServer(void *pEntity, char const *playername);
	virtual void SetCommandClient(int index);
	virtual void ClientSettingsChanged(void *pEdict);
	virtual int ClientConnect(bool *bAllowConnect, void *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
	virtual int ClientCommand(void *pEntity, const void *&args);
	virtual int NetworkIDValidated(const char *pszUserName, const char *pszNetworkID);
	virtual void OnQueryCvarValueFinished(int iCookie, void *pPlayerEntity, int eStatus, const char *pCvarName, const char *pCvarValue);
	virtual void OnEdictAllocated(void *edict);
	virtual void OnEdictFreed(const void *edict);

	bool GetPlugin();
	void SearchPlugin();
};

extern PluginMain pluginMain;
