#pragma once

#include "util/types.hpp"

static constexpr usz MAX_WIIMOTES = 4;
static constexpr usz MAX_WIIMOTE_IR_POINTS = 4;

struct wiimote_ir_point
{
	u16 x = 1023;
	u16 y = 1023;
	u8 size = 0;
};

enum class wiimote_button : u16
{
	None = 0,
	Left = 0x0001,
	Right = 0x0002,
	Down = 0x0004,
	Up = 0x0008,
	Plus = 0x0010,
	Two = 0x0100,
	One = 0x0200,
	B = 0x0400,
	A = 0x0800,
	Minus = 0x1000,
	Home = 0x8000
};
