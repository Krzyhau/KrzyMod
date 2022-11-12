#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameSpawnGelPortal, "Spawn Conversion Blob", 0) {
	engine->ExecuteCommand("ent_create_paint_bomb_portal");
}
