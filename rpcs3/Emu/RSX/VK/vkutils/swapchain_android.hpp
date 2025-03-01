#pragma once

#include "swapchain_core.h"

namespace vk
{
#if defined(ANDROID)
	class swapchain_ANDROID : public native_swapchain_base
	{
		void* surface_handle = nullptr; // FIXME: No idea what the android native surface is called

	public:
		swapchain_ANDROID(physical_device& gpu, u32 present_queue, u32 graphics_queue, u32 transfer_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
			: native_swapchain_base(gpu, present_queue, graphics_queue, transfer_queue, format)
		{}

		~swapchain_ANDROID() {}

		bool init() override
		{
			//TODO: get from `surface`
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
			surface_handle = window_handle;
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

	using swapchain_NATIVE = swapchain_ANDROID;
#endif
}

