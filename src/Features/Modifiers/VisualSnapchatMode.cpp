#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Surface.hpp"
#include "Modules/Scheme.hpp"

CREATE_KRZYMOD_SIMPLE(HUD_PAINT, visualSnapchatMode, "Snapchat Mode", 2.5f, 0) {
	int xScreen, yScreen;
	engine->GetScreenSize(nullptr, xScreen, yScreen);

	float ratio = (float)yScreen / (float)xScreen;
	if (ratio > 1.0f) return;

	float newWidth = yScreen * ratio;
	float barWidth = (xScreen - newWidth) * 0.5f;

	surface->DrawRect(Color(0, 0, 0, 255), 0, 0, barWidth, yScreen);
	surface->DrawRect(Color(0, 0, 0, 255), barWidth + newWidth, 0, xScreen, yScreen);


	auto font = scheme->GetDefaultFont() + 1;
	int fontHeight = surface->GetFontHeight(font);

	float textYPos = yScreen * 0.7f;

	surface->DrawRect(Color(0, 0, 0, 200), 0, textYPos, xScreen, textYPos + fontHeight + 20.0f);

	const char *lmaoText = "YOOO HOW DOES HE DO THAT?!";
	int fontWidth = surface->GetFontLength(font, lmaoText);
	surface->DrawTxt(font, (xScreen - fontWidth) * 0.5f, textYPos + 10.0f, Color(255, 255, 255, 255), lmaoText);
}
