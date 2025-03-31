#include "stdafx.h"

#include "Utilities/mutex.h"
#include "VKRenderPass.h"
#include "vkutils/image.h"

#include "Emu/RSX/Common/unordered_map.hpp"

namespace vk
{
	struct active_renderpass_info_t
	{
		VkRenderPass pass = VK_NULL_HANDLE;
		VkFramebuffer fbo = VK_NULL_HANDLE;
	};

	atomic_t<u64> g_cached_renderpass_key = 0;
	VkRenderPass  g_cached_renderpass = VK_NULL_HANDLE;
	rsx::unordered_map<VkCommandBuffer, active_renderpass_info_t>  g_current_renderpass;

	shared_mutex g_renderpass_cache_mutex;
	rsx::unordered_map<u64, VkRenderPass> g_renderpass_cache;

	// Key structure
	// 0-7 color_format
	// 8-15 depth_format
	// 16-21 sample_counts
	// 22-36 current layouts
	// 37-41 input attachments
	union renderpass_key_blob
	{
	private:
		// Internal utils
		static u64 encode_layout(VkImageLayout layout)
		{
			switch (+layout)
			{
			case VK_IMAGE_LAYOUT_GENERAL:
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				return static_cast<u64>(layout);
			case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
				return 4ull;
			default:
				fmt::throw_exception("Unsupported layout 0x%llx here", static_cast<usz>(layout));
			}
		}

		static VkImageLayout decode_layout(u64 encoded)
		{
			switch (encoded)
			{
			case 1:
			case 2:
			case 3:
				return static_cast<VkImageLayout>(encoded);
			case 4:
				return VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT;
			default:
				fmt::throw_exception("Unsupported layout encoding 0x%llx here", encoded);
			}
		}

	public:
		u64 encoded;

		struct
		{
			u64 color_format  : 8;
			u64 depth_format  : 8;
			u64 sample_count  : 6;
			u64 layout_blob   : 15;
			u64 input_attachments_mask : 5;
		};

		renderpass_key_blob(u64 encoded_) : encoded(encoded_)
		{}

		// Encoders
		inline void set_layout(u32 index, VkImageLayout layout)
		{
			switch (+layout)
			{
			case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_GENERAL:
				layout_blob |= encode_layout(layout) << (index * 3);
				break;
			default:
				fmt::throw_exception("Unsupported image layout 0x%x", static_cast<u32>(layout));
			}
		}

		inline void set_input_attachment(u32 index)
		{
			input_attachments_mask |= (1ull << index);
		}

		inline void set_format(VkFormat format)
		{
			switch (format)
			{
			case VK_FORMAT_D16_UNORM:
			case VK_FORMAT_D32_SFLOAT:
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				depth_format = static_cast<u64>(format);
				break;
			default:
				color_format = static_cast<u64>(format);
				break;
			}
		}

		// Decoders
		inline VkSampleCountFlagBits get_sample_count() const
		{
			return static_cast<VkSampleCountFlagBits>(sample_count);
		}

		inline VkFormat get_color_format() const
		{
			return static_cast<VkFormat>(color_format);
		}

		inline VkFormat get_depth_format() const
		{
			return static_cast<VkFormat>(depth_format);
		}

		std::vector<VkAttachmentReference> get_input_attachments() const
		{
			if (input_attachments_mask == 0) [[likely]]
			{
				return {};
			}

			std::vector<VkAttachmentReference> result;
			for (u32 i = 0; i < 5; ++i)
			{
				if (input_attachments_mask & (1ull << i))
				{
					const auto layout = decode_layout((layout_blob >> (i * 3)) & 0x7);
					result.push_back({i, layout});
				}
			}

			return result;
		}

		std::vector<VkImageLayout> get_image_layouts() const
		{
			std::vector<VkImageLayout> result;

			for (u32 i = 0, layout_offset = 0; i < 5; ++i, layout_offset += 3)
			{
				if (const auto layout_encoding = (layout_blob >> layout_offset) & 0x7)
				{
					result.push_back(decode_layout(layout_encoding));
				}
				else
				{
					break;
				}
			}

			return result;
		}
	};

	u64 get_renderpass_key(const std::vector<vk::image*>& images, const std::vector<u8>& input_attachment_ids)
	{
		renderpass_key_blob key(0);

		for (u32 i = 0; i < ::size32(images); ++i)
		{
			const auto& surface = images[i];
			key.set_format(surface->format());
			key.set_layout(i, surface->current_layout);
		}

		for (const auto& ref_id : input_attachment_ids)
		{
			key.set_input_attachment(ref_id);
		}

		key.sample_count = images[0]->samples();
		return key.encoded;
	}

	u64 get_renderpass_key(const std::vector<vk::image*>& images, u64 previous_key)
	{
		// Partial update; assumes compatible renderpass keys
		renderpass_key_blob key(previous_key);
		key.layout_blob = 0;

		for (u32 i = 0; i < ::size32(images); ++i)
		{
			key.set_layout(i, images[i]->current_layout);
		}

		return key.encoded;
	}

	u64 get_renderpass_key(VkFormat surface_format)
	{
		renderpass_key_blob key(0);
		key.sample_count = 1;

		switch (surface_format)
		{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			key.depth_format = static_cast<u64>(surface_format);
			key.layout_blob = static_cast<u64>(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			break;
		default:
			key.color_format = static_cast<u64>(surface_format);
			key.layout_blob = static_cast<u64>(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			break;
		}

		return key.encoded;
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
		renderpass_key_blob key(renderpass_key);
		VkSampleCountFlagBits samples = static_cast<VkSampleCountFlagBits>(key.sample_count);
		std::vector<VkImageLayout> rtv_layouts;
		VkImageLayout dsv_layout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkFormat color_format = static_cast<VkFormat>(key.color_format);
		VkFormat depth_format = static_cast<VkFormat>(key.depth_format);

		std::vector<VkAttachmentDescription> attachments = {};
		std::vector<VkAttachmentReference> attachment_references;

		rtv_layouts = key.get_image_layouts();
		if (depth_format)
		{
			dsv_layout = rtv_layouts.back();
			rtv_layouts.pop_back();
		}

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

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = attachment_count;
		subpass.pColorAttachments = attachment_count? attachment_references.data() : nullptr;
		subpass.pDepthStencilAttachment = depth_format? &attachment_references.back() : nullptr;

		const auto input_attachments = key.get_input_attachments();
		if (!input_attachments.empty())
		{
			subpass.inputAttachmentCount = ::size32(input_attachments);
			subpass.pInputAttachments = input_attachments.data();
		}

		VkRenderPassCreateInfo rp_info = {};
		rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rp_info.attachmentCount = ::size32(attachments);
		rp_info.pAttachments = attachments.data();
		rp_info.subpassCount = 1;
		rp_info.pSubpasses = &subpass;

		VkRenderPass result;
		CHECK_RESULT(vkCreateRenderPass(dev, &rp_info, NULL, &result));

		g_renderpass_cache[renderpass_key] = result;
		return result;
	}

	void clear_renderpass_cache(VkDevice dev)
	{
		// Wipe current status
		g_cached_renderpass_key = 0;
		g_cached_renderpass = VK_NULL_HANDLE;
		g_current_renderpass.clear();

		// Destroy cache
		for (const auto &renderpass : g_renderpass_cache)
		{
			vkDestroyRenderPass(dev, renderpass.second, nullptr);
		}

		g_renderpass_cache.clear();
	}

	void begin_renderpass(const vk::command_buffer& cmd, VkRenderPass pass, VkFramebuffer target, const coordu& framebuffer_region)
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
		rp_begin.renderArea.offset.x = static_cast<s32>(framebuffer_region.x);
		rp_begin.renderArea.offset.y = static_cast<s32>(framebuffer_region.y);
		rp_begin.renderArea.extent.width = framebuffer_region.width;
		rp_begin.renderArea.extent.height = framebuffer_region.height;

		vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
		renderpass_info = { pass, target };
	}

	void begin_renderpass(VkDevice dev, const vk::command_buffer& cmd, u64 renderpass_key, VkFramebuffer target, const coordu& framebuffer_region)
	{
		if (renderpass_key != g_cached_renderpass_key)
		{
			g_cached_renderpass = get_renderpass(dev, renderpass_key);
			g_cached_renderpass_key = renderpass_key;
		}

		begin_renderpass(cmd, g_cached_renderpass, target, framebuffer_region);
	}

	void end_renderpass(const vk::command_buffer& cmd)
	{
		vkCmdEndRenderPass(cmd);
		g_current_renderpass[cmd] = {};
	}

	bool is_renderpass_open(const vk::command_buffer& cmd)
	{
		return g_current_renderpass[cmd].pass != VK_NULL_HANDLE;
	}

	void renderpass_op(const vk::command_buffer& cmd, const renderpass_op_callback_t& op)
	{
		const auto& active = g_current_renderpass[cmd];
		op(cmd, active.pass, active.fbo);
	}
}
