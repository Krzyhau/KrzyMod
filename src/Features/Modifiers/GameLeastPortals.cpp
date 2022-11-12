#include "Features/KrzyMod.hpp"

CREATE_KRZYMOD_SIMPLE(TRACERAY, gameLeastPortals, "Least Portals", 1.5f, 4) {
	auto tr = (CGameTrace *)info.data;

	tr->surface.flags |= SURF_NOPORTAL;
}
