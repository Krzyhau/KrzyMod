#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameRestartLevel, "restart_level", 2) {
	engine->ExecuteCommand("restart_level");
}
