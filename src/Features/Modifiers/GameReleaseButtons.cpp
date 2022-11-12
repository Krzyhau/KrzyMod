#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameReleaseButtons, "Release All Map Buttons", 0) {
	engine->ExecuteCommand("ent_fire prop_floor_button pressout");
	engine->ExecuteCommand("ent_fire prop_under_floor_button pressout");
}
