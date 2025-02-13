#pragma once

#include "Emu/RSX/Overlays/overlay_controls.h"

namespace rsx
{
	namespace overlays
	{
		class animated_icon : public image_view
		{
		public:
			animated_icon(const char* icon_name);
			animated_icon(const std::vector<u8>& icon_data);

			void update_animation_frame(compiled_resource& result);
			compiled_resource& get_compiled() override;

		protected:
			// Some layout and frame data
			u16 m_start_x = 0; // X and Y offset of frame 0
			u16 m_start_y = 0;
			u16 m_frame_width = 32;  // W and H of each animation frame
			u16 m_frame_height = 32;
			u16 m_spacing_x = 8;     // Spacing between frames in X and Y
			u16 m_spacing_y = 8;
			u16 m_row_length = 12;   // Number of animation frames in the X direction in case of a 2D grid of frames
			u16 m_total_frames = 12; // Total number of available frames
			u64 m_frame_duration_us = 100'000; // Hold duration for each frame

			// Animation playback variables
			int m_current_frame = 0;
			u64 m_current_frame_duration_us = 0;
			u64 m_last_update_timestamp_us = 0;

		private:
			std::unique_ptr<image_info> m_icon;
		};
	}
}

