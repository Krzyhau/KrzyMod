#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, visualHideDynamic, "Hide Dynamic World", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(r_shadows, 0);
	KRZYMOD_CONTROL_CVAR(r_drawentities, 0);
}
