#pragma once
#include "Command.hpp"
#include "Variable.hpp"

class Cheats {
public:
	void Init();
	void Shutdown();
};

extern Variable sv_laser_cube_autoaim;
extern Variable ui_loadingscreen_transition_time;
extern Variable ui_loadingscreen_fadein_time;
extern Variable ui_loadingscreen_mintransition_time;
extern Variable hide_gun_when_holding;
