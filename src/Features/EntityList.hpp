#pragma once
#include "Command.hpp"
#include "Utils.hpp"

namespace EntityList{
	CEntInfo *GetEntityInfoByIndex(int index);
	CEntInfo *GetEntityInfoByName(const char *name);
	CEntInfo *GetEntityInfoByClassName(const char *name);
	IHandleEntity *LookupEntity(const CBaseHandle &handle);
};
