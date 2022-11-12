#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD(visualOrtho, "Orthographic View", 1.5f, 0) {
	if (info.execType == INITIAL) {
		KRZYMOD_CONTROL_CVAR(r_portalsopenall, 1);
		KRZYMOD_CONTROL_CVAR(cl_skip_player_render_in_main_view, 0);
	}

	if (info.execType == OVERRIDE_CAMERA) {
		auto viewSetup = (CViewSetup *)info.data;
		viewSetup->m_bOrtho = true;

		int width, height;
		engine->GetScreenSize(nullptr, width, height);

		float halfWidth = width * 0.5f;
		float halfHeight = height * 0.5f;

		viewSetup->m_OrthoRight = halfWidth;
		viewSetup->m_OrthoLeft = -halfWidth;
		viewSetup->m_OrthoBottom = halfHeight;
		viewSetup->m_OrthoTop = -halfHeight;

		viewSetup->zNear = -10000;
	}
}
