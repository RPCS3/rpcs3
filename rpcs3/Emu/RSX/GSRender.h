#pragma once

#include "Emu/RSX/RSXThread.h"
#include <memory>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
// nothing
#elif defined(HAVE_X11)
// Cannot include Xlib.h before Qt5
// and we don't need all of Xlib anyway
using Display = struct _XDisplay;
using Window = unsigned long;
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

using RSXDebuggerPrograms = std::vector<RSXDebuggerProgram>;

using draw_context_t = void*;

#ifdef _WIN32
using display_handle_t = HWND;
#elif defined(__APPLE__)
using display_handle_t = void*; // NSView
#else
using display_handle_t = std::variant<
#if defined(HAVE_X11) && defined(VK_USE_PLATFORM_WAYLAND_KHR)
	std::pair<Display*, Window>, std::pair<wl_display*, wl_surface*>
#elif defined(HAVE_X11)
	std::pair<Display*, Window>
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	std::pair<wl_display*, wl_surface*>
#endif
>;
#endif

class GSFrameBase
{
public:
	GSFrameBase() = default;
	GSFrameBase(const GSFrameBase&) = delete;
	virtual ~GSFrameBase() = default;

	virtual void close() = 0;
	virtual bool shown() = 0;
	virtual void hide() = 0;
	virtual void show() = 0;
	virtual void toggle_fullscreen() = 0;

	virtual void delete_context(draw_context_t ctx) = 0;
	virtual draw_context_t make_context() = 0;
	virtual void set_current(draw_context_t ctx) = 0;
	virtual void flip(draw_context_t ctx, bool skip_frame = false) = 0;
	virtual int client_width() = 0;
	virtual int client_height() = 0;

	virtual display_handle_t handle() const = 0;

	std::atomic<bool> screenshot_toggle = false;
	virtual void take_screenshot(const std::vector<u8> sshot_data, const u32 sshot_width, const u32 sshot_height, bool is_bgra) = 0;
};

class GSRender : public rsx::thread
{
protected:
	GSFrameBase* m_frame;
	draw_context_t m_context;

public:
	GSRender();
	~GSRender() override;

	void on_init_rsx() override;
	void on_init_thread() override;
	void on_exit() override;

	void flip(const rsx::display_flip_info_t& info) override;

	GSFrameBase* get_frame() { return m_frame; }
};
