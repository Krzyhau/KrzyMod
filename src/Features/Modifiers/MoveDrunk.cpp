#include "Features/KrzyMod.hpp"

#include "Modules/Engine.hpp"

CREATE_KRZYMOD(moveDrunk, "Drunk", 3.5f, 5) {
	float time = engine->GetClientTime() * 0.3f;
	float td = fmin((info.endTime - info.time) * 0.1, fmin(info.time * 0.2, 1.0));

	if (info.execType == PROCESS_MOVEMENT && info.preCall) {
		auto moveData = (CMoveData *)info.data;

		Vector wishDir(moveData->m_flForwardMove, moveData->m_flSideMove);
		float moveAng = (sinf(time * 1.9) + cosf(time * 1.8)) * (sinf(time * 1.7) + cosf(time * 1.6)) * 0.2 * td;
		moveData->m_flForwardMove = wishDir.x * cosf(moveAng) + wishDir.y * sinf(moveAng);
		moveData->m_flSideMove = wishDir.y * cosf(moveAng) + wishDir.x * sinf(moveAng);

		moveData->m_flForwardMove += (sinf(time * 1.5) + cosf(time * 1.4)) * (sinf(time * 1.3) + cosf(time * 1.2)) * 10.0f * td;
		moveData->m_flSideMove += (sinf(time * 1.3) + cosf(time * 1.2)) * (sinf(time * 1.1) + cosf(time * 1.0)) * 10.0f * td;
	}
	if (info.execType == OVERRIDE_CAMERA) {
		auto viewSetup = (CViewSetup *)info.data;
		viewSetup->angles.x += (sinf(time * 1.1) + cosf(time * 1.4)) * (sinf(time * 1.7) + cosf(time * 1.2)) * 5 * td;
		viewSetup->angles.y += (sinf(time * 1.5) + cosf(time * 1.6)) * (sinf(time * 1.5) + cosf(time * 1.1)) * 15 * td;
		viewSetup->angles.z += (sinf(time * 1.4) + cosf(time * 1.7)) * (sinf(time * 1.1) + cosf(time * 1.0)) * 10 * td;

		float fovMod = (sinf(time * 1.2) + cosf(time * 1.3)) * (sinf(time * 1.4) + cosf(time * 1.5)) * 30 * td;
		viewSetup->fov += fovMod;
		viewSetup->fovViewmodel += fovMod;
	}
}
