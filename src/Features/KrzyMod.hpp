#pragma once

#include "Hud/Hud.hpp"
#include <list>

enum KrzyModExecType {
	UNKNOWN,
	INSTANT,
	ENGINE_TICK,
	PROCESS_MOVEMENT,
	OVERRIDE_CAMERA,
	HUD_PAINT
};

struct KrzyModExecInfo {
	KrzyModExecType execType;
	bool preCall;
	bool lastCall;
	float time;
	float endTime;
	void* data;
};


class KrzyModifier {
public:
	KrzyModifier(std::string name, std::string displayName, float executeTime, void *function);

public:
	std::string name;
	std::string displayName;
	float executeTime;
	//modifiers can have different function types depending on the type, trusting the type variable
	void *function; 
};

struct ActiveKrzyModifier {
	KrzyModifier *modifier;
	float time;
	float endTime;
	void Execute(KrzyModExecType type, bool preCall, void* data);
};



class KrzyMod : public Hud {
private:
	int nextModifierID = 9999;
	std::list<ActiveKrzyModifier> activeModifiers;
public:
	float timer = 0;
	std::vector<KrzyModifier *> modifiers;
public:
	KrzyMod();
	bool ShouldDraw() override;
	bool GetCurrentSize(int &w, int &h) override;
	void Paint(int slot) override;
	void Update();
	void IncreaseTimer(float multipler);
	void ActivateModifier(KrzyModifier *modifier);
	void DisableModifier(KrzyModifier *modifier);
	void AddModifier(KrzyModifier* modifier);
	KrzyModifier* GetNextModifier();

	void InvokeProcessMovementEvents(CMoveData *moveData, bool preCall);
	void InvokeOverrideCameraEvents(CViewSetup *view);
	
};

extern KrzyMod krzyMod;


#define CREATE_KRZYMOD(name, displayName, executionTime)                                                      \
	void krzymod_##name##_callback(KrzyModExecInfo info);                                                     \
	KrzyModifier krzymod_##name = KrzyModifier(#name, displayName, executionTime, krzymod_##name##_callback); \
	void krzymod_##name##_callback(KrzyModExecInfo info)

#define CREATE_KRZYMOD_SIMPLE(type,name, displayName, executionTime)                                          \
	void krzymod_##name##_callback2(KrzyModExecInfo info);                                                    \
	CREATE_KRZYMOD(name, displayName, executionTime) {                                                        \
		if (info.execType == type) krzymod_##name##_callback2(info);                                          \
	}                                                                                                         \
	void krzymod_##name##_callback2(KrzyModExecInfo info)

#define CREATE_KRZYMOD_INSTANT(name, displayName) CREATE_KRZYMOD_SIMPLE(INSTANT, name, displayName, 0.0f)