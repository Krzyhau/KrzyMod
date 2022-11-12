#pragma once

#include "Variable.hpp"
#include "KrzyModEffect.hpp"
#include <vector>

class ConvarController {
private:
	bool enabled;
	Variable convar;
	std::string value;
	std::string originalValue;
	bool isArchiveBlocked;
	std::vector<KrzyModEffect*> activators;

public:
	ConvarController(Variable var, std::string value, float time, KrzyModEffect *activator);
	bool IsEnabled() const { return enabled; }
	Variable Convar() const { return convar; }
	void Control();
	void Enable();
	void Disable();

	void AddActivator(KrzyModEffect *effect);
	bool IsActivator(KrzyModEffect *effect);
};