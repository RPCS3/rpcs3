#include "stdafx.h"
#include "pad_types.h"

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
		case pad_button::rs_up: return "Right Stick Up";
		case pad_button::rs_down: return "Right Stick Down";
		case pad_button::rs_left: return "Right Stick Left";
		case pad_button::rs_right: return "Right Stick Right";
		case pad_button::pad_button_max_enum: return unknown;
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
	case pad_button::rs_up: return CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y;
	case pad_button::rs_down: return CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y;
	case pad_button::rs_left: return CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X;
	case pad_button::rs_right: return CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X;
	case pad_button::pad_button_max_enum: return 0;
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
	case pad_button::ls_up: return 0;
	case pad_button::ls_down: return 0;
	case pad_button::ls_left: return 0;
	case pad_button::ls_right: return 0;
	case pad_button::rs_up: return 0;
	case pad_button::rs_down: return 0;
	case pad_button::rs_left: return 0;
	case pad_button::rs_right: return 0;
	case pad_button::pad_button_max_enum: return 0;
	}
	return 0;
}
