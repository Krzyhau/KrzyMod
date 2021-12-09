#include "VGui.hpp"

#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
#include "Modules/Surface.hpp"
#include "PluginMain.hpp"

#include <algorithm>

REDECL(VGui::Paint);


BaseHud::BaseHud(int type, bool drawSecondSplitScreen, int version)
	: type(type)
	, drawSecondSplitScreen(drawSecondSplitScreen)
	, version(version) {
}
bool BaseHud::ShouldDraw() {
	if (!engine->hoststate->m_activeGame) {
		return this->type & HudType_Menu;
	}

	if (engine->IsGamePaused()) {
		return this->type & HudType_Paused;
	}

	if (!engine->IsGamePaused()) {
		return this->type & HudType_InGame;
	}

	return this->type & HudType_LoadingScreen;
}

std::vector<Hud *> &Hud::GetList() {
	static std::vector<Hud *> list;
	return list;
}

Hud::Hud(int type, bool drawSecondSplitScreen, int version)
	: BaseHud(type, drawSecondSplitScreen, version) {
	Hud::GetList().push_back(this);
}


void VGui::Draw(Hud *const &hud) {
	if (hud->ShouldDraw()) {
		hud->Paint(GET_SLOT());
	}
}


// CEngineVGui::Paint
DETOUR(VGui::Paint, PaintMode_t mode) {
	auto result = VGui::Paint(thisptr, mode);

	surface->StartDrawing(surface->matsurface->ThisPtr());

	if (GET_SLOT() == 0) {
		if (mode & PAINT_UIPANELS) {
			for (auto const &hud : vgui->huds) {
				vgui->Draw(hud);
			}
		}
	}

	surface->FinishDrawing();

	return result;
}

bool VGui::IsUIVisible() {
	return this->IsGameUIVisible(this->enginevgui->ThisPtr());
}

bool VGui::Init() {
	this->enginevgui = Interface::Create(this->Name(), "VEngineVGui001");
	if (this->enginevgui) {
		this->IsGameUIVisible = this->enginevgui->Original<_IsGameUIVisible>(Offsets::IsGameUIVisible);

		this->enginevgui->Hook(VGui::Paint_Hook, VGui::Paint, Offsets::Paint);

		for (auto &hud : Hud::GetList()) {
			if (hud->version == SourceGame_Unknown || pluginMain.game->Is(hud->version)) {
				this->huds.push_back(hud);
			}
		}
	}

	return this->hasLoaded = this->enginevgui;
}
void VGui::Shutdown() {
	Interface::Delete(this->enginevgui);
	this->huds.clear();
}

VGui *vgui;
