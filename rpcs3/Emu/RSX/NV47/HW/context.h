#pragma once

#include <util/types.hpp>

#include <array>

namespace rsx
{
	struct context;

	// Static on-device state. Decoders belong to firmware context (see rsx_state).
	struct hw_context
	{
		std::array<u32, 0x10000 / 4> registers{};
		u32 latch{};

		std::array<u32, 544 * 4> transform_program{};
		std::array<u32[4], 512> transform_constants{};
	};
}
