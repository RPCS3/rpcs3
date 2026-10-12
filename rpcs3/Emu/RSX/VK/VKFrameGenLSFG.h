#pragma once

// PS5: Lossless Scaling's frame generation (LSFG), in place of VKFrameGen.h's
// when the Frame Generation Method setting asks for it. The compute chain is
// Eden's port of lsfg-vk, as the Simpsons Game's PS5 port runs it on this
// console (lsfg/, GPL-3.0-or-later, lsfg/README.md); its shaders are the
// SPIR-V in the player's own Lossless.dll (Lossless Scaling, on Steam), read,
// never run, from lsfg_dll_path(). Without the file, or on any failure, the
// built-in frame generation carries on for the session.

#include "util/types.hpp"

#include <memory>
#include <string>

namespace vk
{
	class command_buffer;
	class viewable_image;

	// What vkutils/device.cpp created the device with, of what LSFG needs
	struct ps5_lsfg_device_features
	{
		u32 api_version = 0;
		bool memory_model = false;
		bool memory_model_device_scope = false;
		bool null_descriptor = false;
		bool float16 = false;
		bool storage_extended_formats = false;
		bool storage_read_without_format = false;
		bool storage_write_without_format = false;
	};

	extern ps5_lsfg_device_features g_ps5_lsfg_device_features;

	class lsfg_frame_generator
	{
	public:
		static constexpr u32 max_frames = 3;

		lsfg_frame_generator();
		~lsfg_frame_generator();

		// As frame_generator::process: records the work for the game's new
		// frame `cur` (its first width x height texels), `count` frames
		// between it and the last one, evenly spaced in time, into `out` in the
		// order they are shown, and in any case what the next frame needs of
		// this one. Returns how many it made: none until three frames have
		// gone through the chain after a start, a reset or a new size
		u32 process(const vk::command_buffer& cmd, vk::viewable_image* cur, u32 width, u32 height, u32 count,
			vk::viewable_image* (&out)[max_frames]);

		// Forgets the frames before: the next one starts the chain again
		void reset();

		// It could not run (no Lossless.dll, a GPU without what it needs, an
		// error): the built-in frame generation for the rest of the session
		bool failed() const;

	private:
		struct impl;
		std::unique_ptr<impl> m_impl;
	};

	// Where Lossless.dll is looked for, the first that is there
	// (rpcs3/lossless/Lossless.dll, then rpcs3/Lossless.dll); empty if neither
	std::string lsfg_dll_path();

	// What became of LSFG this session, for the trace and the pause menu
	std::string lsfg_status();
}
