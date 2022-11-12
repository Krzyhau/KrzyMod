#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD(playerLaunchUp, "Polish Space Program", 0.0f, 0) {
	static bool executed = false;
	if (info.execType == INITIAL) executed = false;
	if (info.execType == PROCESS_MOVEMENT && !info.preCall && !executed) {
		auto moveData = (CMoveData *)info.data;
		Vector newVec = moveData->m_vecVelocity + Vector(0, 0, 3600.0f);
		moveData->m_vecVelocity = newVec;
		executed = true;
	}
}
