#pragma once

#include "Emu/Io/PadHandler.h"
#include <mmsystem.h>

class MMJoystickHandler final : public PadHandlerBase
{
public:
	MMJoystickHandler();
	~MMJoystickHandler();

	void Init(const u32 max_connect) override;
	void Close();

private:
	DWORD ThreadProcedure();
	static DWORD WINAPI ThreadProcProxy(LPVOID parameter);

private:
	u32 supportedJoysticks;
	mutable bool active;
	HANDLE thread;
	JOYINFOEX    js_info;
	JOYCAPS   js_caps;
};
