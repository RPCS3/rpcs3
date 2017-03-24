#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellSysconf.h"

logs::channel cellSysconf("cellSysconf");

s32 cellSysconfAbort()
{
	cellSysconf.todo("cellSysconfAbort()");
	return CELL_OK;
}

s32 cellSysconfOpen(u32 type, vm::ptr<CellSysconfCallback> func, vm::ptr<void> userdata, vm::ptr<void> extparam, u32 id)
{
	cellSysconf.todo("cellSysconfOpen(type=%d, func=*0x%x, userdata=*0x%x, extparam=*0x%x, id=%d)", type, func, userdata, extparam, id);
	return CELL_OK;
}

s32 cellSysconfBtGetDeviceList(vm::ptr<CellSysconfBtDeviceList> deviceList)
{
	cellSysconf.todo("cellSysconfBtGetDeviceList(deviceList=*0x%x)", deviceList);
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
