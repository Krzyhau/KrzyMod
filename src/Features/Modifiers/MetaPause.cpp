#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, metaPause, "Pause KrzyMod", 1.0f, 1) {
	KRZYMOD_CONTROL_CVAR(krzymod_timer_multiplier, 0);
}
