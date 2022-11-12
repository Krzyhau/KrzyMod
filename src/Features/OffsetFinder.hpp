#pragma once
#include "Command.hpp"
#include "Utils.hpp"

namespace OffsetFinder{
	void ServerSide(const char *className, const char *propName, int *offset);
	void ClientSide(const char *className, const char *propName, int *offset);

	int16_t Find(SendTable *table, const char *propName);
	int16_t Find(RecvTable *table, const char *propName);
};
