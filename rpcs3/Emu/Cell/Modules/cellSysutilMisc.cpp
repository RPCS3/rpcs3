#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "cellSysutil.h"

LOG_CHANNEL(cellSysutilMisc);

s32 cellSysutilGetLicenseArea()
{
	cellSysutilMisc.warning("cellSysutilGetLicenseArea()");

	// TODO: These values should be read from the Console Region setting, rather than from the Game ID
	// Detecting license area from MRTC and NPIA title IDs is not possible for example
	// Also, there are some games where title ID region doesn't match content ID region 
	switch (const char region = Emu.GetTitleID().size() >= 3u ? Emu.GetTitleID().at(2) : '\0')
	{
	case 'J': return CELL_SYSUTIL_LICENSE_AREA_J;
	case 'U': return CELL_SYSUTIL_LICENSE_AREA_A;
	case 'E': return CELL_SYSUTIL_LICENSE_AREA_E;
	case 'H': return CELL_SYSUTIL_LICENSE_AREA_H;
	case 'K': return CELL_SYSUTIL_LICENSE_AREA_K;
	case 'A': return CELL_SYSUTIL_LICENSE_AREA_C;
	case 'I': return CELL_SYSUTIL_LICENSE_AREA_J; // International content (NPIA)
	default: cellSysutilMisc.todo("Unknown license area: %s", Emu.GetTitleID()); return CELL_SYSUTIL_LICENSE_AREA_OTHER;
	}
}

DECLARE(ppu_module_manager::cellSysutilMisc)("cellSysutilMisc", []()
{
	REG_FUNC(cellSysutilMisc, cellSysutilGetLicenseArea);
});
