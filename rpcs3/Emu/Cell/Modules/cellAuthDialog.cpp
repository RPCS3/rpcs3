#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellSysutil);

// All error codes are unknown at this point in implementation
enum cellSysutilAuthDialogError : u32
{
	CELL_AUTHDIALOG_UNKNOWN_201  = 0x8002D201,
	CELL_AUTHDIALOG_ARG1_IS_ZERO = 0x8002D202,
	CELL_AUTHDIALOG_UNKNOWN_203  = 0x8002D203,
};

template<>
void fmt_class_string<cellSysutilAuthDialogError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_AUTHDIALOG_UNKNOWN_201);
			STR_CASE(CELL_AUTHDIALOG_ARG1_IS_ZERO);
			STR_CASE(CELL_AUTHDIALOG_UNKNOWN_203);
		}

		return unknown;
	});
}

// Decompilation suggests arg1 is s64 but the check is for == 0 instead of >= 0
error_code cellAuthDialogOpen(u64 arg1 /* arg2 */)
{
	cellSysutil.todo("cellAuthDialogOpen(arg1=%u)", arg1);

	if (arg1 == 0)
		return CELL_AUTHDIALOG_ARG1_IS_ZERO;

	return CELL_OK;
}

error_code cellAuthDialogAbort()
{
	cellSysutil.todo("cellAuthDialogAbort()");

	// If it fails the first if condition (not init cond?)
	// return CELL_AUTHDIALOG_UNKNOWN_203;

	return CELL_OK;
}

error_code cellAuthDialogClose(/* arg1 */)
{
	cellSysutil.todo("cellAuthDialogClose()");

	// If it fails the first if condition (not init cond?)
	// return CELL_AUTHDIALOG_UNKNOWN_203;

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellAuthDialogUtility)("cellAuthDialogUtility", []()
{
	REG_FUNC(cellAuthDialogUtility, cellAuthDialogOpen);
	REG_FUNC(cellAuthDialogUtility, cellAuthDialogAbort);
	REG_FUNC(cellAuthDialogUtility, cellAuthDialogClose);
});
