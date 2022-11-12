#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(TRACERAY, gameOnlyWallsPortalable, "Portals Only On Walls", 2.5f, 4) {
	auto tr = (CGameTrace *)info.data;

	if (fabsf(tr->plane.normal.z) > 0.2) {
		tr->surface.flags |= SURF_NOPORTAL;
	}
}
