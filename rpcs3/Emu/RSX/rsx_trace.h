#pragma once
#include <string>
#include <array>
#include <vector>
#include "Utilities/types.h"
#include "rsx_methods.h"

#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>

namespace rsx
{
struct frame_capture_data
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

		template<typename Archive>
		void serialize(Archive & ar)
		{
			ar(name);
			ar(programs);
			ar(state);
			ar(color_buffer);
			ar(depth_stencil);
			ar(index);
		}

	};
	std::vector<std::pair<u32, u32> > command_queue;
	std::vector<draw_state> draw_calls;

	template<typename Archive>
	void serialize(Archive & ar)
	{
		ar(command_queue);
		ar(draw_calls);
	}

	void reset()
	{
		command_queue.clear();
		draw_calls.clear();
	}
};
}
