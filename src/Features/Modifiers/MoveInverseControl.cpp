#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD(moveInverseControl, "Inverse Controls", 3.5f, 0) {
	if (info.execType == INITIAL) {
		//	reverse angles
		Variable yaw = Variable("m_yaw");
		krzyMod.AddConvarController(yaw, std::to_string(yaw.GetFloat() * -1), info.endTime, (KrzyModEffect *)info.data);
		Variable pitch = Variable("m_pitch");
		krzyMod.AddConvarController(pitch, std::to_string(pitch.GetFloat() * -1), info.endTime, (KrzyModEffect *)info.data);
	}
	if (info.execType == PROCESS_MOVEMENT) {
		auto moveData = (CMoveData *)info.data;

		//	reverse movement
		moveData->m_flForwardMove *= -1;
		moveData->m_flSideMove *= -1;

		//	reverse portals
		bool bluePortal = (moveData->m_nButtons & IN_ATTACK) > 0;
		bool orangePortal = (moveData->m_nButtons & IN_ATTACK2) > 0;
		moveData->m_nButtons &= ~(IN_ATTACK | IN_ATTACK2);
		if (bluePortal) moveData->m_nButtons &= IN_ATTACK2;
		if (orangePortal) moveData->m_nButtons &= IN_ATTACK;
	}
}
