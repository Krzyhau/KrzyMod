#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameSpawnGelJump, "Spawn Repulsion Blob", 0) {
	engine->ExecuteCommand("ent_create_paint_bomb_jump");
}
