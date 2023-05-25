#include "stdafx.h"
#include "localized_emu.h"

QString localized_emu::translated_pad_button(pad_button btn)
{
	switch (btn)
	{
	case pad_button::dpad_up: return tr("D-Pad Up");
	case pad_button::dpad_down: return tr("D-Pad Down");
	case pad_button::dpad_left: return tr("D-Pad Left");
	case pad_button::dpad_right: return tr("D-Pad Right");
	case pad_button::select: return tr("Select");
	case pad_button::start: return tr("Start");
	case pad_button::ps: return tr("PS");
	case pad_button::triangle: return tr("Triangle");
	case pad_button::circle: return tr("Circle");
	case pad_button::square: return tr("Square");
	case pad_button::cross: return tr("Cross");
	case pad_button::L1: return tr("L1");
	case pad_button::R1: return tr("R1");
	case pad_button::L2: return tr("L2");
	case pad_button::R2: return tr("R2");
	case pad_button::L3: return tr("L3");
	case pad_button::R3: return tr("R3");
	case pad_button::ls_up: return tr("Left Stick Up");
	case pad_button::ls_down: return tr("Left Stick Down");
	case pad_button::ls_left: return tr("Left Stick Left");
	case pad_button::ls_right: return tr("Left Stick Right");
	case pad_button::ls_x: return tr("Left Stick X-Axis");
	case pad_button::ls_y: return tr("Left Stick Y-Axis");
	case pad_button::rs_up: return tr("Right Stick Up");
	case pad_button::rs_down: return tr("Right Stick Down");
	case pad_button::rs_left: return tr("Right Stick Left");
	case pad_button::rs_right: return tr("Right Stick Right");
	case pad_button::rs_x: return tr("Right Stick X-Axis");
	case pad_button::rs_y: return tr("Right Stick Y-Axis");
	case pad_button::pad_button_max_enum: return "";
	}
	return "";
}
