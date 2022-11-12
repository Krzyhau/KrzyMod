#include "ConvarController.hpp"
#include "Modules/Console.hpp"
#include <cstring>

ConvarController::ConvarController(Variable var, std::string value, float time, KrzyModEffect *activator)
	: convar(var)
	, value(value) {
	AddActivator(activator);
	Enable();
}

void ConvarController::Control() {
	if (strcmp(convar.GetString(), value.c_str()) != 0) {
		convar.SetValue(value.c_str());
	}
}

void ConvarController::Enable() {
	enabled = true;
	originalValue = convar.GetString();
	isArchiveBlocked = convar.GetFlags() & FCVAR_ARCHIVE;
	convar.RemoveFlag(FCVAR_ARCHIVE);
	Control();
}

void ConvarController::Disable() {
	enabled = false;
	convar.SetValue(originalValue.c_str());
	if (isArchiveBlocked) convar.AddFlag(FCVAR_ARCHIVE);
}

void ConvarController::AddActivator(KrzyModEffect* effect) {
	activators.push_back(effect);
}

bool ConvarController::IsActivator(KrzyModEffect* activator) {
	for (KrzyModEffect *eff : activators) {
		if (eff == activator) return true;
	}
	return false;
}