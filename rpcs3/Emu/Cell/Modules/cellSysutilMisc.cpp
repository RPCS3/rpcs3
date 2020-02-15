#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellSysutilMisc);

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
	cellSysutilMisc.warning("cellSysutilGetLicenseArea()");

	switch (const char region = Emu.GetTitleID().at(2))
	{
	case 'J': return CELL_SYSUTIL_LICENSE_AREA_J;
	case 'U': return CELL_SYSUTIL_LICENSE_AREA_A;
	case 'E': return CELL_SYSUTIL_LICENSE_AREA_E;
	case 'H': return CELL_SYSUTIL_LICENSE_AREA_H;
	case 'K': return CELL_SYSUTIL_LICENSE_AREA_K;
	case 'A': return CELL_SYSUTIL_LICENSE_AREA_C;
	default: cellSysutilMisc.todo("Unknown license area: %s", Emu.GetTitleID().c_str()); return CELL_SYSUTIL_LICENSE_AREA_OTHER;
	}
}

DECLARE(ppu_module_manager::cellSysutilMisc)("cellSysutilMisc", []()
{
	REG_FUNC(cellSysutilMisc, cellSysutilGetLicenseArea);
});
