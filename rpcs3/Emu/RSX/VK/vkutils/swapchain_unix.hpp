#pragma once

#include "swapchain_core.h"

#ifdef HAVE_X11
#include <X11/Xutil.h>
#endif

namespace vk
{
#if defined(HAVE_X11)

	class swapchain_X11 : public native_swapchain_base
	{
		Display* display = nullptr;
		Window window = 0;
		XImage* pixmap = nullptr;
		GC gc = nullptr;
		int bit_depth = 24;

	public:
		swapchain_X11(physical_device& gpu, u32 present_queue, u32 graphics_queue, u32 transfer_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
			: native_swapchain_base(gpu, present_queue, graphics_queue, transfer_queue, format)
		{}

		~swapchain_X11() override = default;

		bool init() override
		{
			if (pixmap)
				destroy(false);

			Window root;
			int x, y;
			u32 w = 0, h = 0, border, depth;

			if (XGetGeometry(display, window, &root, &x, &y, &w, &h, &border, &depth))
			{
				m_width = w;
				m_height = h;
				bit_depth = depth;
			}

			if (m_width == 0 || m_height == 0)
			{
				rsx_log.error("Invalid window dimensions %d x %d", m_width, m_height);
				return false;
			}

			XVisualInfo visual{};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
			if (!XMatchVisualInfo(display, DefaultScreen(display), bit_depth, TrueColor, &visual))
#pragma GCC diagnostic pop
			{
				rsx_log.error("Could not find matching visual info!");
				return false;
			}

			pixmap = XCreateImage(display, visual.visual, visual.depth, ZPixmap, 0, nullptr, m_width, m_height, 32, 0);
			init_swapchain_images(dev, 3);
			return true;
		}

		void create(display_handle_t& window_handle) override
		{
			std::visit([&](auto&& p)
				{
					using T = std::decay_t<decltype(p)>;
					if constexpr (std::is_same_v<T, std::pair<Display*, Window>>)
					{
						display = p.first;
						window = p.second;
					}
				}, window_handle);

			if (display == NULL)
			{
				rsx_log.fatal("Could not create virtual display on this window protocol (Wayland?)");
				return;
			}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
			gc = DefaultGC(display, DefaultScreen(display));
#pragma GCC diagnostic pop
		}

		void destroy(bool full = true) override
		{
			pixmap->data = nullptr;
			XDestroyImage(pixmap);
			pixmap = NULL;

			swapchain_images.clear();

			if (full)
				dev.destroy();
		}

		VkResult present(VkSemaphore /*semaphore*/, u32 index) override
		{
			auto& src = swapchain_images[index];
			if (pixmap)
			{
				pixmap->data = static_cast<char*>(src.second->get_pixels());

				XPutImage(display, window, gc, pixmap, 0, 0, 0, 0, m_width, m_height);
				XFlush(display);

				src.second->free_pixels();
			}

			//Release reference
			src.first = false;
			return VK_SUCCESS;
		}
	};

	using swapchain_NATIVE = swapchain_X11;

#endif

#if defined(HAVE_WAYLAND)
	using swapchain_Wayland = native_swapchain_base;

#ifndef HAVE_X11
	using swapchain_NATIVE = swapchain_Wayland;
#endif

#endif

	[[maybe_unused]] static
	VkSurfaceKHR make_WSI_surface(VkInstance vk_instance, display_handle_t window_handle, WSI_config* config)
	{
		VkSurfaceKHR result = VK_NULL_HANDLE;

		std::visit([&](auto&& p)
		{
			using T = std::decay_t<decltype(p)>;

#ifdef HAVE_X11
			if constexpr (std::is_same_v<T, std::pair<Display*, Window>>)
			{
				VkXlibSurfaceCreateInfoKHR createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
				createInfo.dpy = p.first;
				createInfo.window = p.second;
				CHECK_RESULT(vkCreateXlibSurfaceKHR(vk_instance, &createInfo, nullptr, &result));
			}
			else
#endif
#ifdef HAVE_WAYLAND
			if constexpr (std::is_same_v<T, std::pair<wl_display*, wl_surface*>>)
			{
				VkWaylandSurfaceCreateInfoKHR createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
				createInfo.display = p.first;
				createInfo.surface = p.second;
				CHECK_RESULT(vkCreateWaylandSurfaceKHR(vk_instance, &createInfo, nullptr, &result));
				config->supports_automatic_wm_reports = false;
			}
			else
#endif
			{
				static_assert(std::conditional_t<true, std::false_type, T>::value, "Unhandled window_handle type in std::variant");
			}
		}, window_handle);

		return ensure(result, "Failed to initialize Vulkan display surface");
	}
}
