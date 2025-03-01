#pragma once

#include "swapchain_core.h"

namespace vk
{
#if defined(__APPLE__)
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
	};

	using swapchain_NATIVE = swapchain_MacOS;

	static
	VkSurfaceKHR make_WSI_surface(VkInstance vk_instance, display_handle_t handle)
	{
		VkSurfaceKHR result = VK_NULL_HANDLE;
		VkMacOSSurfaceCreateInfoMVK createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
		createInfo.pView = window_handle;

		CHECK_RESULT(vkCreateMacOSSurfaceMVK(vk_instance, &createInfo, NULL, &result));
		return result;
	}
#endif
}
