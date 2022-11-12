#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, visualDeepFried, "Deep Fried", 1.5f, 0) {
	KRZYMOD_CONTROL_CVAR(mat_ambient_light_r, 10);
	KRZYMOD_CONTROL_CVAR(mat_ambient_light_g, 10);
	KRZYMOD_CONTROL_CVAR(mat_ambient_light_b, 10);
}
