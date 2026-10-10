#pragma once

// PS5: frame generation. Before each of the game's frames the screen shows
// up to three made between it and the frame before, one a refresh, as many as
// the time since the game's last frame has refreshes for (VKPresent.cpp): a
// game at 24 fps reaches the screen at up to 60, each of its own frames as
// many refreshes later as there are frames made before it.
//
// Motion is estimated from the pictures alone (a PS3 game gives no motion
// vectors): both frames' brightness at a working size (480, 640 or 960
// across, by the Quality setting), blocks matched at a quarter of that (up to
// about a fifth of the frame away) and
// refined at full working size, the vectors' outliers removed by a median;
// then each pixel of a frame between, at a time t from the last frame (0) to
// this one (1), is taken that far along a vector from both frames and blended
// by t: of the nearest blocks' vectors and their blend, the one whose two ends
// agree best. Where a block matched badly, or the two frames disagree
// there, it takes the frame nearer in time, so a wrong vector costs
// smoothness rather than smearing the picture. The camera's motion is also
// fitted as one smooth field over the frame (pass_camera), and offered as a
// favoured candidate beside the blocks that moved with it: on rows of windows
// the blocks' own vectors are often a window off. And while the camera moves
// too quickly (by the Step aside setting: more than 6% or 3% of the frame's
// width between two game frames, voted from blocks matched at a sixteenth of
// the working size), every frame between is the nearer real frame: such
// motion broke apart.

#include "VKCompute.h"
#include "vkutils/buffer_object.h"
#include "vkutils/image.h"
#include "vkutils/sampler.h"
#include "vkutils/sync.h"

#include <memory>
#include <string>

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
		// setting's (0 Fast, 1 Balanced, 2 Quality), `step` the Step aside
		// setting's (0 never, 1 quick moves, 2 most moves). Returns how many
		// frames it made: none for the first frame, a new size or quality, or
		// none asked for
		u32 process(const vk::command_buffer& cmd, vk::viewable_image* cur, u32 width, u32 height, u32 count, u32 quality, u32 step,
			vk::viewable_image* (&out)[max_frames]);

		// Forgets the last frame: the next one is a first one
		void reset();

	private:
		void allocate(u32 width, u32 height, u32 quality);
		void dispose();

		// A capture, for a report (the pause menu's Save frame generation's
		// frames: g_ps5_frame_gen_capture, the moments wanted): the last frame,
		// this one and the frames between, copied to host memory with the
		// frame's work and, once the GPU has done them, written on a thread as
		// one picture a moment to rpcs3/captures/: a strip saying what it is,
		// then the frames top to bottom in time order (a chat takes a few
		// pictures at a time, not a folder of files)
		void record_capture(const vk::command_buffer& cmd, vk::viewable_image* cur, u32 width, u32 height, u32 count, u32 quality, u32 step);
		void take_capture();
		u32 m_capture_sets = 0;   // still to take
		u32 m_capture_set = 0;    // the next one's number
		u32 m_capture_run = 0;    // the request's time, in the folders' names
		u64 m_capture_next_us = 0;
		std::unique_ptr<vk::viewable_image> m_capture_cur; // this frame, as RGBA8
		std::unique_ptr<vk::buffer> m_capture_buffer;
		std::unique_ptr<vk::event> m_capture_event;        // the copies done
		u32 m_capture_w = 0, m_capture_h = 0, m_capture_count = 0, m_capture_grid_w = 0, m_capture_grid_h = 0;
		std::string m_capture_info;

		u32 m_width = 0;
		u32 m_height = 0;
		u32 m_quality = 0;
		u32 m_work_w = 0;   // the brightness's working size
		u32 m_work_h = 0;
		bool m_have_prev = false;

		std::unique_ptr<vk::viewable_image> m_prev_color;        // the last frame, at its own size
		std::unique_ptr<vk::viewable_image> m_prev_hud;          // the same, its alpha where the HUD stands still (pass_hud)
		std::unique_ptr<vk::viewable_image> m_blur[2];           // the last frame and this one at half size, softened (pass_blur)
		std::unique_ptr<vk::viewable_image> m_luma[2];           // brightness at the working size: [0] the last frame's, [1] this one's
		std::unique_ptr<vk::viewable_image> m_luma_coarse[2];    // and at a quarter of it
		std::unique_ptr<vk::viewable_image> m_luma_rough[2];     // and at a sixteenth
		std::unique_ptr<vk::viewable_image> m_vectors_rough;     // a vector per 4x4 block at the sixteenth size
		std::unique_ptr<vk::viewable_image> m_motion;            // the camera's motion: one texel
		std::unique_ptr<vk::viewable_image> m_camera;            // the camera's field: three texels
		std::unique_ptr<vk::viewable_image> m_camera_moved;      // each block: whether it moved with the field
		std::unique_ptr<vk::buffer> m_motion_read;               // a copy of it the CPU reads, for the trace
		const f32* m_motion_mapped = nullptr;
		std::unique_ptr<vk::viewable_image> m_vectors_coarse;    // a vector per 4x4 block at the coarse size
		std::unique_ptr<vk::viewable_image> m_vectors;           // a vector per 8x8 block at the working size
		std::unique_ptr<vk::viewable_image> m_vectors_smooth;    // the same, outliers removed
		std::unique_ptr<vk::viewable_image> m_vectors_refined[2]; // the rounds of agreeing with the neighbours, in turn
		std::unique_ptr<vk::viewable_image> m_output[max_frames]; // the frames between
	};
}
