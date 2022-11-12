#include "OffsetFinder.hpp"

#include "Command.hpp"
#include "Modules/Client.hpp"
#include "Modules/Console.hpp"
#include "Modules/Server.hpp"
#include "PluginMain.hpp"

#include <cstring>

void OffsetFinder::ServerSide(const char *className, const char *propName, int *offset) {
	if (server->GetAllServerClasses) {
		for (auto curClass = server->GetAllServerClasses(); curClass; curClass = curClass->m_pNext) {
			if (!std::strcmp(curClass->m_pNetworkName, className)) {
				auto result = OffsetFinder::Find(curClass->m_pTable, propName);
				if (result != 0) {
					console->DevMsg("Found %s::%s at %i (server-side)\n", className, propName, result);
					if (offset)
						*offset = result;
				}
				break;
			}
		}
	}

	if (offset && *offset == 0) {
		console->DevWarning("Failed to find offset for: %s::%s (server-side)\n", className, propName);
	}
}
void OffsetFinder::ClientSide(const char *className, const char *propName, int *offset) {
	if (client->GetAllClasses) {
		for (auto curClass = client->GetAllClasses(); curClass; curClass = curClass->m_pNext) {
			if (!std::strcmp(curClass->m_pNetworkName, className)) {
				auto result = OffsetFinder::Find(curClass->m_pRecvTable, propName);
				if (result != 0) {
					console->DevMsg("Found %s::%s at %i (client-side)\n", className, propName, result);
					if (offset)
						*offset = result;
				}
				break;
			}
		}
	}

	if (offset && *offset == 0) {
		console->DevWarning("Failed to find offset for: %s::%s (client-side)\n", className, propName);
	}
}
int16_t OffsetFinder::Find(SendTable *table, const char *propName) {
	for (auto i = 0; i < table->m_nProps; ++i) {
		auto prop = *reinterpret_cast<SendProp *>((uintptr_t)table->m_pProps + sizeof(SendProp) * i);

		auto name = prop.m_pVarName;
		auto offset = prop.m_Offset;
		auto type = prop.m_Type;
		auto nextTable = prop.m_pDataTable;

		if (!std::strcmp(name, propName)) {
			return offset;
		}

		if (type != SendPropType::DPT_DataTable) {
			continue;
		}

		if (auto nextOffset = OffsetFinder::Find(nextTable, propName)) {
			return offset + nextOffset;
		}
	}

	return 0;
}
int16_t OffsetFinder::Find(RecvTable *table, const char *propName) {
	for (auto i = 0; i < table->m_nProps; ++i) {
		auto prop = table->m_pProps[i];

		auto name = prop.m_pVarName;
		auto offset = prop.m_Offset;
		auto type = prop.m_RecvType;
		auto nextTable = prop.m_pDataTable;

		if (!std::strcmp(name, propName)) {
			return offset;
		}

		if (type != SendPropType::DPT_DataTable) {
			continue;
		}

		if (auto nextOffset = OffsetFinder::Find(nextTable, propName)) {
			return offset + nextOffset;
		}
	}

	return 0;
}