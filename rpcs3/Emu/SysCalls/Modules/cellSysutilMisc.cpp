#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellSysutilMisc;

s32 cellSysutilGetLicenseArea()
{
	throw EXCEPTION("");
}

Module cellSysutilMisc("cellSysutilMisc", []()
{
	REG_FUNC(cellSysutilMisc, cellSysutilGetLicenseArea);
});
