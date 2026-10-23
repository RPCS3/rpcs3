// SPDX-License-Identifier: MIT
// PS5_RPCS3: a build without Lossless Scaling's and MihawkGen's frame
// generation (PS5_GPL3_FRAME_GENERATION off, rpcs3/Emu/CMakeLists.txt): their
// code is GPL-3.0-or-later (lsfg/, mihawkgen/), and a title with it in cannot
// be shared. Their interfaces (VKFrameGenLSFG.h, VKFrameGenMihawk.h) are here,
// each failing at once, which leaves the built-in frame generation
// (VKFrameGen.h) as it does on any failure of theirs; VKPresent.cpp and the
// settings (ps5/ps5_ui.cpp) do not offer them.

#include "stdafx.h"
#include "VKFrameGenLSFG.h"
#include "VKFrameGenMihawk.h"

namespace vk
{
	ps5_lsfg_device_features g_ps5_lsfg_device_features;

	struct lsfg_frame_generator::impl
	{
	};

	lsfg_frame_generator::lsfg_frame_generator() = default;
	lsfg_frame_generator::~lsfg_frame_generator() = default;

	u32 lsfg_frame_generator::process(const vk::command_buffer&, vk::viewable_image*, u32, u32, u32, vk::viewable_image* (&)[max_frames])
	{
		return 0;
	}

	void lsfg_frame_generator::reset()
	{
	}

	bool lsfg_frame_generator::failed() const
	{
		return true;
	}

	std::string lsfg_dll_path()
	{
		return {};
	}

	std::string lsfg_status()
	{
		return "not in this build";
	}

	struct mihawk_frame_generator::impl
	{
	};

	mihawk_frame_generator::mihawk_frame_generator() = default;
	mihawk_frame_generator::~mihawk_frame_generator() = default;

	u32 mihawk_frame_generator::process(const vk::command_buffer&, vk::viewable_image*, u32, u32, u32, vk::viewable_image* (&)[max_frames],
		const frame_generation_motion*, bool)
	{
		return 0;
	}

	void mihawk_frame_generator::reset()
	{
	}

	bool mihawk_frame_generator::failed() const
	{
		return true;
	}

	std::string mihawk_status()
	{
		return "not in this build";
	}
}
