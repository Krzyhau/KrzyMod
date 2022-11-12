#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameRemovePaint, "Remove All Paint", 0) {
	engine->ExecuteCommand("removeallpaint");
}
