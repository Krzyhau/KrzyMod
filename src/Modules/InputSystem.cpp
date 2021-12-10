#include "InputSystem.hpp"

#include "Cheats.hpp"
#include "Command.hpp"
#include "Interface.hpp"
#include "Module.hpp"
#include "Offsets/Offsets.hpp"
#include "PluginMain.hpp"
#include "Utils.hpp"

int InputSystem::GetButton(const char *pString) {
	return this->StringToButtonCode(this->g_InputSystem->ThisPtr(), pString);
}

bool InputSystem::IsKeyDown(ButtonCode_t key) {
	return this->IsButtonDown(this->g_InputSystem->ThisPtr(), key);
}

void InputSystem::GetCursorPos(int &x, int &y) {
	return this->GetCursorPosition(this->g_InputSystem->ThisPtr(), x, y);
}

void InputSystem::SetCursorPos(int x, int y) {
	return this->SetCursorPosition(this->g_InputSystem->ThisPtr(), x, y);
}

bool InputSystem::Init() {
	this->g_InputSystem = Interface::Create(this->Name(), "InputSystemVersion001");
	if (this->g_InputSystem) {
		this->StringToButtonCode = this->g_InputSystem->Original<_StringToButtonCode>(Offsets::StringToButtonCode);

		this->IsButtonDown = this->g_InputSystem->Original<_IsButtonDown>(Offsets::IsButtonDown);
		this->GetCursorPosition = this->g_InputSystem->Original<_GetCursorPosition>(Offsets::GetCursorPosition);
		this->SetCursorPosition = this->g_InputSystem->Original<_SetCursorPosition>(Offsets::SetCursorPosition);
	}

	auto unbind = Command("unbind");
	if (!!unbind) {
		auto cc_unbind_callback = (uintptr_t)unbind.ThisPtr()->m_pCommandCallback;
		this->KeySetBinding = Memory::Read<_KeySetBinding>(cc_unbind_callback + Offsets::Key_SetBinding);
	}

	return this->hasLoaded = this->g_InputSystem && !!unbind;
}
void InputSystem::Shutdown() {
	Interface::Delete(this->g_InputSystem);
}

InputSystem *inputSystem;
