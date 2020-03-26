#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellSysutil.h"

LOG_CHANNEL(cellSysutilNpEula);

struct sceNpEulaStructUnk0
{
	char buf[0xc];
};

// All error codes are unknown at this point in implementation and just guesses
enum cellSysutilNpEulaError : u32
{
	CELL_SYSUTIL_NP_EULA_NOT_INIT      = 0x8002E500,
	CELL_SYSUTIL_NP_EULA_INVALID_PARAM = 0x8002E501,
	CELL_SYSUTIL_NP_EULA_UNKNOWN_502   = 0x8002E502,
	CELL_SYSUTIL_NP_EULA_UNKNOWN_503   = 0x8002E503,
};

template<>
void fmt_class_string<cellSysutilNpEulaError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SYSUTIL_NP_EULA_NOT_INIT);
			STR_CASE(CELL_SYSUTIL_NP_EULA_INVALID_PARAM);
			STR_CASE(CELL_SYSUTIL_NP_EULA_UNKNOWN_502);
			STR_CASE(CELL_SYSUTIL_NP_EULA_UNKNOWN_503);
		}

		return unknown;
	});
}

// Seen on: Resistance 3, Uncharted 2
error_code sceNpEulaCheckEulaStatus(vm::ptr<sceNpEulaStructUnk0> arg1, u32 arg2, u64 arg3, u32 arg4, vm::ptr<CellSysutilCallback> callback1)
{
	cellSysutilNpEula.todo("sceNpEulaCheckEulaStatus(arg1=*0x%x, arg2=0x%x, arg3=0x%x, arg4=0x%x, callback1=*0x%x)", arg1, arg2, arg3, arg4, callback1);

	if (!arg1)
		return CELL_SYSUTIL_NP_EULA_INVALID_PARAM;

	if (arg4 == 0)
		return CELL_SYSUTIL_NP_EULA_INVALID_PARAM;

	return CELL_OK;
}

// Seen on: Resistance 3
error_code sceNpEulaAbort()
{
	cellSysutilNpEula.todo("sceNpEulaAbort()");

	// Can return CELL_SYSUTIL_NP_EULA_NOT_INIT

	return CELL_OK;
}

// Seen on: Resistance 3, Uncharted 2
error_code sceNpEulaShowCurrentEula(vm::ptr<sceNpEulaStructUnk0> arg1, u64 arg2, vm::ptr<CellSysutilCallback> callback1, vm::ptr<CellSysutilCallback> callback2)
{
	cellSysutilNpEula.todo("sceNpEulaShowCurrentEula(arg1=*0x%x, arg2=0x%x, callback1=*0x%x, callback2=*0x%x)", arg1, arg2, callback1, callback2);

	if (!arg1)
		return CELL_SYSUTIL_NP_EULA_INVALID_PARAM;

	if (!callback1)
		return CELL_SYSUTIL_NP_EULA_INVALID_PARAM;

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSysutilNpEula)("cellSysutilNpEula", []()
{
	REG_FUNC(cellSysutilNpEula, sceNpEulaCheckEulaStatus);
	REG_FUNC(cellSysutilNpEula, sceNpEulaAbort);
	REG_FUNC(cellSysutilNpEula, sceNpEulaShowCurrentEula);
});
