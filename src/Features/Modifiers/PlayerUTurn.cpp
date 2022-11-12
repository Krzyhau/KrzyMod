#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD(playerUTurn, "U-Turn", 0.0f, 0) {
	static bool executed = false;
	if (info.execType == INITIAL) executed = false;
	if (info.execType == PROCESS_MOVEMENT && !info.preCall && !executed) {
		auto moveData = (CMoveData *)info.data;
		moveData->m_vecVelocity = moveData->m_vecVelocity * -1.0f;

		QAngle viewangles = engine->GetAngles(GET_SLOT());
		viewangles.y += 180;
		viewangles.x *= -1;
		engine->SetAngles(GET_SLOT(), viewangles);

		executed = true;
	}
}
