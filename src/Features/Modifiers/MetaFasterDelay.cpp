#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, metaFasterDelay, "x4 KrzyMod Speed", 1.0f, 1) {
	KRZYMOD_CONTROL_CVAR(krzymod_timer_multiplier, 4);
}
