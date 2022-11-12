#include "Features/KrzyMod.hpp"
#include "Modules/Server.hpp"

CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, moveAirlock, "No Air Control", 3.5f, 0) {
	void *player = server->GetPlayer(1);
	if (!player) return;
	unsigned int groundEntity = *reinterpret_cast<unsigned int *>((uintptr_t)player + Offsets::S_m_hGroundEntity);
	bool grounded = groundEntity != 0xFFFFFFFF;

	auto moveData = (CMoveData *)info.data;

	if (!grounded) {
		moveData->m_flForwardMove = 0;
		moveData->m_flSideMove = 0;
	}
}
