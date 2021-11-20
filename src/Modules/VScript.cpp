#include "VScript.hpp"

#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"
#include "Command.hpp"
#include "Console.hpp"
#include "Engine.hpp"

REDECL(VScript::CreateVM);
DETOUR_T(void*, VScript::CreateVM, int language) {
	auto scriptVM = VScript::CreateVM(thisptr, language);

	// if engine state is 4, client-side script VM is created. we want server one
	if (engine->hoststate->m_currentState != 4) {
		vscript->g_pScriptVM = scriptVM;
		if(!vscript->Run) vscript->Run = Memory::VMT<_Run>(vscript->g_pScriptVM, Offsets::ScriptRun);
	}

	return scriptVM;
}

VScript::VScript()
	: Module() {
}

void VScript::RunScript(const char* script) {
	if (vscript->g_pScriptVM != nullptr) {
		Run(vscript->g_pScriptVM, script, true);
	}
}

bool VScript::Init() {
	this->scriptmanager = Interface::Create(this->Name(), "VScriptManager009");
	if (this->scriptmanager) {
		this->scriptmanager->Hook(VScript::CreateVM_Hook, VScript::CreateVM, Offsets::CreateVM);
	}

	return this->hasLoaded = this->scriptmanager;
}
void VScript::Shutdown() {
	Interface::Delete(this->scriptmanager);
}

VScript* vscript;



CON_COMMAND(sar_test_vscript, "sar_test_vscript - tests vscript functionality\n") {
	vscript->RunScript(R"#(
		print("You should now see a set of random numbers: ");
		for(local i=0;i<10;i++){
			print(RandomInt(0,9));
		} print("\n");
	)#");
}