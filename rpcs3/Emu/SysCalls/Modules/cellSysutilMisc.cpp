#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellSysutilMisc;

// License areas
enum
{
	CELL_SYSUTIL_LICENSE_AREA_J     = 0,
	CELL_SYSUTIL_LICENSE_AREA_A     = 1,
	CELL_SYSUTIL_LICENSE_AREA_E     = 2,
	CELL_SYSUTIL_LICENSE_AREA_H     = 3,
	CELL_SYSUTIL_LICENSE_AREA_K     = 4,
	CELL_SYSUTIL_LICENSE_AREA_C     = 5,
	CELL_SYSUTIL_LICENSE_AREA_OTHER = 100,
};

s32 cellSysutilGetLicenseArea()
{
	cellSysutilMisc.Warning("cellSysutilGetLicenseArea()");

	if (Ini.HLELicenseArea.GetValue() == 0)
	{
		return CELL_SYSUTIL_LICENSE_AREA_J;
	}
	else if (Ini.HLELicenseArea.GetValue() == 1)
	{
		return CELL_SYSUTIL_LICENSE_AREA_A;
	}
	else if (Ini.HLELicenseArea.GetValue() == 2)
	{
		return CELL_SYSUTIL_LICENSE_AREA_E;
	}
	else if (Ini.HLELicenseArea.GetValue() == 3)
	{
		return CELL_SYSUTIL_LICENSE_AREA_H;
	}
	else if (Ini.HLELicenseArea.GetValue() == 4)
	{
		return CELL_SYSUTIL_LICENSE_AREA_K;
	}
	else if (Ini.HLELicenseArea.GetValue() == 4)
	{
		return CELL_SYSUTIL_LICENSE_AREA_C;
	}
	else
	{
		return CELL_SYSUTIL_LICENSE_AREA_OTHER;
	}
}

Module cellSysutilMisc("cellSysutilMisc", []()
{
	REG_FUNC(cellSysutilMisc, cellSysutilGetLicenseArea);
});
