#include "Cheats.hpp"

#include "Game.hpp"
#include "Modules/Client.hpp"
#include "Modules/Console.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"
#include "Offsets/Offsets.hpp"

#include <cstring>

Variable sv_laser_cube_autoaim;
Variable ui_loadingscreen_transition_time;
Variable ui_loadingscreen_fadein_time;
Variable ui_loadingscreen_mintransition_time;
Variable hide_gun_when_holding;


void Cheats::Init() {
	sv_laser_cube_autoaim = Variable("sv_laser_cube_autoaim");
	ui_loadingscreen_transition_time = Variable("ui_loadingscreen_transition_time");
	ui_loadingscreen_fadein_time = Variable("ui_loadingscreen_fadein_time");
	ui_loadingscreen_mintransition_time = Variable("ui_loadingscreen_mintransition_time");
	hide_gun_when_holding = Variable("hide_gun_when_holding");

	Variable::RegisterAll();
	Command::RegisterAll();

}
void Cheats::Shutdown() {
	Variable::UnregisterAll();
	Command::UnregisterAll();
}
