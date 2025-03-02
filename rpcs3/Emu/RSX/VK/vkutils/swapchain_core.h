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
		swapchain_image_RPCS3(render_device& dev, const memory_type_mapping& memory_map, u32 width, u32 height);

		void do_dma_transfer(command_buffer& cmd);

		u32 get_required_memory_size() const;

		void* get_pixels();

		void free_pixels();
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
		swapchain_base(physical_device& gpu, u32 present_queue, u32 graphics_queue, u32 transfer_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM);

		virtual ~swapchain_base() = default;

		virtual void create(display_handle_t& handle) = 0;
		virtual void destroy(bool full = true) = 0;
		virtual bool init() = 0;

		virtual u32 get_swap_image_count() const = 0;
		virtual VkImage get_image(u32 index) = 0;
		virtual VkResult acquire_next_swapchain_image(VkSemaphore semaphore, u64 timeout, u32* result) = 0;
		virtual void end_frame(command_buffer& cmd, u32 index) = 0;
		virtual VkResult present(VkSemaphore semaphore, u32 index) = 0;
		virtual VkImageLayout get_optimal_present_layout() const = 0;

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

		VkFormat get_surface_format() const
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

	using WSI_swapchain_base = abstract_swapchain_impl<swapchain_image_WSI>;

	class native_swapchain_base : public abstract_swapchain_impl<std::pair<bool, std::unique_ptr<swapchain_image_RPCS3>>>
	{
	public:
		using abstract_swapchain_impl::abstract_swapchain_impl;

		VkResult acquire_next_swapchain_image(VkSemaphore semaphore, u64 timeout, u32* result) override;

		// Clients must implement these methods to render without WSI support
		bool init() override
		{
			fmt::throw_exception("Native swapchain is not implemented yet!");
		}

		void create(display_handle_t& /*window_handle*/) override
		{
			fmt::throw_exception("Native swapchain is not implemented yet!");
		}

		void destroy(bool /*full*/ = true) override
		{
			fmt::throw_exception("Native swapchain is not implemented yet!");
		}

		VkResult present(VkSemaphore /*semaphore*/, u32 /*index*/) override
		{
			fmt::throw_exception("Native swapchain is not implemented yet!");
		}

		// Generic accessors
		void end_frame(command_buffer& cmd, u32 index) override
		{
			swapchain_images[index].second->do_dma_transfer(cmd);
		}

		VkImage get_image(u32 index) override
		{
			return swapchain_images[index].second->value;
		}

		VkImageLayout get_optimal_present_layout() const override
		{
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		}

	protected:
		void init_swapchain_images(render_device& dev, u32 preferred_count) override;
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
		void init_swapchain_images(render_device& dev, u32 preferred_count = 0) override;

	public:
		swapchain_WSI(vk::physical_device& gpu, u32 present_queue, u32 graphics_queue, u32 transfer_queue, VkFormat format, VkSurfaceKHR surface, VkColorSpaceKHR color_space, bool force_wm_reporting_off);

		~swapchain_WSI() override = default;

		void create(display_handle_t&) override
		{}

		void destroy(bool = true) override;

		std::pair<VkSurfaceCapabilitiesKHR, bool> init_surface_capabilities();

		using WSI_swapchain_base::init;
		bool init() override;

		bool supports_automatic_wm_reports() const override
		{
			return m_wm_reports_flag;
		}

		VkResult acquire_next_swapchain_image(VkSemaphore semaphore, u64 timeout, u32* result) override
		{
			return vkAcquireNextImageKHR(dev, m_vk_swapchain, timeout, semaphore, VK_NULL_HANDLE, result);
		}

		void end_frame(command_buffer& /*cmd*/, u32 /*index*/) override
		{}

		VkResult present(VkSemaphore semaphore, u32 image) override;

		VkImage get_image(u32 index) override
		{
			return swapchain_images[index].value;
		}

		VkImageLayout get_optimal_present_layout() const override
		{
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}
	};

	struct WSI_config
	{
		bool supports_automatic_wm_reports = true;
	};
}
