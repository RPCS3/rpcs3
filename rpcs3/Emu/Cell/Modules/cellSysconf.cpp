#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellSysutil.h"

#include "cellSysconf.h"

LOG_CHANNEL(cellSysconf);

template<>
void fmt_class_string<CellSysConfError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SYSCONF_ERROR_PARAM);
		}

		return unknown;
	});
}

error_code cellSysconfAbort()
{
	cellSysconf.todo("cellSysconfAbort()");
	return CELL_OK;
}

error_code cellSysconfOpen(u32 type, vm::ptr<CellSysconfCallback> func, vm::ptr<void> userdata, vm::ptr<void> extparam, u32 id)
{
	cellSysconf.todo("cellSysconfOpen(type=%d, func=*0x%x, userdata=*0x%x, extparam=*0x%x, id=%d)", type, func, userdata, extparam, id);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		func(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellSysconfBtGetDeviceList(vm::ptr<CellSysconfBtDeviceList> deviceList)
{
	cellSysconf.todo("cellSysconfBtGetDeviceList(deviceList=*0x%x)", deviceList);

	if (!deviceList)
	{
		return CELL_SYSCONF_ERROR_PARAM;
	}

	return CELL_OK;
}

void cellSysutil_Sysconf_init()
{
	REG_FUNC(cellSysutil, cellSysconfAbort);
	REG_FUNC(cellSysutil, cellSysconfOpen);
}

DECLARE(ppu_module_manager::cellSysconf)("cellSysconfExtUtility", []()
{
	REG_FUNC(cellSysconfExtUtility, cellSysconfBtGetDeviceList);
});
