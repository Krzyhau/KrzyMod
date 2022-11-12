#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, gameMoonGravity, "Moon Gravity", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(sv_gravity, 200);
	KRZYMOD_CONTROL_CVAR(sv_krzymod_jump_height, 135);
}
