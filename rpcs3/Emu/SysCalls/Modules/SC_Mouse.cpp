#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/Io/Mouse.h"
#include "SC_Mouse.h"

extern Module *sys_io;

int cellMouseInit(u32 max_connect)
{
	sys_io->Warning("cellMouseInit(max_connect=%d)", max_connect);
	if(Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_ALREADY_INITIALIZED;
	if(max_connect > 7) return CELL_MOUSE_ERROR_INVALID_PARAMETER;

	Emu.GetMouseManager().Init(max_connect);
	return CELL_OK;
}


int cellMouseClearBuf(u32 port_no)
{
	sys_io->Log("cellMouseClearBuf(port_no=%d)", port_no);
	if(!Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_UNINITIALIZED;
	if(port_no >= Emu.GetMouseManager().GetMice().size()) return CELL_MOUSE_ERROR_INVALID_PARAMETER;

	//?

	return CELL_OK;
}

int cellMouseEnd()
{
	sys_io->Log("cellMouseEnd()");
	if(!Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_UNINITIALIZED;
	Emu.GetMouseManager().Close();
	return CELL_OK;
}

int cellMouseGetInfo(vm::ptr<CellMouseInfo> info)
{
	sys_io->Log("cellMouseGetInfo(info_addr=0x%x)", info.addr());
	if(!Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_UNINITIALIZED;

	const MouseInfo& current_info = Emu.GetMouseManager().GetInfo();
	info->max_connect = current_info.max_connect;
	info->now_connect = current_info.now_connect;
	info->info = current_info.info;
	for(u32 i=0; i<CELL_MAX_MICE; i++)	info->vendor_id[i] = current_info.vendor_id[i];
	for(u32 i=0; i<CELL_MAX_MICE; i++)	info->product_id[i] = current_info.product_id[i];
	for(u32 i=0; i<CELL_MAX_MICE; i++)	info->status[i] = current_info.status[i];
	
	return CELL_OK;
}

int cellMouseInfoTabletMode(u32 port_no, vm::ptr<CellMouseInfoTablet> info)
{
	sys_io->Log("cellMouseInfoTabletMode(port_no=%d,info_addr=0x%x)", port_no, info.addr());
	if(!Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_UNINITIALIZED;
	if(port_no >= Emu.GetMouseManager().GetMice().size()) return CELL_MOUSE_ERROR_INVALID_PARAMETER;

	info->is_supported = 0; // Unimplemented: (0=Tablet mode is not supported)
	info->mode = 1; // Unimplemented: (1=Mouse mode)
	
	return CELL_OK;
}

int cellMouseGetData(u32 port_no, vm::ptr<CellMouseData> data)
{
	sys_io->Log("cellMouseGetData(port_no=%d,data_addr=0x%x)", port_no, data.addr());
	if(!Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_UNINITIALIZED;
	if(port_no >= Emu.GetMouseManager().GetMice().size()) return CELL_MOUSE_ERROR_NO_DEVICE;
	
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

int cellMouseGetDataList(u32 port_no, vm::ptr<CellMouseDataList> data)
{
	UNIMPLEMENTED_FUNC(sys_io);
	
	return CELL_OK;
}

int cellMouseSetTabletMode(u32 port_no, u32 mode)
{
	UNIMPLEMENTED_FUNC(sys_io);
	
	return CELL_OK;
}

int cellMouseGetTabletDataList(u32 port_no, u32 data_addr)
{
	UNIMPLEMENTED_FUNC(sys_io);
	
	return CELL_OK;
}

int cellMouseGetRawData(u32 port_no, u32 data_addr)
{
	UNIMPLEMENTED_FUNC(sys_io);

	/*sys_io->Log("cellMouseGetRawData(port_no=%d,data_addr=0x%x)", port_no, data.addr());
	if(!Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_UNINITIALIZED;
	if(port_no >= Emu.GetMouseManager().GetMice().size()) return CELL_MOUSE_ERROR_NO_DEVICE;

	CellMouseRawData& current_rawdata = Emu.GetMouseManager().GetRawData(port_no);
	data += current_rawdata.len;
	for(s32 i=0; i<current_rawdata.len; i++)
	{
		data += current_rawdata.data[i];
	}

	current_rawdata.len = 0;*/

	return CELL_OK;
}
