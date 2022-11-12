#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, visualRtxOn, "RTX On", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(mat_vignette_enable, 1);
	KRZYMOD_CONTROL_CVAR(mat_bloom_scalefactor_scalar, 10);
	KRZYMOD_CONTROL_CVAR(mat_motion_blur_strength, 10);
}
