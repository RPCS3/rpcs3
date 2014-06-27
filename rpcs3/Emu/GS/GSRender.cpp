#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "GSRender.h"

GSLockCurrent::GSLockCurrent(GSLockType type) : GSLock(Emu.GetGSManager().GetRender(), type)
{
}
