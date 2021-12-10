#include "Version.hpp"


#include "Hud.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Surface.hpp"

#define WATERMARK_MSG "KrzyMod v1.0"

class WatermarkHud : public Hud {
public:
	WatermarkHud()
		: Hud(HudType_InGame | HudType_Paused | HudType_Menu | HudType_LoadingScreen, false) {
	}

	bool ShouldDraw() override {
		return true;
	}

	bool GetCurrentSize(int &w, int &h) override {
		return false;
	}

	void Paint(int slot) override {
		int screenWidth, screenHeight;
		engine->GetScreenSize(nullptr, screenWidth, screenHeight);

		Surface::HFont font = 6;

		int height = surface->GetFontHeight(font);
		int width = surface->GetFontLength(font, "%s", WATERMARK_MSG);

		surface->DrawTxt(font, screenWidth - width - 10, screenHeight - height - 10, Color{255, 255, 255, 100}, "%s", WATERMARK_MSG);
	}
};

WatermarkHud watermark;
