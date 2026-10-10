#pragma once

#include "upscaling.h"

#include "../VKHelpers.h"
#include "../VKResourceManager.h"

#include <algorithm>
#include <cstdlib>
#include <memory>

namespace vk
{
	// Sharp bilinear scaling. The image is first scaled up by an integer factor using nearest neighbor filtering
	// and then scaled to the final size using bilinear filtering.
	// This keeps pixels crisp while avoiding the uneven pixel sizes of plain nearest neighbor scaling at non-integer ratios.
	class sharp_bilinear_upscale_pass : public upscaler
	{
		std::unique_ptr<vk::viewable_image> m_prescaled;

		void dispose_image()
		{
			if (m_prescaled && m_prescaled->value)
			{
				vk::get_resource_manager()->dispose(m_prescaled);
			}
			else
			{
				m_prescaled.reset();
			}
		}

		bool initialize_image(u32 w, u32 h)
		{
			dispose_image();

			const auto pdev = vk::get_current_renderer();

			// Intermediate data is only ever blitted around, so a regular 8-bit format that supports blits in both directions is all we need
			constexpr VkFlags required_bits = VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT;
			VkFormat data_format = VK_FORMAT_UNDEFINED;

			for (const VkFormat format : { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM })
			{
				if ((pdev->get_format_properties(format).optimalTilingFeatures & required_bits) == required_bits)
				{
					data_format = format;
					break;
				}
			}

			if (data_format == VK_FORMAT_UNDEFINED)
			{
				rsx_log.error("Sharp bilinear scaling is not supported by this driver and hardware combination. Will fall back to bilinear upscaling.");
				return false;
			}

			m_prescaled = std::make_unique<vk::viewable_image>(
				*pdev,                                               // Owner
				pdev->get_memory_mapping().device_local,             // Must be in device optimal memory
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				data_format,
				w, h, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT,                // Dimensions (w, h, d, mips, layers, samples)
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				VK_IMAGE_CREATE_ALLOW_NULL_RPCS3,                    // Allow creation to fail if there is no memory
				VMM_ALLOCATION_POOL_SWAPCHAIN,
				RSX_FORMAT_CLASS_COLOR);

			if (m_prescaled->value == VK_NULL_HANDLE)
			{
				dispose_image();
				rsx_log.warning("Sharp bilinear scaling is enabled, but the system is out of memory. Will fall back to bilinear upscaling.");
				return false;
			}

			return true;
		}

	public:
		~sharp_bilinear_upscale_pass()
		{
			dispose_image();
		}

		vk::viewable_image* scale_output(
			const vk::command_buffer& cmd,          // CB
			vk::viewable_image* src,                // Source input
			VkImage present_surface,                // Present target. May be VK_NULL_HANDLE for some passes
			VkImageLayout present_surface_layout,   // Present surface layout, or VK_IMAGE_LAYOUT_UNDEFINED if no present target is provided
			const VkImageBlit& request,             // Scaling request information
			rsx::flags32_t mode                     // Mode
		) override
		{
			if (!(mode & UPSCALE_AND_COMMIT))
			{
				// Upscaling source only is unsupported
				return src;
			}

			ensure(present_surface);

			const u32 src_w = std::abs(request.srcOffsets[1].x - request.srcOffsets[0].x);
			const u32 src_h = std::abs(request.srcOffsets[1].y - request.srcOffsets[0].y);
			const u32 dst_w = std::abs(request.dstOffsets[1].x - request.dstOffsets[0].x);
			const u32 dst_h = std::abs(request.dstOffsets[1].y - request.dstOffsets[0].y);

			// Integer prescale factors. Only useful when upscaling.
			const u32 scale_x = (src_w > 0 && dst_w > src_w) ? utils::aligned_div(dst_w, src_w) : 1;
			const u32 scale_y = (src_h > 0 && dst_h > src_h) ? utils::aligned_div(dst_h, src_h) : 1;

			bool prescale = (scale_x > 1 || scale_y > 1);
			const u32 prescaled_w = src_w * scale_x;
			const u32 prescaled_h = src_h * scale_y;

			if (prescale && (!m_prescaled || m_prescaled->width() != prescaled_w || m_prescaled->height() != prescaled_h))
			{
				prescale = initialize_image(prescaled_w, prescaled_h);
			}

			src->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			if (!prescale)
			{
				// Plain bilinear is all that is needed (or all that is possible)
				vkCmdBlitImage(cmd, src->value, src->current_layout, present_surface, present_surface_layout, 1, &request, VK_FILTER_LINEAR);
				src->pop_layout(cmd);
				return nullptr;
			}

			// Pass 1: Integer upscale with nearest filtering. Mirroring is preserved by the source offsets.
			VkImageBlit prescale_request = request;
			prescale_request.dstOffsets[0] = { 0, 0, 0 };
			prescale_request.dstOffsets[1] = { static_cast<s32>(prescaled_w), static_cast<s32>(prescaled_h), 1 };

			m_prescaled->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			vkCmdBlitImage(cmd, src->value, src->current_layout, m_prescaled->value, m_prescaled->current_layout, 1, &prescale_request, VK_FILTER_NEAREST);
			src->pop_layout(cmd);

			// Pass 2: Scale to the final size with bilinear filtering
			VkImageBlit final_request = request;
			final_request.srcSubresource = prescale_request.dstSubresource;
			final_request.srcOffsets[0] = { 0, 0, 0 };
			final_request.srcOffsets[1] = { static_cast<s32>(prescaled_w), static_cast<s32>(prescaled_h), 1 };

			m_prescaled->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			vkCmdBlitImage(cmd, m_prescaled->value, m_prescaled->current_layout, present_surface, present_surface_layout, 1, &final_request, VK_FILTER_LINEAR);
			return nullptr;
		}
	};
}
