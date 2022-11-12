#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, gameLargeTimescale, "Timescale x2", 2.5f, 2) {
	KRZYMOD_CONTROL_CVAR(host_timescale, 2);
}
