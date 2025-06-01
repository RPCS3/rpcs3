#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/system_config.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Io/pad_types.h"
#include "Emu/RSX/Overlays/overlay_debug_overlay.h"
#include "Input/pad_thread.h"
#include "Input/product_info.h"
#include "cellPad.h"

error_code sys_config_start(ppu_thread& ppu);
error_code sys_config_stop(ppu_thread& ppu);

extern bool is_input_allowed();

LOG_CHANNEL(cellPad);

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

extern void sys_io_serialize(utils::serial& ar);

pad_info::pad_info(utils::serial& ar)
	: max_connect(ar)
	, port_setting(ar)
	, reported_info(ar)
{
	//reported_info = {};
	sys_io_serialize(ar);
}

void pad_info::save(utils::serial& ar)
{
	USING_SERIALIZATION_VERSION(sys_io);

	ar(max_connect, port_setting, reported_info);

	sys_io_serialize(ar);
}

void show_debug_overlay(const CellPadData& data, const Pad& pad, const pad_info& config)
{
	const u32 setting = config.port_setting[pad.m_player_id];
	const u16 d1 = data.button[CELL_PAD_BTN_OFFSET_DIGITAL1];
	const u16 d2 = data.button[CELL_PAD_BTN_OFFSET_DIGITAL2];

	std::string text = fmt::format(
		">        Name:     Raw   Value   Pressure\n"
		">\n"
		">         Len:   %13d\n"
		">     Digital:   %5s   %5s\n"
		">       Press:   %5s   %5s\n"
		">      Sensor:   %5s   %5s\n"
		">\n"
		">   Digital 1:  0x%04x  0x%04x\n"
		">   Digital 2:  0x%04x  0x%04x\n"
		">\n"
		">    D-Pad Up:   %5d   %5d   %5d\n"
		">  D-Pad Down:   %5d   %5d   %5d\n"
		">  D-Pad Left:   %5d   %5d   %5d\n"
		"> D-Pad Right:   %5d   %5d   %5d\n"
		">       Cross:   %5d   %5d   %5d\n"
		">      Square:   %5d   %5d   %5d\n"
		">      Circle:   %5d   %5d   %5d\n"
		">    Triangle:   %5d   %5d   %5d\n"
		">       Start:   %5d   %5d\n"
		">      Select:   %5d   %5d\n"
		">          PS:   %5d   %5d\n"
		">          L1:   %5d   %5d   %5d\n"
		">          L2:   %5d   %5d   %5d\n"
		">          L3:   %5d   %5d\n"
		">          R1:   %5d   %5d   %5d\n"
		">          R2:   %5d   %5d   %5d\n"
		">          R3:   %5d   %5d\n"
		">        LS X:   %5d   %5d\n"
		">        LS Y:   %5d   %5d\n"
		">        RS X:   %5d   %5d\n"
		">        RS Y:   %5d   %5d\n"
		">\n"
		">    Sensor X:   %5d   %5d\n"
		">    Sensor Y:   %5d   %5d\n"
		">    Sensor Z:   %5d   %5d\n"
		">    Sensor G:   %5d   %5d\n"
		">\n"
		">         PID:  0x%04x\n"
		">         VID:  0x%04x\n"
		"> Device Type:  0x%08x\n"
		">  Class Type:  0x%08x\n"
		,
		data.len,
		"on", data.len >= CELL_PAD_LEN_CHANGE_DEFAULT ? "on" : "off",
		(setting & CELL_PAD_SETTING_PRESS_ON) ? "on" : "off", data.len >= CELL_PAD_LEN_CHANGE_PRESS_ON ? "on" : "off",
		(setting & CELL_PAD_SETTING_SENSOR_ON) ? "on" : "off", data.len >= CELL_PAD_LEN_CHANGE_SENSOR_ON ? "on" : "off",
		pad.m_digital_1, d1,
		pad.m_digital_2, d2,
		pad.m_press_up, !!(d1 & CELL_PAD_CTRL_UP), data.button[CELL_PAD_BTN_OFFSET_PRESS_UP],
		pad.m_press_down, !!(d1 & CELL_PAD_CTRL_DOWN), data.button[CELL_PAD_BTN_OFFSET_PRESS_DOWN],
		pad.m_press_left, !!(d1 & CELL_PAD_CTRL_LEFT), data.button[CELL_PAD_BTN_OFFSET_PRESS_LEFT],
		pad.m_press_right, !!(d1 & CELL_PAD_CTRL_RIGHT), data.button[CELL_PAD_BTN_OFFSET_PRESS_RIGHT],
		pad.m_press_cross, !!(d2 & CELL_PAD_CTRL_CROSS), data.button[CELL_PAD_BTN_OFFSET_PRESS_CROSS],
		pad.m_press_square, !!(d2 & CELL_PAD_CTRL_SQUARE), data.button[CELL_PAD_BTN_OFFSET_PRESS_SQUARE],
		pad.m_press_circle, !!(d2 & CELL_PAD_CTRL_CIRCLE), data.button[CELL_PAD_BTN_OFFSET_PRESS_CIRCLE],
		pad.m_press_triangle, !!(d2 & CELL_PAD_CTRL_TRIANGLE), data.button[CELL_PAD_BTN_OFFSET_PRESS_TRIANGLE],
		!!(pad.m_digital_1 & CELL_PAD_CTRL_START), !!(d1 & CELL_PAD_CTRL_START),
		!!(pad.m_digital_1 & CELL_PAD_CTRL_SELECT), !!(d1 & CELL_PAD_CTRL_SELECT),
		!!(pad.m_digital_1 & CELL_PAD_CTRL_PS), !!(d1 & CELL_PAD_CTRL_PS),
		pad.m_press_L1, !!(d2 & CELL_PAD_CTRL_L1), data.button[CELL_PAD_BTN_OFFSET_PRESS_L1],
		pad.m_press_L2, !!(d2 & CELL_PAD_CTRL_L2), data.button[CELL_PAD_BTN_OFFSET_PRESS_L2],
		!!(pad.m_digital_1 & CELL_PAD_CTRL_L3), !!(d1 & CELL_PAD_CTRL_L3),
		pad.m_press_R1, !!(d2 & CELL_PAD_CTRL_R1), data.button[CELL_PAD_BTN_OFFSET_PRESS_R1],
		pad.m_press_R2, !!(d2 & CELL_PAD_CTRL_R2), data.button[CELL_PAD_BTN_OFFSET_PRESS_R2],
		!!(pad.m_digital_1 & CELL_PAD_CTRL_R3), !!(d1 & CELL_PAD_CTRL_R3),
		pad.m_analog_left_x, data.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X],
		pad.m_analog_left_y, data.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y],
		pad.m_analog_right_x, data.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X],
		pad.m_analog_right_y, data.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y],
		pad.m_sensor_x, data.button[CELL_PAD_BTN_OFFSET_SENSOR_X],
		pad.m_sensor_y, data.button[CELL_PAD_BTN_OFFSET_SENSOR_Y],
		pad.m_sensor_z, data.button[CELL_PAD_BTN_OFFSET_SENSOR_Z],
		pad.m_sensor_g, data.button[CELL_PAD_BTN_OFFSET_SENSOR_G],
		pad.m_product_id,
		pad.m_vendor_id,
		pad.m_device_type,
		pad.m_class_type
	);

	rsx::overlays::set_debug_overlay_text(std::move(text));
}

extern void send_sys_io_connect_event(usz index, u32 state);

bool cellPad_NotifyStateChange(usz index, u64 /*state*/, bool locked, bool is_blocking = true)
{
	auto info = g_fxo->try_get<pad_info>();

	if (!info)
	{
		return true;
	}

	std::unique_lock lock(pad::g_pad_mutex, std::defer_lock);

	if (locked)
	{
		if (is_blocking)
		{
			lock.lock();
		}
		else
		{
			if (!lock.try_lock())
			{
				return false;
			}
		}
	}

	if (index >= info->get_max_connect())
	{
		return true;
	}

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();
	const auto& pad = pads[index];

	if (pad->is_fake_pad)
	{
		return true;
	}

	pad_data_internal& reported_info = info->reported_info[index];
	const u32 old_status = reported_info.port_status;

	// Ignore sent status for now, use the latest instead
	// NOTE 1: The state's CONNECTED bit should currently be identical to the current
	//         m_port_status CONNECTED bit when called from our pad handlers.
	// NOTE 2: Make sure to propagate all other status bits to the reported status.
	const u32 new_status = pad->m_port_status;

	if (~(old_status ^ new_status) & CELL_PAD_STATUS_CONNECTED)
	{
		// old and new have the same connection status
		return true;
	}

	reported_info.port_status = new_status | CELL_PAD_STATUS_ASSIGN_CHANGES;
	reported_info.device_capability = pad->m_device_capability;
	reported_info.device_type = pad->m_device_type;
	reported_info.pclass_type = pad->m_class_type;
	reported_info.pclass_profile = pad->m_class_profile;

	if (pad->m_vendor_id == 0 || pad->m_product_id == 0)
	{
		// Fallback to defaults
		const std::vector<input::product_info> input_products = input::get_products_by_class(pad->m_class_type);
		const input::product_info& product = ::at32(input_products, 0);
		reported_info.vendor_id = product.vendor_id;
		reported_info.product_id = product.product_id;
	}
	else
	{
		reported_info.vendor_id = pad->m_vendor_id;
		reported_info.product_id = pad->m_product_id;
	}

	return true;
}

extern void pad_state_notify_state_change(usz index, u32 state)
{
	send_sys_io_connect_event(index, state);
}

error_code cellPadInit(ppu_thread& ppu, u32 max_connect)
{
	cellPad.warning("cellPadInit(max_connect=%d)", max_connect);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (config.max_connect)
		return CELL_PAD_ERROR_ALREADY_INITIALIZED;

	if (max_connect == 0 || max_connect > CELL_MAX_PADS)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	sys_config_start(ppu);

	config.max_connect = max_connect;
	config.port_setting.fill(CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF);
	config.reported_info = {};

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();

	for (usz i = 0; i < config.get_max_connect(); ++i)
	{
		if (!pads[i]->is_fake_pad && pads[i]->is_connected())
		{
			send_sys_io_connect_event(i, CELL_PAD_STATUS_CONNECTED);
		}
	}

	return CELL_OK;
}

error_code cellPadEnd(ppu_thread& ppu)
{
	cellPad.notice("cellPadEnd()");

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect.exchange(0))
		return CELL_PAD_ERROR_UNINITIALIZED;

	sys_config_stop(ppu);
	return CELL_OK;
}

void clear_pad_buffer(const std::shared_ptr<Pad>& pad)
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

	pad->m_sensor_x = DEFAULT_MOTION_X;
	pad->m_sensor_y = DEFAULT_MOTION_Y;
	pad->m_sensor_z = DEFAULT_MOTION_Z;
	pad->m_sensor_g = DEFAULT_MOTION_G;
}

error_code cellPadClearBuf(u32 port_no)
{
	cellPad.trace("cellPadClearBuf(port_no=%d)", port_no);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (port_no >= CELL_MAX_PADS)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	if (port_no >= config.get_max_connect())
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();
	const auto& pad = pads[port_no];

	if (pad->is_fake_pad || !config.is_reportedly_connected(port_no) || !pad->is_connected())
		return not_an_error(CELL_PAD_ERROR_NO_DEVICE);

	clear_pad_buffer(pad);

	return CELL_OK;
}

void pad_get_data(u32 port_no, CellPadData* data, bool get_periph_data = false)
{
	auto& config = g_fxo->get<pad_info>();
	const auto handler = pad::get_pad_thread();
	const auto& pad = handler->GetPads()[port_no];
	const PadInfo& rinfo = handler->GetInfo();

	if (rinfo.system_info & CELL_PAD_INFO_INTERCEPTED)
	{
		data->len = CELL_PAD_LEN_NO_CHANGE;
		return;
	}

	const u32 setting = config.port_setting[port_no];
	bool btnChanged = false;

	if (rinfo.ignore_input || !is_input_allowed())
	{
		// Needed for Hotline Miami and Ninja Gaiden Sigma after dialogs were closed and buttons are still pressed.
		// Gran Turismo 6 would keep registering the Start button during OSK Dialogs if this wasn't cleared and if we'd return with len as CELL_PAD_LEN_NO_CHANGE.
		clear_pad_buffer(pad);
	}
	else if (pad->ldd)
	{
		if (setting & CELL_PAD_SETTING_SENSOR_ON)
			data->len = CELL_PAD_LEN_CHANGE_SENSOR_ON;
		else
			data->len = (setting & CELL_PAD_SETTING_PRESS_ON) ? CELL_PAD_LEN_CHANGE_PRESS_ON : CELL_PAD_LEN_CHANGE_DEFAULT;
		std::memcpy(data->button, pad->ldd_data.button, data->len * sizeof(u16));
		return;
	}
	else
	{
		const u16 d1Initial = pad->m_digital_1;
		const u16 d2Initial = pad->m_digital_2;

		// Check if this pad is configured as a skateboard which ignores sticks and pressure button values.
		// Curiously it maps infrared on the press value of the face buttons for some reason.
		const bool use_piggyback = pad->m_class_type == CELL_PAD_PCLASS_TYPE_SKATEBOARD;

		const auto set_value = [&btnChanged, use_piggyback, &pad](u16& value, u16 new_value, bool force_processing = false, u16 old_max_value = 255, u16 new_max_value = 255)
		{
			if (use_piggyback)
			{
				if (!force_processing)
					return;

				// Some piggyback values need to be scaled down on emulated devices
				if (old_max_value != new_max_value)
				{
					if (pad->m_class_type == CELL_PAD_PCLASS_TYPE_SKATEBOARD && pad->m_pad_handler != pad_handler::skateboard)
					{
						new_value = static_cast<u16>(new_max_value * std::clamp(new_value / static_cast<f32>(old_max_value), 0.0f, 1.0f));
					}
				}
			}

			if (value != new_value)
			{
				btnChanged = true;
				value = new_value;
			}
		};

		for (const Button& button : pad->m_buttons_external)
		{
			// here we check btns, and set pad accordingly,
			// if something changed, set btnChanged

			switch (button.m_offset)
			{
			case CELL_PAD_BTN_OFFSET_DIGITAL1:
			{
				if (button.m_pressed)
					pad->m_digital_1 |= button.m_outKeyCode;
				else
					pad->m_digital_1 &= ~button.m_outKeyCode;

				switch (button.m_outKeyCode)
				{
				case CELL_PAD_CTRL_LEFT:  set_value(pad->m_press_left,  button.m_value); break;
				case CELL_PAD_CTRL_DOWN:  set_value(pad->m_press_down,  button.m_value); break;
				case CELL_PAD_CTRL_RIGHT: set_value(pad->m_press_right, button.m_value); break;
				case CELL_PAD_CTRL_UP:    set_value(pad->m_press_up,    button.m_value); break;
				// These aren't pressure btns
				case CELL_PAD_CTRL_R3:
				case CELL_PAD_CTRL_L3:
				case CELL_PAD_CTRL_START:
				case CELL_PAD_CTRL_SELECT:
				default: break;
				}
				break;
			}
			case CELL_PAD_BTN_OFFSET_DIGITAL2:
			{
				if (button.m_pressed)
					pad->m_digital_2 |= button.m_outKeyCode;
				else
					pad->m_digital_2 &= ~button.m_outKeyCode;

				switch (button.m_outKeyCode)
				{
				case CELL_PAD_CTRL_SQUARE:   set_value(pad->m_press_square,   button.m_value); break;
				case CELL_PAD_CTRL_CROSS:    set_value(pad->m_press_cross,    button.m_value); break;
				case CELL_PAD_CTRL_CIRCLE:   set_value(pad->m_press_circle,   button.m_value); break;
				case CELL_PAD_CTRL_TRIANGLE: set_value(pad->m_press_triangle, button.m_value); break;
				case CELL_PAD_CTRL_R1:       set_value(pad->m_press_R1,       button.m_value); break;
				case CELL_PAD_CTRL_L1:       set_value(pad->m_press_L1,       button.m_value); break;
				case CELL_PAD_CTRL_R2:       set_value(pad->m_press_R2,       button.m_value); break;
				case CELL_PAD_CTRL_L2:       set_value(pad->m_press_L2,       button.m_value); break;
				default: break;
				}
				break;
			}
			case CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK:
			{
				switch (button.m_outKeyCode)
				{
				case CELL_PAD_CTRL_PRESS_RIGHT:    set_value(pad->m_press_right,    button.m_value, true); break;
				case CELL_PAD_CTRL_PRESS_LEFT:     set_value(pad->m_press_left,     button.m_value, true); break;
				case CELL_PAD_CTRL_PRESS_UP:       set_value(pad->m_press_up,       button.m_value, true); break;
				case CELL_PAD_CTRL_PRESS_DOWN:     set_value(pad->m_press_down,     button.m_value, true); break;
				case CELL_PAD_CTRL_PRESS_TRIANGLE: set_value(pad->m_press_triangle, button.m_value, true, 255, 63); break; // Infrared on RIDE Skateboard
				case CELL_PAD_CTRL_PRESS_CIRCLE:   set_value(pad->m_press_circle,   button.m_value, true, 255, 63); break; // Infrared on RIDE Skateboard
				case CELL_PAD_CTRL_PRESS_CROSS:    set_value(pad->m_press_cross,    button.m_value, true, 255, 63); break; // Infrared on RIDE Skateboard
				case CELL_PAD_CTRL_PRESS_SQUARE:   set_value(pad->m_press_square,   button.m_value, true, 255, 63); break; // Infrared on RIDE Skateboard
				case CELL_PAD_CTRL_PRESS_L1:       set_value(pad->m_press_L1,       button.m_value, true); break;
				case CELL_PAD_CTRL_PRESS_R1:       set_value(pad->m_press_R1,       button.m_value, true); break;
				case CELL_PAD_CTRL_PRESS_L2:       set_value(pad->m_press_L2,       button.m_value, true); break;
				case CELL_PAD_CTRL_PRESS_R2:       set_value(pad->m_press_R2,       button.m_value, true); break;
				default: break;
				}
				break;
			}
			default:
				break;
			}
		}

		for (const AnalogStick& stick : pad->m_sticks_external)
		{
			switch (stick.m_offset)
			{
			case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X:  set_value(pad->m_analog_left_x,  stick.m_value); break;
			case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y:  set_value(pad->m_analog_left_y,  stick.m_value); break;
			case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X: set_value(pad->m_analog_right_x, stick.m_value); break;
			case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y: set_value(pad->m_analog_right_y, stick.m_value); break;
			default: break;
			}
		}

		if (setting & CELL_PAD_SETTING_SENSOR_ON)
		{
			for (const AnalogSensor& sensor : pad->m_sensors)
			{
				switch (sensor.m_offset)
				{
				case CELL_PAD_BTN_OFFSET_SENSOR_X: set_value(pad->m_sensor_x, sensor.m_value, true); break;
				case CELL_PAD_BTN_OFFSET_SENSOR_Y: set_value(pad->m_sensor_y, sensor.m_value, true); break;
				case CELL_PAD_BTN_OFFSET_SENSOR_Z: set_value(pad->m_sensor_z, sensor.m_value, true); break;
				case CELL_PAD_BTN_OFFSET_SENSOR_G: set_value(pad->m_sensor_g, sensor.m_value, true); break;
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
		static std::array<steady_clock::time_point, CELL_PAD_MAX_PORT_NUM> last_update = { };
		const auto now = steady_clock::now();

		if (btnChanged || pad->m_buffer_cleared || now - last_update[port_no] >= 10ms)
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
			data->button[CELL_PAD_BTN_OFFSET_PRESS_R1] = pad->m_press_R1;
			data->button[CELL_PAD_BTN_OFFSET_PRESS_L2] = pad->m_press_L2;
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

	if (!get_periph_data || data->len <= CELL_PAD_LEN_CHANGE_SENSOR_ON)
	{
		return;
	}

	const auto get_pressure_value = [setting](u16 val, u16 min, u16 max) -> u16
	{
		if (setting & CELL_PAD_SETTING_PRESS_ON)
		{
			return std::clamp(val, min, max);
		}

		if (val > 0)
		{
			return max;
		}

		return 0;
	};

	// TODO: support for 'unique' controllers, which goes in offsets 24+ in padData (CELL_PAD_PCLASS_BTN_OFFSET)
	// TODO: update data->len accordingly

	switch (pad->m_class_profile)
	{
	default:
	case CELL_PAD_PCLASS_TYPE_STANDARD:
	case CELL_PAD_PCLASS_TYPE_NAVIGATION:
	case CELL_PAD_PCLASS_TYPE_SKATEBOARD:
	{
		break;
	}
	case CELL_PAD_PCLASS_TYPE_GUITAR:
	{
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_1]      = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_2]      = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_3]      = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_4]      = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_5]      = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_STRUM_UP]    = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_STRUM_DOWN]  = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_WHAMMYBAR]   = 0x80; // 0x80 – 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_H1]     = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_H2]     = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_H3]     = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_H4]     = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_H5]     = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_5WAY_EFFECT] = 0x0019; // One of 5 values: 0x0019, 0x004C, 0x007F (or 0x0096), 0x00B2, 0x00E5 (or 0x00E2)
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_TILT_SENS]   = get_pressure_value(0, 0x0, 0xFF);
		break;
	}
	case CELL_PAD_PCLASS_TYPE_DRUM:
	{
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DRUM_SNARE]     = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DRUM_TOM]       = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DRUM_TOM2]      = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DRUM_TOM_FLOOR] = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DRUM_KICK]      = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DRUM_CYM_HiHAT] = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DRUM_CYM_CRASH] = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DRUM_CYM_RIDE]  = get_pressure_value(0, 0x0, 0xFF);
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DRUM_KICK2]     = get_pressure_value(0, 0x0, 0xFF);
		break;
	}
	case CELL_PAD_PCLASS_TYPE_DJ:
	{
		// First deck
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DJ_MIXER_ATTACK]     = 0;    // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DJ_MIXER_CROSSFADER] = 0;    // 0x0 - 0x3FF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DJ_MIXER_DSP_DIAL]   = 0;    // 0x0 - 0x3FF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK1_STREAM1]    = 0;    // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK1_STREAM2]    = 0;    // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK1_STREAM3]    = 0;    // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK1_PLATTER]    = 0x80; // 0x0 - 0xFF (neutral: 0x80)

		// Second deck
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK2_STREAM1]    = 0;    // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK2_STREAM2]    = 0;    // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK2_STREAM3]    = 0;    // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK2_PLATTER]    = 0x80; // 0x0 - 0xFF (neutral: 0x80)
		break;
	}
	case CELL_PAD_PCLASS_TYPE_DANCEMAT:
	{
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_CIRCLE]   = 0; // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_CROSS]    = 0; // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_TRIANGLE] = 0; // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_SQUARE]   = 0; // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_RIGHT]    = 0; // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_LEFT]     = 0; // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_UP]       = 0; // 0x0 or 0xFF
		data->button[CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_DOWN]     = 0; // 0x0 or 0xFF
		break;
	}
	}
}

error_code cellPadGetData(u32 port_no, vm::ptr<CellPadData> data)
{
	cellPad.trace("cellPadGetData(port_no=%d, data=*0x%x)", port_no, data);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (port_no >= CELL_MAX_PADS || !data)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	if (port_no >= config.get_max_connect())
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();
	const auto& pad = pads[port_no];

	if (pad->is_fake_pad || !config.is_reportedly_connected(port_no) || !pad->is_connected())
		return not_an_error(CELL_PAD_ERROR_NO_DEVICE);

	pad_get_data(port_no, data.get_ptr());

	if (g_cfg.io.debug_overlay && !g_cfg.video.debug_overlay && port_no == 0)
	{
		show_debug_overlay(*data, *pad, config);
	}

	return CELL_OK;
}

error_code cellPadPeriphGetInfo(vm::ptr<CellPadPeriphInfo> info)
{
	cellPad.trace("cellPadPeriphGetInfo(info=*0x%x)", info);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (!info)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	const auto handler = pad::get_pad_thread();
	const PadInfo& rinfo = handler->GetInfo();

	std::memset(info.get_ptr(), 0, sizeof(CellPadPeriphInfo));

	info->max_connect = config.max_connect;
	info->system_info = rinfo.system_info;

	u32 now_connect = 0;

	for (u32 i = 0; i < config.get_max_connect(); ++i)
	{
		pad_data_internal& reported_info = config.reported_info[i];

		info->port_status[i] = reported_info.port_status;
		info->port_setting[i] = config.port_setting[i];

		reported_info.port_status &= ~CELL_PAD_STATUS_ASSIGN_CHANGES;

		if (~reported_info.port_status & CELL_PAD_STATUS_CONNECTED)
		{
			continue;
		}

		info->device_capability[i] = reported_info.device_capability;
		info->device_type[i] = reported_info.device_type;
		info->pclass_type[i] = reported_info.pclass_type;
		info->pclass_profile[i] = reported_info.pclass_profile;

		now_connect++;
	}

	info->now_connect = now_connect;

	return CELL_OK;
}

error_code cellPadPeriphGetData(u32 port_no, vm::ptr<CellPadPeriphData> data)
{
	cellPad.trace("cellPadPeriphGetData(port_no=%d, data=*0x%x)", port_no, data);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	// port_no can only be 0-6 in this function
	if (port_no >= CELL_PAD_MAX_PORT_NUM || !data)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	if (port_no >= config.get_max_connect())
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();
	const auto& pad = pads[port_no];

	if (pad->is_fake_pad || !config.is_reportedly_connected(port_no) || !pad->is_connected())
		return not_an_error(CELL_PAD_ERROR_NO_DEVICE);

	pad_get_data(port_no, &data->cellpad_data, true);

	data->pclass_type = pad->m_class_type;
	data->pclass_profile = pad->m_class_profile;

	return CELL_OK;
}

error_code cellPadGetRawData(u32 port_no, vm::ptr<CellPadData> data)
{
	cellPad.todo("cellPadGetRawData(port_no=%d, data=*0x%x)", port_no, data);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (port_no >= CELL_MAX_PADS || !data)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	if (port_no >= config.get_max_connect())
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();
	const auto& pad = pads[port_no];

	if (pad->is_fake_pad || !config.is_reportedly_connected(port_no) || !pad->is_connected())
		return not_an_error(CELL_PAD_ERROR_NO_DEVICE);

	// ?

	return CELL_OK;
}

error_code cellPadGetDataExtra(u32 port_no, vm::ptr<u32> device_type, vm::ptr<CellPadData> data)
{
	cellPad.trace("cellPadGetDataExtra(port_no=%d, device_type=*0x%x, data=*0x%x)", port_no, device_type, data);

	// TODO: This is used just to get data from a BD/CEC remote,
	// but if the port isn't a remote, device type is set to CELL_PAD_DEV_TYPE_STANDARD and just regular cellPadGetData is returned

	if (auto err = cellPadGetData(port_no, data))
	{
		return err;
	}

	if (device_type) // no error is returned on NULL
	{
		*device_type = CELL_PAD_DEV_TYPE_STANDARD;
	}

	// Set BD data
	data->button[24] = 0x0;
	data->button[25] = 0x0;

	return CELL_OK;
}

error_code cellPadSetActDirect(u32 port_no, vm::ptr<CellPadActParam> param)
{
	cellPad.trace("cellPadSetActDirect(port_no=%d, param=*0x%x)", port_no, param);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

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

	if (port_no >= config.get_max_connect())
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();
	const auto& pad = pads[port_no];

	if (pad->is_fake_pad || !config.is_reportedly_connected(port_no) || !pad->is_connected())
		return not_an_error(CELL_PAD_ERROR_NO_DEVICE);

	// TODO: find out if this is checked here or later or at all
	if (!(pad->m_device_capability & CELL_PAD_CAPABILITY_ACTUATOR))
		return CELL_PAD_ERROR_UNSUPPORTED_GAMEPAD;

	handler->SetRumble(port_no, param->motor[1], param->motor[0] > 0);

	return CELL_OK;
}

error_code cellPadGetInfo(vm::ptr<CellPadInfo> info)
{
	cellPad.trace("cellPadGetInfo(info=*0x%x)", info);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (!info)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	std::memset(info.get_ptr(), 0, sizeof(CellPadInfo));

	const auto handler = pad::get_pad_thread();
	const PadInfo& rinfo = handler->GetInfo();
	info->max_connect = config.max_connect;
	info->system_info = rinfo.system_info;

	u32 now_connect = 0;

	for (u32 i = 0; i < config.get_max_connect(); ++i)
	{
		pad_data_internal& reported_info = config.reported_info[i];
		reported_info.port_status &= ~CELL_PAD_STATUS_ASSIGN_CHANGES; // TODO: should ASSIGN flags be cleared here?

		info->status[i] = reported_info.port_status;

		if (~reported_info.port_status & CELL_PAD_STATUS_CONNECTED)
		{
			continue;
		}

		info->vendor_id[i] = reported_info.vendor_id;
		info->product_id[i] = reported_info.product_id;

		now_connect++;
	}

	info->now_connect = now_connect;
	return CELL_OK;
}

error_code cellPadGetInfo2(vm::ptr<CellPadInfo2> info)
{
	cellPad.trace("cellPadGetInfo2(info=*0x%x)", info);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (!info)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	std::memset(info.get_ptr(), 0, sizeof(CellPadInfo2));

	const auto handler = pad::get_pad_thread();
	const PadInfo& rinfo = handler->GetInfo();
	info->max_connect = config.get_max_connect(); // Here it is forcibly clamped
	info->system_info = rinfo.system_info;

	u32 now_connect = 0;

	const auto& pads = handler->GetPads();

	for (u32 i = 0; i < config.get_max_connect(); ++i)
	{
		pad_data_internal& reported_info = config.reported_info[i];

		info->port_status[i] = reported_info.port_status;
		info->port_setting[i] = config.port_setting[i];

		reported_info.port_status &= ~CELL_PAD_STATUS_ASSIGN_CHANGES;

		if (~reported_info.port_status & CELL_PAD_STATUS_CONNECTED)
		{
			continue;
		}

		info->device_capability[i] = pads[i]->m_device_capability;
		info->device_type[i] = pads[i]->m_device_type;

		now_connect++;
	}

	info->now_connect = now_connect;
	return CELL_OK;
}

error_code cellPadGetCapabilityInfo(u32 port_no, vm::ptr<CellPadCapabilityInfo> info)
{
	cellPad.trace("cellPadGetCapabilityInfo(port_no=%d, data_addr:=0x%x)", port_no, info.addr());

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (port_no >= CELL_MAX_PADS || !info)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	if (port_no >= config.get_max_connect())
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();
	const auto& pad = pads[port_no];

	if (pad->is_fake_pad || !config.is_reportedly_connected(port_no) || !pad->is_connected())
		return not_an_error(CELL_PAD_ERROR_NO_DEVICE);

	// Should return the same as device capability mask, psl1ght has it backwards in pad->h
	memset(info->info, 0, CELL_PAD_MAX_CAPABILITY_INFO * sizeof(u32));
	info->info[0] = pad->m_device_capability;

	return CELL_OK;
}

error_code cellPadSetPortSetting(u32 port_no, u32 port_setting)
{
	cellPad.trace("cellPadSetPortSetting(port_no=%d, port_setting=0x%x)", port_no, port_setting);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (port_no >= CELL_MAX_PADS)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	// CELL_PAD_ERROR_NO_DEVICE is not returned in this case.
	if (port_no >= CELL_PAD_MAX_PORT_NUM)
		return CELL_OK;

	config.port_setting[port_no] = port_setting;

	// can also return CELL_PAD_ERROR_UNSUPPORTED_GAMEPAD <- Update: seems to be just internal and ignored

	return CELL_OK;
}

error_code cellPadInfoPressMode(u32 port_no)
{
	cellPad.trace("cellPadInfoPressMode(port_no=%d)", port_no);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (port_no >= CELL_MAX_PADS)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	if (port_no >= config.get_max_connect())
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();
	const auto& pad = pads[port_no];

	if (pad->is_fake_pad || !config.is_reportedly_connected(port_no) || !pad->is_connected())
		return not_an_error(CELL_PAD_ERROR_NO_DEVICE);

	return not_an_error((pad->m_device_capability & CELL_PAD_CAPABILITY_PRESS_MODE) ? 1 : 0);
}

error_code cellPadInfoSensorMode(u32 port_no)
{
	cellPad.trace("cellPadInfoSensorMode(port_no=%d)", port_no);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (port_no >= CELL_MAX_PADS)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	if (port_no >= config.get_max_connect())
		return CELL_PAD_ERROR_NO_DEVICE;

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();
	const auto& pad = pads[port_no];

	if (pad->is_fake_pad || !config.is_reportedly_connected(port_no) || !pad->is_connected())
		return not_an_error(CELL_PAD_ERROR_NO_DEVICE);

	return not_an_error((pad->m_device_capability & CELL_PAD_CAPABILITY_SENSOR_MODE) ? 1 : 0);
}

error_code cellPadSetPressMode(u32 port_no, u32 mode)
{
	cellPad.trace("cellPadSetPressMode(port_no=%d, mode=%d)", port_no, mode);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (port_no >= CELL_PAD_MAX_PORT_NUM)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	// CELL_PAD_ERROR_NO_DEVICE is not returned in this case.
	if (port_no >= CELL_PAD_MAX_PORT_NUM)
		return CELL_OK;

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();
	const auto& pad = pads[port_no];

	// TODO: find out if this is checked here or later or at all
	if (!(pad->m_device_capability & CELL_PAD_CAPABILITY_PRESS_MODE))
		return CELL_PAD_ERROR_UNSUPPORTED_GAMEPAD;

	if (mode)
		config.port_setting[port_no] |= CELL_PAD_SETTING_PRESS_ON;
	else
		config.port_setting[port_no] &= ~CELL_PAD_SETTING_PRESS_ON;

	return CELL_OK;
}

error_code cellPadSetSensorMode(u32 port_no, u32 mode)
{
	cellPad.trace("cellPadSetSensorMode(port_no=%d, mode=%d)", port_no, mode);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	if (port_no >= CELL_MAX_PADS)
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	// CELL_PAD_ERROR_NO_DEVICE is not returned in this case.
	if (port_no >= CELL_PAD_MAX_PORT_NUM)
		return CELL_OK;

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();
	const auto& pad = pads[port_no];

	// TODO: find out if this is checked here or later or at all
	if (!(pad->m_device_capability & CELL_PAD_CAPABILITY_SENSOR_MODE))
		return CELL_PAD_ERROR_UNSUPPORTED_GAMEPAD;

	if (mode)
		config.port_setting[port_no] |= CELL_PAD_SETTING_SENSOR_ON;
	else
		config.port_setting[port_no] &= ~CELL_PAD_SETTING_SENSOR_ON;

	return CELL_OK;
}

error_code cellPadLddRegisterController()
{
	cellPad.warning("cellPadLddRegisterController()");

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_pad_thread();

	const s32 handle = handler->AddLddPad();

	if (handle < 0)
		return CELL_PAD_ERROR_TOO_MANY_DEVICES;

	config.port_setting[handle] = 0;

	cellPad_NotifyStateChange(handle, CELL_PAD_STATUS_CONNECTED, false);

	return not_an_error(handle);
}

error_code cellPadLddDataInsert(s32 handle, vm::ptr<CellPadData> data)
{
	cellPad.trace("cellPadLddDataInsert(handle=%d, data=*0x%x)", handle, data);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_pad_thread();
	auto& pads = handler->GetPads();

	if (handle < 0 || static_cast<u32>(handle) >= pads.size() || !data) // data == NULL stalls on decr
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	if (!pads[handle]->ldd)
		return CELL_PAD_ERROR_NO_DEVICE;

	pads[handle]->ldd_data = *data;

	return CELL_OK;
}

error_code cellPadLddGetPortNo(s32 handle)
{
	cellPad.trace("cellPadLddGetPortNo(handle=%d)", handle);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();

	if (handle < 0 || static_cast<u32>(handle) >= pads.size())
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	if (!pads[handle]->ldd)
		return CELL_PAD_ERROR_FATAL; // might be incorrect

	// Other possible return values: CELL_PAD_ERROR_EBUSY, CELL_EBUSY
	return not_an_error(handle); // handle is port
}

error_code cellPadLddUnregisterController(s32 handle)
{
	cellPad.warning("cellPadLddUnregisterController(handle=%d)", handle);

	std::lock_guard lock(pad::g_pad_mutex);

	auto& config = g_fxo->get<pad_info>();

	if (!config.max_connect)
		return CELL_PAD_ERROR_UNINITIALIZED;

	const auto handler = pad::get_pad_thread();
	const auto& pads = handler->GetPads();

	if (handle < 0 || static_cast<u32>(handle) >= pads.size())
		return CELL_PAD_ERROR_INVALID_PARAMETER;

	if (!pads[handle]->ldd)
		return CELL_PAD_ERROR_NO_DEVICE;

	handler->UnregisterLddPad(handle);
	cellPad_NotifyStateChange(handle, CELL_PAD_STATUS_DISCONNECTED, false);

	return CELL_OK;
}

error_code cellPadFilterIIRInit(vm::ptr<CellPadFilterIIRSos> pSos, s32 cutoff)
{
	cellPad.todo("cellPadFilterIIRInit(pSos=*0x%x, cutoff=%d)", pSos, cutoff);

	if (!pSos) // TODO: does this check for cutoff > 2 ?
	{
		return CELL_PADFILTER_ERROR_INVALID_PARAMETER;
	}

	return CELL_OK;
}

u32 cellPadFilterIIRFilter(vm::ptr<CellPadFilterIIRSos> pSos, u32 filterIn)
{
	cellPad.todo("cellPadFilterIIRFilter(pSos=*0x%x, filterIn=%d)", pSos, filterIn);

	// TODO: apply filter

	return std::clamp(filterIn, 0u, 1023u);
}

s32 sys_io_3733EA3C(u32 port_no, vm::ptr<u32> device_type, vm::ptr<CellPadData> data)
{
	// Used by the ps1 emulator built into the firmware
	// Seems to call the same function that getdataextra does
	cellPad.trace("sys_io_3733EA3C(port_no=%d, device_type=*0x%x, data=*0x%x)", port_no, device_type, data);
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
