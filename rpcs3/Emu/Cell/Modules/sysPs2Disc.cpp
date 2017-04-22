#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel sysPs2Disc("sysPs2Disc", logs::level::notice);

s32 sysPs2Disc_A84FD3C3()
{
	UNIMPLEMENTED_FUNC(sysPs2Disc);
	return CELL_OK;
}

s32 sysPs2Disc_BB7CD1AE()
{
	UNIMPLEMENTED_FUNC(sysPs2Disc);
	return CELL_OK;
}

DECLARE(ppu_module_manager::sysPs2Disc)("sysPs2Disc", []()
{
	REG_FNID(sysPs2Disc, 0xA84FD3C3, sysPs2Disc_A84FD3C3);
	REG_FNID(sysPs2Disc, 0xBB7CD1AE, sysPs2Disc_BB7CD1AE);
});
