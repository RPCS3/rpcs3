#pragma once

#ifdef HAVE_X11
#include <X11/Xutil.h>
#endif

#include "../../display.h"
#include "../VulkanAPI.h"
#include "image.h"

#include <memory>

namespace vk
{
	struct swapchain_image_WSI
	{
		VkImage value = VK_NULL_HANDLE;
	};

	class swapchain_image_RPCS3 : public image
	{
		std::unique_ptr<buffer> m_dma_buffer;
		u32 m_width = 0;
		u32 m_height = 0;

	public:
		swapchain_image_RPCS3(render_device& dev, const memory_type_mapping& memory_map, u32 width, u32 height)
			:image(dev, memory_map.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TYPE_2D, VK_FORMAT_B8G8R8A8_UNORM, width, height, 1, 1, 1,
				VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 0)
		{
			m_width = width;
			m_height = height;
			current_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			m_dma_buffer = std::make_unique<buffer>(dev, m_width * m_height * 4, memory_map.host_visible_coherent,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0);
		}

		void do_dma_transfer(command_buffer& cmd)
		{
			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = m_width;
			copyRegion.bufferImageHeight = m_height;
			copyRegion.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.imageOffset = {};
			copyRegion.imageExtent = { m_width, m_height, 1 };

			change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			vkCmdCopyImageToBuffer(cmd, value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_dma_buffer->value, 1, &copyRegion);
			change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		}

		u32 get_required_memory_size() const
		{
			return m_width * m_height * 4;
		}

		void* get_pixels()
		{
			return m_dma_buffer->map(0, VK_WHOLE_SIZE);
		}

		void free_pixels()
		{
			m_dma_buffer->unmap();
		}
	};

	class swapchain_base
	{
	protected:
		render_device dev;

		display_handle_t window_handle{};
		u32 m_width = 0;
		u32 m_height = 0;
		VkFormat m_surface_format = VK_FORMAT_B8G8R8A8_UNORM;

		virtual void init_swapchain_images(render_device& dev, u32 count) = 0;

	public:
		swapchain_base(physical_device& gpu, u32 present_queue, u32 graphics_queue, u32 transfer_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
		{
			dev.create(gpu, graphics_queue, present_queue, transfer_queue);
			m_surface_format = format;
		}

		virtual ~swapchain_base() = default;

		virtual void create(display_handle_t& handle) = 0;
		virtual void destroy(bool full = true) = 0;
		virtual bool init() = 0;

		virtual u32 get_swap_image_count() const = 0;
		virtual VkImage get_image(u32 index) = 0;
		virtual VkResult acquire_next_swapchain_image(VkSemaphore semaphore, u64 timeout, u32* result) = 0;
		virtual void end_frame(command_buffer& cmd, u32 index) = 0;
		virtual VkResult present(VkSemaphore semaphore, u32 index) = 0;
		virtual VkImageLayout get_optimal_present_layout() = 0;

		virtual bool supports_automatic_wm_reports() const
		{
			return false;
		}

		bool init(u32 w, u32 h)
		{
			m_width = w;
			m_height = h;
			return init();
		}

		const vk::render_device& get_device()
		{
			return dev;
		}

		VkFormat get_surface_format()
		{
			return m_surface_format;
		}

		bool is_headless() const
		{
			return (dev.get_present_queue() == VK_NULL_HANDLE);
		}
	};

	template<typename T>
	class abstract_swapchain_impl : public swapchain_base
	{
	protected:
		std::vector<T> swapchain_images;

	public:
		abstract_swapchain_impl(physical_device& gpu, u32 present_queue, u32 graphics_queue, u32 transfer_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
			: swapchain_base(gpu, present_queue, graphics_queue, transfer_queue, format)
		{}

		~abstract_swapchain_impl() override = default;

		u32 get_swap_image_count() const override
		{
			return ::size32(swapchain_images);
		}

		using swapchain_base::init;
	};

	using native_swapchain_base = abstract_swapchain_impl<std::pair<bool, std::unique_ptr<swapchain_image_RPCS3>>>;
	using WSI_swapchain_base = abstract_swapchain_impl<swapchain_image_WSI>;

	#ifdef _WIN32

	class swapchain_WIN32 : public native_swapchain_base
	{
		HDC hDstDC = NULL;
		HDC hSrcDC = NULL;
		HBITMAP hDIB = NULL;
		LPVOID hPtr = NULL;

	public:
		swapchain_WIN32(physical_device& gpu, u32 present_queue, u32 graphics_queue, u32 transfer_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
			: native_swapchain_base(gpu, present_queue, graphics_queue, transfer_queue, format)
		{}

		~swapchain_WIN32() {}

		bool init() override
		{
			if (hDIB || hSrcDC)
				destroy(false);

			RECT rect;
			GetClientRect(window_handle, &rect);
			m_width = rect.right - rect.left;
			m_height = rect.bottom - rect.top;

			if (m_width == 0 || m_height == 0)
			{
				rsx_log.error("Invalid window dimensions %d x %d", m_width, m_height);
				return false;
			}

			BITMAPINFO bitmap = {};
			bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bitmap.bmiHeader.biWidth = m_width;
			bitmap.bmiHeader.biHeight = m_height * -1;
			bitmap.bmiHeader.biPlanes = 1;
			bitmap.bmiHeader.biBitCount = 32;
			bitmap.bmiHeader.biCompression = BI_RGB;

			hSrcDC = CreateCompatibleDC(hDstDC);
			hDIB = CreateDIBSection(hSrcDC, &bitmap, DIB_RGB_COLORS, &hPtr, NULL, 0);
			SelectObject(hSrcDC, hDIB);
			init_swapchain_images(dev, 3);
			return true;
		}

		void create(display_handle_t& handle) override
		{
			window_handle = handle;
			hDstDC = GetDC(handle);
		}

		void destroy(bool full = true) override
		{
			DeleteObject(hDIB);
			DeleteDC(hSrcDC);
			hDIB = NULL;
			hSrcDC = NULL;

			swapchain_images.clear();

			if (full)
			{
				ReleaseDC(window_handle, hDstDC);
				hDstDC = NULL;

				dev.destroy();
			}
		}

		VkResult present(VkSemaphore /*semaphore*/, u32 image) override
		{
			auto& src = swapchain_images[image];
			GdiFlush();

			if (hSrcDC)
			{
				memcpy(hPtr, src.second->get_pixels(), src.second->get_required_memory_size());
				BitBlt(hDstDC, 0, 0, m_width, m_height, hSrcDC, 0, 0, SRCCOPY);
				src.second->free_pixels();
			}

			src.first = false;
			return VK_SUCCESS;
		}
	#elif defined(__APPLE__)

	class swapchain_MacOS : public native_swapchain_base
	{
		void* nsView = nullptr;

	public:
		swapchain_MacOS(physical_device& gpu, u32 present_queue, u32 graphics_queue, u32 transfer_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
			: native_swapchain_base(gpu, present_queue, graphics_queue, transfer_queue, format)
		{}

		~swapchain_MacOS() {}

		bool init() override
		{
			//TODO: get from `nsView`
			m_width = 0;
			m_height = 0;

			if (m_width == 0 || m_height == 0)
			{
				rsx_log.error("Invalid window dimensions %d x %d", m_width, m_height);
				return false;
			}

			init_swapchain_images(dev, 3);
			return true;
		}

		void create(display_handle_t& window_handle) override
		{
			nsView = window_handle;
		}

		void destroy(bool full = true) override
		{
			swapchain_images.clear();

			if (full)
				dev.destroy();
		}

		VkResult present(VkSemaphore /*semaphore*/, u32 /*index*/) override
		{
			fmt::throw_exception("Native macOS swapchain is not implemented yet!");
		}
	#elif defined(HAVE_X11)

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
	#else

	class swapchain_Wayland : public native_swapchain_base
	{

	public:
		swapchain_Wayland(physical_device& gpu, u32 present_queue, u32 graphics_queue, u32 transfer_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
			: native_swapchain_base(gpu, present_queue, graphics_queue, transfer_queue, format)
		{}

		~swapchain_Wayland() {}

		bool init() override
		{
			fmt::throw_exception("Native Wayland swapchain is not implemented yet!");
		}

		void create(display_handle_t& window_handle) override
		{
			fmt::throw_exception("Native Wayland swapchain is not implemented yet!");
		}

		void destroy(bool full = true) override
		{
			fmt::throw_exception("Native Wayland swapchain is not implemented yet!");
		}

		VkResult present(VkSemaphore /*semaphore*/, u32 index) override
		{
			fmt::throw_exception("Native Wayland swapchain is not implemented yet!");
		}
	#endif

		VkResult acquire_next_swapchain_image(VkSemaphore /*semaphore*/, u64 /*timeout*/, u32* result) override
		{
			u32 index = 0;
			for (auto& p : swapchain_images)
			{
				if (!p.first)
				{
					p.first = true;
					*result = index;
					return VK_SUCCESS;
				}

				++index;
			}

			return VK_NOT_READY;
		}

		void end_frame(command_buffer& cmd, u32 index) override
		{
			swapchain_images[index].second->do_dma_transfer(cmd);
		}

		VkImage get_image(u32 index) override
		{
			return swapchain_images[index].second->value;
		}

		VkImageLayout get_optimal_present_layout() override
		{
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		}

	protected:
		void init_swapchain_images(render_device& dev, u32 preferred_count) override
		{
			swapchain_images.resize(preferred_count);
			for (auto& img : swapchain_images)
			{
				img.second = std::make_unique<swapchain_image_RPCS3>(dev, dev.get_memory_mapping(), m_width, m_height);
				img.first = false;
			}
		}
	};

	class swapchain_WSI : public WSI_swapchain_base
	{
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VkColorSpaceKHR m_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		VkSwapchainKHR m_vk_swapchain = nullptr;

		PFN_vkCreateSwapchainKHR _vkCreateSwapchainKHR = nullptr;
		PFN_vkDestroySwapchainKHR _vkDestroySwapchainKHR = nullptr;
		PFN_vkGetSwapchainImagesKHR _vkGetSwapchainImagesKHR = nullptr;
		PFN_vkAcquireNextImageKHR _vkAcquireNextImageKHR = nullptr;
		PFN_vkQueuePresentKHR _vkQueuePresentKHR = nullptr;

		bool m_wm_reports_flag = false;

	protected:
		void init_swapchain_images(render_device& dev, u32 /*preferred_count*/ = 0) override
		{
			u32 nb_swap_images = 0;
			_vkGetSwapchainImagesKHR(dev, m_vk_swapchain, &nb_swap_images, nullptr);

			if (!nb_swap_images) fmt::throw_exception("Driver returned 0 images for swapchain");

			std::vector<VkImage> vk_images;
			vk_images.resize(nb_swap_images);
			_vkGetSwapchainImagesKHR(dev, m_vk_swapchain, &nb_swap_images, vk_images.data());

			swapchain_images.resize(nb_swap_images);
			for (u32 i = 0; i < nb_swap_images; ++i)
			{
				swapchain_images[i].value = vk_images[i];
			}
		}

	public:
		swapchain_WSI(vk::physical_device& gpu, u32 present_queue, u32 graphics_queue, u32 transfer_queue, VkFormat format, VkSurfaceKHR surface, VkColorSpaceKHR color_space, bool force_wm_reporting_off)
			: WSI_swapchain_base(gpu, present_queue, graphics_queue, transfer_queue, format)
		{
			_vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(vkGetDeviceProcAddr(dev, "vkCreateSwapchainKHR"));
			_vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(vkGetDeviceProcAddr(dev, "vkDestroySwapchainKHR"));
			_vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(vkGetDeviceProcAddr(dev, "vkGetSwapchainImagesKHR"));
			_vkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(vkGetDeviceProcAddr(dev, "vkAcquireNextImageKHR"));
			_vkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(vkGetDeviceProcAddr(dev, "vkQueuePresentKHR"));

			m_surface = surface;
			m_color_space = color_space;

			if (!force_wm_reporting_off)
			{
				switch (gpu.get_driver_vendor())
				{
				case driver_vendor::AMD:
					break;
				case driver_vendor::INTEL:
	#ifdef _WIN32
					break;
	#endif
				case driver_vendor::NVIDIA:
				case driver_vendor::RADV:
					m_wm_reports_flag = true;
					break;
				default:
					break;
				}
			}
		}

		~swapchain_WSI() override = default;

		void create(display_handle_t&) override
		{}

		void destroy(bool = true) override
		{
			if (VkDevice pdev = dev)
			{
				if (m_vk_swapchain)
				{
					_vkDestroySwapchainKHR(pdev, m_vk_swapchain, nullptr);
				}

				dev.destroy();
			}
		}

		std::pair<VkSurfaceCapabilitiesKHR, bool> init_surface_capabilities()
		{
#ifdef _WIN32
			if (g_cfg.video.vk.force_disable_exclusive_fullscreen_mode && dev.get_surface_capabilities_2_support())
			{
				HMONITOR hmonitor = MonitorFromWindow(window_handle, MONITOR_DEFAULTTOPRIMARY);
				if (hmonitor)
				{
					VkSurfaceCapabilities2KHR pSurfaceCapabilities = {};
					pSurfaceCapabilities.sType                     = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;

					VkPhysicalDeviceSurfaceInfo2KHR pSurfaceInfo = {};
					pSurfaceInfo.sType                           = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
					pSurfaceInfo.surface                         = m_surface;

					VkSurfaceCapabilitiesFullScreenExclusiveEXT full_screen_exclusive_capabilities = {};
					VkSurfaceFullScreenExclusiveWin32InfoEXT full_screen_exclusive_win32_info      = {};
					full_screen_exclusive_capabilities.sType                                       = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_FULL_SCREEN_EXCLUSIVE_EXT;

					pSurfaceCapabilities.pNext = &full_screen_exclusive_capabilities;

					full_screen_exclusive_win32_info.sType    = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
					full_screen_exclusive_win32_info.hmonitor = hmonitor;

					pSurfaceInfo.pNext = &full_screen_exclusive_win32_info;

					auto getPhysicalDeviceSurfaceCapabilities2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR>(
						vkGetInstanceProcAddr(dev.gpu(), "vkGetPhysicalDeviceSurfaceCapabilities2KHR")
					);
					ensure(getPhysicalDeviceSurfaceCapabilities2KHR);
					CHECK_RESULT(getPhysicalDeviceSurfaceCapabilities2KHR(dev.gpu(), &pSurfaceInfo, &pSurfaceCapabilities));

					return { pSurfaceCapabilities.surfaceCapabilities, !!full_screen_exclusive_capabilities.fullScreenExclusiveSupported };
				}
				else
				{
					rsx_log.warning("Swapchain: failed to get monitor for the window");
				}
			}
#endif
			VkSurfaceCapabilitiesKHR surface_descriptors = {};
			CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev.gpu(), m_surface, &surface_descriptors));
			return { surface_descriptors, false };
		}

		using WSI_swapchain_base::init;
		bool init() override
		{
			if (dev.get_present_queue() == VK_NULL_HANDLE)
			{
				rsx_log.error("Cannot create WSI swapchain without a present queue");
				return false;
			}

			VkSwapchainKHR old_swapchain = m_vk_swapchain;
			vk::physical_device& gpu = const_cast<vk::physical_device&>(dev.gpu());

			auto [surface_descriptors, should_disable_exclusive_full_screen] = init_surface_capabilities();

			if (surface_descriptors.maxImageExtent.width < m_width ||
				surface_descriptors.maxImageExtent.height < m_height)
			{
				rsx_log.error("Swapchain: Swapchain creation failed because dimensions cannot fit. Max = %d, %d, Requested = %d, %d",
					surface_descriptors.maxImageExtent.width, surface_descriptors.maxImageExtent.height, m_width, m_height);

				return false;
			}

			if (surface_descriptors.currentExtent.width != UINT32_MAX)
			{
				if (surface_descriptors.currentExtent.width == 0 || surface_descriptors.currentExtent.height == 0)
				{
					rsx_log.warning("Swapchain: Current surface extent is a null region. Is the window minimized?");
					return false;
				}

				m_width = surface_descriptors.currentExtent.width;
				m_height = surface_descriptors.currentExtent.height;
			}

			u32 nb_available_modes = 0;
			CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &nb_available_modes, nullptr));

			std::vector<VkPresentModeKHR> present_modes(nb_available_modes);
			CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &nb_available_modes, present_modes.data()));

			VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
			std::vector<VkPresentModeKHR> preferred_modes;

			if (!g_cfg.video.vk.force_fifo)
			{
				// List of preferred modes in decreasing desirability
				// NOTE: Always picks "triple-buffered vsync" types if possible
				if (!g_cfg.video.vsync)
				{
					preferred_modes = { VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR };
				}
			}

			bool mode_found = false;
			for (VkPresentModeKHR preferred_mode : preferred_modes)
			{
				//Search for this mode in supported modes
				for (VkPresentModeKHR mode : present_modes)
				{
					if (mode == preferred_mode)
					{
						swapchain_present_mode = mode;
						mode_found = true;
						break;
					}
				}

				if (mode_found)
					break;
			}

			rsx_log.notice("Swapchain: present mode %d in use.", static_cast<int>(swapchain_present_mode));

			u32 nb_swap_images = surface_descriptors.minImageCount + 1;
			if (surface_descriptors.maxImageCount > 0)
			{
				//Try to negotiate for a triple buffer setup
				//In cases where the front-buffer isnt available for present, its better to have a spare surface
				nb_swap_images = std::max(surface_descriptors.minImageCount + 2u, 3u);

				if (nb_swap_images > surface_descriptors.maxImageCount)
				{
					// Application must settle for fewer images than desired:
					nb_swap_images = surface_descriptors.maxImageCount;
				}
			}

			VkSurfaceTransformFlagBitsKHR pre_transform = surface_descriptors.currentTransform;
			if (surface_descriptors.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
				pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

			VkSwapchainCreateInfoKHR swap_info = {};
			swap_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swap_info.surface = m_surface;
			swap_info.minImageCount = nb_swap_images;
			swap_info.imageFormat = m_surface_format;
			swap_info.imageColorSpace = m_color_space;

			swap_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			swap_info.preTransform = pre_transform;
			swap_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swap_info.imageArrayLayers = 1;
			swap_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swap_info.presentMode = swapchain_present_mode;
			swap_info.oldSwapchain = old_swapchain;
			swap_info.clipped = true;

			swap_info.imageExtent.width = std::max(m_width, surface_descriptors.minImageExtent.width);
			swap_info.imageExtent.height = std::max(m_height, surface_descriptors.minImageExtent.height);

	#ifdef _WIN32
			VkSurfaceFullScreenExclusiveInfoEXT full_screen_exclusive_info = {};
			if (should_disable_exclusive_full_screen)
			{
				full_screen_exclusive_info.sType               = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
				full_screen_exclusive_info.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT;

				swap_info.pNext = &full_screen_exclusive_info;
			}

			rsx_log.notice("Swapchain: requesting full screen exclusive mode %d.", static_cast<int>(full_screen_exclusive_info.fullScreenExclusive));
	#endif

			_vkCreateSwapchainKHR(dev, &swap_info, nullptr, &m_vk_swapchain);

			if (old_swapchain)
			{
				if (!swapchain_images.empty())
				{
					swapchain_images.clear();
				}

				_vkDestroySwapchainKHR(dev, old_swapchain, nullptr);
			}

			init_swapchain_images(dev);
			return true;
		}

		bool supports_automatic_wm_reports() const override
		{
			return m_wm_reports_flag;
		}

		VkResult acquire_next_swapchain_image(VkSemaphore semaphore, u64 timeout, u32* result) override
		{
			return vkAcquireNextImageKHR(dev, m_vk_swapchain, timeout, semaphore, VK_NULL_HANDLE, result);
		}

		void end_frame(command_buffer& /*cmd*/, u32 /*index*/) override
		{
		}

		VkResult present(VkSemaphore semaphore, u32 image) override
		{
			VkPresentInfoKHR present = {};
			present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			present.pNext = nullptr;
			present.swapchainCount = 1;
			present.pSwapchains = &m_vk_swapchain;
			present.pImageIndices = &image;

			if (semaphore != VK_NULL_HANDLE)
			{
				present.waitSemaphoreCount = 1;
				present.pWaitSemaphores = &semaphore;
			}

			return _vkQueuePresentKHR(dev.get_present_queue(), &present);
		}

		VkImage get_image(u32 index) override
		{
			return swapchain_images[index].value;
		}

		VkImageLayout get_optimal_present_layout() override
		{
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}
	};
}
