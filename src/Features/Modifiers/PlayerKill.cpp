#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(playerKill, "kill.", 0) {
	engine->ExecuteCommand("kill");
}
