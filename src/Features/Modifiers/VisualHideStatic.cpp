#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, visualHideStatic, "Hide Static World", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(r_portalsopenall, 1);
	KRZYMOD_CONTROL_CVAR(r_drawworld, 0);
	KRZYMOD_CONTROL_CVAR(r_drawstaticprops, 0);
}
