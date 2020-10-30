#pragma once

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
// nothing
#elif defined(HAVE_X11)
// Cannot include Xlib.h before Qt5
// and we don't need all of Xlib anyway
using Display = struct _XDisplay;
using Window  = unsigned long;
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#include <wayland-client.h>
#endif

#ifdef _WIN32
using display_handle_t = HWND;
#elif defined(__APPLE__)
using display_handle_t = void*; // NSView
#else
#include <variant>
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

using draw_context_t = void*;
