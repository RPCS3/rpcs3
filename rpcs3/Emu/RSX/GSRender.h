#pragma once

#include "GSFrameBase.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/system_config_types.h"

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

class GSRender : public rsx::thread, public rsx::render_backend
{
protected:
	// Renderer side of presentation. The window itself is the thread's (rsx::display_output); m_frame does not own it.
	GSFrameBase* m_frame = nullptr;
	draw_context_t m_context = nullptr;
	vsync_mode m_vsync_mode{};

public:
	~GSRender() override;

	GSRender(utils::serial* ar) noexcept;

	void on_init() override;
	void on_shutdown() override;

	bool present_frame(const rsx::display_flip_info_t& info) override;
	f64 get_display_refresh_rate() const override;
};
