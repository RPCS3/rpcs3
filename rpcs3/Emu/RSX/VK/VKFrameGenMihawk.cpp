// SPDX-License-Identifier: GPL-3.0-or-later
// PS5_RPCS3: MihawkGen's frame generation behind RPCS3's present path
// (VKFrameGenMihawk.h): the generator in mihawkgen/ over RPCS3's device, fed
// the game's frames and writing the frames between into RPCS3 images the
// present path shows as it does the built-in frame generation's. Built like
// RetroArch's integration in PS5_MihawkGen (integrations/retroarch): the last
// frame and this one kept as RGBA8, the network's method recorded for each
// time between them.
//
// The motion assist (build 146). On frames saved on my console in GTA IV's
// quick moves (the game at 20 to 30 fps, build 145) the network doubled cars,
// shadows and the running player: things moved 35 to 50 of the built-in's 640
// working texels between two game frames, past the network's reach, and it fell
// back to the frames' blend. So the network also runs on the two frames carried
// to the time of the frame between along the built-in frame generation's block
// vectors, averaged over 15x15 blocks (mihawkgen/assist/smooth.comp,
// prewarp.comp), which leaves it the rest of the motion; and each pixel is the
// run whose two reads of the frames agree better about there (disagree.comp,
// combine.comp), with the HUD's own pixels the frames' blend. In a numpy model
// of the network's shaders (its frames within 1.4 to 2.2 levels in 255 of the
// console's), on those four moments, the pre-aligned run was taken for 67 to
// 71% of the picture, the disagreement of the reads went from 7 to 10 levels to
// 3 to 5, and the cars, shadows, railings and the player came out single; the
// road's repeating shadows kept some tears. It costs a second run of the
// network for each frame made.

#include "stdafx.h"
#include "VKFrameGenMihawk.h"
#include "VKFrameGen.h"
#include "VKHelpers.h"
#include "vkutils/device.h"
#include "vkutils/image.h"

#include "mihawkgen/framegen.h"
#include "mihawkgen/assist/smooth.comp.h"
#include "mihawkgen/assist/prewarp.comp.h"
#include "mihawkgen/assist/disagree.comp.h"
#include "mihawkgen/assist/combine.comp.h"

#include <mutex>
#include <utility>

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
	constexpr u32 c_ring = 16;           // descriptor sets a pass rotates through: one the GPU may still read is never rewritten
	constexpr s32 c_smooth_radius = 7;   // the vectors averaged over 15x15 blocks

	std::mutex g_status_lock;
	std::string g_status = "not started";

	void set_status(std::string text)
	{
		std::lock_guard lock(g_status_lock);
		g_status = std::move(text);
	}

	std::unique_ptr<vk::viewable_image> make_image(u32 width, u32 height, VkFormat format, VkImageUsageFlags usage)
	{
		const auto pdev = vk::get_current_renderer();
		auto image = std::make_unique<vk::viewable_image>(
			*pdev, pdev->get_memory_mapping().device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_TYPE_2D, format, width, height, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT,
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

	void compute_barrier(const vk::command_buffer& cmd)
	{
		memory_barrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
	}

	// One of the assist's compute passes: `samplers` combined image samplers,
	// then `storages` storage images, and up to 64 bytes of push constants
	struct assist_pass
	{
		VkDevice dev = VK_NULL_HANDLE;
		VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
		VkPipelineLayout layout = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkDescriptorSet sets[c_ring]{};
		u32 next = 0;
		u32 samplers = 0;
		u32 storages = 0;

		bool create(VkDevice device, VkDescriptorPool pool, const u32* code, usz bytes, u32 sampler_count, u32 storage_count)
		{
			dev = device;
			samplers = sampler_count;
			storages = storage_count;
			VkDescriptorSetLayoutBinding bindings[8]{};
			for (u32 i = 0; i < samplers + storages; i++)
			{
				bindings[i].binding = i;
				bindings[i].descriptorType = i < samplers ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				bindings[i].descriptorCount = 1;
				bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			}
			VkDescriptorSetLayoutCreateInfo set_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
			set_info.bindingCount = samplers + storages;
			set_info.pBindings = bindings;
			if (vkCreateDescriptorSetLayout(dev, &set_info, nullptr, &set_layout) != VK_SUCCESS)
			{
				return false;
			}
			const VkPushConstantRange push = { VK_SHADER_STAGE_COMPUTE_BIT, 0, 64 };
			VkPipelineLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
			layout_info.setLayoutCount = 1;
			layout_info.pSetLayouts = &set_layout;
			layout_info.pushConstantRangeCount = 1;
			layout_info.pPushConstantRanges = &push;
			if (vkCreatePipelineLayout(dev, &layout_info, nullptr, &layout) != VK_SUCCESS)
			{
				return false;
			}
			VkShaderModuleCreateInfo module_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
			module_info.codeSize = bytes;
			module_info.pCode = code;
			VkShaderModule module = VK_NULL_HANDLE;
			if (vkCreateShaderModule(dev, &module_info, nullptr, &module) != VK_SUCCESS)
			{
				return false;
			}
			VkComputePipelineCreateInfo pipeline_info = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
			pipeline_info.stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
			pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			pipeline_info.stage.module = module;
			pipeline_info.stage.pName = "main";
			pipeline_info.layout = layout;
			const VkResult made = vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline);
			vkDestroyShaderModule(dev, module, nullptr);
			if (made != VK_SUCCESS)
			{
				return false;
			}
			VkDescriptorSetLayout layouts[c_ring];
			std::fill_n(layouts, c_ring, set_layout);
			VkDescriptorSetAllocateInfo alloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
			alloc.descriptorPool = pool;
			alloc.descriptorSetCount = c_ring;
			alloc.pSetLayouts = layouts;
			return vkAllocateDescriptorSets(dev, &alloc, sets) == VK_SUCCESS;
		}

		void destroy()
		{
			if (pipeline) vkDestroyPipeline(dev, pipeline, nullptr);
			if (layout) vkDestroyPipelineLayout(dev, layout, nullptr);
			if (set_layout) vkDestroyDescriptorSetLayout(dev, set_layout, nullptr);
			pipeline = VK_NULL_HANDLE;
			layout = VK_NULL_HANDLE;
			set_layout = VK_NULL_HANDLE;
		}

		// The sampled views with their layouts, then the storage views (GENERAL)
		void run(const vk::command_buffer& cmd, VkSampler sampler, std::initializer_list<std::pair<VkImageView, VkImageLayout>> sampled,
			std::initializer_list<VkImageView> storage, const void* push, u32 push_size, u32 width, u32 height)
		{
			const VkDescriptorSet set = sets[next];
			next = (next + 1) % c_ring;
			VkDescriptorImageInfo infos[8]{};
			VkWriteDescriptorSet writes[8]{};
			u32 i = 0;
			for (const auto& [view, layout_of] : sampled)
			{
				infos[i] = { sampler, view, layout_of };
				writes[i] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				writes[i].dstSet = set;
				writes[i].dstBinding = i;
				writes[i].descriptorCount = 1;
				writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writes[i].pImageInfo = &infos[i];
				i++;
			}
			for (const VkImageView view : storage)
			{
				infos[i] = { VK_NULL_HANDLE, view, VK_IMAGE_LAYOUT_GENERAL };
				writes[i] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				writes[i].dstSet = set;
				writes[i].dstBinding = i;
				writes[i].descriptorCount = 1;
				writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				writes[i].pImageInfo = &infos[i];
				i++;
			}
			vkUpdateDescriptorSets(dev, i, writes, 0, nullptr);
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &set, 0, nullptr);
			vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, push_size, push);
			vkCmdDispatch(cmd, utils::aligned_div(width, 8u), utils::aligned_div(height, 8u), 1);
		}
	};
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

		// The motion assist's own: its passes, the pre-aligned frames, the
		// network's two frames, the reads' disagreement and the smoothed vectors
		bool assist_ready = false;
		bool assist_failed = false;
		VkDescriptorPool pool = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
		assist_pass smooth, prewarp, disagree, combine;
		std::unique_ptr<vk::viewable_image> aligned[2];
		std::unique_ptr<vk::viewable_image> made[2];   // the network on the frames as they are, and on the pre-aligned ones
		std::unique_ptr<vk::viewable_image> disagreement;
		std::unique_ptr<vk::viewable_image> smoothed;

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

		void dispose_assist()
		{
			for (assist_pass* p : { &smooth, &prewarp, &disagree, &combine }) p->destroy();
			const VkDevice dev = *vk::get_current_renderer();
			if (pool) vkDestroyDescriptorPool(dev, pool, nullptr);
			if (sampler) vkDestroySampler(dev, sampler, nullptr);
			pool = VK_NULL_HANDLE;
			sampler = VK_NULL_HANDLE;
			for (auto& image : aligned) image.reset();
			for (auto& image : made) image.reset();
			disagreement.reset();
			smoothed.reset();
			assist_ready = false;
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
			dispose_assist();
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
			// Two slots: the frames as they are (0) and pre-aligned (1), whose
			// flows the assist reads after both runs. The generator rotates its
			// descriptor sets within a slot, and each call here waits on the last
			// (process' barriers)
			info.slots = 2;
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
				image = make_image(w, h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
			}
			for (auto& image : out)
			{
				image = make_image(w, h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
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

		// The assist's pipelines and images, at the first frame that asks for
		// it; a failure leaves the network on the frames as they are
		bool prepare_assist(const frame_generation_motion& motion)
		{
			const u32 grid_w = motion.vectors->width();
			const u32 grid_h = motion.vectors->height();
			if (assist_ready && smoothed && smoothed->width() == grid_w && smoothed->height() == grid_h)
			{
				return true;
			}
			if (assist_failed)
			{
				return false;
			}
			if (assist_ready)
			{
				// (a new working size of the built-in's: only the smoothed vectors change)
				vkDeviceWaitIdle(*vk::get_current_renderer());
				smoothed = make_image(grid_w, grid_h, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				return !!smoothed;
			}
			const auto fail_assist = [&](const char* what)
			{
				rsx_log.error("MihawkGen motion assist: %s; MihawkGen on the frames as they are", what);
				assist_failed = true;
				dispose_assist();
				return false;
			};
			const VkDevice dev = *vk::get_current_renderer();
			const VkDescriptorPoolSize sizes[2] = {
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * c_ring * 8 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4 * c_ring * 2 } };
			VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
			pool_info.maxSets = 4 * c_ring;
			pool_info.poolSizeCount = 2;
			pool_info.pPoolSizes = sizes;
			if (vkCreateDescriptorPool(dev, &pool_info, nullptr, &pool) != VK_SUCCESS)
			{
				return fail_assist("no descriptor pool");
			}
			VkSamplerCreateInfo sampler_info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
			sampler_info.magFilter = VK_FILTER_LINEAR;
			sampler_info.minFilter = VK_FILTER_LINEAR;
			sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			if (vkCreateSampler(dev, &sampler_info, nullptr, &sampler) != VK_SUCCESS)
			{
				return fail_assist("no sampler");
			}
			if (!smooth.create(dev, pool, ps5_mg_smooth, sizeof(ps5_mg_smooth), 1, 1) ||
				!prewarp.create(dev, pool, ps5_mg_prewarp, sizeof(ps5_mg_prewarp), 3, 2) ||
				!disagree.create(dev, pool, ps5_mg_disagree, sizeof(ps5_mg_disagree), 6, 1) ||
				!combine.create(dev, pool, ps5_mg_combine, sizeof(ps5_mg_combine), 6, 1))
			{
				return fail_assist("its pipelines could not be made");
			}
			u32 flow_w = 0, flow_h = 0, work_w = 0, work_h = 0;
			f32 scale = 0;
			if (!fg_context_neural_flow(ctx, 0, &flow_w, &flow_h, &scale, &work_w, &work_h))
			{
				return fail_assist("the network's flow is not there");
			}
			const VkImageUsageFlags rw = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			for (auto& image : aligned) image = make_image(width, height, VK_FORMAT_R8G8B8A8_UNORM, rw);
			for (auto& image : made) image = make_image(width, height, VK_FORMAT_R8G8B8A8_UNORM, rw);
			disagreement = make_image(flow_w, flow_h, VK_FORMAT_R16G16B16A16_SFLOAT, rw);
			smoothed = make_image(grid_w, grid_h, VK_FORMAT_R16G16B16A16_SFLOAT, rw);
			if (!aligned[0] || !aligned[1] || !made[0] || !made[1] || !disagreement || !smoothed)
			{
				return fail_assist("no memory for its images");
			}
			assist_ready = true;
			rsx_log.success("MihawkGen motion assist: ready (flow %ux%u, the built-in's vectors %ux%u)", flow_w, flow_h, grid_w, grid_h);
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

		// One frame between at time t with the assist, into out[k]
		void assisted(const vk::command_buffer& cmd, const frame_generation_motion& motion, vk::viewable_image* last, vk::viewable_image* next,
			f32 t, vk::viewable_image* target)
		{
			constexpr VkImageLayout ro = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			constexpr VkImageLayout gen = VK_IMAGE_LAYOUT_GENERAL;
			const s32 size[2] = { static_cast<s32>(width), static_cast<s32>(height) };

			// The frames carried to t along the smoothed vectors
			for (auto& image : aligned) image->change_layout(cmd, gen);
			struct { s32 w, h; f32 work_w, work_h, grid_x, grid_y, t; } pp{ size[0], size[1], motion.work_w, motion.work_h, motion.grid_x, motion.grid_y, t };
			prewarp.run(cmd, sampler, { { view_of(last), ro }, { view_of(next), ro }, { view_of(smoothed.get()), gen } },
				{ view_of(aligned[0].get()), view_of(aligned[1].get()) }, &pp, sizeof(pp), width, height);
			compute_barrier(cmd);
			for (auto& image : aligned) image->change_layout(cmd, ro);

			// The network on the frames as they are (slot 0) and pre-aligned (slot 1)
			fg_frame_pair pair{};
			pair.t = t;
			pair.prev_color = view_of(last);
			pair.next_color = view_of(next);
			pair.prev_depth = pair.prev_color;
			pair.next_depth = pair.next_color;
			pair.next_motion = pair.next_color;
			for (auto& image : made) image->change_layout(cmd, gen);
			fg_record_generate(ctx, cmd, 0, FG_METHOD_NEURAL, &pair, view_of(made[0].get()));
			compute_barrier(cmd);
			pair.prev_color = view_of(aligned[0].get());
			pair.next_color = view_of(aligned[1].get());
			pair.prev_depth = pair.prev_color;
			pair.next_depth = pair.next_color;
			pair.next_motion = pair.next_color;
			fg_record_generate(ctx, cmd, 1, FG_METHOD_NEURAL, &pair, view_of(made[1].get()));
			compute_barrier(cmd);

			// How far each run's reads disagree, at the flow's size
			u32 flow_w = 0, flow_h = 0, work_w = 0, work_h = 0;
			f32 scale = 0;
			const VkImageView flow_raw = fg_context_neural_flow(ctx, 0, &flow_w, &flow_h, &scale, &work_w, &work_h);
			const VkImageView flow_aligned = fg_context_neural_flow(ctx, 1, &flow_w, &flow_h, &scale, &work_w, &work_h);
			disagreement->change_layout(cmd, gen);
			struct { s32 w, h; f32 work_w, work_h, scale; } dp{ static_cast<s32>(flow_w), static_cast<s32>(flow_h), static_cast<f32>(work_w), static_cast<f32>(work_h), scale };
			disagree.run(cmd, sampler, { { view_of(last), ro }, { view_of(next), ro }, { view_of(aligned[0].get()), ro }, { view_of(aligned[1].get()), ro },
				{ flow_raw, gen }, { flow_aligned, gen } }, { view_of(disagreement.get()) }, &dp, sizeof(dp), flow_w, flow_h);
			compute_barrier(cmd);

			// Each pixel the better run's, the HUD's own the frames' blend
			target->change_layout(cmd, gen);
			combine.run(cmd, sampler, { { view_of(last), ro }, { view_of(next), ro }, { view_of(made[0].get()), gen }, { view_of(made[1].get()), gen },
				{ view_of(disagreement.get()), gen }, { view_of(motion.vectors), gen } }, { view_of(target) }, &pp, sizeof(pp), width, height);
			compute_barrier(cmd);
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
		vk::viewable_image* (&out)[max_frames], const frame_generation_motion* motion, bool assist)
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

		vk::viewable_image* const last_frame = m.frames[m.latest].get();
		vk::viewable_image* const next_frame = m.frames[next].get();
		const bool assisted = assist && motion && motion->vectors && m.prepare_assist(*motion);
		if (assisted)
		{
			// The built-in's vectors, averaged, once for all the frames between
			m.smoothed->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
			struct { s32 w, h, radius; } sp{ static_cast<s32>(motion->vectors->width()), static_cast<s32>(motion->vectors->height()), c_smooth_radius };
			m.smooth.run(cmd, m.sampler, { { view_of(motion->vectors), VK_IMAGE_LAYOUT_GENERAL } }, { view_of(m.smoothed.get()) }, &sp, sizeof(sp),
				motion->vectors->width(), motion->vectors->height());
			compute_barrier(cmd);
		}

		fg_frame_pair pair{};
		pair.prev_color = view_of(last_frame);
		pair.next_color = view_of(next_frame);
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
				compute_barrier(cmd);
			}
			const f32 t = static_cast<f32>(k + 1) / static_cast<f32>(count + 1);
			if (assisted)
			{
				m.assisted(cmd, *motion, last_frame, next_frame, t, m.out[k].get());
			}
			else
			{
				m.out[k]->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
				pair.t = t;
				fg_record_generate(m.ctx, cmd, 0, FG_METHOD_NEURAL, &pair, view_of(m.out[k].get()));
			}
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
