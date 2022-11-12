#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameSpawnGelWater, "Spawn Water Blob", 0) {
	engine->ExecuteCommand("ent_create_paint_bomb_erase");
}
