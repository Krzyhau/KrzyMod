#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameLoadQuick, "Load Quicksave", 2) {
	engine->ExecuteCommand("load quick");
}
