#pragma once

#include "Emu/Io/PadHandler.h"
#include <Xinput.h>

class XInputPadHandler : public PadHandlerBase
{
public:
	XInputPadHandler();
	~XInputPadHandler();

	void Init(const u32 max_connect) override;
	void Close() override;

private:
	typedef void (WINAPI * PFN_XINPUTENABLE)(BOOL);
	typedef DWORD (WINAPI * PFN_XINPUTGETSTATE)(DWORD, XINPUT_STATE *);

private:
	DWORD ThreadProcedure();
	static DWORD WINAPI ThreadProcProxy(LPVOID parameter);

private:
	mutable bool active;
	HANDLE thread;
	HMODULE library;
	PFN_XINPUTGETSTATE xinputGetState;
	PFN_XINPUTENABLE xinputEnable;

};
