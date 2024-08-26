#pragma once

#include <util/types.hpp>

namespace rsx
{
	class thread;
	struct rsx_state;

#if 0
	// TODO: Separate GRAPH context from RSX state
	struct GRAPH_context
	{
		u32 id;
		std::array<u32, 0x10000 / 4> registers;

		GRAPH_context(u32 ctx_id)
			: id(ctx_id)
		{
			std::fill(registers.begin(), registers.end(), 0);
		}
	};
#endif

	struct context
	{
		thread* rsxthr;
		// GRAPH_context* graph;
		rsx_state* register_state;
	};
}
