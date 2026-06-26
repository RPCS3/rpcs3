#pragma once

#include "openxr_common.h"

#include <array>
#include <vector>

namespace openxr
{
	// Shows the frames shared by RSX in the headset (see openxr_frame_share.h)
	class renderer
	{
	public:
		struct context
		{
			XrSession session = XR_NULL_HANDLE;
			XrSpace reference_space = XR_NULL_HANDLE;
			VkPhysicalDevice vk_physical = VK_NULL_HANDLE;
			VkDevice vk_device = VK_NULL_HANDLE;
			u32 vk_queue_family = 0;
			u32 vk_queue_index = 0;
		};

		// Returns the device extensions needed for headset rendering, or false if they are not supported
		static bool get_device_extensions(VkPhysicalDevice physical_device, u32 vk_api_version, std::vector<const char*>& extensions);

		// Has to be called after the session was created. The device has to be created with get_device_extensions.
		bool init(const context& ctx);

		// Has to be called before the session and the Vulkan device are destroyed
		void destroy();

		// Has to be called between xrBeginFrame and xrEndFrame. Returns the number of layers.
		u32 render(XrTime display_time, bool should_render, std::array<const XrCompositionLayerBaseHeader*, 2>& layers);

	private:
		static constexpr u64 invalid_handle = umax;

		struct shared_image
		{
			VkImage image{};
			VkDeviceMemory memory{};
			u64 handle = invalid_handle;

			// Signaled by the OpenGL backend of RSX (see openxr_frame_share.h)
			VkSemaphore semaphore{};
			u64 semaphore_handle = invalid_handle;
		};

		bool create_swapchain();
		bool create_shared_images();
		void copy_latest_frame();
		u32 build_layers(XrTime display_time, std::array<const XrCompositionLayerBaseHeader*, 2>& layers);

		context m_ctx{}; // The session and the device are owned by the session thread

		bool m_rendering_active = false;
		bool m_has_swapchain_image = false; // True once at least one frame was released to the swapchain
		XrSwapchain m_swapchain{XR_NULL_HANDLE};
		std::vector<VkImage> m_swapchain_images;
		VkFormat m_shared_format = VK_FORMAT_UNDEFINED;
		std::vector<shared_image> m_shared_images;
		VkQueue m_vk_queue{};
		VkCommandPool m_vk_command_pool{};
		VkCommandBuffer m_vk_command_buffer{};
		VkFence m_vk_fence{};

		// Storage for the composition layers. They have to stay valid until xrEndFrame.
		std::array<XrCompositionLayerQuad, 2> m_quad_layers{};
		XrCompositionLayerProjection m_projection_layer{};
		std::array<XrCompositionLayerProjectionView, 2> m_projection_views{};
	};

	// A tiny quad that we submit while we don't render anything else.
	// Some runtimes (e.g. SteamVR) don't give input focus to a session that submits frames without layers.
	class placeholder_layer
	{
	public:
		// Has to be called after the session was created
		bool init(XrSession session, XrSpace reference_space);

		// Has to be called before the session and the Vulkan device are destroyed
		void destroy();

		// Returns nullptr if init failed
		const XrCompositionLayerBaseHeader* get_layer() const
		{
			return m_swapchain ? reinterpret_cast<const XrCompositionLayerBaseHeader*>(&m_layer) : nullptr;
		}

	private:
		static constexpr u32 image_size = 16;

		XrSwapchain m_swapchain{XR_NULL_HANDLE};
		XrCompositionLayerQuad m_layer{};
	};
}
