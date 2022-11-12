#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameSpawnTurret, "Spawn Turret", 0) {
	engine->ExecuteCommand("ent_create npc_portal_turret_floor");
}
