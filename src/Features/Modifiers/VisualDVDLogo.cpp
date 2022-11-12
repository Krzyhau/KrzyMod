#include "Features/KrzyMod.hpp"

#include "Modules/Engine.hpp"
#include "Modules/Surface.hpp"

CREATE_KRZYMOD_SIMPLE(HUD_PAINT, visualDvdLogo, "DVD Logo", 4.5f, 0) {
	int xScreen, yScreen;
	engine->GetScreenSize(nullptr, xScreen, yScreen);

	// procedurally generating DVD logo.
	// yes, I'm a madman
	static int dvdLogoTexID = 0;
	if (dvdLogoTexID == 0) {
		dvdLogoTexID = surface->CreateNewTextureID(surface->matsurface->ThisPtr(), true);

		const int texWidth = 248;
		const int texHeight = 140;

		unsigned char texDataRGBA[texWidth * texHeight * 4];

		for (int y = 0; y < texHeight; y++) {
			for (int x = 0; x < texWidth; x++) {
				Vector pos(x / (float)texWidth, y / (float)texHeight);

				//	disk at the bottom
				bool diskCircle1 = ((pos - Vector(0.5, 0.8)) * Vector(1.0, 3.3)).Length() < 0.485;
				bool diskCircle2 = ((pos - Vector(0.503, 0.793)) * Vector(1.0, 2.3)).Length() < 0.115;
				bool disk = diskCircle1 && !diskCircle2;
				pos.x -= (1.0 - pos.y) * 0.1 - 0.02;  //	ofsetting for tilted look
				// V and parts of D's
				bool v = ((fabsf((pos.y - fmaxf(0, 0.5 - fabsf(powf(pos.x, 0.45) - 0.695) * 4.6)))) < 0.155) && pos.y > 0.056 && pos.x < 0.7 && pos.x > 0;
				// using """modulo""" to repeat D's
				bool xNot0 = pos.x > 0;
				Vector pos2 = pos;
				if (pos2.x >= 0.615f) pos2.x -= 0.615f;
				// D's
				bool d1 = ((pos2 - Vector(0.09, 0.3)) * Vector(0.89, 0.82)).Length() < 0.2;
				bool d2 = ((pos2 - Vector(0.09, 0.3)) * Vector(0.89, 0.82)).Length() < 0.12;
				bool d = xNot0 && ((d1 && !d2 && pos2.x > 0.07) || (pos2.x < 0.09 && pos2.y > 0.2 && pos2.y < 0.544));

				int pixelColor = (disk || v || d) ? 255 : 0;

				int pixelPos = (y * texWidth + x) * 4;
				texDataRGBA[pixelPos + 0] = pixelColor;
				texDataRGBA[pixelPos + 1] = pixelColor;
				texDataRGBA[pixelPos + 2] = pixelColor;
				texDataRGBA[pixelPos + 3] = pixelColor;
			}
		}
		surface->DrawSetTextureRGBA(surface->matsurface->ThisPtr(), dvdLogoTexID, texDataRGBA, texWidth, texHeight);
	}

	float dvdWidth = xScreen * 0.2;
	float dvdHeight = dvdWidth * 0.5;
	float maxX = xScreen - dvdWidth;
	float maxY = yScreen - dvdHeight;
	float speed = dvdWidth;  //	move at speed of your own width per second

	float time = engine->GetClientTime() * speed;

	float posX = fmod(time, maxX);
	float posY = fmod(time, maxY);
	int bouncesX = (int)(time / maxX);
	int bouncesY = (int)(time / maxY);
	if (bouncesX % 2 == 1) posX = maxX - posX;
	if (bouncesY % 2 == 1) posY = maxY - posY;

	static int previousBounceCounter = 0;
	static Color dvdColor = Color(255, 255, 255, 255);
	if (bouncesX + bouncesY != previousBounceCounter) {
		previousBounceCounter = bouncesX + bouncesY;
		dvdColor.SetColor(Math::RandomNumber(100, 255), Math::RandomNumber(100, 255), Math::RandomNumber(100, 255));
	}

	surface->DrawSetColor(surface->matsurface->ThisPtr(), dvdColor.r(), dvdColor.g(), dvdColor.b(), dvdColor.a());
	surface->DrawSetTexture(surface->matsurface->ThisPtr(), dvdLogoTexID);
	surface->DrawTexturedRect(surface->matsurface->ThisPtr(), posX, posY, posX + dvdWidth, posY + dvdHeight);
}
