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

/*
	Plugin structure based on SourceAutoRecord
	https://github.com/p2sr/SourceAutoRecord
*/


#define PLUGIN_NAME "KrzyMod"
#define PLUGIN_VAR "krzymod"
#define PLUGIN_VAR_NAME(var) krzymod_##var
#define PLUGIN_DESC "KrzyMod is basically a chaos mod for Portal 2."
#define PLUGIN_BUILT __TIME__ " " __DATE__
#define PLUGIN_WEB "https://nekzor.github.io/SourceAutoRecord or https://wiki.portal2.sr/SAR"
#define PLUGIN_SIGNATURE \
	new char[26] { 65, 114, 101, 32, 121, 111, 117, 32, 104, 97, 112, 112, 121, 32, 110, 111, 119, 44, 32, 74, 97, 109, 101, 114, 63, 00 }

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


class MainPlugin : public IServerPluginCallbacks {
public:
	Modules *modules;
	Features *features;
	Cheats *cheats;
	Plugin *plugin;
	Game *game;

private:
	std::thread findPluginThread;

public:
	MainPlugin();

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

extern MainPlugin mainPlugin;
