#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Io/PadHandler.h"
#include "cellPad.h"

extern logs::channel sys_io;

s32 cellPadInit(u32 max_connect)
{
	sys_io.warning("cellPadInit(max_connect=%d)", max_connect);

	const auto handler = fxm::import<PadHandlerBase>(Emu.GetCallbacks().get_pad_handler);

	if (!handler)
		return CELL_PAD_ERROR_ALREADY_INITIALIZED;

	handler->Init(std::min(max_connect, CELL_PAD_MAX_PORT_NUM));

	return CELL_OK;
}

s32 cellPadEnd()
{
	sys_io.notice("cellPadEnd()");

	if (!fxm::remove<PadHandlerBase>())
		return CELL_PAD_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellPadClearBuf(u32 port_no)
{
	sys_io.trace("cellPadClearBuf(port_no=%d)", port_no);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const PadInfo& rinfo = handler->GetInfo();

	if (port_no >= rinfo.max_connect)
		return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	//Set 'm_buffer_cleared' to force a resend of everything
	//might as well also reset everything in our pad 'buffer' to nothing as well

	std::vector<Pad>& pads = handler->GetPads();
	Pad& pad = pads[port_no];

	pad.m_buffer_cleared = true;
	pad.m_analog_left_x = pad.m_analog_left_y = pad.m_analog_right_x = pad.m_analog_right_y = 128;

	pad.m_digital_1 = pad.m_digital_2 = 0;
	pad.m_press_right = pad.m_press_left = pad.m_press_up = pad.m_press_down = 0;
	pad.m_press_triangle = pad.m_press_circle = pad.m_press_cross = pad.m_press_square = 0;
	pad.m_press_L1 = pad.m_press_L2 = pad.m_press_R1 = pad.m_press_R2 = 0;

	//~399 on sensor y is a level non moving controller
	pad.m_sensor_y = 399;
	pad.m_sensor_x = pad.m_sensor_z = pad.m_sensor_g = 512;
	
	return CELL_OK;
}

s32 cellPadGetData(u32 port_no, vm::ptr<CellPadData> data)
{
	sys_io.trace("cellPadGetData(port_no=%d, data=*0x%x)", port_no, data);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	std::vector<Pad>& pads = handler->GetPads();

	const PadInfo& rinfo = handler->GetInfo();

	if (port_no >= rinfo.max_connect)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	//We have a choice here of NO_DEVICE or READ_FAILED...lets try no device for now
	if (port_no >= rinfo.now_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	Pad& pad = pads[port_no];

	u16 d1Initial, d2Initial; 
	d1Initial = pad.m_digital_1;
	d2Initial = pad.m_digital_2;
	bool btnChanged = false;
	for(Button& button : pad.m_buttons)
	{
		//here we check btns, and set pad accordingly, 
		//if something changed, set btnChanged

		if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL1)
		{
			if (button.m_pressed) pad.m_digital_1 |= button.m_outKeyCode;
			else pad.m_digital_1 &= ~button.m_outKeyCode;

			switch (button.m_outKeyCode)
			{
			case CELL_PAD_CTRL_LEFT: 
				if (pad.m_press_left != button.m_value) btnChanged = true;
				pad.m_press_left = button.m_value;
				break;
			case CELL_PAD_CTRL_DOWN: 
				if (pad.m_press_down != button.m_value) btnChanged = true;
				pad.m_press_down = button.m_value; 
				break;
			case CELL_PAD_CTRL_RIGHT: 
				if (pad.m_press_right != button.m_value) btnChanged = true;
				pad.m_press_right = button.m_value; 
				break;
			case CELL_PAD_CTRL_UP: 
				if (pad.m_press_up != button.m_value) btnChanged = true;
				pad.m_press_up = button.m_value; 
				break;
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
			case CELL_PAD_CTRL_SQUARE:
				if (pad.m_press_square != button.m_value) btnChanged = true;
				pad.m_press_square = button.m_value;
				break;
			case CELL_PAD_CTRL_CROSS:
				if (pad.m_press_cross != button.m_value) btnChanged = true;
				pad.m_press_cross = button.m_value; 
				break;
			case CELL_PAD_CTRL_CIRCLE: 
				if (pad.m_press_circle != button.m_value) btnChanged = true;
				pad.m_press_circle = button.m_value;
				break;
			case CELL_PAD_CTRL_TRIANGLE:
				if (pad.m_press_triangle != button.m_value) btnChanged = true;
				pad.m_press_triangle = button.m_value;
				break;
			case CELL_PAD_CTRL_R1: 
				if (pad.m_press_R1 != button.m_value) btnChanged = true;
				pad.m_press_R1 = button.m_value; 
				break;
			case CELL_PAD_CTRL_L1: 
				if (pad.m_press_L1 != button.m_value) btnChanged = true;
				pad.m_press_L1 = button.m_value; 
				break;
			case CELL_PAD_CTRL_R2: 
				if (pad.m_press_R2 != button.m_value) btnChanged = true;
				pad.m_press_R2 = button.m_value; 
				break;
			case CELL_PAD_CTRL_L2: 
				if (pad.m_press_L2 != button.m_value) btnChanged = true;
				pad.m_press_L2 = button.m_value; 
				break;
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

	for (const AnalogStick& stick : pad.m_sticks)
	{
		switch (stick.m_offset)
		{
		case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X: 
			if (pad.m_analog_left_x != stick.m_value) btnChanged = true;
			pad.m_analog_left_x = stick.m_value; 
			break;
		case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y: 
			if (pad.m_analog_left_y != stick.m_value) btnChanged = true;
			pad.m_analog_left_y = stick.m_value; 
			break;
		case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X: 
			if (pad.m_analog_right_x != stick.m_value) btnChanged = true;
			pad.m_analog_right_x = stick.m_value; 
			break;
		case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y: 
			if (pad.m_analog_right_y != stick.m_value) btnChanged = true;
			pad.m_analog_right_y = stick.m_value; 
			break;
		default: break;
		}
	}

	for (const AnalogSensor& sensor : pad.m_sensors)
	{
		switch (sensor.m_offset)
		{
		case CELL_PAD_BTN_OFFSET_SENSOR_X:
			if (pad.m_sensor_x != sensor.m_value) btnChanged = true;
			pad.m_sensor_x = sensor.m_value;
			break;
		case CELL_PAD_BTN_OFFSET_SENSOR_Y:
			if (pad.m_sensor_y != sensor.m_value) btnChanged = true;
			pad.m_sensor_y = sensor.m_value;
			break;
		case CELL_PAD_BTN_OFFSET_SENSOR_Z:
			if (pad.m_sensor_z != sensor.m_value) btnChanged = true;
			pad.m_sensor_z = sensor.m_value;
			break;
		case CELL_PAD_BTN_OFFSET_SENSOR_G:
			if (pad.m_sensor_g != sensor.m_value) btnChanged = true;
			pad.m_sensor_g = sensor.m_value;
			break;
		default: break;
		}
	}

	if (d1Initial != pad.m_digital_1 || d2Initial != pad.m_digital_2)
	{
		btnChanged = true;
	}

	//not sure if this should officially change with capabilities/portsettings :(
	data->len = 24;

	if (pad.m_buffer_cleared)
	{
		pad.m_buffer_cleared = false;
	}
	else if (!btnChanged)
	{
		data->len = 0;
	}
	data->button[0] = 0x0; // always 0
	// bits 15-8 reserved, 7-4 = 0x7, 3-0: data->len/2;
	data->button[1] = (0x7 << 4) | std::min(data->len / 2, 15);
	//lets still send new data anyway, not sure whats expected still
	data->button[CELL_PAD_BTN_OFFSET_DIGITAL1]       = pad.m_digital_1;
	data->button[CELL_PAD_BTN_OFFSET_DIGITAL2]       = pad.m_digital_2;
	data->button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X] = pad.m_analog_right_x;
	data->button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y] = pad.m_analog_right_y;
	data->button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X]  = pad.m_analog_left_x;
	data->button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y]  = pad.m_analog_left_y;
	data->button[CELL_PAD_BTN_OFFSET_PRESS_RIGHT]    = pad.m_press_right;
	data->button[CELL_PAD_BTN_OFFSET_PRESS_LEFT]     = pad.m_press_left;
	data->button[CELL_PAD_BTN_OFFSET_PRESS_UP]       = pad.m_press_up;
	data->button[CELL_PAD_BTN_OFFSET_PRESS_DOWN]     = pad.m_press_down;
	data->button[CELL_PAD_BTN_OFFSET_PRESS_TRIANGLE] = pad.m_press_triangle;
	data->button[CELL_PAD_BTN_OFFSET_PRESS_CIRCLE]   = pad.m_press_circle;
	data->button[CELL_PAD_BTN_OFFSET_PRESS_CROSS]    = pad.m_press_cross;
	data->button[CELL_PAD_BTN_OFFSET_PRESS_SQUARE]   = pad.m_press_square;
	data->button[CELL_PAD_BTN_OFFSET_PRESS_L1]       = pad.m_press_L1;
	data->button[CELL_PAD_BTN_OFFSET_PRESS_L2]       = pad.m_press_L2;
	data->button[CELL_PAD_BTN_OFFSET_PRESS_R1]       = pad.m_press_R1;
	data->button[CELL_PAD_BTN_OFFSET_PRESS_R2]       = pad.m_press_R2;
	data->button[CELL_PAD_BTN_OFFSET_SENSOR_X]       = pad.m_sensor_x;
	data->button[CELL_PAD_BTN_OFFSET_SENSOR_Y]       = pad.m_sensor_y;
	data->button[CELL_PAD_BTN_OFFSET_SENSOR_Z]       = pad.m_sensor_z;
	data->button[CELL_PAD_BTN_OFFSET_SENSOR_G]       = pad.m_sensor_g;

	return CELL_OK;
}

s32 cellPadPeriphGetInfo(vm::ptr<CellPadPeriphInfo> info)
{
	sys_io.trace("cellPadPeriphGetInfo(info=*0x%x)", info);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const PadInfo& rinfo = handler->GetInfo();

	std::memset(info.get_ptr(), 0, sizeof(CellPadPeriphInfo));

	info->max_connect = rinfo.max_connect;
	info->now_connect = rinfo.now_connect;
	info->system_info = rinfo.system_info;

	std::vector<Pad>& pads = handler->GetPads();

	// TODO: Support other types of controllers
	for (u32 i = 0; i < CELL_PAD_MAX_PORT_NUM; ++i)
	{
		if (i >= pads.size())
			break;

		info->port_status[i] = pads[i].m_port_status;
		info->port_setting[i] = pads[i].m_port_setting;
		info->device_capability[i] = pads[i].m_device_capability;
		info->device_type[i] = pads[i].m_device_type;
		info->pclass_type[i] = CELL_PAD_PCLASS_TYPE_STANDARD;
		info->pclass_profile[i] = 0x0;
	}

	return CELL_OK;
}

s32 cellPadPeriphGetData(u32 port_no, vm::ptr<CellPadPeriphData> data)
{
	sys_io.trace("cellPadPeriphGetData(port_no=%d, data=*0x%x)", port_no, data);
	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const PadInfo& rinfo = handler->GetInfo();

	if (port_no >= rinfo.max_connect)
		return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	// todo: support for 'unique' controllers, which goes in offsets 24+ in padData
	data->pclass_type = CELL_PAD_PCLASS_TYPE_STANDARD;
	data->pclass_profile = 0x0;
	return cellPadGetData(port_no, vm::get_addr(&data->cellpad_data));
}

s32 cellPadGetRawData(u32 port_no, vm::ptr<CellPadData> data)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellPadGetDataExtra(u32 port_no, vm::ptr<u32> device_type, vm::ptr<CellPadData> data)
{
	sys_io.trace("cellPadGetDataExtra(port_no=%d, device_type=*0x%x, device_type=*0x%x)", port_no, device_type, data);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const PadInfo& rinfo = handler->GetInfo();

	if (port_no >= rinfo.max_connect)
		return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	// TODO: This is used just to get data from a BD/CEC remote,
	// but if the port isnt a remote, device type is set to 0 and just regular cellPadGetData is returned

	*device_type = 0;

	// set BD data before just incase
	data->button[24] = 0x0;
	data->button[25] = 0x0;

	return cellPadGetData(port_no, data);
}

s32 cellPadSetActDirect(u32 port_no, vm::ptr<CellPadActParam> param)
{
	sys_io.trace("cellPadSetActDirect(port_no=%d, param=*0x%x)", port_no, param);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const PadInfo& rinfo = handler->GetInfo();

	if (port_no >= rinfo.max_connect)
		return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	handler->SetRumble(port_no, param->motor[1], param->motor[0] > 0);

	return CELL_OK;
}

s32 cellPadGetInfo(vm::ptr<CellPadInfo> info)
{
	sys_io.trace("cellPadGetInfo(info=*0x%x)", info);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	std::memset(info.get_ptr(), 0, sizeof(CellPadInfo));

	const PadInfo& rinfo = handler->GetInfo();
	info->max_connect = rinfo.max_connect;
	info->now_connect = rinfo.now_connect;
	info->system_info = rinfo.system_info;

	std::vector<Pad>& pads = handler->GetPads();

	for (u32 i=0; i<CELL_MAX_PADS; ++i)
	{
		if (i >= pads.size())
			break;

		info->status[i] = pads[i].m_port_status;
		pads[i].m_port_status &= ~CELL_PAD_STATUS_ASSIGN_CHANGES;
		info->product_id[i] = 0x0268;
		info->vendor_id[i] = 0x054C;
	}

	return CELL_OK;
}

s32 cellPadGetInfo2(vm::ptr<CellPadInfo2> info)
{
	sys_io.trace("cellPadGetInfo2(info=*0x%x)", info);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	std::memset(info.get_ptr(), 0, sizeof(CellPadInfo2));

	const PadInfo& rinfo = handler->GetInfo();
	info->max_connect = rinfo.max_connect;
	info->now_connect = rinfo.now_connect;
	info->system_info = rinfo.system_info;

	std::vector<Pad>& pads = handler->GetPads();

	for (u32 i=0; i<CELL_PAD_MAX_PORT_NUM; ++i)
	{
		if (i >= pads.size())
			break;

		info->port_status[i] = pads[i].m_port_status;
		pads[i].m_port_status &= ~CELL_PAD_STATUS_ASSIGN_CHANGES;
		info->port_setting[i] = pads[i].m_port_setting;
		info->device_capability[i] = pads[i].m_device_capability;
		info->device_type[i] = pads[i].m_device_type;
	}

	return CELL_OK;
}

s32 cellPadGetCapabilityInfo(u32 port_no, vm::ptr<CellCapabilityInfo> info)
{
	sys_io.trace("cellPadGetCapabilityInfo(port_no=%d, data_addr:=0x%x)", port_no, info.addr());

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const PadInfo& rinfo = handler->GetInfo();

	if (port_no >= rinfo.max_connect)
		return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	const std::vector<Pad>& pads = handler->GetPads();

	//Should return the same as device capability mask, psl1ght has it backwards in pad.h
	info->info[0] = pads[port_no].m_device_capability;

	return CELL_OK;
}

s32 cellPadSetPortSetting(u32 port_no, u32 port_setting)
{
	sys_io.trace("cellPadSetPortSetting(port_no=%d, port_setting=0x%x)", port_no, port_setting);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const PadInfo& rinfo = handler->GetInfo();

	if (port_no >= rinfo.max_connect)
		return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	std::vector<Pad>& pads = handler->GetPads();
	pads[port_no].m_port_setting = port_setting;

	return CELL_OK;
}

s32 cellPadInfoPressMode(u32 port_no)
{
	sys_io.trace("cellPadInfoPressMode(port_no=%d)", port_no);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const PadInfo& rinfo = handler->GetInfo();

	if (port_no >= rinfo.max_connect)
		return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	const std::vector<Pad>& pads = handler->GetPads();

	return (pads[port_no].m_device_capability & CELL_PAD_CAPABILITY_PRESS_MODE) > 0;
}

s32 cellPadInfoSensorMode(u32 port_no)
{
	sys_io.trace("cellPadInfoSensorMode(port_no=%d)", port_no);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const PadInfo& rinfo = handler->GetInfo();

	if (port_no >= rinfo.max_connect)
		return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	const std::vector<Pad>& pads = handler->GetPads();

	return (pads[port_no].m_device_capability & CELL_PAD_CAPABILITY_SENSOR_MODE) > 0;
}

s32 cellPadSetPressMode(u32 port_no, u32 mode)
{
	sys_io.trace("cellPadSetPressMode(port_no=%d, mode=%d)", port_no, mode);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (mode != 0 && mode != 1)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const PadInfo& rinfo = handler->GetInfo();

	if (port_no >= rinfo.max_connect)
		return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	std::vector<Pad>& pads = handler->GetPads();

	if (mode)
		pads[port_no].m_port_setting |= CELL_PAD_SETTING_PRESS_ON;
	else
		pads[port_no].m_port_setting &= ~CELL_PAD_SETTING_PRESS_ON;

	return CELL_OK;
}

s32 cellPadSetSensorMode(u32 port_no, u32 mode)
{
	sys_io.trace("cellPadSetSensorMode(port_no=%d, mode=%d)", port_no, mode);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (mode != 0 && mode != 1)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const PadInfo& rinfo = handler->GetInfo();

	if (port_no >= rinfo.max_connect)
		return CELL_PAD_ERROR_INVALID_PARAMETER;
	if (port_no >= rinfo.now_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	std::vector<Pad>& pads = handler->GetPads();

	if (mode)
		pads[port_no].m_port_setting |= CELL_PAD_SETTING_SENSOR_ON;
	else
		pads[port_no].m_port_setting &= ~CELL_PAD_SETTING_SENSOR_ON;

	return CELL_OK;
}

s32 cellPadLddRegisterController()
{
	sys_io.todo("cellPadLddRegisterController()");

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellPadLddDataInsert(s32 handle, vm::ptr<CellPadData> data)
{
	sys_io.todo("cellPadLddDataInsert(handle=%d, data=*0x%x)", handle, data);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellPadLddGetPortNo(s32 handle)
{
	sys_io.todo("cellPadLddGetPortNo(handle=%d)", handle);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	return CELL_OK;
}

s32 cellPadLddUnregisterController(s32 handle)
{
	sys_io.todo("cellPadLddUnregisterController(handle=%d)", handle);

	const auto handler = fxm::get<PadHandlerBase>();

	if (!handler)
		return CELL_PAD_ERROR_UNINITIALIZED;

	return CELL_OK;
}


void cellPad_init()
{
	REG_FUNC(sys_io, cellPadInit);
	REG_FUNC(sys_io, cellPadEnd);
	REG_FUNC(sys_io, cellPadClearBuf);
	REG_FUNC(sys_io, cellPadGetData);
	REG_FUNC(sys_io, cellPadGetRawData); //
	REG_FUNC(sys_io, cellPadGetDataExtra);
	REG_FUNC(sys_io, cellPadSetActDirect);
	REG_FUNC(sys_io, cellPadGetInfo); //
	REG_FUNC(sys_io, cellPadGetInfo2);
	REG_FUNC(sys_io, cellPadPeriphGetInfo);
	REG_FUNC(sys_io, cellPadPeriphGetData);
	REG_FUNC(sys_io, cellPadSetPortSetting);
	REG_FUNC(sys_io, cellPadInfoPressMode); //
	REG_FUNC(sys_io, cellPadInfoSensorMode); //
	REG_FUNC(sys_io, cellPadSetPressMode); //
	REG_FUNC(sys_io, cellPadSetSensorMode); //
	REG_FUNC(sys_io, cellPadGetCapabilityInfo); //
	REG_FUNC(sys_io, cellPadLddRegisterController);
	REG_FUNC(sys_io, cellPadLddDataInsert);
	REG_FUNC(sys_io, cellPadLddGetPortNo);
	REG_FUNC(sys_io, cellPadLddUnregisterController);
}
