#pragma once

#include "Variable.hpp"
#include "Modules/VGui.hpp"
#include <list>
#include <chrono>
#include "Utils/TwitchConnection.hpp"

enum KrzyModExecType {
	UNKNOWN,
	INITIAL,
	ENGINE_TICK,
	PROCESS_MOVEMENT,
	OVERRIDE_CAMERA,
	HUD_PAINT,
	LAST,
	TRACERAY
};

struct KrzyModExecInfo {
	KrzyModExecType execType;
	bool preCall;
	float time;
	float endTime;
	void* data;
};


class KrzyModEffect {
public:
	KrzyModEffect(std::string name, std::string displayName, float executeTime, int groupID, void (*function)(KrzyModExecInfo info));

public:
	std::string name;
	std::string displayName;
	float durationMultiplier;
	int groupID;
	//modifiers can have different function types depending on the type, trusting the type variable
	void (*function)(KrzyModExecInfo info);
};

struct KrzyModActiveEffect {
	KrzyModEffect *effect;
	float time;
	float duration;
	void Update(float dt);
	void Execute(KrzyModExecType type, bool preCall, void* data);
};

struct KrzyModConvarControl {
	Variable convar;
	std::string value;
	std::string originalValue;
	bool isArchiveBlocked;
	float remainingTime;
	KrzyModEffect *parentEffect;
	KrzyModConvarControl(Variable var, std::string value, float time, KrzyModEffect *parent);
	void Update(float dt);
};

struct KrzyModVote {
	int voteNumber = 0;
	KrzyModEffect *effect = nullptr;
	int votes = 0;
};


class KrzyMod : public Hud {
private:
	int nextEffectID = -1;
	KrzyModEffect *selectedEffect = nullptr;
	std::list<KrzyModConvarControl> convarControllers;

	std::chrono::high_resolution_clock::time_point startTime;
	std::chrono::high_resolution_clock::time_point startTimeModified;
	std::chrono::high_resolution_clock::time_point lastUpdate;
	float duration = 30;

	KrzyModVote oldVotes[4];
	KrzyModVote votes[4];
	bool evaluatedVoting = false;
	TwitchConnection twitchCon;
	std::vector<std::string> votingPeople;

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
	void ActivateEffect(KrzyModEffect *effect);
	void DisableEffect(KrzyModEffect *effect);
	void AddEffect(KrzyModEffect *effect);
	

	void AddConvarController(Variable convar, std::string newValue, float time, KrzyModEffect *parent);
	KrzyModEffect *GetNextEffect(bool increaseCounter = true);
	void RandomizeEffectOrder();
	bool Vote(int num);

	void InvokeProcessMovementEvents(CMoveData *moveData, bool preCall);
	void InvokeOverrideCameraEvents(CViewSetup *view);
	void InvokeTraceRayEvents(CGameTrace *tr);
	
};

extern KrzyMod krzyMod;


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