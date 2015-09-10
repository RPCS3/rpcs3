#include "stdafx.h"
#include "Emu/System.h"
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

	auto region = Emu.GetTitleID().substr(3, 1).c_str();
	if (region == "J")
	{
		return CELL_SYSUTIL_LICENSE_AREA_J;
	}
	else if (region == "U")
	{
		return CELL_SYSUTIL_LICENSE_AREA_A;
	}
	else if (region == "E")
	{
		return CELL_SYSUTIL_LICENSE_AREA_E;
	}
	else if (region == "H")
	{
		return CELL_SYSUTIL_LICENSE_AREA_H;
	}
	else if (region == "K")
	{
		return CELL_SYSUTIL_LICENSE_AREA_K;
	}
	else if (region == "A")
	{
		return CELL_SYSUTIL_LICENSE_AREA_C;
	}
	else
	{
		cellSysutilMisc.Todo("Unknown license area: %s (%s)", region, Emu.GetTitleID().c_str());
		return CELL_SYSUTIL_LICENSE_AREA_OTHER;
	}
}

Module cellSysutilMisc("cellSysutilMisc", []()
{
	REG_FUNC(cellSysutilMisc, cellSysutilGetLicenseArea);
});
