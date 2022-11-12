#include "Features/KrzyMod.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(gameChangePortalgunLinkage, "These aren't my portals!", 0) {
	engine->ExecuteCommand("change_portalgun_linkage_id 0 100 1");
}
