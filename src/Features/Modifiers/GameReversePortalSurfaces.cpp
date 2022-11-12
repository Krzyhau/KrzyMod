#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(TRACERAY, gameReversePortalSurfaces, "Reverse Surface Portalability", 2.5f, 4) {
	auto tr = (CGameTrace *)info.data;

	tr->surface.flags ^= SURF_NOPORTAL;
}
