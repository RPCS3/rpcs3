#pragma once
#include "rsx_replay.h"

namespace rsx
{
	class thread;
	namespace capture
	{
		void capture_draw_memory(thread* rsx);
		void capture_image_in(thread* rsx, frame_capture_data::replay_command& replay_command);
		void capture_buffer_notify(thread* rsx, frame_capture_data::replay_command& replay_command);
		void capture_display_tile_state(thread* rsx, frame_capture_data::replay_command& replay_command);
	}
}
