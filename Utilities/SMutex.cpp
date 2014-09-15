#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/CPU/CPUThread.h"

#include "Utilities/SMutex.h"

bool SM_IsAborted()
{
	return Emu.IsStopped();
}

void SM_Sleep()
{
	if (NamedThreadBase* t = GetCurrentNamedThread())
	{
		t->WaitForAnySignal();
	}
	else
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
