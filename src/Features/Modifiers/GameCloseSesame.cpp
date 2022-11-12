#include "Features/KrzyMod.hpp"
#include "Modules/Server.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameCloseSesame, "Close Sesame!", 0) {
	for (int i = 0; i < Offsets::NUM_ENT_ENTRIES; ++i) {
		void *ent = server->m_EntPtrArray[i].m_pEntity;
		if (!ent) continue;

		const char *entName = server->GetEntityName(ent);
		if (entName == nullptr) continue;
		if (!std::strstr(entName, "door_close_relay") && !std::strstr(entName, "close_door")) continue;
		std::string command = "ent_fire " + std::string(entName) + " trigger";
		engine->ExecuteCommand(command.c_str());
	}
}
