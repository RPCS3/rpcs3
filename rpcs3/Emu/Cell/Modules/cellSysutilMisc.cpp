#include "stdafx.h"
#include "Emu/system_config.h"
#include "Emu/Cell/PPUModule.h"
#include "cellSysutil.h"

LOG_CHANNEL(cellSysutilMisc);

s32 cellSysutilGetLicenseArea()
{
	cellSysutilMisc.warning("cellSysutilGetLicenseArea()");

	const CellSysutilLicenseArea license_area = g_cfg.sys.license_area;
	cellSysutilMisc.notice("cellSysutilGetLicenseArea(): %s", license_area);
	return license_area;
}

DECLARE(ppu_module_manager::cellSysutilMisc)("cellSysutilMisc", []()
{
	REG_FUNC(cellSysutilMisc, cellSysutilGetLicenseArea);
});
