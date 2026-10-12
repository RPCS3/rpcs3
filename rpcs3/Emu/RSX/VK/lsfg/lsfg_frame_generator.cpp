// SPDX-License-Identifier: GPL-3.0-or-later
// PS5_RPCS3: Lossless Scaling's frame generation behind RPCS3's present path
// (VKFrameGenLSFG.h): the Eden/lsfg-vk compute chain in this folder over
// RPCS3's device, fed the game's frames and writing the frames between into
// RPCS3 images the present path shows as it does the built-in frame
// generation's. Adapted from the Simpsons Game PS5 port's
// vulkan_framegen_prospero.cpp (README.md). This file and the chain are built
// with exceptions (rpcs3/Emu/CMakeLists.txt), which the chain uses for its
// errors; none leaves process(): a failure turns LSFG off for the session.

#include "stdafx.h"
#include "../VKFrameGenLSFG.h"
#include "../VKHelpers.h"
#include "../vkutils/device.h"
#include "../vkutils/image.h"
#include "Utilities/File.h"

#include "lsfg_chain.h"
#include "lsfg_shaders.h"
#include "vulkan_adapter.h"

#include <deque>
#include <exception>
#include <mutex>
#include <stdexcept>

namespace vk
{
	ps5_lsfg_device_features g_ps5_lsfg_device_features;
}

namespace
{
	namespace fg = rex::ui::vulkan::framegen;

	constexpr u32 c_max_frames = vk::lsfg_frame_generator::max_frames;

	// The chain's own memory, at most, as in the Simpsons Game port
	constexpr VkDeviceSize c_memory_budget = VkDeviceSize(512) << 20;

	// A chain replaced by one of a new size is kept this many frames after its
	// last use, by when the GPU is done with it
	constexpr u64 c_retire_frames = 8;

	std::mutex g_status_lock;
	std::string g_status = "not started";

	void set_status(std::string text)
	{
		std::lock_guard lock(g_status_lock);
		g_status = std::move(text);
	}

	struct image_view_holder
	{
		VkDevice dev = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;

		image_view_holder() = default;
		image_view_holder(const image_view_holder&) = delete;
		image_view_holder& operator=(const image_view_holder&) = delete;

		~image_view_holder()
		{
			if (view)
			{
				vkDestroyImageView(dev, view, nullptr);
			}
		}
	};

	// Everything of one size: the chain and its memory, and the frames between
	// as RPCS3 images, with the views the chain writes them through
	struct sized_resources
	{
		VkExtent2D extent;
		fg::MemoryAllocator allocator; // before the chain: its images' memory outlives them
		std::unique_ptr<fg::LsfgChain> chain;
		std::unique_ptr<vk::viewable_image> out[c_max_frames];
		image_view_holder views[c_max_frames]; // after the images: destroyed before them
		bool initialized = false;

		sized_resources(const fg::Device& device, const fg::LsfgShaders& shaders, VkExtent2D size)
			: extent(size)
			, allocator(device, c_memory_budget)
		{
			// Optical flow at the frame's own size (flow scale 1), as the port runs it
			chain = std::make_unique<fg::LsfgChain>(device, allocator, shaders, size, fg::LSFG_DEFAULT_FORMAT, 1.0f);

			const auto pdev = vk::get_current_renderer();
			for (u32 i = 0; i < c_max_frames; i++)
			{
				out[i] = std::make_unique<vk::viewable_image>(
					*pdev, pdev->get_memory_mapping().device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					VK_IMAGE_TYPE_2D, fg::LSFG_DEFAULT_FORMAT, size.width, size.height, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
					vk::VK_IMAGE_CREATE_ALLOW_NULL_RPCS3, vk::VMM_ALLOCATION_POOL_SWAPCHAIN, RSX_FORMAT_CLASS_COLOR);
				if (out[i]->value == VK_NULL_HANDLE)
				{
					throw std::runtime_error("no memory for the frames between");
				}

				VkImageViewCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
				info.image = out[i]->value;
				info.viewType = VK_IMAGE_VIEW_TYPE_2D;
				info.format = fg::LSFG_DEFAULT_FORMAT;
				info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
				views[i].dev = *pdev;
				fg::Check(vkCreateImageView(*pdev, &info, nullptr, &views[i].view), "vkCreateImageView");
			}

			// Every count's targets before any submission refers to their descriptors
			for (u32 count = 1; count <= c_max_frames; count++)
			{
				for (u32 index = 0; index < count; index++)
				{
					chain->SetTarget(device, count, index, index, views[index].view);
				}
			}
		}
	};

	void memory_barrier(const vk::command_buffer& cmd, VkPipelineStageFlags from, VkAccessFlags from_access, VkPipelineStageFlags to, VkAccessFlags to_access)
	{
		VkMemoryBarrier memory = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		memory.srcAccessMask = from_access;
		memory.dstAccessMask = to_access;
		vkCmdPipelineBarrier(cmd, from, to, 0, 1, &memory, 0, nullptr, 0, nullptr);
	}

	VkImageMemoryBarrier image_barrier(VkImage image, VkImageLayout from, VkImageLayout to, VkAccessFlags from_access, VkAccessFlags to_access)
	{
		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.srcAccessMask = from_access;
		barrier.dstAccessMask = to_access;
		barrier.oldLayout = from;
		barrier.newLayout = to;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		return barrier;
	}

	// The game's frame into the chain's input: its first width x height texels,
	// as RGBA8 (the same size: a format conversion, nothing scaled)
	void copy_input(const vk::command_buffer& cmd, vk::viewable_image* cur, u32 width, u32 height, fg::LsfgImage& input)
	{
		cur->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		const VkImageMemoryBarrier before = image_barrier(input.Handle(), input.Layout(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &before);

		VkImageBlit region = {};
		region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		region.srcOffsets[1] = { static_cast<s32>(width), static_cast<s32>(height), 1 };
		region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		region.dstOffsets[1] = { static_cast<s32>(input.Extent().width), static_cast<s32>(input.Extent().height), 1 };
		vkCmdBlitImage(cmd, cur->value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, input.Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);

		const VkImageMemoryBarrier after = image_barrier(input.Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &after);
		input.SetLayout(VK_IMAGE_LAYOUT_GENERAL);

		cur->pop_layout(cmd);
	}
}

namespace vk
{
	struct lsfg_frame_generator::impl
	{
		std::unique_ptr<rex::ui::vulkan::VulkanDevice> native;
		std::unique_ptr<fg::Device> device;
		std::unique_ptr<fg::LsfgShaders> shaders;
		std::unique_ptr<sized_resources> sized;
		std::deque<std::pair<u64, std::unique_ptr<sized_resources>>> retired;
		u64 frames = 0;      // calls to process
		u64 input_count = 0; // frames into the chain since its start
		u32 warm = 0;        // shared passes since then
		bool failed = false;

		void fail(const std::string& reason)
		{
			if (failed)
			{
				return;
			}
			failed = true;
			rsx_log.error("Lossless Scaling frame generation: %s; the built-in frame generation for the rest of the session", reason);
			set_status("failed: " + reason);
		}

		void check_formats(VkFormat source_format) const
		{
			const VkPhysicalDevice gpu = native->physical_device();
			const VkFormatFeatureFlags compute = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
				VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
			for (const VkFormat format : { fg::LSFG_DEFAULT_FORMAT, fg::LSFG_FLOW_FORMAT, fg::LSFG_MOTION_FORMAT })
			{
				VkFormatProperties properties{};
				vkGetPhysicalDeviceFormatProperties(gpu, format, &properties);
				const VkFormatFeatureFlags required = compute | (format == fg::LSFG_DEFAULT_FORMAT ? VK_FORMAT_FEATURE_BLIT_DST_BIT : 0);
				if ((properties.optimalTilingFeatures & required) != required)
				{
					throw std::runtime_error(fmt::format("format %d lacks the compute, sampling or transfer support the chain needs", static_cast<int>(format)));
				}
			}
			VkFormatProperties source{};
			vkGetPhysicalDeviceFormatProperties(gpu, source_format, &source);
			if (!(source.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
			{
				throw std::runtime_error(fmt::format("the game's frames (format %d) cannot be copied into the chain", static_cast<int>(source_format)));
			}
		}

		void start(VkFormat source_format)
		{
			const std::string path = lsfg_dll_path();
			if (path.empty())
			{
				throw std::runtime_error("no Lossless.dll in rpcs3/lossless/ or rpcs3/");
			}

			const auto& f = g_ps5_lsfg_device_features;
			rex::ui::vulkan::VulkanDeviceProperties properties;
			properties.apiVersion = f.api_version;
			properties.vulkanMemoryModel = f.memory_model;
			properties.vulkanMemoryModelDeviceScope = f.memory_model_device_scope;
			properties.nullDescriptor = f.null_descriptor;
			properties.shaderFloat16 = f.float16;
			properties.shaderStorageImageExtendedFormats = f.storage_extended_formats;
			properties.shaderStorageImageReadWithoutFormat = f.storage_read_without_format;
			properties.shaderStorageImageWriteWithoutFormat = f.storage_write_without_format;

			const auto pdev = vk::get_current_renderer();
			native = std::make_unique<rex::ui::vulkan::VulkanDevice>(*pdev, pdev->gpu(), properties);
			device = std::make_unique<fg::Device>(*native);
			check_formats(source_format);
			shaders = std::make_unique<fg::LsfgShaders>(*device, path);

			const char* variant = shaders->Variant() == fg::ShaderVariant::NativeFp16 ? "16-bit" : "32-bit";
			rsx_log.success("Lossless Scaling frame generation: %s shaders from %s (%u bytes)", variant, path, shaders->SourceBytes());
			set_status(fmt::format("running (%s shaders)", variant));
		}
	};

	lsfg_frame_generator::lsfg_frame_generator()
		: m_impl(std::make_unique<impl>())
	{
	}

	lsfg_frame_generator::~lsfg_frame_generator() = default;

	bool lsfg_frame_generator::failed() const
	{
		return m_impl->failed;
	}

	void lsfg_frame_generator::reset()
	{
		// The images and their layouts stay: earlier work may still use them.
		// Three new frames through the shared passes replace the old history
		m_impl->input_count = 0;
		m_impl->warm = 0;
	}

	u32 lsfg_frame_generator::process(const vk::command_buffer& cmd, vk::viewable_image* cur, u32 width, u32 height, u32 count,
		vk::viewable_image* (&out)[max_frames])
	{
		auto& m = *m_impl;
		if (m.failed || !cur || !width || !height)
		{
			return 0;
		}

		m.frames++;
		while (!m.retired.empty() && m.frames - m.retired.front().first > c_retire_frames)
		{
			m.retired.pop_front();
		}

		width = std::min<u32>(width, cur->width());
		height = std::min<u32>(height, cur->height());
		count = std::min(count, max_frames);

		// Everything allocated and written before the first command: a failure
		// here leaves the command buffer as it was
		try
		{
			if (!m.shaders)
			{
				m.start(cur->format());
			}
			if (!m.sized || m.sized->extent.width != width || m.sized->extent.height != height)
			{
				if (width > 3840 || height > 2160)
				{
					throw std::runtime_error(fmt::format("a frame of %ux%u, larger than the chain takes (3840x2160)", width, height));
				}
				if (m.sized)
				{
					m.retired.emplace_back(m.frames, std::move(m.sized));
				}
				m.sized = std::make_unique<sized_resources>(*m.device, *m.shaders, VkExtent2D{ width, height });
				m.input_count = 0;
				m.warm = 0;
				rsx_log.notice("Lossless Scaling frame generation: %ux%u, %u KiB of images in %u allocations", width, height,
					m.sized->allocator.Budget().used / 1024, m.sized->allocator.Budget().count);
			}
		}
		catch (const std::exception& e)
		{
			m.fail(e.what());
			return 0;
		}

		auto& r = *m.sized;
		u32 made = 0;
		try
		{
			const fg::vk::CommandBuffer commands(*m.native, cmd);

			// What the last frame's presentation read of these images is done
			memory_barrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT);

			if (!r.initialized)
			{
				r.allocator.InitializeImages(commands);
				for (auto& image : r.out)
				{
					image->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
				}
				r.initialized = true;
			}

			const u64 index = m.input_count++;
			copy_input(cmd, cur, width, height, r.chain->Input(index));
			if (m.input_count < 2)
			{
				return 0;
			}

			r.chain->DispatchShared(commands, index);
			if (++m.warm < fg::LSFG_HISTORY_SLOTS || !count)
			{
				return 0;
			}

			for (u32 k = 0; k < count; k++)
			{
				r.out[k]->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
				r.chain->DispatchGeneration(commands, index, count, k, k, r.out[k]->value, r.extent);
				out[k] = r.out[k].get();
			}
			made = count;

			// The frames between, written, for the presentation to read
			memory_barrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT);
		}
		catch (const std::exception& e)
		{
			// The chain's images are never used again; RPCS3's own are in the
			// layouts it tracks
			m.fail(e.what());
			return 0;
		}
		return made;
	}

	std::string lsfg_dll_path()
	{
		for (const std::string& path : { fs::get_config_dir() + "lossless/Lossless.dll", fs::get_config_dir() + "Lossless.dll" })
		{
			if (fs::is_file(path))
			{
				return path;
			}
		}
		return {};
	}

	std::string lsfg_status()
	{
		std::lock_guard lock(g_status_lock);
		return g_status;
	}
}
