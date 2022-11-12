#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, visualBlackout, "Blackout", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(fog_override, 1);
	KRZYMOD_CONTROL_CVAR(fog_maxdensity, 1);
	KRZYMOD_CONTROL_CVAR(fog_start, -5000);
	KRZYMOD_CONTROL_CVAR(fog_end, 200);
	KRZYMOD_CONTROL_CVAR(fog_color, 0 0 0);
}
