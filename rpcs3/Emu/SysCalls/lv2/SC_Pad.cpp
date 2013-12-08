#include "stdafx.h"
#include "Emu/Io/Pad.h"
#include "Emu/SysCalls/SysCalls.h"

extern Module sys_io;

enum CELL_PAD_ERROR_CODE
{
	CELL_PAD_ERROR_FATAL						= 0x80121101,
	CELL_PAD_ERROR_INVALID_PARAMETER			= 0x80121102,
	CELL_PAD_ERROR_ALREADY_INITIALIZED			= 0x80121103,
	CELL_PAD_ERROR_UNINITIALIZED				= 0x80121104,
	CELL_PAD_ERROR_RESOURCE_ALLOCATION_FAILED	= 0x80121105,
	CELL_PAD_ERROR_DATA_READ_FAILED				= 0x80121106,
	CELL_PAD_ERROR_NO_DEVICE					= 0x80121107,
	CELL_PAD_ERROR_UNSUPPORTED_GAMEPAD			= 0x80121108,
	CELL_PAD_ERROR_TOO_MANY_DEVICES				= 0x80121109,
	CELL_PAD_ERROR_EBUSY						= 0x8012110a,
};

struct CellPadData
{
	s32 len;
	u16 button[CELL_PAD_MAX_CODES];
};

struct CellPadInfo
{
	u32 max_connect;
	u32 now_connect;
	u32 system_info;
	u16 vendor_id[CELL_MAX_PADS];
	u16 product_id[CELL_MAX_PADS];
	u8  status[CELL_MAX_PADS];
};

struct CellPadInfo2
{
	u32 max_connect;
	u32 now_connect;
	u32 system_info;
	u32 port_status[CELL_PAD_MAX_PORT_NUM];
	u32 port_setting[CELL_PAD_MAX_PORT_NUM];
	u32 device_capability[CELL_PAD_MAX_PORT_NUM];
	u32 device_type[CELL_PAD_MAX_PORT_NUM];
};

int cellPadInit(u32 max_connect)
{
	sys_io.Log("cellPadInit(max_connect=%d)", max_connect);
	if(Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_ALREADY_INITIALIZED;
	Emu.GetPadManager().Init(max_connect);
	return CELL_OK;
}

int cellPadEnd()
{
	sys_io.Log("cellPadEnd()");
	if(!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	Emu.GetPadManager().Close();
	return CELL_OK;
}

int cellPadClearBuf(u32 port_no)
{
	sys_io.Log("cellPadClearBuf(port_no=%d)", port_no);
	if(!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	if(port_no >= Emu.GetPadManager().GetPads().GetCount()) return CELL_PAD_ERROR_INVALID_PARAMETER;

	//?

	return CELL_OK;
}

int cellPadGetData(u32 port_no, u32 data_addr)
{
	//ConLog.Warning("cellPadGetData[port_no: %d, data_addr: 0x%x]", port_no, data_addr);
	const Array<Pad>& pads = Emu.GetPadManager().GetPads();
	if(!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	if(port_no >= pads.GetCount()) return CELL_PAD_ERROR_INVALID_PARAMETER;

	const Pad& pad = pads[port_no];
	CellPadData data;
	memset(&data, 0, sizeof(CellPadData));

	u16 d1 = 0;
	u16 d2 = 0;

	const Array<Button>& buttons = pads[port_no].m_buttons;
	pads[port_no].m_port_status &= ~CELL_PAD_STATUS_ASSIGN_CHANGES;

	s32 len = 0;
	for(uint i=0; i<buttons.GetCount(); ++i)
	{
		if(!buttons[i].m_pressed) continue;

		switch(buttons[i].m_offset)
		{
		case CELL_PAD_BTN_OFFSET_DIGITAL1: if(!(d1 & buttons[i].m_outKeyCode)){d1 |= buttons[i].m_outKeyCode; len++;} break;
		case CELL_PAD_BTN_OFFSET_DIGITAL2: if(!(d2 & buttons[i].m_outKeyCode)){d2 |= buttons[i].m_outKeyCode; len++;} break;
		}

		if(buttons[i].m_flush)
		{
			buttons[i].m_pressed = false;
			buttons[i].m_flush = false;
		}
	}

	data.len = re(len);
	data.button[CELL_PAD_BTN_OFFSET_DIGITAL1]		= re(d1);
	data.button[CELL_PAD_BTN_OFFSET_DIGITAL2]		= re(d2);
	data.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X] = re(pad.m_analog_right_x);
	data.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y] = re(pad.m_analog_right_y);
	data.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X]	= re(pad.m_analog_left_x);
	data.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y]	= re(pad.m_analog_left_y);
	data.button[CELL_PAD_BTN_OFFSET_PRESS_RIGHT]	= re(pad.m_press_right);
	data.button[CELL_PAD_BTN_OFFSET_PRESS_LEFT]		= re(pad.m_press_left);
	data.button[CELL_PAD_BTN_OFFSET_PRESS_UP]		= re(pad.m_press_up);
	data.button[CELL_PAD_BTN_OFFSET_PRESS_DOWN]		= re(pad.m_press_down);
	data.button[CELL_PAD_BTN_OFFSET_PRESS_TRIANGLE] = re(pad.m_press_triangle);
	data.button[CELL_PAD_BTN_OFFSET_PRESS_CIRCLE]	= re(pad.m_press_circle);
	data.button[CELL_PAD_BTN_OFFSET_PRESS_CROSS]	= re(pad.m_press_cross);
	data.button[CELL_PAD_BTN_OFFSET_PRESS_SQUARE]	= re(pad.m_press_square);
	data.button[CELL_PAD_BTN_OFFSET_PRESS_L1]		= re(pad.m_press_L1);
	data.button[CELL_PAD_BTN_OFFSET_PRESS_L2]		= re(pad.m_press_L2);
	data.button[CELL_PAD_BTN_OFFSET_PRESS_R1]		= re(pad.m_press_R1);
	data.button[CELL_PAD_BTN_OFFSET_PRESS_R2]		= re(pad.m_press_R2);
	data.button[CELL_PAD_BTN_OFFSET_SENSOR_X]		= re(pad.m_sensor_x);
	data.button[CELL_PAD_BTN_OFFSET_SENSOR_Y]		= re(pad.m_sensor_y);
	data.button[CELL_PAD_BTN_OFFSET_SENSOR_Z]		= re(pad.m_sensor_z);
	data.button[CELL_PAD_BTN_OFFSET_SENSOR_G]		= re(pad.m_sensor_g);

	Memory.WriteData(data_addr, data);

	return CELL_OK;
}

int cellPadGetDataExtra(u32 port_no, u32 device_type_addr, u32 data_addr)
{
	sys_io.Log("cellPadGetDataExtra(port_no=%d, device_type_addr=0x%x, device_type_addr=0x%x)", port_no, device_type_addr, data_addr);
	if(!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	if(port_no >= Emu.GetPadManager().GetPads().GetCount()) return CELL_PAD_ERROR_INVALID_PARAMETER;
	return CELL_OK;
}

int cellPadSetActDirect(u32 port_no, u32 param_addr)
{
	sys_io.Log("cellPadSetActDirect(port_no=%d, param_addr=0x%x)", port_no, param_addr);
	if(!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	if(port_no >= Emu.GetPadManager().GetPads().GetCount()) return CELL_PAD_ERROR_INVALID_PARAMETER;
	return CELL_OK;
}

int cellPadGetInfo(u32 info_addr)
{
	sys_io.Log("cellPadGetInfo(info_addr=0x%x)", info_addr);
	if(!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;

	CellPadInfo info;
	memset(&info, 0, sizeof(CellPadInfo));

	const PadInfo& rinfo = Emu.GetPadManager().GetInfo();
	info.max_connect = re(rinfo.max_connect);
	info.now_connect = re(rinfo.now_connect);
	info.system_info = re(rinfo.system_info);

	const Array<Pad>& pads = Emu.GetPadManager().GetPads();

	for(u32 i=0; i<CELL_MAX_PADS; ++i)
	{
		if(i >= pads.GetCount()) break;

		info.status[i] = re(pads[i].m_port_status);
		info.product_id[i] = 0xdead; //TODO
		info.vendor_id[i] = 0xbeaf; //TODO
	}

	Memory.WriteData(info_addr, info);

	return CELL_OK;
}

int cellPadGetInfo2(u32 info_addr)
{
	sys_io.Log("cellPadGetInfo2(info_addr=0x%x)", info_addr);
	if(!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;

	CellPadInfo2 info;
	memset(&info, 0, sizeof(CellPadInfo2));

	const PadInfo& rinfo = Emu.GetPadManager().GetInfo();
	info.max_connect = re(rinfo.max_connect);
	info.now_connect = re(rinfo.now_connect);
	info.system_info = re(rinfo.system_info);

	const Array<Pad>& pads = Emu.GetPadManager().GetPads();

	for(u32 i=0; i<CELL_PAD_MAX_PORT_NUM; ++i)
	{
		if(i >= pads.GetCount()) break;
		info.port_status[i] = re(pads[i].m_port_status);
		info.port_setting[i] = re(pads[i].m_port_setting);
		info.device_capability[i] = re(pads[i].m_device_capability);
		info.device_type[i] = re(pads[i].m_device_type);
	}

	Memory.WriteData(info_addr, info);

	return CELL_OK;
}

int cellPadSetPortSetting(u32 port_no, u32 port_setting)
{
	sys_io.Log("cellPadSetPortSetting(port_no=%d, port_setting=0x%x)", port_no, port_setting);
	if(!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	Array<Pad>& pads = Emu.GetPadManager().GetPads();
	if(port_no >= pads.GetCount()) return CELL_PAD_ERROR_INVALID_PARAMETER;

	pads[port_no].m_port_setting = port_setting;

	return CELL_OK;
}
