#pragma once

#include "Emu/RSX/RSXThread.h"
#include <memory>

#ifdef _WIN32
#include <windows.h>
#else
// Cannot include Xlib.h before Qt5
// and we don't need all of Xlib anyway
typedef struct _XDisplay Display;
typedef unsigned long Window;
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#include <wayland-client.h>
#endif

struct RSXDebuggerProgram
{
	u32 id;
	u32 vp_id;
	u32 fp_id;
	std::string vp_shader;
	std::string fp_shader;
	bool modified;

	RSXDebuggerProgram()
		: modified(false)
	{
	}
};

enum wm_event
{
	none, //nothing
	geometry_change_notice, //about to start resizing and/or moving the window
	geometry_change_in_progress, //window being resized and/or moved
	window_resized, //window was resized
	window_minimized, //window was minimized
	window_restored, //window was restored from a minimized state
	window_moved, //window moved without resize
	window_visibility_changed
};

using RSXDebuggerPrograms = std::vector<RSXDebuggerProgram>;

using draw_context_t = void*;

#ifdef _WIN32
	using display_handle_t = HWND;
#else
	using display_handle_t = std::variant<
		std::pair<Display*, Window>
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
		, std::pair<wl_display*, wl_surface*>
#endif
	>;
#endif

class GSFrameBase
{
public:
	GSFrameBase() = default;
	GSFrameBase(const GSFrameBase&) = delete;

	virtual void close() = 0;
	virtual bool shown() = 0;
	virtual void hide() = 0;
	virtual void show() = 0;

	virtual void delete_context(draw_context_t ctx) = 0;
	virtual draw_context_t make_context() = 0;
	virtual void set_current(draw_context_t ctx) = 0;
	virtual void flip(draw_context_t ctx, bool skip_frame=false) = 0;
	virtual int client_width() = 0;
	virtual int client_height() = 0;

	virtual display_handle_t handle() const = 0;

protected:

	//window manager event management
	wm_event m_raised_event;
	std::atomic_bool wm_event_raised = {};
	std::atomic_bool wm_event_queue_enabled = {};

public:
	//synchronize native window access
	std::mutex wm_event_lock;

	virtual wm_event get_default_wm_event() const = 0;

	void clear_wm_events()
	{
		m_raised_event = wm_event::none;
		wm_event_raised.store(false);
	}

	wm_event get_wm_event()
	{
		if (wm_event_raised.load(std::memory_order_consume))
		{
			auto result = m_raised_event;
			m_raised_event = wm_event::none;
			wm_event_raised.store(false);
			return result;
		}

		return get_default_wm_event();
	}

	void disable_wm_event_queue()
	{
		wm_event_queue_enabled.store(false);
	}

	void enable_wm_event_queue()
	{
		wm_event_queue_enabled.store(true);
	}
};

class GSRender : public rsx::thread
{
protected:
	GSFrameBase* m_frame;
	draw_context_t m_context;

public:
	GSRender();
	virtual ~GSRender();

	void on_init_rsx() override;
	void on_init_thread() override;
	void on_exit() override;

	void flip(int buffer) override;
};
