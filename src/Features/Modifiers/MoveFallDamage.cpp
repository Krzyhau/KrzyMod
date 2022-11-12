#include "Features/KrzyMod.hpp"
#include "Modules/Server.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, moveFallDamage, "Short Fall Boots", 3.5f, 0) {
	if (info.preCall) return;

	static float velLastFrame = 0;
	static float lastGroundedState = true;

	void *player = server->GetPlayer(1);
	if (!player) return;
	unsigned int groundEntity = *reinterpret_cast<unsigned int *>((uintptr_t)player + Offsets::S_m_hGroundEntity);
	bool grounded = groundEntity != 0xFFFFFFFF;


	if (!lastGroundedState && grounded) {
		float damage = fmaxf((velLastFrame - 300.0f) * 0.25f, 0);
		std::string command("hurtme ");
		command += std::to_string(damage * 2);
		engine->ExecuteCommand(command.c_str());
	}

	auto moveData = (CMoveData *)info.data;
	velLastFrame = fabs(moveData->m_vecVelocity.z);
	lastGroundedState = grounded;
}
