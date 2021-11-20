#pragma once
#include "Interface.hpp"
#include "Module.hpp"
#include "Utils.hpp"

class VScript : public Module {
public:
	Interface* scriptmanager = nullptr;

	void* g_pScriptVM = nullptr;

	using _Run = int(__rescall*)(void* thisptr, const char* pszScript, bool bWait);
	_Run Run = nullptr;

public:
	VScript();
	bool Init() override;
	void Shutdown() override;
	const char* Name() override { return MODULE("vscript"); }
	void RunScript(const char* script);

	// CScriptManager::CreateVM
	DECL_DETOUR_T(void*, CreateVM, int language);
};

extern VScript* vscript;
