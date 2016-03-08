#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/Io/Mouse.h"
#include "cellMouse.h"

extern Module<> sys_io;

s32 cellMouseInit(u32 max_connect)
{
	sys_io.warning("cellMouseInit(max_connect=%d)", max_connect);

	if (Emu.GetMouseManager().IsInited())
	{
		return CELL_MOUSE_ERROR_ALREADY_INITIALIZED;
	}

	if (max_connect > 7)
	{
		return CELL_MOUSE_ERROR_INVALID_PARAMETER;
	}

	Emu.GetMouseManager().Init(max_connect);
	return CELL_OK;
}


s32 cellMouseClearBuf(u32 port_no)
{
	sys_io.trace("cellMouseClearBuf(port_no=%d)", port_no);

	if (!Emu.GetMouseManager().IsInited())
	{
		return CELL_MOUSE_ERROR_UNINITIALIZED;
	}

	if (port_no >= Emu.GetMouseManager().GetMice().size())
	{
		return CELL_MOUSE_ERROR_INVALID_PARAMETER;
	}

	//?

	return CELL_OK;
}

s32 cellMouseEnd()
{
	sys_io.trace("cellMouseEnd()");

	if (!Emu.GetMouseManager().IsInited())
	{
		return CELL_MOUSE_ERROR_UNINITIALIZED;
	}

	Emu.GetMouseManager().Close();
	return CELL_OK;
}

s32 cellMouseGetInfo(vm::ptr<CellMouseInfo> info)
{
	sys_io.trace("cellMouseGetInfo(info=*0x%x)", info);

	if (!Emu.GetMouseManager().IsInited())
	{
		return CELL_MOUSE_ERROR_UNINITIALIZED;
	}

	const MouseInfo& current_info = Emu.GetMouseManager().GetInfo();
	info->max_connect = current_info.max_connect;
	info->now_connect = current_info.now_connect;
	info->info = current_info.info;
	for (u32 i=0; i<CELL_MAX_MICE; i++)	info->vendor_id[i] = current_info.vendor_id[i];
	for (u32 i=0; i<CELL_MAX_MICE; i++)	info->product_id[i] = current_info.product_id[i];
	for (u32 i=0; i<CELL_MAX_MICE; i++)	info->status[i] = current_info.status[i];
	
	return CELL_OK;
}

s32 cellMouseInfoTabletMode(u32 port_no, vm::ptr<CellMouseInfoTablet> info)
{
	sys_io.trace("cellMouseInfoTabletMode(port_no=%d, info=*0x%x)", port_no, info);
	if (!Emu.GetMouseManager().IsInited())
	{
		return CELL_MOUSE_ERROR_UNINITIALIZED;
	}

	if (port_no >= Emu.GetMouseManager().GetMice().size())
	{
		return CELL_MOUSE_ERROR_INVALID_PARAMETER;
	}

	info->is_supported = 0; // Unimplemented: (0=Tablet mode is not supported)
	info->mode = 1; // Unimplemented: (1=Mouse mode)
	
	return CELL_OK;
}

s32 cellMouseGetData(u32 port_no, vm::ptr<CellMouseData> data)
{
	sys_io.trace("cellMouseGetData(port_no=%d, data=*0x%x)", port_no, data);
	if (!Emu.GetMouseManager().IsInited())
	{
		return CELL_MOUSE_ERROR_UNINITIALIZED;
	}
	if (port_no >= Emu.GetMouseManager().GetMice().size())
	{
		return CELL_MOUSE_ERROR_NO_DEVICE;
	}
	
	MouseData& current_data = Emu.GetMouseManager().GetData(port_no);
	data->update = current_data.update;
	data->buttons = current_data.buttons;
	data->x_axis = current_data.x_axis;
	data->y_axis = current_data.y_axis;
	data->wheel = current_data.wheel;
	data->tilt = current_data.tilt;

	current_data.update = CELL_MOUSE_DATA_NON;
	current_data.x_axis = 0;
	current_data.y_axis = 0;
	current_data.wheel = 0;

	return CELL_OK;
}

s32 cellMouseGetDataList(u32 port_no, vm::ptr<CellMouseDataList> data)
{
	UNIMPLEMENTED_FUNC(sys_io);
	
	return CELL_OK;
}

s32 cellMouseSetTabletMode(u32 port_no, u32 mode)
{
	UNIMPLEMENTED_FUNC(sys_io);
	
	return CELL_OK;
}

s32 cellMouseGetTabletDataList(u32 port_no, u32 data_addr)
{
	UNIMPLEMENTED_FUNC(sys_io);
	
	return CELL_OK;
}

s32 cellMouseGetRawData(u32 port_no, vm::ptr<struct CellMouseRawData> data)
{
	sys_io.todo("cellMouseGetRawData(port_no=%d, data=*0x%x)", port_no, data);
	/*if (!Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_UNINITIALIZED;
	if (port_no >= Emu.GetMouseManager().GetMice().size()) return CELL_MOUSE_ERROR_NO_DEVICE;

	CellMouseRawData& current_rawdata = Emu.GetMouseManager().GetRawData(port_no);
	data += current_rawdata.len;
	for(s32 i=0; i<current_rawdata.len; i++)
	{
		data += current_rawdata.data[i];
	}

	current_rawdata.len = 0;*/

	return CELL_OK;
}

void cellMouse_init()
{
	REG_FUNC(sys_io, cellMouseInit);
	REG_FUNC(sys_io, cellMouseClearBuf);
	REG_FUNC(sys_io, cellMouseEnd);
	REG_FUNC(sys_io, cellMouseGetInfo);
	REG_FUNC(sys_io, cellMouseInfoTabletMode);
	REG_FUNC(sys_io, cellMouseGetData);
	REG_FUNC(sys_io, cellMouseGetDataList);
	REG_FUNC(sys_io, cellMouseSetTabletMode);
	REG_FUNC(sys_io, cellMouseGetTabletDataList);
	REG_FUNC(sys_io, cellMouseGetRawData);
}
