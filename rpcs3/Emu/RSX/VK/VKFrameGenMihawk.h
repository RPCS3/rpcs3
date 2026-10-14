#pragma once

// PS5: MihawkGen's frame generation (mihawkgen/, GPL-3.0-or-later,
// mihawkgen/README.md), in place of VKFrameGen.h's when the Frame Generation
// Method setting asks for it. A trained network of 432,308 parameters as
// compute shaders: the game's last frame and this one in, a frame at any time
// between them out, from the pictures alone. Its weights are compiled in
// (mihawkgen/network.fgw). On any failure the built-in frame generation
// carries on for the session.

#include "util/types.hpp"

#include <memory>
#include <string>

namespace vk
{
	class command_buffer;
	class viewable_image;

	class mihawk_frame_generator
	{
	public:
		static constexpr u32 max_frames = 3;

		mihawk_frame_generator();
		~mihawk_frame_generator();

		// As frame_generator::process: records the work for the game's new
		// frame `cur` (its first width x height texels), `count` frames
		// between it and the last one, evenly spaced in time, into `out` in the
		// order they are shown, and in any case keeps this frame for the next.
		// Returns how many it made: none for the first frame after a start, a
		// reset or a new size
		u32 process(const vk::command_buffer& cmd, vk::viewable_image* cur, u32 width, u32 height, u32 count,
			vk::viewable_image* (&out)[max_frames]);

		// Forgets the last frame: the next one is a first one
		void reset();

		// It could not run: the built-in frame generation for the rest of the session
		bool failed() const;

	private:
		struct impl;
		std::unique_ptr<impl> m_impl;
	};

	// What became of MihawkGen this session, for the trace and the pause menu
	std::string mihawk_status();
}
