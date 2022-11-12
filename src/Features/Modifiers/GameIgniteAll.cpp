#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameIgniteAll, "Ignite Everything", 0) {
	engine->ExecuteCommand("ent_fire * ignite");
}
