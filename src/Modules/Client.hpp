#pragma once
#include "Command.hpp"
#include "Interface.hpp"
#include "Module.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

#include <cstdint>

enum class CMStatus {
	CHALLENGE,
	WRONG_WARP,
	NONE,
};

class Client : public Module {
private:
	Interface *g_ClientDLL = nullptr;
	Interface *g_pClientMode = nullptr;
	Interface *g_HUDQuickInfo = nullptr;
	Interface *s_EntityList = nullptr;
	Interface *g_Input = nullptr;
	Interface *g_HudChat = nullptr;

public:
	using _GetClientEntity = void *(__rescall *)(void *thisptr, int entnum);
	using _KeyDown = int(__cdecl *)(void *b, const char *c);
	using _KeyUp = int(__cdecl *)(void *b, const char *c);
	using _GetAllClasses = ClientClass *(*)();
	using _ShouldDraw = bool(__rescall *)(void *thisptr);
	using _ChatPrintf = void (*)(void *thisptr, int iPlayerIndex, int iFilter, const char *fmt, ...);
	using _IN_ActivateMouse = void (*)(void *thisptr);
	using _IN_DeactivateMouse = void (*)(void *thisptr);

	_GetClientEntity GetClientEntity = nullptr;
	_KeyDown KeyDown = nullptr;
	_KeyUp KeyUp = nullptr;
	_GetAllClasses GetAllClasses = nullptr;
	_ShouldDraw ShouldDraw = nullptr;
	_ChatPrintf ChatPrintf = nullptr;
	_IN_ActivateMouse IN_ActivateMouse = nullptr;
	_IN_DeactivateMouse IN_DeactivateMouse = nullptr;

	std::string lastLevelName;

public:
	DECL_M(GetAbsOrigin, Vector);
	DECL_M(GetAbsAngles, QAngle);
	DECL_M(GetLocalVelocity, Vector);
	DECL_M(GetViewOffset, Vector);

	void *GetPlayer(int index);
	void CalcButtonBits(int nSlot, int &bits, int in_button, int in_ignore, kbutton_t *button, bool reset);
	bool ShouldDrawCrosshair();
	void Chat(TextColor color, const char *fmt, ...);
	void QueueChat(TextColor color, const char *fmt, ...);
	void FlushChatQueue();
	void SetMouseActivated(bool state);
	CMStatus GetChallengeStatus();
	int GetSplitScreenPlayerSlot(void *entity);

public:

	// CHLClient::LevelInitPreEntity
	DECL_DETOUR(LevelInitPreEntity, const char *levelName);

	// CInput::CreateMove
	DECL_DETOUR(CInput_CreateMove, int sequence_number, float input_sample_frametime, bool active);

	// CInput::GetButtonBits
	DECL_DETOUR(GetButtonBits, bool bResetState);
	// ClientModeShared::OverrideView
	DECL_DETOUR(OverrideView, CViewSetup *m_View);

	bool Init() override;
	void Shutdown() override;
	const char *Name() override { return MODULE("client"); }

private:
	std::vector<std::pair<TextColor, std::string>> chatQueue;
};

extern Client *client;

extern Variable cl_showpos;
extern Variable cl_sidespeed;
extern Variable cl_backspeed;
extern Variable cl_forwardspeed;
extern Variable in_forceuser;
extern Variable cl_fov;
extern Variable prevent_crouch_jump;
