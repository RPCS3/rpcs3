#include "stdafx.h"
#include "Emu/Io/Mouse.h"
#include "Emu/SysCalls/SysCalls.h"

extern Module sys_io;

enum CELL_MOUSE_ERROR_CODE
{
	CELL_MOUSE_ERROR_FATAL                      = 0x80121201,
	CELL_MOUSE_ERROR_INVALID_PARAMETER          = 0x80121202,
	CELL_MOUSE_ERROR_ALREADY_INITIALIZED        = 0x80121203,
	CELL_MOUSE_ERROR_UNINITIALIZED              = 0x80121204,
	CELL_MOUSE_ERROR_RESOURCE_ALLOCATION_FAILED = 0x80121205,
	CELL_MOUSE_ERROR_DATA_READ_FAILED           = 0x80121206,
	CELL_MOUSE_ERROR_NO_DEVICE                  = 0x80121207,
	CELL_MOUSE_ERROR_SYS_SETTING_FAILED         = 0x80121208,
};

int cellMouseInit(u32 max_connect)
{
	sys_io.Log("cellMouseInit(max_connect=%d)", max_connect);
	if(Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_ALREADY_INITIALIZED;
	if(max_connect > 7) return CELL_MOUSE_ERROR_INVALID_PARAMETER;

	Emu.GetMouseManager().Init(max_connect);
	return CELL_OK;
}


int cellMouseClearBuf(u32 port_no)
{
	sys_io.Log("cellMouseClearBuf(port_no=%d)", port_no);
	if(!Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_UNINITIALIZED;
	if(port_no >= Emu.GetMouseManager().GetMice().size()) return CELL_MOUSE_ERROR_INVALID_PARAMETER;

	//?

	return CELL_OK;
}

int cellMouseEnd()
{
	sys_io.Log("cellMouseEnd()");
	if(!Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_UNINITIALIZED;
	Emu.GetMouseManager().Close();
	return CELL_OK;
}

int cellMouseGetInfo(mem_class_t info)
{
	sys_io.Log("cellMouseGetInfo(info_addr=0x%x)", info.GetAddr());
	if(!Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_UNINITIALIZED;

	const MouseInfo& current_info = Emu.GetMouseManager().GetInfo();
	info += current_info.max_connect;
	info += current_info.now_connect;
	info += current_info.info;
	for(u32 i=0; i<CELL_MAX_MICE; i++)	info += current_info.vendor_id[i];
	for(u32 i=0; i<CELL_MAX_MICE; i++)	info += current_info.product_id[i];
	for(u32 i=0; i<CELL_MAX_MICE; i++)	info += current_info.status[i];
	
	return CELL_OK;
}

int cellMouseInfoTabletMode(u32 port_no, mem_class_t info)
{
	sys_io.Log("cellMouseInfoTabletMode(port_no=%d,info_addr=0x%x)", port_no, info.GetAddr());
	if(!Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_UNINITIALIZED;
	if(port_no >= Emu.GetMouseManager().GetMice().size()) return CELL_MOUSE_ERROR_INVALID_PARAMETER;

	info += 0;		// Unimplemented: (0=Tablet mode is not supported)
	info += 1;		// Unimplemented: (1=Mouse mode)
	
	return CELL_OK;
}

int cellMouseGetData(u32 port_no, mem_class_t data)
{
	sys_io.Log("cellMouseGetData(port_no=%d,data_addr=0x%x)", port_no, data.GetAddr());
	if(!Emu.GetMouseManager().IsInited()) return CELL_MOUSE_ERROR_UNINITIALIZED;
	if(port_no >= Emu.GetMouseManager().GetMice().size()) return CELL_MOUSE_ERROR_NO_DEVICE;
	
	CellMouseData& current_data = Emu.GetMouseManager().GetData(port_no);
	data += current_data.update;
	data += current_data.buttons;
	data += current_data.x_axis;
	data += current_data.y_axis;
	data += current_data.wheel;
	data += current_data.tilt;

	current_data.update = CELL_MOUSE_DATA_NON;
	current_data.x_axis = 0;
	current_data.y_axis = 0;
	current_data.wheel = 0;

	return CELL_OK;
}

int cellMouseGetDataList(u32 port_no, mem_class_t data)
{
	UNIMPLEMENTED_FUNC(sys_io);
	
	return CELL_OK;
}

int cellMouseSetTabletMode(u32 port_no, u32 mode)
{
	UNIMPLEMENTED_FUNC(sys_io);
	
	return CELL_OK;
}

int cellMouseGetTabletDataList(u32 port_no, mem_class_t data)
{
	UNIMPLEMENTED_FUNC(sys_io);
	
	return CELL_OK;
}

int cellMouseGetRawData(u32 port_no, mem_class_t data)
{
	UNIMPLEMENTED_FUNC(sys_io);

	/*sys_io.Log("cellMouseGetRawData(port_no=%d,data_addr=0x%x)", port_no, data.GetAddr());
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
