#pragma once
#include "Interface.hpp"
#include "Module.hpp"
#include "Utils.hpp"
#include "Game.hpp"

#include <vector>
#include <array>

enum HudType {
	HudType_NotSpecified = 0,
	HudType_InGame = (1 << 0),
	HudType_Paused = (1 << 1),
	HudType_Menu = (1 << 2),
	HudType_LoadingScreen = (1 << 3)
};

class BaseHud {
public:
	int type;
	bool drawSecondSplitScreen;
	int version;

public:
	BaseHud(int type, bool drawSecondSplitScreen, int version);
	virtual bool ShouldDraw();
};

class Hud : public BaseHud {
public:
	static std::vector<Hud *> &GetList();

public:
	Hud(int type, bool drawSecondSplitScreen = false, int version = SourceGame_Unknown);

public:
	virtual bool GetCurrentSize(int &xSize, int &ySize) = 0;
	virtual void Paint(int slot) = 0;

	float PositionFromString(const char *str, bool isX);
};



class VGui : public Module {
public:
	Interface *enginevgui = nullptr;

private:
	std::vector<Hud *> huds = std::vector<Hud *>();

	int lastProgressBar = 0;
	int progressBarCount = 0;

private:
	void Draw(Hud *const &hud);
public:
	using _IsGameUIVisible = bool(__rescall *)(void *thisptr);

	_IsGameUIVisible IsGameUIVisible = nullptr;

	bool IsUIVisible();

	// CEngineVGui::Paint
	DECL_DETOUR(Paint, PaintMode_t mode);

	bool Init() override;
	void Shutdown() override;
	const char *Name() override { return MODULE("engine"); }
};

extern VGui *vgui;
