#pragma once

#include <util/types.hpp>
#include "Emu/RSX/Common/bitfield.hpp"

namespace rsx
{
	// Decoded pipeline state shared by every backend.
	// Written only by the core renderer. Backends read it and translate it into host API state.
	struct draw_state_t
	{
		// Drawing state groups. A group's bit in 'dirty' is set when its contents changed since the backend last consumed them.
		enum group : u32
		{
			raster         = (1 << 0),
			depth_stencil  = (1 << 1),
			blend          = (1 << 2),
			multisample    = (1 << 3),
			viewport       = (1 << 4),
			input_assembly = (1 << 5),
			clear          = (1 << 6),
			program_env    = (1 << 7),

			all = raster | depth_stencil | blend | multisample | viewport | input_assembly | clear | program_env
		};

		bitmask_t<group> dirty;
	};
}
