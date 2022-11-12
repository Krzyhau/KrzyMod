#include "KrzyModEffect.hpp"
#include "KrzyMod.hpp"

KrzyModEffect::KrzyModEffect(std::string name, std::string displayName, float durationMultiplier, int groupID, void (*function)(KrzyModExecInfo info))
	: name(name)
	, displayName(displayName)
	, durationMultiplier(durationMultiplier)
	, groupID(groupID)
	, function(function) {
	krzyMod.AddEffect(this);
}

void KrzyModActiveEffect::Update(float dt) {
	time += dt;
	if (time > duration) Execute(LAST, true, effect);
}

void KrzyModActiveEffect::Execute(KrzyModExecType type, bool preCall, void *data) {
	((void (*)(KrzyModExecInfo))effect->function)({type, preCall, time, duration, data});
}