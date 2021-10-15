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
	draw_context_t m_context;

public:
	GSRender();
	~GSRender() override;

	void on_init_thread() override;
	void on_exit() override;

	void flip(const rsx::display_flip_info_t& info) override;

	GSFrameBase* get_frame() const { return m_frame; }
};
