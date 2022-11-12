#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, gameSmallTimescale, "Timescale 0.2", 1.5f, 2) {
	KRZYMOD_CONTROL_CVAR(host_timescale, 0.5);
}
