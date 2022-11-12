#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(PROCESS_MOVEMENT, moveAlwaysDuck, "I'm A Duck!", 2.5f, 0) {
	auto moveData = (CMoveData *)info.data;
	moveData->m_nButtons -= moveData->m_nButtons & IN_JUMP;
	moveData->m_nButtons |= IN_DUCK;
}
