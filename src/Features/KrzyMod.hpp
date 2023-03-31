#pragma once

#include "Variable.hpp"
#include "KrzyModClient.hpp"
#include "KrzyModEffect.hpp"
#include "ConvarController.hpp"
#include "Modules/VGui.hpp"
#include <list>
#include <chrono>
#include "Utils/TwitchConnection.hpp"


struct KrzyModVote {
	int voteNumber = 0;
	KrzyModEffect *effect = nullptr;
	int votes = 0;
};


class KrzyMod : public Hud {
private:
	int nextEffectID = -1;
	KrzyModEffect *selectedEffect = nullptr;
	std::list<ConvarController> convarControllers;

	std::chrono::high_resolution_clock::time_point startTime;
	std::chrono::high_resolution_clock::time_point startTimeModified;
	std::chrono::high_resolution_clock::time_point lastUpdate;
	float duration = 30;

	KrzyModVote oldVotes[4];
	KrzyModVote votes[4];
	bool evaluatedVoting = false;
	TwitchConnection twitchCon;
	std::vector<std::string> votingPeople;

	KrzyModClient modClient;

public:
	std::vector<KrzyModEffect *> effects;
	std::list<KrzyModActiveEffect> activeEffects;

public:
	KrzyMod();
	~KrzyMod();
	bool IsEnabled();
	bool ShouldDraw() override;
	bool GetCurrentSize(int &w, int &h) override;
	void Paint(int slot) override;
	void Update();
	void Stop();
	float GetTime(bool modified = false, bool scaled = false);
	void ResetTimer();
	void UpdateTimer(float timeBase, float currTimer);
	void ActivateEffect(KrzyModEffect *effect);
	void ActivateEffect(KrzyModEffect *effect, float duration, float time);
	void DisableEffect(KrzyModEffect *effect);
	void AddEffect(KrzyModEffect *effect);
	KrzyModEffect *GetEffectByName(std::string name);

	void AddConvarController(Variable convar, std::string newValue, float time, KrzyModEffect *parent);
	KrzyModEffect *GetNextEffect(bool increaseCounter = true);
	void RandomizeEffectOrder();
	bool Vote(int num);

	void TryCreateRoom(std::string server, int port);
	void TryJoinRoom(std::string server, int port, int roomID);
	bool IsSpectator();

	void InvokeProcessMovementEvents(CMoveData *moveData, bool preCall);
	void InvokeOverrideCameraEvents(CViewSetup *view);
	void InvokeTraceRayEvents(CGameTrace *tr);
	
};

extern KrzyMod krzyMod;

extern Variable krzymod_enabled;
extern Variable krzymod_time_base;
extern Variable krzymod_timer_multiplier;
extern Variable krzymod_primary_font;
extern Variable krzymod_secondary_font;
extern Variable krzymod_debug;
extern Variable krzymod_ignore_pausing_locally;
extern Variable krzymod_host_spectator;

extern Variable krzymod_vote_enabled;
extern Variable krzymod_vote_double_numbering;
extern Variable krzymod_vote_channel;
extern Variable krzymod_vote_proportional;


#define CREATE_KRZYMOD(name, displayName, executionTime, groupID)                                                         \
	void krzymod_effect_##name##_callback(KrzyModExecInfo info);                                                                    \
	KrzyModEffect *krzymod_effect_##name = new KrzyModEffect(#name, displayName, executionTime, groupID, krzymod_effect_##name##_callback); \
	void krzymod_effect_##name##_callback(KrzyModExecInfo info)

#define CREATE_KRZYMOD_SIMPLE(type, name, displayName, executionTime, groupID)    \
	void krzymod_effect_##name##_callback2(KrzyModExecInfo info);                           \
	CREATE_KRZYMOD(name, displayName, executionTime, groupID) {                   \
		if (info.execType == type) krzymod_effect_##name##_callback2(info);                 \
	}                                                                             \
	void krzymod_effect_##name##_callback2(KrzyModExecInfo info)

#define CREATE_KRZYMOD_INSTANT(name, displayName, groupID) CREATE_KRZYMOD_SIMPLE(INITIAL, name, displayName, 0.0f, groupID)

#define KRZYMOD_CONTROL_CVAR(name, value) krzyMod.AddConvarController(Variable(#name), #value, info.endTime, (KrzyModEffect *)info.data);