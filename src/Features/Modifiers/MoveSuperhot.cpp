#include "Features/KrzyMod.hpp"

#include "Modules/Engine.hpp"

CREATE_KRZYMOD(moveSuperhot, "SUPER HOT", 3.5f, 2) {
	static Vector prevAngles;
	static float superHotValue;
	static Variable host_timescale("host_timescale");
	if (info.execType == INITIAL) {
		superHotValue = 1;
	}
	if (info.execType == PROCESS_MOVEMENT) {
		if (!info.preCall) return;
		auto moveData = (CMoveData *)info.data;

		bool shouldIncrease = moveData->m_flForwardMove != 0 || moveData->m_flSideMove != 0;

		Vector newAngles = QAngleToVector(engine->GetAngles(GET_SLOT()));
		if ((prevAngles - newAngles).Length() > 1) shouldIncrease = true;

		superHotValue = fminf(fmaxf(superHotValue + 0.04 * (shouldIncrease ? 1 : -1), 0.1), 1.0);

		host_timescale.SetValue(superHotValue);

		prevAngles = newAngles;
	}
	if (info.execType == LAST) {
		host_timescale.SetValue(1);
	}
}
