#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, moveWStuck, "Help My W Is Stuck", 2.5f, 0) {
	if (!info.preCall) return;
	auto moveData = (CMoveData *)info.data;
	moveData->m_flForwardMove += 175.0f;
}
