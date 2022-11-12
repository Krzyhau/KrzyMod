#include "Features/KrzyMod.hpp"

#include "Modules/Engine.hpp"

CREATE_KRZYMOD(moveAutojump, "Jump Script", 3.5f, 0) {
	if (info.execType == INITIAL) {
		KRZYMOD_CONTROL_CVAR(sv_krzymod_autojump, 1);
	}
	if (info.execType == ENGINE_TICK) {
		engine->ExecuteCommand("+jump");
	}
	if (info.execType == LAST) {
		engine->ExecuteCommand("-jump");
	}
}
