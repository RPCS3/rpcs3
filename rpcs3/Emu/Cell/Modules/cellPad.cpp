#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_process.h"

#include "Emu/Io/pad_types.h"
#include "Input/pad_thread.h"
#include "cellPad.h"

LOG_CHANNEL(sys_io);

template<>
void fmt_class_string<CellPadError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_PAD_ERROR_FATAL);
			STR_CASE(CELL_PAD_ERROR_INVALID_PARAMETER);
			STR_CASE(CELL_PAD_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_PAD_ERROR_UNINITIALIZED);
			STR_CASE(CELL_PAD_ERROR_RESOURCE_ALLOCATION_FAILED);
			STR_CASE(CELL_PAD_ERROR_DATA_READ_FAILED);
			STR_CASE(CELL_PAD_ERROR_NO_DEVICE);
			STR_CASE(CELL_PAD_ERROR_UNSUPPORTED_GAMEPAD);
			STR_CASE(CELL_PAD_ERROR_TOO_MANY_DEVICES);
			STR_CASE(CELL_PAD_ERROR_EBUSY);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<CellPadFilterError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_PADFILTER_ERROR_INVALID_PARAMETER);
		}

		return unknown;
	});
}

error_code cellPadInit(u32 max_connect)
{
	sys_io.warning("cellPadInit(max_connect=%d)", max_connect);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (config->max_connect)
		return CELL_PAD_ERROR_ALREADY_INITIALIZED;

	if (max_connect == 0 || max_connect > CELL_MAX_PADS)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	config->max_connect = std::min<u32>(max_connect, CELL_PAD_MAX_PORT_NUM);
	config->port_setting.fill(CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF);
	return CELL_OK;
}

error_code cellPadEnd()
{
	sys_io.notice("cellPadEnd()");

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect.exchange(0))
		return CELL_PAD_ERROR_UNINITIALIZED;

	return CELL_OK;
}

void clear_pad_buffer(const std::shared_ptr<Pad> pad)
{
	if (!pad)
		return;

	// Set 'm_buffer_cleared' to force a resend of everything
	// might as well also reset everything in our pad 'buffer' to nothing as well

	pad->m_buffer_cleared = true;
	pad->m_analog_left_x = pad->m_analog_left_y = pad->m_analog_right_x = pad->m_analog_right_y = 128;

	pad->m_digital_1 = pad->m_digital_2 = 0;
	pad->m_press_right = pad->m_press_left = pad->m_press_up = pad->m_press_down = 0;
	pad->m_press_triangle = pad->m_press_circle = pad->m_press_cross = pad->m_press_square = 0;
	pad->m_press_L1 = pad->m_press_L2 = pad->m_press_R1 = pad->m_press_R2 = 0;

	// ~399 on sensor y is a level non moving controller
	pad->m_sensor_y = 399;
	pad->m_sensor_x = pad->m_sensor_z = pad->m_sensor_g = 512;
}

error_code cellPadClearBuf(u32 port_no)
{
	sys_io.trace("cellPadClearBuf(port_no=%d)", port_no);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (port_no >= CELL_MAX_PADS)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const auto& pads = handler->GetPads();

	if (port_no >= config->max_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto pad = pads[port_no];

	if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
		return CELL_PAD_ERROR_NO_DEVICE;

	clear_pad_buffer(pad);

	return CELL_OK;
}

error_code cellPadGetData(u32 port_no, vm::ptr<CellPadData> data)
{
	sys_io.trace("cellPadGetData(port_no=%d, data=*0x%x)", port_no, data);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (port_no >= CELL_MAX_PADS || !data)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const auto& pads = handler->GetPads();

	if (port_no >= config->max_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto pad = pads[port_no];
	const auto setting = config->port_setting[port_no];

	if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
		return CELL_PAD_ERROR_NO_DEVICE;

	const PadInfo& rinfo = handler->GetInfo();

	if (rinfo.system_info & CELL_PAD_INFO_INTERCEPTED)
	{
		data->len = CELL_PAD_LEN_NO_CHANGE;
		return CELL_OK;
	}

	bool btnChanged = false;

	if (rinfo.ignore_input)
	{
		// Needed for Hotline Miami and Ninja Gaiden Sigma after dialogs were closed and buttons are still pressed.
		// Gran Turismo 6 would keep registering the Start button during OSK Dialogs if this wasn't cleared and if we'd return with len as CELL_PAD_LEN_NO_CHANGE.
		clear_pad_buffer(pad);
	}
	else if (pad->ldd)
	{
		memcpy(data.get_ptr(), pad->ldd_data, sizeof(CellPadData));
		if (setting & CELL_PAD_SETTING_SENSOR_ON)
			data->len = CELL_PAD_LEN_CHANGE_SENSOR_ON;
		else
			data->len = (setting & CELL_PAD_SETTING_PRESS_ON) ? CELL_PAD_LEN_CHANGE_PRESS_ON : CELL_PAD_LEN_CHANGE_DEFAULT;
		return CELL_OK;
	}
	else
	{
		u16 d1Initial = pad->m_digital_1;
		u16 d2Initial = pad->m_digital_2;

		for (Button& button : pad->m_buttons)
		{
			// here we check btns, and set pad accordingly,
			// if something changed, set btnChanged

			if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL1)
			{
				if (button.m_pressed)
					pad->m_digital_1 |= button.m_outKeyCode;
				else
					pad->m_digital_1 &= ~button.m_outKeyCode;

				switch (button.m_outKeyCode)
				{
				case CELL_PAD_CTRL_LEFT:
					if (pad->m_press_left != button.m_value) btnChanged = true;
					pad->m_press_left = button.m_value;
					break;
				case CELL_PAD_CTRL_DOWN:
					if (pad->m_press_down != button.m_value) btnChanged = true;
					pad->m_press_down = button.m_value;
					break;
				case CELL_PAD_CTRL_RIGHT:
					if (pad->m_press_right != button.m_value) btnChanged = true;
					pad->m_press_right = button.m_value;
					break;
				case CELL_PAD_CTRL_UP:
					if (pad->m_press_up != button.m_value) btnChanged = true;
					pad->m_press_up = button.m_value;
					break;
				// These arent pressure btns
				case CELL_PAD_CTRL_R3:
				case CELL_PAD_CTRL_L3:
				case CELL_PAD_CTRL_START:
				case CELL_PAD_CTRL_SELECT:
				default: break;
				}
			}
			else if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL2)
			{
				if (button.m_pressed)
					pad->m_digital_2 |= button.m_outKeyCode;
				else
					pad->m_digital_2 &= ~button.m_outKeyCode;

				switch (button.m_outKeyCode)
				{
				case CELL_PAD_CTRL_SQUARE:
					if (pad->m_press_square != button.m_value) btnChanged = true;
					pad->m_press_square = button.m_value;
					break;
				case CELL_PAD_CTRL_CROSS:
					if (pad->m_press_cross != button.m_value) btnChanged = true;
					pad->m_press_cross = button.m_value;
					break;
				case CELL_PAD_CTRL_CIRCLE:
					if (pad->m_press_circle != button.m_value) btnChanged = true;
					pad->m_press_circle = button.m_value;
					break;
				case CELL_PAD_CTRL_TRIANGLE:
					if (pad->m_press_triangle != button.m_value) btnChanged = true;
					pad->m_press_triangle = button.m_value;
					break;
				case CELL_PAD_CTRL_R1:
					if (pad->m_press_R1 != button.m_value) btnChanged = true;
					pad->m_press_R1 = button.m_value;
					break;
				case CELL_PAD_CTRL_L1:
					if (pad->m_press_L1 != button.m_value) btnChanged = true;
					pad->m_press_L1 = button.m_value;
					break;
				case CELL_PAD_CTRL_R2:
					if (pad->m_press_R2 != button.m_value) btnChanged = true;
					pad->m_press_R2 = button.m_value;
					break;
				case CELL_PAD_CTRL_L2:
					if (pad->m_press_L2 != button.m_value) btnChanged = true;
					pad->m_press_L2 = button.m_value;
					break;
				default: break;
				}
			}
		}

		for (const AnalogStick& stick : pad->m_sticks)
		{
			switch (stick.m_offset)
			{
			case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X:
				if (pad->m_analog_left_x != stick.m_value) btnChanged = true;
				pad->m_analog_left_x = stick.m_value;
				break;
			case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y:
				if (pad->m_analog_left_y != stick.m_value) btnChanged = true;
				pad->m_analog_left_y = stick.m_value;
				break;
			case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X:
				if (pad->m_analog_right_x != stick.m_value) btnChanged = true;
				pad->m_analog_right_x = stick.m_value;
				break;
			case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y:
				if (pad->m_analog_right_y != stick.m_value) btnChanged = true;
				pad->m_analog_right_y = stick.m_value;
				break;
			default: break;
			}
		}

		if (setting & CELL_PAD_SETTING_SENSOR_ON)
		{
			for (const AnalogSensor& sensor : pad->m_sensors)
			{
				switch (sensor.m_offset)
				{
				case CELL_PAD_BTN_OFFSET_SENSOR_X:
					if (pad->m_sensor_x != sensor.m_value) btnChanged = true;
					pad->m_sensor_x = sensor.m_value;
					break;
				case CELL_PAD_BTN_OFFSET_SENSOR_Y:
					if (pad->m_sensor_y != sensor.m_value) btnChanged = true;
					pad->m_sensor_y = sensor.m_value;
					break;
				case CELL_PAD_BTN_OFFSET_SENSOR_Z:
					if (pad->m_sensor_z != sensor.m_value) btnChanged = true;
					pad->m_sensor_z = sensor.m_value;
					break;
				case CELL_PAD_BTN_OFFSET_SENSOR_G:
					if (pad->m_sensor_g != sensor.m_value) btnChanged = true;
					pad->m_sensor_g = sensor.m_value;
					break;
				default: break;
				}
			}
		}

		if (d1Initial != pad->m_digital_1 || d2Initial != pad->m_digital_2)
		{
			btnChanged = true;
		}
	}

	if (setting & CELL_PAD_SETTING_SENSOR_ON)
	{
		// report back new data every ~10 ms even if the input doesn't change
		// this is observed behaviour when using a Dualshock 3 controller
		static std::array<std::chrono::time_point<steady_clock>, CELL_PAD_MAX_PORT_NUM> last_update = { };
		const std::chrono::time_point<steady_clock> now = steady_clock::now();

		if (btnChanged || pad->m_buffer_cleared || (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update[port_no]).count() >= 10))
		{
			data->len = CELL_PAD_LEN_CHANGE_SENSOR_ON;
			last_update[port_no] = now;
		}
		else
		{
			data->len = CELL_PAD_LEN_NO_CHANGE;
		}
	}
	else if (btnChanged || pad->m_buffer_cleared)
	{
		// only give back valid data if a controller state changed
		data->len = (setting & CELL_PAD_SETTING_PRESS_ON) ? CELL_PAD_LEN_CHANGE_PRESS_ON : CELL_PAD_LEN_CHANGE_DEFAULT;
	}
	else
	{
		// report no state changes
		data->len = CELL_PAD_LEN_NO_CHANGE;
	}

	pad->m_buffer_cleared = false;

	// only update parts of the output struct depending on the controller setting
	if (data->len > CELL_PAD_LEN_NO_CHANGE)
	{
		data->button[0] = 0x0; // always 0
		// bits 15-8 reserved, 7-4 = 0x7, 3-0: data->len/2;
		data->button[1] = (0x7 << 4) | std::min(data->len / 2, 15);

		data->button[CELL_PAD_BTN_OFFSET_DIGITAL1] = pad->m_digital_1;
		data->button[CELL_PAD_BTN_OFFSET_DIGITAL2] = pad->m_digital_2;
		data->button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X] = pad->m_analog_right_x;
		data->button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y] = pad->m_analog_right_y;
		data->button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X] = pad->m_analog_left_x;
		data->button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y] = pad->m_analog_left_y;

		if (setting & CELL_PAD_SETTING_PRESS_ON)
		{
			data->button[CELL_PAD_BTN_OFFSET_PRESS_RIGHT] = pad->m_press_right;
			data->button[CELL_PAD_BTN_OFFSET_PRESS_LEFT] = pad->m_press_left;
			data->button[CELL_PAD_BTN_OFFSET_PRESS_UP] = pad->m_press_up;
			data->button[CELL_PAD_BTN_OFFSET_PRESS_DOWN] = pad->m_press_down;
			data->button[CELL_PAD_BTN_OFFSET_PRESS_TRIANGLE] = pad->m_press_triangle;
			data->button[CELL_PAD_BTN_OFFSET_PRESS_CIRCLE] = pad->m_press_circle;
			data->button[CELL_PAD_BTN_OFFSET_PRESS_CROSS] = pad->m_press_cross;
			data->button[CELL_PAD_BTN_OFFSET_PRESS_SQUARE] = pad->m_press_square;
			data->button[CELL_PAD_BTN_OFFSET_PRESS_L1] = pad->m_press_L1;
			data->button[CELL_PAD_BTN_OFFSET_PRESS_L2] = pad->m_press_L2;
			data->button[CELL_PAD_BTN_OFFSET_PRESS_R1] = pad->m_press_R1;
			data->button[CELL_PAD_BTN_OFFSET_PRESS_R2] = pad->m_press_R2;
		}
		else
		{
			// Clear area if setting is not used
			constexpr u32 area_lengh = (CELL_PAD_LEN_CHANGE_PRESS_ON - CELL_PAD_LEN_CHANGE_DEFAULT) * sizeof(u16);
			std::memset(&data->button[CELL_PAD_LEN_CHANGE_DEFAULT], 0, area_lengh);
		}

		if (data->len == CELL_PAD_LEN_CHANGE_SENSOR_ON)
		{
			data->button[CELL_PAD_BTN_OFFSET_SENSOR_X] = pad->m_sensor_x;
			data->button[CELL_PAD_BTN_OFFSET_SENSOR_Y] = pad->m_sensor_y;
			data->button[CELL_PAD_BTN_OFFSET_SENSOR_Z] = pad->m_sensor_z;
			data->button[CELL_PAD_BTN_OFFSET_SENSOR_G] = pad->m_sensor_g;
		}
	}

	return CELL_OK;
}

error_code cellPadPeriphGetInfo(vm::ptr<CellPadPeriphInfo> info)
{
	sys_io.trace("cellPadPeriphGetInfo(info=*0x%x)", info);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (!info)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const PadInfo& rinfo = handler->GetInfo();

	std::memset(info.get_ptr(), 0, sizeof(CellPadPeriphInfo));

	info->max_connect = config->max_connect;
	info->now_connect = rinfo.now_connect;
	info->system_info = rinfo.system_info;

	const auto& pads = handler->GetPads();

	// TODO: Support other types of controllers
	for (u32 i = 0; i < CELL_PAD_MAX_PORT_NUM; ++i)
	{
		if (i >= config->max_connect)
			break;

		info->port_status[i] = pads[i]->m_port_status;
		pads[i]->m_port_status &= ~CELL_PAD_STATUS_ASSIGN_CHANGES;
		info->port_setting[i] = config->port_setting[i];
		info->device_capability[i] = pads[i]->m_device_capability;
		info->device_type[i] = pads[i]->m_device_type;
		info->pclass_type[i] = pads[i]->m_class_type;
		info->pclass_profile[i] = 0x0;
	}

	return CELL_OK;
}

error_code cellPadPeriphGetData(u32 port_no, vm::ptr<CellPadPeriphData> data)
{
	sys_io.trace("cellPadPeriphGetData(port_no=%d, data=*0x%x)", port_no, data);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	// port_no can only be 0-6 in this function
	if (port_no >= CELL_PAD_MAX_PORT_NUM || !data)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const auto& pads = handler->GetPads();

	if (port_no >= config->max_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto pad = pads[port_no];

	if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
		return CELL_PAD_ERROR_NO_DEVICE;

	// todo: support for 'unique' controllers, which goes in offsets 24+ in padData
	data->pclass_type = pad->m_class_type;
	data->pclass_profile = 0x0;

	return cellPadGetData(port_no, data.ptr(&CellPadPeriphData::cellpad_data));
}

error_code cellPadGetRawData(u32 port_no, vm::ptr<CellPadData> data)
{
	sys_io.todo("cellPadGetRawData(port_no=%d, data=*0x%x)", port_no, data);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (port_no >= CELL_MAX_PADS || !data)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const auto& pads = handler->GetPads();

	if (port_no >= config->max_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto pad = pads[port_no];

	if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
		return CELL_PAD_ERROR_NO_DEVICE;

	// ?

	return CELL_OK;
}

error_code cellPadGetDataExtra(u32 port_no, vm::ptr<u32> device_type, vm::ptr<CellPadData> data)
{
	sys_io.trace("cellPadGetDataExtra(port_no=%d, device_type=*0x%x, data=*0x%x)", port_no, device_type, data);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (port_no >= CELL_MAX_PADS || !data)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const auto& pads = handler->GetPads();

	if (port_no >= config->max_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto pad = pads[port_no];

	if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
		return CELL_PAD_ERROR_NO_DEVICE;

	// TODO: This is used just to get data from a BD/CEC remote,
	// but if the port isnt a remote, device type is set to 0 and just regular cellPadGetData is returned

	if (device_type) // no error is returned on NULL
	{
		*device_type = 0;
	}

	// set BD data before just incase
	data->button[24] = 0x0;
	data->button[25] = 0x0;

	return cellPadGetData(port_no, data);
}

error_code cellPadSetActDirect(u32 port_no, vm::ptr<CellPadActParam> param)
{
	sys_io.trace("cellPadSetActDirect(port_no=%d, param=*0x%x)", port_no, param);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (port_no >= CELL_MAX_PADS || !param)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	// Note: signed check unlike the usual unsigned check
	if (static_cast<s32>(g_ps3_process_info.sdk_ver) > 0x1FFFFF)
	{
		// make sure reserved bits are 0
		for (int i = 0; i < 6; i++)
		{
			if (param->reserved[i])
				return CELL_PAD_ERROR_INVALID_PARAMETER;
		}
	}

	const auto& pads = handler->GetPads();

	if (port_no >= config->max_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto pad = pads[port_no];

	if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
		return CELL_PAD_ERROR_NO_DEVICE;

	// TODO: find out if this is checked here or later or at all
	if (!(pad->m_device_capability & CELL_PAD_CAPABILITY_ACTUATOR))
		return CELL_PAD_ERROR_UNSUPPORTED_GAMEPAD;

	handler->SetRumble(port_no, param->motor[1], param->motor[0] > 0);

	return CELL_OK;
}

error_code cellPadGetInfo(vm::ptr<CellPadInfo> info)
{
	sys_io.trace("cellPadGetInfo(info=*0x%x)", info);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (!info)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	std::memset(info.get_ptr(), 0, sizeof(CellPadInfo));

	const PadInfo& rinfo = handler->GetInfo();
	info->max_connect = config->max_connect;
	info->now_connect = rinfo.now_connect;
	info->system_info = rinfo.system_info;

	const auto& pads = handler->GetPads();

	for (u32 i = 0; i < CELL_MAX_PADS; ++i)
	{
		if (i >= config->max_connect)
			break;

		pads[i]->m_port_status &= ~CELL_PAD_STATUS_ASSIGN_CHANGES; // TODO: should ASSIGN flags be cleared here?
		info->status[i] = pads[i]->m_port_status;

		// TODO: Allow selecting different product IDs
		switch (pads[i]->m_class_type)
		{
		case CELL_PAD_PCLASS_TYPE_GUITAR:
			// Sony Computer Entertainment America
			info->vendor_id[i] = 0x12BA;
			// RedOctane Guitar (Guitar Hero)
			info->product_id[i] = 0x0100;
			// Harmonix Guitar (Rock Band)
			// info->product_id[i] = 0x0200;
			break;
		case CELL_PAD_PCLASS_TYPE_DRUM:
			// Sony Computer Entertainment America
			info->vendor_id[i] = 0x12BA;
			// RedOctane Drum Kit (Guitar Hero)
			info->product_id[i] = 0x0120;
			// Harmonix Drum Kit (Rock Band)
			// info->product_id[i] = 0x0210;
			break;
		case CELL_PAD_PCLASS_TYPE_DJ:
			// Sony Computer Entertainment America
			info->vendor_id[i] = 0x12BA;
			// DJ Hero Turntable
			info->product_id[i] = 0x0140;
			break;
		case CELL_PAD_PCLASS_TYPE_DANCEMAT:
			// Konami Digital Entertainment
			info->vendor_id[i] = 0x1CCF;
			// Dance Dance Revolution Mat
			info->product_id[i] = 0x0140;
			break;
		default:
			// Sony Corp.
			info->vendor_id[i] = 0x054C;
			// PlayStation 3 Controller
			info->product_id[i] = 0x0268;
			break;
		}
	}

	return CELL_OK;
}

error_code cellPadGetInfo2(vm::ptr<CellPadInfo2> info)
{
	sys_io.trace("cellPadGetInfo2(info=*0x%x)", info);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (!info)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	std::memset(info.get_ptr(), 0, sizeof(CellPadInfo2));

	const PadInfo& rinfo = handler->GetInfo();
	info->max_connect = config->max_connect;
	info->now_connect = rinfo.now_connect;
	info->system_info = rinfo.system_info;

	const auto& pads = handler->GetPads();

	for (u32 i = 0; i < CELL_PAD_MAX_PORT_NUM; ++i)
	{
		if (i >= config->max_connect)
			break;

		info->port_status[i] = pads[i]->m_port_status;
		pads[i]->m_port_status &= ~CELL_PAD_STATUS_ASSIGN_CHANGES;
		info->port_setting[i] = config->port_setting[i];
		info->device_capability[i] = pads[i]->m_device_capability;
		info->device_type[i] = pads[i]->m_device_type;
	}

	return CELL_OK;
}

error_code cellPadGetCapabilityInfo(u32 port_no, vm::ptr<CellPadCapabilityInfo> info)
{
	sys_io.trace("cellPadGetCapabilityInfo(port_no=%d, data_addr:=0x%x)", port_no, info.addr());

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (port_no >= CELL_MAX_PADS || !info)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const auto& pads = handler->GetPads();

	if (port_no >= config->max_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto pad = pads[port_no];

	if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
		return CELL_PAD_ERROR_NO_DEVICE;

	// Should return the same as device capability mask, psl1ght has it backwards in pad->h
	memset(info->info, 0, CELL_PAD_MAX_CAPABILITY_INFO * sizeof(u32));
	info->info[0] = pad->m_device_capability;

	return CELL_OK;
}

error_code cellPadSetPortSetting(u32 port_no, u32 port_setting)
{
	sys_io.trace("cellPadSetPortSetting(port_no=%d, port_setting=0x%x)", port_no, port_setting);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (port_no >= CELL_MAX_PADS)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	// CELL_PAD_ERROR_NO_DEVICE is not returned in this case.
	if (port_no >= CELL_PAD_MAX_PORT_NUM)
		return CELL_OK;

	config->port_setting[port_no] = port_setting;

	// can also return CELL_PAD_ERROR_UNSUPPORTED_GAMEPAD

	return CELL_OK;
}

error_code cellPadInfoPressMode(u32 port_no)
{
	sys_io.trace("cellPadInfoPressMode(port_no=%d)", port_no);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (port_no >= CELL_MAX_PADS)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const auto& pads = handler->GetPads();

	if (port_no >= config->max_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto pad = pads[port_no];

	if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
		return CELL_PAD_ERROR_NO_DEVICE;

	return not_an_error((pad->m_device_capability & CELL_PAD_CAPABILITY_PRESS_MODE) ? 1 : 0);
}

error_code cellPadInfoSensorMode(u32 port_no)
{
	sys_io.trace("cellPadInfoSensorMode(port_no=%d)", port_no);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (port_no >= CELL_MAX_PADS)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const auto& pads = handler->GetPads();

	if (port_no >= config->max_connect)
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto pad = pads[port_no];

	if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
		return CELL_PAD_ERROR_NO_DEVICE;

	return not_an_error((pad->m_device_capability & CELL_PAD_CAPABILITY_SENSOR_MODE) ? 1 : 0);
}

error_code cellPadSetPressMode(u32 port_no, u32 mode)
{
	sys_io.trace("cellPadSetPressMode(port_no=%d, mode=%d)", port_no, mode);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (port_no >= CELL_PAD_MAX_PORT_NUM)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const auto& pads = handler->GetPads();

	// CELL_PAD_ERROR_NO_DEVICE is not returned in this case.
	if (port_no >= CELL_PAD_MAX_PORT_NUM)
		return CELL_OK;

	const auto pad = pads[port_no];

	// TODO: find out if this is checked here or later or at all
	if (!(pad->m_device_capability & CELL_PAD_CAPABILITY_PRESS_MODE))
		return CELL_PAD_ERROR_UNSUPPORTED_GAMEPAD;

	if (mode)
		config->port_setting[port_no] |= CELL_PAD_SETTING_PRESS_ON;
	else
		config->port_setting[port_no] &= ~CELL_PAD_SETTING_PRESS_ON;

	return CELL_OK;
}

error_code cellPadSetSensorMode(u32 port_no, u32 mode)
{
	sys_io.trace("cellPadSetSensorMode(port_no=%d, mode=%d)", port_no, mode);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (port_no >= CELL_MAX_PADS)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const auto& pads = handler->GetPads();

	// CELL_PAD_ERROR_NO_DEVICE is not returned in this case.
	if (port_no >= CELL_PAD_MAX_PORT_NUM)
		return CELL_OK;

	const auto pad = pads[port_no];

	// TODO: find out if this is checked here or later or at all
	if (!(pad->m_device_capability & CELL_PAD_CAPABILITY_SENSOR_MODE))
		return CELL_PAD_ERROR_UNSUPPORTED_GAMEPAD;

	if (mode)
		config->port_setting[port_no] |= CELL_PAD_SETTING_SENSOR_ON;
	else
		config->port_setting[port_no] &= ~CELL_PAD_SETTING_SENSOR_ON;

	return CELL_OK;
}

error_code cellPadLddRegisterController()
{
	sys_io.warning("cellPadLddRegisterController()");

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	const s32 handle = handler->AddLddPad();

	if (handle < 0)
		return CELL_PAD_ERROR_TOO_MANY_DEVICES;

	auto& pads = handler->GetPads();
	pads[handle]->Init(CELL_PAD_STATUS_CONNECTED | CELL_PAD_STATUS_ASSIGN_CHANGES | CELL_PAD_STATUS_CUSTOM_CONTROLLER, CELL_PAD_CAPABILITY_PS3_CONFORMITY, CELL_PAD_DEV_TYPE_LDD, 0);
	config->port_setting[handle] = 0;

	return not_an_error(handle);
}

error_code cellPadLddDataInsert(s32 handle, vm::ptr<CellPadData> data)
{
	sys_io.trace("cellPadLddDataInsert(handle=%d, data=*0x%x)", handle, data);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (handle < 0 || !data) // data == NULL stalls on decr
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	auto& pads = handler->GetPads();
	if (!pads[handle]->ldd)
		return CELL_PAD_ERROR_NO_DEVICE;

	memcpy(pads[handle]->ldd_data, data.get_ptr(), sizeof(CellPadData));

	return CELL_OK;
}

error_code cellPadLddGetPortNo(s32 handle)
{
	sys_io.trace("cellPadLddGetPortNo(handle=%d)", handle);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (handle < 0)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	auto& pads = handler->GetPads();
	if (!pads[handle]->ldd)
		return CELL_PAD_ERROR_FATAL; // might be incorrect

	// Other possible return values: CELL_PAD_ERROR_EBUSY, CELL_EBUSY
	return not_an_error(handle); // handle is port
}

error_code cellPadLddUnregisterController(s32 handle)
{
	sys_io.warning("cellPadLddUnregisterController(handle=%d)", handle);

	std::lock_guard lock(pad::g_pad_mutex);

	const auto config = g_fxo->get<pad_info>();

	if (!config->max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_current_handler();

	if (handle < 0)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const auto& pads = handler->GetPads();

	if (!pads[handle]->ldd)
		return CELL_PAD_ERROR_NO_DEVICE;

	handler->UnregisterLddPad(handle);

	return CELL_OK;
}

error_code cellPadFilterIIRInit(vm::ptr<CellPadFilterIIRSos> pSos, s32 cutoff)
{
	sys_io.todo("cellPadFilterIIRInit(pSos=*0x%x, cutoff=%d)", pSos, cutoff);

	if (!pSos) // TODO: does this check for cutoff > 2 ?
	{
		return CELL_PADFILTER_ERROR_INVALID_PARAMETER;
	}

	return CELL_OK;
}

u32 cellPadFilterIIRFilter(vm::ptr<CellPadFilterIIRSos> pSos, u32 filterIn)
{
	sys_io.todo("cellPadFilterIIRFilter(pSos=*0x%x, filterIn=%d)", pSos, filterIn);

	// TODO: apply filter

	return std::clamp(filterIn, 0u, 1023u);
}

s32 sys_io_3733EA3C(u32 port_no, vm::ptr<u32> device_type, vm::ptr<CellPadData> data)
{
	// Used by the ps1 emulator built into the firmware
	// Seems to call the same function that getdataextra does
	sys_io.trace("sys_io_3733EA3C(port_no=%d, device_type=*0x%x, data=*0x%x)", port_no, device_type, data);
	return cellPadGetDataExtra(port_no, device_type, data);
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

	REG_FUNC(sys_io, cellPadFilterIIRInit);
	REG_FUNC(sys_io, cellPadFilterIIRFilter);

	REG_FNID(sys_io, 0x3733EA3C, sys_io_3733EA3C);
}
