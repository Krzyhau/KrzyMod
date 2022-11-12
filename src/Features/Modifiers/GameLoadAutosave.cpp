#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameLoadAutosave, "Load Autosave", 2) {
	engine->ExecuteCommand("load autosave");
}
