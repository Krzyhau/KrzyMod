#include "Features/KrzyMod.hpp"

#include "Modules/Engine.hpp"

CREATE_KRZYMOD(moveDelayInput, "Delayed Inputs", 1.5f, 5) {
	const int LAG_COUNT = 30;
	static int oldButtons[LAG_COUNT];
	static Vector oldWishDirs[LAG_COUNT];
	static QAngle oldAngles[LAG_COUNT];
	QAngle angles = engine->GetAngles(GET_SLOT());
	if (info.execType == INITIAL) {
		for (int i = 0; i < LAG_COUNT; i++) {
			oldWishDirs[i] = {0, 0, 0};
			oldAngles[i] = angles;
			oldButtons[i] = 0;
		}
	}
	if (info.execType == PROCESS_MOVEMENT && info.preCall) {
		auto moveData = (CMoveData *)info.data;

		for (int i = 0; i < LAG_COUNT - 1; i++) {
			oldWishDirs[i] = oldWishDirs[i + 1];
			oldButtons[i] = oldButtons[i + 1];
		}
		oldWishDirs[LAG_COUNT - 1] = {moveData->m_flSideMove, moveData->m_flForwardMove};
		oldButtons[LAG_COUNT - 1] = moveData->m_nButtons;

		moveData->m_flSideMove = oldWishDirs[0].x;
		moveData->m_flForwardMove = oldWishDirs[0].y;
		moveData->m_nButtons = oldButtons[0];

		moveData->m_vecViewAngles = moveData->m_vecAbsViewAngles = moveData->m_vecAngles = oldAngles[0];
	}
	if (info.execType == OVERRIDE_CAMERA) {
		for (int i = 0; i < LAG_COUNT - 1; i++) {
			oldAngles[i] = oldAngles[i + 1];
		}
		oldAngles[LAG_COUNT - 1] = engine->GetAngles(GET_SLOT());

		auto viewSetup = (CViewSetup *)info.data;

		viewSetup->angles = oldAngles[0];
	}
}
