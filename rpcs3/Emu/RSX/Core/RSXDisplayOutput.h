#pragma once

#include <util/types.hpp>
#include "Emu/system_config_types.h"

class GSFrameBase;

namespace rsx
{
	// The host window the emulated display is presented to. Owned by the RSX thread; it outlives any renderer.
	// Draw contexts and swapchains are renderer objects created from the frame, not part of this.
	class display_output
	{
		GSFrameBase* m_frame = nullptr;
		bool m_continuous_mode = false;

	public:
		// Takes the frame from the emulator callbacks. There is no frame when the UI does not provide one.
		display_output();
		~display_output();

		display_output(const display_output&) = delete;
		display_output& operator=(const display_output&) = delete;

		GSFrameBase* frame() const { return m_frame; }

		// In continuous mode the window stays open when emulation stops.
		void set_continuous_mode(bool continuous_mode) { m_continuous_mode = continuous_mode; }

		// The RSX thread has finished. Hides the window unless it is kept for the next run.
		void on_exit();

		// Notification event when the renderer has changed. Ensures that the open window can support the requested renderer.
		void prepare_game_window(video_renderer type);

		// Refresh rate of the display the window is on.
		f64 refresh_rate() const;
	};
}
