#pragma once
#include <string>
#include <array>
#include <vector>
#include "Utilities/types.h"
#include "Emu/RSX/rsx_methods.h"

namespace rsx
{
struct frame_trace_data
{
	struct draw_state
	{
		std::string name;
		std::pair<std::string, std::string> programs;
		rsx::rsx_state state;
		std::array<std::vector<gsl::byte>, 4> color_buffer;
		std::array<std::vector<gsl::byte>, 2> depth_stencil;
		std::vector<gsl::byte> index;
		u32 vertex_count;

	};

	std::vector<std::pair<u32, u32>> command_queue;
	std::vector<draw_state> draw_calls;

	void reset()
	{
		command_queue.clear();
		draw_calls.clear();
	}
};
}
