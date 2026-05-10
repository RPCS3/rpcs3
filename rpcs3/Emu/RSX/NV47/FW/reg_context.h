#pragma once

#include <util/types.hpp>

namespace rsx
{
	// TODO: Basically replaces parts of the current "rsx_state" object
	struct reg_context
	{
		u32 registers[1];
	};
}
