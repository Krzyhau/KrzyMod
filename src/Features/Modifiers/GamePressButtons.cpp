#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gamePressButtons, "Press All Map Buttons", 0) {
	engine->ExecuteCommand("ent_fire prop_floor_button pressin");
	engine->ExecuteCommand("ent_fire prop_under_floor_button pressin");
	engine->ExecuteCommand("ent_fire prop_under_button press");
	engine->ExecuteCommand("ent_fire prop_button press");
	engine->ExecuteCommand("ent_fire func_button press");
}
