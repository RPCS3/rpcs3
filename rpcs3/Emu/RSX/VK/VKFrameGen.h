#pragma once

// PS5: frame generation. Before each of the game's frames the screen shows
// up to three made between it and the frame before, one a refresh, as many as
// the time since the game's last frame has refreshes for (VKPresent.cpp): a
// game at 24 fps reaches the screen at up to 60, each of its own frames as
// many refreshes later as there are frames made before it.
//
// Motion is estimated from the pictures alone (a PS3 game gives no motion
// vectors): both frames' brightness at a working size (480, 640 or 960
// across, by the Quality setting), blocks matched at a quarter of that and
// refined at full working size, the vectors' outliers removed by a median;
// then each pixel of a frame between, at a time t from the last frame (0) to
// this one (1), is taken that far along a vector from both frames and blended
// by t: of the nearest blocks' vectors and their blend, the one whose two ends
// agree best. Where a block matched badly, or the two frames disagree
// there, it takes the frame nearer in time, so a wrong vector costs
// smoothness rather than smearing the picture.

#include "VKCompute.h"
#include "vkutils/image.h"
#include "vkutils/sampler.h"

#include <memory>

namespace vk
{
	class frame_generator
	{
	public:
		// The most frames made between two of the game's
		static constexpr u32 max_frames = 3;

		~frame_generator();

		// Records the work for the game's new frame `cur` (its first width x
		// height texels): `count` frames between it and the last one, evenly
		// spaced in time, into `out` in the order they are to be shown; and in
		// any case what the next frame needs of this one. `quality` is the
		// setting's (0 Fast, 1 Balanced, 2 Quality). Returns how many frames
		// it made: none for the first frame, a new size or quality, or none
		// asked for
		u32 process(const vk::command_buffer& cmd, vk::viewable_image* cur, u32 width, u32 height, u32 count, u32 quality,
			vk::viewable_image* (&out)[max_frames]);

		// Forgets the last frame: the next one is a first one
		void reset();

	private:
		void allocate(u32 width, u32 height, u32 quality);
		void dispose();

		u32 m_width = 0;
		u32 m_height = 0;
		u32 m_quality = 0;
		u32 m_work_w = 0;   // the brightness's working size
		u32 m_work_h = 0;
		bool m_have_prev = false;

		std::unique_ptr<vk::viewable_image> m_prev_color;        // the last frame, at its own size
		std::unique_ptr<vk::viewable_image> m_luma[2];           // brightness at the working size: [0] the last frame's, [1] this one's
		std::unique_ptr<vk::viewable_image> m_luma_coarse[2];    // and at a quarter of it
		std::unique_ptr<vk::viewable_image> m_vectors_coarse;    // a vector per 4x4 block at the coarse size
		std::unique_ptr<vk::viewable_image> m_vectors;           // a vector per 8x8 block at the working size
		std::unique_ptr<vk::viewable_image> m_vectors_smooth;    // the same, outliers removed
		std::unique_ptr<vk::viewable_image> m_output[max_frames]; // the frames between
	};
}
