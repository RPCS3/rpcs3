#include "stdafx.h"
#include "pad_types.h"
#include "Emu/system_config.h"
#include "Emu/RSX/Overlays/overlay_message.h"

template <>
void fmt_class_string<pad_button>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](pad_button value)
	{
		switch (value)
		{
		case pad_button::dpad_up: return "D-Pad Up";
		case pad_button::dpad_down: return "D-Pad Down";
		case pad_button::dpad_left: return "D-Pad Left";
		case pad_button::dpad_right: return "D-Pad Right";
		case pad_button::select: return "Select";
		case pad_button::start: return "Start";
		case pad_button::ps: return "PS";
		case pad_button::triangle: return "Triangle";
		case pad_button::circle: return "Circle";
		case pad_button::square: return "Square";
		case pad_button::cross: return "Cross";
		case pad_button::L1: return "L1";
		case pad_button::R1: return "R1";
		case pad_button::L2: return "L2";
		case pad_button::R2: return "R2";
		case pad_button::L3: return "L3";
		case pad_button::R3: return "R3";
		case pad_button::ls_up: return "Left Stick Up";
		case pad_button::ls_down: return "Left Stick Down";
		case pad_button::ls_left: return "Left Stick Left";
		case pad_button::ls_right: return "Left Stick Right";
		case pad_button::ls_x: return "Left Stick X-Axis";
		case pad_button::ls_y: return "Left Stick Y-Axis";
		case pad_button::rs_up: return "Right Stick Up";
		case pad_button::rs_down: return "Right Stick Down";
		case pad_button::rs_left: return "Right Stick Left";
		case pad_button::rs_right: return "Right Stick Right";
		case pad_button::rs_x: return "Right Stick X-Axis";
		case pad_button::rs_y: return "Right Stick Y-Axis";
		case pad_button::pad_button_max_enum: return "";
		case pad_button::mouse_button_1: return "Mouse Button 1";
		case pad_button::mouse_button_2: return "Mouse Button 2";
		case pad_button::mouse_button_3: return "Mouse Button 3";
		case pad_button::mouse_button_4: return "Mouse Button 4";
		case pad_button::mouse_button_5: return "Mouse Button 5";
		case pad_button::mouse_button_6: return "Mouse Button 6";
		case pad_button::mouse_button_7: return "Mouse Button 7";
		case pad_button::mouse_button_8: return "Mouse Button 8";
		}

		return unknown;
	});
}

u32 pad_button_offset(pad_button button)
{
	switch (button)
	{
	case pad_button::dpad_up: return CELL_PAD_BTN_OFFSET_DIGITAL1;
	case pad_button::dpad_down: return CELL_PAD_BTN_OFFSET_DIGITAL1;
	case pad_button::dpad_left: return CELL_PAD_BTN_OFFSET_DIGITAL1;
	case pad_button::dpad_right: return CELL_PAD_BTN_OFFSET_DIGITAL1;
	case pad_button::select: return CELL_PAD_BTN_OFFSET_DIGITAL1;
	case pad_button::start: return CELL_PAD_BTN_OFFSET_DIGITAL1;
	case pad_button::ps: return CELL_PAD_BTN_OFFSET_DIGITAL1;
	case pad_button::triangle: return CELL_PAD_BTN_OFFSET_DIGITAL2;
	case pad_button::circle: return CELL_PAD_BTN_OFFSET_DIGITAL2;
	case pad_button::square: return CELL_PAD_BTN_OFFSET_DIGITAL2;
	case pad_button::cross: return CELL_PAD_BTN_OFFSET_DIGITAL2;
	case pad_button::L1: return CELL_PAD_BTN_OFFSET_DIGITAL2;
	case pad_button::R1: return CELL_PAD_BTN_OFFSET_DIGITAL2;
	case pad_button::L2: return CELL_PAD_BTN_OFFSET_DIGITAL2;
	case pad_button::R2: return CELL_PAD_BTN_OFFSET_DIGITAL2;
	case pad_button::L3: return CELL_PAD_BTN_OFFSET_DIGITAL1;
	case pad_button::R3: return CELL_PAD_BTN_OFFSET_DIGITAL1;
	case pad_button::ls_up: return CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y;
	case pad_button::ls_down: return CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y;
	case pad_button::ls_left: return CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X;
	case pad_button::ls_right: return CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X;
	case pad_button::ls_x: return CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X;
	case pad_button::ls_y: return CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y;
	case pad_button::rs_up: return CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y;
	case pad_button::rs_down: return CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y;
	case pad_button::rs_left: return CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X;
	case pad_button::rs_right: return CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X;
	case pad_button::rs_x: return CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X;
	case pad_button::rs_y: return CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y;
	case pad_button::pad_button_max_enum:
	case pad_button::mouse_button_1:
	case pad_button::mouse_button_2:
	case pad_button::mouse_button_3:
	case pad_button::mouse_button_4:
	case pad_button::mouse_button_5:
	case pad_button::mouse_button_6:
	case pad_button::mouse_button_7:
	case pad_button::mouse_button_8:
		return 0;
	}
	return 0;
}

u32 pad_button_keycode(pad_button button)
{
	switch (button)
	{
	case pad_button::dpad_up: return CELL_PAD_CTRL_UP;
	case pad_button::dpad_down: return CELL_PAD_CTRL_DOWN;
	case pad_button::dpad_left: return CELL_PAD_CTRL_LEFT;
	case pad_button::dpad_right: return CELL_PAD_CTRL_RIGHT;
	case pad_button::select: return CELL_PAD_CTRL_SELECT;
	case pad_button::start: return CELL_PAD_CTRL_START;
	case pad_button::ps: return CELL_PAD_CTRL_PS;
	case pad_button::triangle: return CELL_PAD_CTRL_TRIANGLE;
	case pad_button::circle: return CELL_PAD_CTRL_CIRCLE;
	case pad_button::square: return CELL_PAD_CTRL_SQUARE;
	case pad_button::cross: return CELL_PAD_CTRL_CROSS;
	case pad_button::L1: return CELL_PAD_CTRL_L1;
	case pad_button::R1: return CELL_PAD_CTRL_R1;
	case pad_button::L2: return CELL_PAD_CTRL_L2;
	case pad_button::R2: return CELL_PAD_CTRL_R2;
	case pad_button::L3: return CELL_PAD_CTRL_L3;
	case pad_button::R3: return CELL_PAD_CTRL_R3;
	case pad_button::ls_up: return static_cast<u32>(axis_direction::positive);
	case pad_button::ls_down: return static_cast<u32>(axis_direction::negative);
	case pad_button::ls_left: return static_cast<u32>(axis_direction::negative);
	case pad_button::ls_right: return static_cast<u32>(axis_direction::positive);
	case pad_button::ls_x: return static_cast<u32>(axis_direction::both);
	case pad_button::ls_y: return static_cast<u32>(axis_direction::both);
	case pad_button::rs_up: return static_cast<u32>(axis_direction::positive);
	case pad_button::rs_down: return static_cast<u32>(axis_direction::negative);
	case pad_button::rs_left: return static_cast<u32>(axis_direction::negative);
	case pad_button::rs_right: return static_cast<u32>(axis_direction::positive);
	case pad_button::rs_x: return static_cast<u32>(axis_direction::both);
	case pad_button::rs_y: return static_cast<u32>(axis_direction::both);
	case pad_button::pad_button_max_enum: return 0;
	case pad_button::mouse_button_1: return 1;
	case pad_button::mouse_button_2: return 2;
	case pad_button::mouse_button_3: return 3;
	case pad_button::mouse_button_4: return 4;
	case pad_button::mouse_button_5: return 5;
	case pad_button::mouse_button_6: return 6;
	case pad_button::mouse_button_7: return 7;
	case pad_button::mouse_button_8: return 8;
	}
	return 0;
}

u32 get_axis_keycode(u32 offset, u16 value)
{
	switch (offset)
	{
	case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X:  return static_cast<u32>(value > 127 ? axis_direction::positive : axis_direction::negative);
	case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y:  return static_cast<u32>(value < 128 ? axis_direction::positive : axis_direction::negative);
	case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X: return static_cast<u32>(value > 127 ? axis_direction::positive : axis_direction::negative);
	case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y: return static_cast<u32>(value < 128 ? axis_direction::positive : axis_direction::negative);
	default: return static_cast<u32>(axis_direction::both);
	}
}

void ps_move_data::reset_sensors()
{
	quaternion = default_quaternion;
	accelerometer_x = 0.0f;
	accelerometer_y = 0.0f;
	accelerometer_z = 0.0f;
	gyro_x = 0.0f;
	gyro_y = 0.0f;
	gyro_z = 0.0f;
	magnetometer_x = 0.0f;
	magnetometer_y = 0.0f;
	magnetometer_z = 0.0f;
}

bool Pad::get_pressure_intensity_button_active(bool is_toggle_mode, u32 player_id)
{
	if (m_pressure_intensity_button_index < 0)
	{
		return false;
	}

	const Button& pressure_intensity_button = m_buttons[m_pressure_intensity_button_index];

	if (is_toggle_mode)
	{
		const bool pressed = pressure_intensity_button.m_pressed;

		if (std::exchange(m_pressure_intensity_button_pressed, pressed) != pressed)
		{
			if (pressed)
			{
				m_pressure_intensity_toggled = !m_pressure_intensity_toggled;

				if (g_cfg.misc.show_pressure_intensity_toggle_hint)
				{
					const std::string player_id_string = std::to_string(player_id + 1);
					if (m_pressure_intensity_toggled)
					{
						rsx::overlays::queue_message(get_localized_string(localized_string_id::RSX_OVERLAYS_PRESSURE_INTENSITY_TOGGLED_ON, player_id_string.c_str()), 3'000'000);
					}
					else
					{
						rsx::overlays::queue_message(get_localized_string(localized_string_id::RSX_OVERLAYS_PRESSURE_INTENSITY_TOGGLED_OFF, player_id_string.c_str()), 3'000'000);
					}
				}
			}
		}

		return m_pressure_intensity_toggled;
	}

	return pressure_intensity_button.m_pressed;
}

bool Pad::get_analog_limiter_button_active(bool is_toggle_mode, u32 player_id)
{
	if (m_analog_limiter_button_index < 0)
	{
		return false;
	}

	const Button& analog_limiter_button = m_buttons[m_analog_limiter_button_index];

	if (is_toggle_mode)
	{
		const bool pressed = analog_limiter_button.m_pressed;

		if (std::exchange(m_analog_limiter_button_pressed, pressed) != pressed)
		{
			if (pressed)
			{
				m_analog_limiter_toggled = !m_analog_limiter_toggled;

				if (g_cfg.misc.show_analog_limiter_toggle_hint)
				{
					const std::string player_id_string = std::to_string(player_id + 1);
					if (m_analog_limiter_toggled)
					{
						rsx::overlays::queue_message(get_localized_string(localized_string_id::RSX_OVERLAYS_ANALOG_LIMITER_TOGGLED_ON, player_id_string.c_str()), 3'000'000);
					}
					else
					{
						rsx::overlays::queue_message(get_localized_string(localized_string_id::RSX_OVERLAYS_ANALOG_LIMITER_TOGGLED_OFF, player_id_string.c_str()), 3'000'000);
					}
				}
			}
		}

		return m_analog_limiter_toggled;
	}

	return analog_limiter_button.m_pressed;
}

bool Pad::get_orientation_reset_button_active()
{
	if (m_orientation_reset_button_index < 0)
	{
		return false;
	}

	return m_buttons[m_orientation_reset_button_index].m_pressed;
}
