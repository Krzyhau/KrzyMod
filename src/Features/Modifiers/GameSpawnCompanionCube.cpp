#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameSpawnCompanionCube, "Spawn Companion Cube", 0) {
	engine->ExecuteCommand("ent_create_portal_companion_cube");
}
