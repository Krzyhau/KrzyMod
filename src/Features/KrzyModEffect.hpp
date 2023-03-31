#pragma once

#include <string>

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
	void *data;
};


class KrzyModEffect {
public:
	KrzyModEffect(std::string name, std::string displayName, float executeTime, int groupID, void (*function)(KrzyModExecInfo info));

public:
	std::string name;
	std::string displayName;
	float durationMultiplier;
	int groupID;
	bool blacklisted = false;
	//	modifiers can have different function types depending on the type, trusting the type variable
	void (*function)(KrzyModExecInfo info);

	bool IsMeta() { return name.find("meta") != std::string::npos; };
};

struct KrzyModActiveEffect {
	KrzyModEffect *effect;
	float time;
	float duration;
	void Update(float dt);
	void Execute(KrzyModExecType type, bool preCall, void *data = nullptr);
};