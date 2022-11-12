#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, visualPS2Graphics, "PS2 Graphics", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(mat_picmip, 4);
	KRZYMOD_CONTROL_CVAR(r_lod, 3);
}
