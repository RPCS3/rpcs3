#include "stdafx.h"

#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(sceNpPlus);

error_code sceNpManagerIsSP()
{
	sceNpPlus.warning("sceNpManagerIsSP()");
	// TODO seems to be cut to 1 byte by pshome likely a bool but may be more.
	return not_an_error(1);
}

DECLARE(ppu_module_manager::sceNpPlus)("sceNpPlus", []()
{
	REG_FUNC(sceNpPlus, sceNpManagerIsSP);
});
