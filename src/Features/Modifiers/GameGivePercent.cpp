#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameGivePercent, "Give%%%%", 0) {
	for (int i = 0; i < 30; i++) engine->ExecuteCommand("give prop_weighted_cube");
}
