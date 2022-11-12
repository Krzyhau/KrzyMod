#include "Features/KrzyMod.hpp"

#include "Modules/Engine.hpp"
#include "Modules/Client.hpp"

CREATE_KRZYMOD(viewGTA2, "GTA II", 3.5f, 5) {
	if (info.execType == INITIAL) {
		KRZYMOD_CONTROL_CVAR(cl_skip_player_render_in_main_view, 0);
	}
	if (info.execType == PROCESS_MOVEMENT && info.preCall) {
		auto moveData = (CMoveData *)info.data;

		auto wishDir = Vector(moveData->m_flSideMove, moveData->m_flForwardMove);
		float moveForce = wishDir.Length();
		float moveAng = RAD2DEG(atan2f(wishDir.y, wishDir.x));

		moveData->m_flForwardMove = moveForce;
		moveData->m_flSideMove = 0;

		if (moveForce > 0) {
			QAngle finalAngles;
			finalAngles.y = moveAng;
			finalAngles.x = 0;
			if (moveData->m_nButtons & IN_DUCK) finalAngles.x = 89;
			if (moveData->m_vecVelocity.z > 20) finalAngles.x = -89;

			engine->SetAngles(GET_SLOT(), finalAngles);
			moveData->m_vecAngles = finalAngles;
			moveData->m_vecViewAngles = finalAngles;
			moveData->m_vecAbsViewAngles = finalAngles;
		}
	}
	if (info.execType == OVERRIDE_CAMERA) {
		auto viewSetup = (CViewSetup *)info.data;
		void *player = client->GetPlayer(1);
		if (!player) return;

		auto pPos = client->GetAbsOrigin(player);

		viewSetup->origin = pPos + Vector(0, 0, 400);
		viewSetup->angles = {90.0f, 90.0f, 0};
	}
}
