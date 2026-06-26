#pragma once

#include "VulkanAPI.h"
#include "Utilities/geometry.h"
#include "Emu/Io/openxr/openxr_frame_share.h"

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace vk
{
	class render_device;
	class command_buffer;
	class image;
	struct framebuffer_holder;

	// Copies the presented frames into the images shared by the OpenXR handler (see Emu/Io/openxr/openxr_frame_share.h)
	class openxr_frame_share
	{
	public:
		struct frame_token
		{
			s32 slot = -1;
			u64 generation = 0;
		};

		// Draws the overlays into the framebuffer with the given render pass, using area as the screen area
		using overlay_callback = std::function<void(framebuffer_holder* fbo, VkRenderPass pass, const areau& area)>;

		void create(const render_device* dev);
		void destroy();

		// Records a copy of the presented frame into a free slot. right_image is only used if eyes.separate_images is set.
		// If draw_overlays is set, the overlays are drawn into a copy of the images first, since RSX draws them directly to the window.
		// The slot is only shown in the headset after on_frame_finished is called for the returned token.
		frame_token present(const command_buffer& cmd, image* left_image, image* right_image, const openxr::eye_layout& eyes, const overlay_callback& draw_overlays);

		// Has to be called once the command buffer containing the copy has finished (or failed)
		void on_frame_finished(const frame_token& token, bool success);

	private:
		image* add_overlays(const command_buffer& cmd, image* src, u32 index, std::initializer_list<areai> areas, const overlay_callback& draw_overlays);

		std::array<std::unique_ptr<image>, 2> m_overlay_images; // Copies of the eye images with overlays

		struct imported_slot
		{
			VkImage image = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
		};

		struct import_set
		{
			u64 generation = 0;
			std::vector<imported_slot> slots;
			u32 pending_frames = 0; // Frames that were recorded with these images and did not finish yet
		};

		bool update_imports();
		bool import_images(import_set& set);
		void destroy_set(import_set& set);

		const render_device* m_device = nullptr;
		std::unique_ptr<import_set> m_current;
		std::vector<std::unique_ptr<import_set>> m_retired; // Replaced sets that are still used by frames in flight
		u64 m_generation = 0;
		bool m_import_failed = false; // Only try once per generation
	};
}
