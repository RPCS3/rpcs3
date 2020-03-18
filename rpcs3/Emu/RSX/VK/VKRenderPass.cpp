#include "stdafx.h"

#include "Utilities/mutex.h"
#include "VKRenderPass.h"

namespace vk
{
	struct active_renderpass_info_t
	{
		VkRenderPass pass = VK_NULL_HANDLE;
		VkFramebuffer fbo = VK_NULL_HANDLE;
	};

	atomic_t<u64> g_cached_renderpass_key = 0;
	VkRenderPass  g_cached_renderpass = VK_NULL_HANDLE;
	std::unordered_map<VkCommandBuffer, active_renderpass_info_t>  g_current_renderpass;

	shared_mutex g_renderpass_cache_mutex;
	std::unordered_map<u64, VkRenderPass> g_renderpass_cache;

	u64 get_renderpass_key(const std::vector<vk::image*>& images)
	{
		// Key structure
		// 0-8 color_format
		// 8-16 depth_format
		// 16-21 sample_counts
		// 21-37 current layouts
		u64 key = 0;
		u64 layout_offset = 22;
		for (const auto &surface : images)
		{
			const auto format_code = u64(surface->format()) & 0xFF;
			switch (format_code)
			{
			case VK_FORMAT_D16_UNORM:
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				key |= (format_code << 8);
				break;
			default:
				key |= format_code;
				break;
			}

			switch (const auto layout = surface->current_layout)
			{
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_GENERAL:
				key |= (u64(layout) << layout_offset);
				layout_offset += 3;
				break;
			default:
				fmt::throw_exception("Unsupported image layout 0x%x", static_cast<u32>(layout));
			}
		}

		key |= u64(images[0]->samples()) << 16;
		return key;
	}

	u64 get_renderpass_key(const std::vector<vk::image*>& images, u64 previous_key)
	{
		// Partial update; assumes compatible renderpass keys
		const u64 layout_mask = (0x7FFFull << 22);

		u64 key = previous_key & ~layout_mask;
		u64 layout_offset = 22;

		for (const auto &surface : images)
		{
			switch (const auto layout = surface->current_layout)
			{
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_GENERAL:
				key |= (u64(layout) << layout_offset);
				layout_offset += 3;
				break;
			default:
				fmt::throw_exception("Unsupported image layout 0x%x", static_cast<u32>(layout));
			}
		}

		return key;
	}

	u64 get_renderpass_key(VkFormat surface_format)
	{
		u64 key = (1ull << 16);

		switch (surface_format)
		{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			key |= (u64(surface_format) << 8);
			key |= (u64(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) << 22);
			break;
		default:
			key |= u64(surface_format);
			key |= (u64(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) << 22);
			break;
		}

		return key;
	}

	VkRenderPass get_renderpass(VkDevice dev, u64 renderpass_key)
	{
		// 99.999% of checks will go through this block once on-disk shader cache has loaded
		{
			reader_lock lock(g_renderpass_cache_mutex);

			auto found = g_renderpass_cache.find(renderpass_key);
			if (found != g_renderpass_cache.end())
			{
				return found->second;
			}
		}

		std::lock_guard lock(g_renderpass_cache_mutex);

		// Check again
		auto found = g_renderpass_cache.find(renderpass_key);
		if (found != g_renderpass_cache.end())
		{
			return found->second;
		}

		// Decode
		VkSampleCountFlagBits samples = VkSampleCountFlagBits((renderpass_key >> 16) & 0xF);
		std::vector<VkImageLayout> rtv_layouts;
		VkImageLayout dsv_layout;

		u64 layout_offset = 22;
		for (int n = 0; n < 5; ++n)
		{
			const VkImageLayout layout = VkImageLayout((renderpass_key >> layout_offset) & 0x7);
			layout_offset += 3;

			if (layout)
			{
				rtv_layouts.push_back(layout);
			}
			else
			{
				break;
			}
		}

		VkFormat color_format = VkFormat(renderpass_key & 0xFF);
		VkFormat depth_format = VkFormat((renderpass_key >> 8) & 0xFF);

		if (depth_format)
		{
			dsv_layout = rtv_layouts.back();
			rtv_layouts.pop_back();
		}

		std::vector<VkAttachmentDescription> attachments = {};
		std::vector<VkAttachmentReference> attachment_references;

		u32 attachment_count = 0;
		for (const auto &layout : rtv_layouts)
		{
			VkAttachmentDescription color_attachment_description = {};
			color_attachment_description.format = color_format;
			color_attachment_description.samples = samples;
			color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			color_attachment_description.initialLayout = layout;
			color_attachment_description.finalLayout = layout;

			attachments.push_back(color_attachment_description);
			attachment_references.push_back({ attachment_count++, layout });
		}

		if (depth_format)
		{
			VkAttachmentDescription depth_attachment_description = {};
			depth_attachment_description.format = depth_format;
			depth_attachment_description.samples = samples;
			depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			depth_attachment_description.initialLayout = dsv_layout;
			depth_attachment_description.finalLayout = dsv_layout;
			attachments.push_back(depth_attachment_description);

			attachment_references.push_back({ attachment_count, dsv_layout });
		}

		VkSubpassDependency null_subpass_dependencies[2] =
		{
			{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				.srcAccessMask = 0,
				.dstAccessMask = 0,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
			},
			{
				.srcSubpass = 0,
				.dstSubpass = VK_SUBPASS_EXTERNAL,
				.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				.srcAccessMask = 0,
				.dstAccessMask = 0,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
			}
		};

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = attachment_count;
		subpass.pColorAttachments = attachment_count? attachment_references.data() : nullptr;
		subpass.pDepthStencilAttachment = depth_format? &attachment_references.back() : nullptr;

		VkRenderPassCreateInfo rp_info = {};
		rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rp_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		rp_info.pAttachments = attachments.data();
		rp_info.subpassCount = 1;
		rp_info.pSubpasses = &subpass;

		if (vk::get_driver_vendor() == vk::driver_vendor::RADV)
		{
			// So, according to spec, if you do not explicitly define the relationship between a subpass and its neighbours,
			// a full pipeline barrier will be inserted for you, which is awful for performance.
			// However, only RADV seems to do this and it does not work for other vendors.
			// NVIDIA specifically slows to a crawl even with a TOP_OF_PIPE->TOP_OF_PIPE dependency with no dependent access.
			// RPCS3 does not actually want to declare any dependency here, we have explicit pipeline barriers for our tasks.
			// Workaround: Only provide this null dep chain for RADV

			rp_info.dependencyCount = 2;
			rp_info.pDependencies = null_subpass_dependencies;
		}

		VkRenderPass result;
		CHECK_RESULT(vkCreateRenderPass(dev, &rp_info, NULL, &result));

		g_renderpass_cache[renderpass_key] = result;
		return result;
	}

	void clear_renderpass_cache(VkDevice dev)
	{
		for (const auto &renderpass : g_renderpass_cache)
		{
			vkDestroyRenderPass(dev, renderpass.second, nullptr);
		}

		g_renderpass_cache.clear();
	}

	void begin_renderpass(VkCommandBuffer cmd, VkRenderPass pass, VkFramebuffer target, const coordu& framebuffer_region)
	{
		auto& renderpass_info = g_current_renderpass[cmd];
		if (renderpass_info.pass == pass && renderpass_info.fbo == target)
		{
			return;
		}
		else if (renderpass_info.pass != VK_NULL_HANDLE)
		{
			end_renderpass(cmd);
		}

		VkRenderPassBeginInfo rp_begin = {};
		rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rp_begin.renderPass = pass;
		rp_begin.framebuffer = target;
		rp_begin.renderArea.offset.x = static_cast<int32_t>(framebuffer_region.x);
		rp_begin.renderArea.offset.y = static_cast<int32_t>(framebuffer_region.y);
		rp_begin.renderArea.extent.width = framebuffer_region.width;
		rp_begin.renderArea.extent.height = framebuffer_region.height;

		vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
		renderpass_info = { pass, target };
	}

	void begin_renderpass(VkDevice dev, VkCommandBuffer cmd, u64 renderpass_key, VkFramebuffer target, const coordu& framebuffer_region)
	{
		if (renderpass_key != g_cached_renderpass_key)
		{
			g_cached_renderpass = get_renderpass(dev, renderpass_key);
			g_cached_renderpass_key = renderpass_key;
		}

		begin_renderpass(cmd, g_cached_renderpass, target, framebuffer_region);
	}

	void end_renderpass(VkCommandBuffer cmd)
	{
		vkCmdEndRenderPass(cmd);
		g_current_renderpass[cmd] = {};
	}

	bool is_renderpass_open(VkCommandBuffer cmd)
	{
		return g_current_renderpass[cmd].pass != VK_NULL_HANDLE;
	}
}
