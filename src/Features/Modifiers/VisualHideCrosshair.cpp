#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, visualHideCrosshair, "Hide Crosshair", 3.5f, 0) {
	KRZYMOD_CONTROL_CVAR(cl_drawhud, 0);
}
