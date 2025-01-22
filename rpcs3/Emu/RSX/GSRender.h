#pragma once

#include "GSFrameBase.h"
#include "Emu/RSX/RSXThread.h"

enum wm_event
{
	none,                        // nothing
	toggle_fullscreen,           // user is requesting a fullscreen switch
	geometry_change_notice,      // about to start resizing and/or moving the window
	geometry_change_in_progress, // window being resized and/or moved
	window_resized,              // window was resized
	window_minimized,            // window was minimized
	window_restored,             // window was restored from a minimized state
	window_moved,                // window moved without resize
	window_visibility_changed
};

class GSRender : public rsx::thread
{
protected:
	GSFrameBase* m_frame;
	draw_context_t m_context = nullptr;
	bool m_continuous_mode = false;

public:
	~GSRender() override;

	GSRender(utils::serial* ar) noexcept;

	void set_continuous_mode(bool continuous_mode) { m_continuous_mode = continuous_mode; }

	void on_init_thread() override;
	void on_exit() override;

	void flip(const rsx::display_flip_info_t& info) override;
	f64 get_display_refresh_rate() const override;

	GSFrameBase* get_frame() const { return m_frame; }
};
