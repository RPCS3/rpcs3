#include "stdafx.h"
#include "Emu/Io/Pad.h"
#include "Emu/SysCalls/SysCalls.h"

extern Module sys_io;

enum CELL_PAD_ERROR_CODE
{
	CELL_PAD_ERROR_FATAL                      = 0x80121101,
	CELL_PAD_ERROR_INVALID_PARAMETER          = 0x80121102,
	CELL_PAD_ERROR_ALREADY_INITIALIZED        = 0x80121103,
	CELL_PAD_ERROR_UNINITIALIZED              = 0x80121104,
	CELL_PAD_ERROR_RESOURCE_ALLOCATION_FAILED = 0x80121105,
	CELL_PAD_ERROR_DATA_READ_FAILED           = 0x80121106,
	CELL_PAD_ERROR_NO_DEVICE                  = 0x80121107,
	CELL_PAD_ERROR_UNSUPPORTED_GAMEPAD        = 0x80121108,
	CELL_PAD_ERROR_TOO_MANY_DEVICES           = 0x80121109,
	CELL_PAD_ERROR_EBUSY                      = 0x8012110a,
};

struct CellPadData
{
	be_t<s32> len;
	be_t<u16> button[CELL_PAD_MAX_CODES];
};

struct CellPadInfo
{
	be_t<u32> max_connect;
	be_t<u32> now_connect;
	be_t<u32> system_info;
	be_t<u16> vendor_id[CELL_MAX_PADS];
	be_t<u16> product_id[CELL_MAX_PADS];
	u8 status[CELL_MAX_PADS];
};

struct CellPadInfo2
{
	be_t<u32> max_connect;
	be_t<u32> now_connect;
	be_t<u32> system_info;
	be_t<u32> port_status[CELL_PAD_MAX_PORT_NUM];
	be_t<u32> port_setting[CELL_PAD_MAX_PORT_NUM];
	be_t<u32> device_capability[CELL_PAD_MAX_PORT_NUM];
	be_t<u32> device_type[CELL_PAD_MAX_PORT_NUM];
};

struct CellCapabilityInfo
{
	be_t<u32> info[CELL_PAD_MAX_CAPABILITY_INFO];
};

int cellPadInit(u32 max_connect)
{
	sys_io.Log("cellPadInit(max_connect=%d)", max_connect);
	if(Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_ALREADY_INITIALIZED;
	if (max_connect > CELL_PAD_MAX_PORT_NUM) return CELL_PAD_ERROR_INVALID_PARAMETER;
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
	const PadInfo& rinfo = Emu.GetPadManager().GetInfo();
	if (port_no >= rinfo.max_connect) return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect) return CELL_PAD_ERROR_NO_DEVICE;

	//It seems the system is supposed keeps track of previous values, and reports paddata with len 0 if 
	//nothing has changed.

	//We can at least reset the pad back to its default values for now
	std::vector<Pad>& pads = Emu.GetPadManager().GetPads();
	Pad& pad = pads[port_no];

	pad.m_analog_left_x = pad.m_analog_left_y = pad.m_analog_right_x = pad.m_analog_right_y = 128;

	pad.m_digital_1 = pad.m_digital_2 = 0;
	pad.m_press_right = pad.m_press_left = pad.m_press_up = pad.m_press_down = 0;
	pad.m_press_triangle = pad.m_press_circle = pad.m_press_cross = pad.m_press_square = 0;
	pad.m_press_L1 = pad.m_press_L2 = pad.m_press_R1 = pad.m_press_R2 = 0;

	//~399 on sensor y is a level non moving controller
	pad.m_sensor_y = 399;
	pad.m_sensor_x = pad.m_sensor_z = pad.m_sensor_g = 0;
	
	return CELL_OK;
}

int cellPadGetData(u32 port_no, u32 data_addr)
{
	sys_io.Log("cellPadGetData[port_no: %d, data_addr: 0x%x]", port_no, data_addr);
	std::vector<Pad>& pads = Emu.GetPadManager().GetPads();
	if(!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	const PadInfo& rinfo = Emu.GetPadManager().GetInfo();
	if(port_no >= rinfo.max_connect) return CELL_PAD_ERROR_INVALID_PARAMETER;
	//We have a choice here of NO_DEVICE or READ_FAILED...lets try no device for now
	if(port_no >= rinfo.now_connect) return CELL_PAD_ERROR_NO_DEVICE;

	Pad& pad = pads[port_no];
	CellPadData data;
	memset(&data, 0, sizeof(CellPadData));

	for(Button& button : pad.m_buttons)
	{
		//using an if/else here, not doing switch in switch
		//plus side is this gives us the ability to check if anything changed eventually

		if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL1)
		{
			if (button.m_pressed) pad.m_digital_1 |= button.m_outKeyCode;
			else pad.m_digital_1 &= ~button.m_outKeyCode;

			switch (button.m_outKeyCode)
			{
			case CELL_PAD_CTRL_LEFT: pad.m_press_left = button.m_value; break;
			case CELL_PAD_CTRL_DOWN: pad.m_press_down = button.m_value; break;
			case CELL_PAD_CTRL_RIGHT: pad.m_press_right = button.m_value; break;
			case CELL_PAD_CTRL_UP: pad.m_press_up = button.m_value; break;
			//These arent pressure btns
			case CELL_PAD_CTRL_R3:
			case CELL_PAD_CTRL_L3:
			case CELL_PAD_CTRL_START:
			case CELL_PAD_CTRL_SELECT:
			default: break;
			}
		}
		else if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL2)
		{
			if (button.m_pressed) pad.m_digital_2 |= button.m_outKeyCode;
			else pad.m_digital_2 &= ~button.m_outKeyCode;

			switch (button.m_outKeyCode)
			{
			case CELL_PAD_CTRL_SQUARE: pad.m_press_square = button.m_value; break;
			case CELL_PAD_CTRL_CROSS: pad.m_press_cross = button.m_value; break;
			case CELL_PAD_CTRL_CIRCLE: pad.m_press_circle = button.m_value; break;
			case CELL_PAD_CTRL_TRIANGLE: pad.m_press_triangle = button.m_value; break;
			case CELL_PAD_CTRL_R1: pad.m_press_R1 = button.m_value; break;
			case CELL_PAD_CTRL_L1: pad.m_press_L1 = button.m_value; break;
			case CELL_PAD_CTRL_R2: pad.m_press_R2 = button.m_value; break;
			case CELL_PAD_CTRL_L2: pad.m_press_L2 = button.m_value; break;
			default: break;
			}
		}

		if(button.m_flush)
		{
			button.m_pressed = false;
			button.m_flush = false;
			button.m_value = 0;
		}
	}

	for (const AnalogStick& stick : pads[port_no].m_sticks)
	{
		switch (stick.m_offset)
		{
		case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X: pad.m_analog_left_x = stick.m_value; break;
		case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y: pad.m_analog_left_y = stick.m_value; break;
		case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X: pad.m_analog_right_x = stick.m_value; break;
		case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y: pad.m_analog_right_y = stick.m_value; break;
		default: break;
		}

		/*if (stick.m_max_pressed && !stick.m_min_pressed)
			*res = 255;
		if (stick.m_min_pressed && !stick.m_max_pressed)
			*res = 0;
		*/
	}
	
	data.len = pad.m_buttons.size();
	data.button[CELL_PAD_BTN_OFFSET_DIGITAL1]       = pad.m_digital_1;
	data.button[CELL_PAD_BTN_OFFSET_DIGITAL2]       = pad.m_digital_2;
	data.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X] = pad.m_analog_right_x;
	data.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y] = pad.m_analog_right_y;
	data.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X]  = pad.m_analog_left_x;
	data.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y]  = pad.m_analog_left_y;
	data.button[CELL_PAD_BTN_OFFSET_PRESS_RIGHT]    = pad.m_press_right;
	data.button[CELL_PAD_BTN_OFFSET_PRESS_LEFT]     = pad.m_press_left;
	data.button[CELL_PAD_BTN_OFFSET_PRESS_UP]       = pad.m_press_up;
	data.button[CELL_PAD_BTN_OFFSET_PRESS_DOWN]     = pad.m_press_down;
	data.button[CELL_PAD_BTN_OFFSET_PRESS_TRIANGLE] = pad.m_press_triangle;
	data.button[CELL_PAD_BTN_OFFSET_PRESS_CIRCLE]   = pad.m_press_circle;
	data.button[CELL_PAD_BTN_OFFSET_PRESS_CROSS]    = pad.m_press_cross;
	data.button[CELL_PAD_BTN_OFFSET_PRESS_SQUARE]   = pad.m_press_square;
	data.button[CELL_PAD_BTN_OFFSET_PRESS_L1]       = pad.m_press_L1;
	data.button[CELL_PAD_BTN_OFFSET_PRESS_L2]       = pad.m_press_L2;
	data.button[CELL_PAD_BTN_OFFSET_PRESS_R1]       = pad.m_press_R1;
	data.button[CELL_PAD_BTN_OFFSET_PRESS_R2]       = pad.m_press_R2;
	data.button[CELL_PAD_BTN_OFFSET_SENSOR_X]       = pad.m_sensor_x;
	data.button[CELL_PAD_BTN_OFFSET_SENSOR_Y]       = pad.m_sensor_y;
	data.button[CELL_PAD_BTN_OFFSET_SENSOR_Z]       = pad.m_sensor_z;
	data.button[CELL_PAD_BTN_OFFSET_SENSOR_G]       = pad.m_sensor_g;

	Memory.WriteData(data_addr, data);

	return CELL_OK;
}

int cellPadGetDataExtra(u32 port_no, u32 device_type_addr, u32 data_addr)
{
	sys_io.Log("cellPadGetDataExtra(port_no=%d, device_type_addr=0x%x, device_type_addr=0x%x)", port_no, device_type_addr, data_addr);
	if(!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	const PadInfo& rinfo = Emu.GetPadManager().GetInfo();
	if (port_no >= rinfo.max_connect) return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect) return CELL_PAD_ERROR_NO_DEVICE;

	return CELL_OK;
}

int cellPadSetActDirect(u32 port_no, u32 param_addr)
{
	sys_io.Log("cellPadSetActDirect(port_no=%d, param_addr=0x%x)", port_no, param_addr);
	if(!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	const PadInfo& rinfo = Emu.GetPadManager().GetInfo();
	if (port_no >= rinfo.max_connect) return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect) return CELL_PAD_ERROR_NO_DEVICE;

	return CELL_OK;
}

int cellPadGetInfo(u32 info_addr)
{
	sys_io.Log("cellPadGetInfo(info_addr=0x%x)", info_addr);
	if(!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;

	CellPadInfo info;
	memset(&info, 0, sizeof(CellPadInfo));

	const PadInfo& rinfo = Emu.GetPadManager().GetInfo();
	info.max_connect = rinfo.max_connect;
	info.now_connect = rinfo.now_connect;
	info.system_info = rinfo.system_info;

	//Can't have this as const, we need to reset Assign Changes Flag here
	std::vector<Pad>& pads = Emu.GetPadManager().GetPads();

	for(u32 i=0; i<CELL_MAX_PADS; ++i)
	{
		if(i >= pads.size())
			break;

		info.status[i] = pads[i].m_port_status;
		pads[i].m_port_status &= ~CELL_PAD_STATUS_ASSIGN_CHANGES;
		info.product_id[i] = 0x0268;
		info.vendor_id[i] = 0x054C;
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
	info.max_connect = rinfo.max_connect;
	info.now_connect = rinfo.now_connect;
	info.system_info = rinfo.system_info;

	std::vector<Pad>& pads = Emu.GetPadManager().GetPads();

	for(u32 i=0; i<CELL_PAD_MAX_PORT_NUM; ++i)
	{
		if(i >= pads.size())
			break;

		info.port_status[i] = pads[i].m_port_status;
		pads[i].m_port_status &= ~CELL_PAD_STATUS_ASSIGN_CHANGES;
		info.port_setting[i] = pads[i].m_port_setting;
		info.device_capability[i] = pads[i].m_device_capability;
		info.device_type[i] = pads[i].m_device_type;
	}

	Memory.WriteData(info_addr, info);

	return CELL_OK;
}

int cellPadGetCapabilityInfo(u32 port_no, mem32_t info_addr)
{
	sys_io.Log("cellPadGetCapabilityInfo[port_no: %d, data_addr: 0x%x]", port_no, info_addr.GetAddr());
	if (!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	const PadInfo& rinfo = Emu.GetPadManager().GetInfo();
	if (port_no >= rinfo.max_connect) return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect) return CELL_PAD_ERROR_NO_DEVICE;

	const std::vector<Pad>& pads = Emu.GetPadManager().GetPads();

	CellCapabilityInfo data;
	memset(&data, 0, sizeof(CellCapabilityInfo));

	//Should return the same as device capability mask, psl1ght has it backwards in pad.h
	data.info[0] = pads[port_no].m_device_capability;

	Memory.WriteData(info_addr.GetAddr(), data);

	return CELL_OK;
}

int cellPadSetPortSetting(u32 port_no, u32 port_setting)
{
	sys_io.Log("cellPadSetPortSetting(port_no=%d, port_setting=0x%x)", port_no, port_setting);
	if(!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	if ((port_setting < CELL_PAD_SETTING_PRESS_ON) || port_setting >(CELL_PAD_SETTING_PRESS_ON | CELL_PAD_SETTING_SENSOR_ON) && port_setting != 0)
		return CELL_PAD_ERROR_INVALID_PARAMETER;
	const PadInfo& rinfo = Emu.GetPadManager().GetInfo();
	if (port_no >= rinfo.max_connect) return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect) return CELL_PAD_ERROR_NO_DEVICE;

	std::vector<Pad>& pads = Emu.GetPadManager().GetPads();
	pads[port_no].m_port_setting = port_setting;

	return CELL_OK;
}

int cellPadInfoPressMode(u32 port_no)
{
	sys_io.Log("cellPadInfoPressMode(port_no=%d)", port_no);
	if (!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	const PadInfo& rinfo = Emu.GetPadManager().GetInfo();
	if (port_no >= rinfo.max_connect) return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect) return CELL_PAD_ERROR_NO_DEVICE;

	const std::vector<Pad>& pads = Emu.GetPadManager().GetPads();

	return (pads[port_no].m_device_capability & CELL_PAD_CAPABILITY_PRESS_MODE) > 0;
}

int cellPadInfoSensorMode(u32 port_no)
{
	sys_io.Log("cellPadInfoSensorMode(port_no=%d)", port_no);
	if (!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	const PadInfo& rinfo = Emu.GetPadManager().GetInfo();
	if (port_no >= rinfo.max_connect) return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect) return CELL_PAD_ERROR_NO_DEVICE;

	const std::vector<Pad>& pads = Emu.GetPadManager().GetPads();

	return (pads[port_no].m_device_capability & CELL_PAD_CAPABILITY_SENSOR_MODE) > 0;
}

int cellPadSetPressMode(u32 port_no, u32 mode)
{
	sys_io.Log("cellPadSetPressMode(port_no=%d)", port_no);
	if (!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	if (mode != 0 || mode != 1) return CELL_PAD_ERROR_INVALID_PARAMETER;
	const PadInfo& rinfo = Emu.GetPadManager().GetInfo();
	if (port_no >= rinfo.max_connect) return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect) return CELL_PAD_ERROR_NO_DEVICE;

	std::vector<Pad>& pads = Emu.GetPadManager().GetPads();

	if (mode)
		pads[port_no].m_port_setting |= CELL_PAD_SETTING_PRESS_ON;
	else
		pads[port_no].m_port_setting &= ~CELL_PAD_SETTING_PRESS_ON;

	return CELL_OK;
}

int cellPadSetSensorMode(u32 port_no, u32 mode)
{
	sys_io.Log("cellPadSetPressMode(port_no=%d)", port_no);
	if (!Emu.GetPadManager().IsInited()) return CELL_PAD_ERROR_UNINITIALIZED;
	if (mode != 0 || mode != 1) return CELL_PAD_ERROR_INVALID_PARAMETER;
	const PadInfo& rinfo = Emu.GetPadManager().GetInfo();
	if (port_no >= rinfo.max_connect) return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect) return CELL_PAD_ERROR_NO_DEVICE;

	std::vector<Pad>& pads = Emu.GetPadManager().GetPads();

	if (mode)
		pads[port_no].m_port_setting |= CELL_PAD_SETTING_SENSOR_ON;
	else
		pads[port_no].m_port_setting &= ~CELL_PAD_SETTING_SENSOR_ON;

	return CELL_OK;
}