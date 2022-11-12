#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, gameMaxBounciness, "Maximum Repulsiveness", 3.5f, 0) {
	KRZYMOD_CONTROL_CVAR(bounce_paint_min_speed, 3600);
}
