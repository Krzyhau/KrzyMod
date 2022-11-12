#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, moveMarioJump, "Mario Jump", 3.5f, 0) {
	KRZYMOD_CONTROL_CVAR(sv_krzymod_jump_height, 150);
}
