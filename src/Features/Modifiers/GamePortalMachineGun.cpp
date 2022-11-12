#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(INITIAL, gamePortalMachineGun, "Portal MacHINE GUN!", 2.5f, 0) {
	KRZYMOD_CONTROL_CVAR(portalgun_fire_delay, 0);
	KRZYMOD_CONTROL_CVAR(portalgun_held_button_fire_fire_delay, 0);
}
