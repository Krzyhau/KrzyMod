#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, gameNoFriction, "Caution! Wet Floor!", 3.5f, 0) {
	KRZYMOD_CONTROL_CVAR(sv_friction, 0.2);
}
