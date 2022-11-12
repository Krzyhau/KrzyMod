#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(OVERRIDE_CAMERA, viewQuakeFov, "Quake FOV", 3.5f, 5) {
	auto viewSetup = (CViewSetup *)info.data;
	viewSetup->fov *= 1.6;
	viewSetup->fovViewmodel *= 1.5;
}
