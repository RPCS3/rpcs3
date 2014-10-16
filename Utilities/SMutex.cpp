#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/CPU/CPUThread.h"

#include "Utilities/SMutex.h"

bool SM_IsAborted()
{
	return Emu.IsStopped();
}
