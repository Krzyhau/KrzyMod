#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(OVERRIDE_CAMERA, viewUpsideDown, "Upside Down View", 2.5f, 0) {
	auto viewSetup = (CViewSetup *)info.data;
	viewSetup->angles.z += 180;
}
