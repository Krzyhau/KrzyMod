#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, gameSmallPhysscale, "Slowy Props", 3.5f, 3) {
	KRZYMOD_CONTROL_CVAR(phys_timescale, 0.5);
}
