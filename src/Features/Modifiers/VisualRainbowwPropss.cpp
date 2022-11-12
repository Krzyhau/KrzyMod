#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"

CREATE_KRZYMOD(visualRainbowwPropss, "RainbowwPropss", 4.5f, 0) {
	if (info.execType == INITIAL) {
		KRZYMOD_CONTROL_CVAR(r_colorstaticprops, 1);
	}
	if (info.execType == ENGINE_TICK && info.preCall) {
		float time = engine->GetClientTime();

		for (int i = 0; i < Offsets::NUM_ENT_ENTRIES; ++i) {
			void *ent = server->m_EntPtrArray[i].m_pEntity;
			if (!ent) continue;

			const char *entClass = server->GetEntityClassName(ent);
			if (!std::strstr(entClass, "prop") && !std::strstr(entClass, "npc")) continue;

			float colorVal = fmodf((time + i) * 0.3f, 1.0f);

			Vector color = {
				fminf(fmaxf(fabs(colorVal * 6.0 - 3.0) - 1.0, 0), 1) * 255,
				fminf(fmaxf(2.0 - fabs(colorVal * 6.0 - 2.0), 0), 1) * 255,
				fminf(fmaxf(2.0 - fabs(colorVal * 6.0 - 4.0), 0), 1) * 255};

			server->SetKeyValueVector(nullptr, ent, "rendercolor", color);
		}
	}
}
