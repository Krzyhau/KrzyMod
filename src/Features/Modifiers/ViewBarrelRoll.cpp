#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD(viewBarrelRoll, "Do A Barrel Roll!", 1.5f, 0) {
	static int mult = -1;
	if (info.execType == INITIAL) {
		mult = Math::RandomNumber(0.0f, 1.0f) > 0.5f ? 1 : -1;
	}
	if (info.execType == OVERRIDE_CAMERA) {
		auto viewSetup = (CViewSetup *)info.data;
		float t = fmodf(info.time * 0.5, 1.0f);
		float angle = t < 0.5 ? 4.0 * t * t * t : 1.0 - pow(-2.0 * t + 2.0, 3.0) * 0.5;

		viewSetup->angles.z += mult * angle * 360.0f;
	}
}
