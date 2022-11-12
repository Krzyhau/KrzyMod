#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD(playerLaunchRandom, "Yeet Player", 0.0f, 0) {
	static bool executed = false;
	if (info.execType == INITIAL) executed = false;
	if (info.execType == PROCESS_MOVEMENT && !info.preCall && !executed) {
		auto moveData = (CMoveData *)info.data;
		Vector newVec = moveData->m_vecVelocity + Vector(
			Math::RandomNumber(-1.0f, 1.0f), 
			Math::RandomNumber(-1.0f, 1.0f), 
			Math::RandomNumber(0.0f, 1.0f) //different range here to give more interesting yeets
		).Normalize() * Math::RandomNumber(500.0f,1500.0f);
		moveData->m_vecVelocity = newVec;
		executed = true;
	}
}
