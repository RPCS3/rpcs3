// SPDX-License-Identifier: GPL-3.0-or-later
// PS5_RPCS3: MihawkGen's frame generation behind RPCS3's present path
// (VKFrameGenMihawk.h): the generator in mihawkgen/ over RPCS3's device, fed
// the game's frames and writing the frames between into RPCS3 images the
// present path shows as it does the built-in frame generation's. Built like
// RetroArch's integration in PS5_MihawkGen (integrations/retroarch): the last
// frame and this one kept as RGBA8, the network's method recorded for each
// time between them.

#include "stdafx.h"
#include "VKFrameGenMihawk.h"
#include "VKHelpers.h"
#include "vkutils/device.h"
#include "vkutils/image.h"

#include "mihawkgen/framegen.h"

#include <mutex>

// The network's weights, compiled in (rpcs3/Emu/CMakeLists.txt names the file
// and rebuilds this unit when it changes)
__asm__(
	".section .rodata\n"
	".balign 16\n"
	".globl ps5_mihawkgen_network\n"
	".hidden ps5_mihawkgen_network\n"
	"ps5_mihawkgen_network:\n"
	".incbin \"" PS5_MIHAWKGEN_NETWORK "\"\n"
	".globl ps5_mihawkgen_network_end\n"
	".hidden ps5_mihawkgen_network_end\n"
	"ps5_mihawkgen_network_end:\n"
	".previous\n");
extern "C" const u8 ps5_mihawkgen_network[];
extern "C" const u8 ps5_mihawkgen_network_end[];

namespace
{
	constexpr u32 c_max_frames = vk::mihawk_frame_generator::max_frames;

	std::mutex g_status_lock;
	std::string g_status = "not started";

	void set_status(std::string text)
	{
		std::lock_guard lock(g_status_lock);
		g_status = std::move(text);
	}

	std::unique_ptr<vk::viewable_image> make_image(u32 width, u32 height, VkImageUsageFlags usage)
	{
		const auto pdev = vk::get_current_renderer();
		auto image = std::make_unique<vk::viewable_image>(
			*pdev, pdev->get_memory_mapping().device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_TILING_OPTIMAL, usage,
			vk::VK_IMAGE_CREATE_ALLOW_NULL_RPCS3, vk::VMM_ALLOCATION_POOL_SWAPCHAIN, RSX_FORMAT_CLASS_COLOR);
		if (image->value == VK_NULL_HANDLE)
		{
			return nullptr;
		}
		return image;
	}

	VkImageView view_of(vk::viewable_image* image)
	{
		return image->get_view(rsx::default_remap_vector.with_encoding(vk::VK_REMAP_IDENTITY))->value;
	}

	void memory_barrier(const vk::command_buffer& cmd, VkPipelineStageFlags from, VkAccessFlags from_access, VkPipelineStageFlags to, VkAccessFlags to_access)
	{
		VkMemoryBarrier memory = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		memory.srcAccessMask = from_access;
		memory.dstAccessMask = to_access;
		vkCmdPipelineBarrier(cmd, from, to, 0, 1, &memory, 0, nullptr, 0, nullptr);
	}
}

namespace vk
{
	struct mihawk_frame_generator::impl
	{
		fg_context* ctx = nullptr;
		u32 width = 0;
		u32 height = 0;
		std::unique_ptr<vk::viewable_image> frames[2]; // the last frame and this one, RGBA8, in turn
		std::unique_ptr<vk::viewable_image> out[c_max_frames];
		u32 latest = 0;         // which of frames holds the newest
		bool have_prev = false;
		bool failed = false;

		void fail(const std::string& reason)
		{
			if (failed)
			{
				return;
			}
			failed = true;
			rsx_log.error("MihawkGen frame generation: %s; the built-in frame generation for the rest of the session", reason);
			set_status("failed: " + reason);
		}

		// Everything of the last size. The context's pipelines and images, and
		// these, may still be in use: a new size is rare (a resolution change)
		void dispose()
		{
			if (!ctx && !frames[0])
			{
				return;
			}
			vkDeviceWaitIdle(*vk::get_current_renderer());
			fg_context_destroy(ctx);
			ctx = nullptr;
			for (auto& image : frames) image.reset();
			for (auto& image : out) image.reset();
			width = height = 0;
			have_prev = false;
		}

		bool allocate(u32 w, u32 h)
		{
			dispose();
			const auto pdev = vk::get_current_renderer();
			fg_create_info info{};
			info.device = *pdev;
			info.physical_device = pdev->gpu();
			info.width = w;
			info.height = h;
			// One slot: the generator rotates its descriptor sets within it, and
			// each call here waits on the last (process' barriers)
			info.slots = 1;
			info.output_format = VK_FORMAT_R8G8B8A8_UNORM;
			if (const char* wrong = fg_check_create_info(&info))
			{
				fail(wrong);
				return false;
			}
			char error[256] = "";
			if (fg_context_create(&info, &ctx, error, sizeof(error)) != VK_SUCCESS)
			{
				fail(error);
				return false;
			}
			const usz bytes = static_cast<usz>(ps5_mihawkgen_network_end - ps5_mihawkgen_network);
			if (fg_context_load_network(ctx, ps5_mihawkgen_network, bytes, error, sizeof(error)) != VK_SUCCESS)
			{
				fail(error);
				return false;
			}
			for (auto& image : frames)
			{
				image = make_image(w, h, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
			}
			for (auto& image : out)
			{
				image = make_image(w, h, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
			}
			if (!frames[0] || !frames[1] || !out[0] || !out[1] || !out[2])
			{
				fail(fmt::format("no memory for its images at %ux%u", w, h));
				return false;
			}
			width = w;
			height = h;
			rsx_log.success("MihawkGen frame generation: %ux%u, a network of %u bytes", w, h, static_cast<u32>(bytes));
			set_status(fmt::format("running (%ux%u)", w, h));
			return true;
		}

		// The game's frame, its first width x height texels, into frames[index]
		// as RGBA8 (the same size: a format conversion, nothing scaled)
		void copy_input(const vk::command_buffer& cmd, vk::viewable_image* cur, u32 index)
		{
			vk::viewable_image* dst = frames[index].get();
			cur->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			dst->change_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			VkImageBlit region = {};
			region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			region.srcOffsets[1] = { static_cast<s32>(width), static_cast<s32>(height), 1 };
			region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			region.dstOffsets[1] = { static_cast<s32>(width), static_cast<s32>(height), 1 };
			vkCmdBlitImage(cmd, cur->value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);
			dst->change_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			cur->pop_layout(cmd);
		}
	};

	mihawk_frame_generator::mihawk_frame_generator()
		: m_impl(std::make_unique<impl>())
	{
	}

	mihawk_frame_generator::~mihawk_frame_generator()
	{
		m_impl->dispose();
	}

	bool mihawk_frame_generator::failed() const
	{
		return m_impl->failed;
	}

	void mihawk_frame_generator::reset()
	{
		m_impl->have_prev = false;
	}

	u32 mihawk_frame_generator::process(const vk::command_buffer& cmd, vk::viewable_image* cur, u32 width, u32 height, u32 count,
		vk::viewable_image* (&out)[max_frames])
	{
		auto& m = *m_impl;
		if (m.failed || !cur || !width || !height)
		{
			return 0;
		}
		width = std::min<u32>(width, cur->width());
		height = std::min<u32>(height, cur->height());
		count = std::min(count, max_frames);

		if (width != m.width || height != m.height)
		{
			if (!m.allocate(width, height))
			{
				m.dispose();
				return 0;
			}
		}

		// What the last frame's work and presentation did with these images is done
		memory_barrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT);

		const u32 next = m.have_prev ? (m.latest ^ 1) : m.latest;
		m.copy_input(cmd, cur, next);
		if (!m.have_prev || !count)
		{
			m.latest = next;
			m.have_prev = true;
			return 0;
		}

		fg_frame_pair pair{};
		pair.prev_color = view_of(m.frames[m.latest].get());
		pair.next_color = view_of(m.frames[next].get());
		// (the neural method reads the colour alone; its other inputs are bound
		// to the frames so that every binding names a valid view)
		pair.prev_depth = pair.prev_color;
		pair.next_depth = pair.next_color;
		pair.next_motion = pair.next_color;
		for (u32 k = 0; k < count; k++)
		{
			if (k)
			{
				// The last call's reads of the generator's images done before this one writes them
				memory_barrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
			}
			m.out[k]->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
			pair.t = static_cast<f32>(k + 1) / static_cast<f32>(count + 1);
			fg_record_generate(m.ctx, cmd, 0, FG_METHOD_NEURAL, &pair, view_of(m.out[k].get()));
			out[k] = m.out[k].get();
		}

		// The frames between, written, for what reads them next
		memory_barrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT);
		m.latest = next;
		return count;
	}

	std::string mihawk_status()
	{
		std::lock_guard lock(g_status_lock);
		return g_status;
	}
}
