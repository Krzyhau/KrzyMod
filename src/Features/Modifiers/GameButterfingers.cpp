#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"

CREATE_KRZYMOD_SIMPLE(ENGINE_TICK, gameButterfingers, "Butterfingers!", 2.5f, 0) {
	static bool pressedUseLastTick = false;
	if (pressedUseLastTick) {
		engine->ExecuteCommand("-use");
	}

	void *player = server->GetPlayer(1);
	if (!player) return;

	auto m_hUseEntity = *reinterpret_cast<int *>((uintptr_t)player + Offsets::m_hUseEntity);
	bool isHoldingSth = m_hUseEntity != 0xFFFFFFFF;

	if (isHoldingSth && Math::RandomNumber(0.0f, 1.0f) > 0.95f) {
		engine->ExecuteCommand("+use");
		pressedUseLastTick = true;
	}
}
