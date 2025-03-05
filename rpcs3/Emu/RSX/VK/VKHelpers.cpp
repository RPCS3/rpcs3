#include "stdafx.h"
#include "VKHelpers.h"
#include "VKGSRender.h"
#include "VKCompute.h"
#include "VKRenderPass.h"
#include "VKFramebuffer.h"
#include "VKResolveHelper.h"
#include "VKResourceManager.h"
#include "VKDMA.h"
#include "VKRenderPass.h"

#include "vkutils/scratch.h"
#include "vkutils/device.h"
#include <unordered_map>

namespace vk
{
	extern chip_class g_chip_class;

	std::unordered_map<u32, std::unique_ptr<vk::compute_task>> g_compute_tasks;
	std::unordered_map<u32, std::unique_ptr<vk::overlay_pass>> g_overlay_passes;

	rsx::atomic_bitmask_t<runtime_state, u64> g_runtime_state;

	// Driver compatibility workarounds
	VkFlags g_heap_compatible_buffer_types = 0;
	driver_vendor g_driver_vendor = driver_vendor::unknown;
	bool g_drv_no_primitive_restart = false;
	bool g_drv_sanitize_fp_values = false;
	bool g_drv_disable_fence_reset = false;
	bool g_drv_emulate_cond_render = false;
	bool g_drv_strict_query_scopes = false;
	bool g_drv_force_reuse_query_pools = false;

	u64 g_num_processed_frames = 0;
	u64 g_num_total_frames = 0;

	void reset_overlay_passes()
	{
		for (const auto& p : g_overlay_passes)
		{
			p.second->free_resources();
		}
	}

	void reset_global_resources()
	{
		// FIXME: These two shouldn't exist
		vk::reset_resolve_resources();
		vk::reset_overlay_passes();

		get_upload_heap()->reset_allocation_stats();
	}

	void destroy_global_resources()
	{
		VkDevice dev = *g_render_device;
		vk::clear_renderpass_cache(dev);
		vk::clear_framebuffer_cache();
		vk::clear_resolve_helpers();
		vk::clear_dma_resources();
		vk::clear_scratch_resources();

		vk::get_upload_heap()->destroy();

		g_compute_tasks.clear();

		for (const auto& p : g_overlay_passes)
		{
			p.second->destroy();
		}
		g_overlay_passes.clear();

		// This must be the last item destroyed
		vk::get_resource_manager()->destroy();

		// Statistics counter reset. Also verifies that everything was deleted.
		vk::vmm_reset();
	}

	const vk::render_device *get_current_renderer()
	{
		return g_render_device;
	}

	void set_current_renderer(const vk::render_device &device)
	{
		g_render_device = &device;
		g_runtime_state.clear();
		g_drv_no_primitive_restart = false;
		g_drv_sanitize_fp_values = false;
		g_drv_disable_fence_reset = false;
		g_drv_emulate_cond_render = (g_cfg.video.relaxed_zcull_sync && !g_render_device->get_conditional_render_support());
		g_num_processed_frames = 0;
		g_num_total_frames = 0;
		g_heap_compatible_buffer_types = 0;

		const auto& gpu = g_render_device->gpu();
		const auto gpu_name = gpu.get_name();

		g_driver_vendor = gpu.get_driver_vendor();
		g_chip_class = gpu.get_chip_class();

		switch (g_driver_vendor)
		{
		case driver_vendor::AMD:
			// Primitive restart on older GCN is still broken
			g_drv_no_primitive_restart = (g_chip_class == vk::chip_class::AMD_gcn_generic);
			break;
		case driver_vendor::RADV:
			// Previous bugs with fence reset and primitive restart seem to have been fixed with newer drivers
			break;
		case driver_vendor::NVIDIA:
			// Nvidia cards are easily susceptible to NaN poisoning
			g_drv_sanitize_fp_values = true;
			break;
		case driver_vendor::INTEL:
		case driver_vendor::ANV:
			// INTEL vulkan drivers are mostly OK, workarounds are applied when creating the device
			break;
		case driver_vendor::MVK:
			// Apple GPUs / moltenVK need more testing
			break;
		case driver_vendor::LAVAPIPE:
			// This software device works well, with poor performance as the only downside
			break;
		case driver_vendor::V3DV:
			// Broadcom GPUs need more testing, driver currently largely unstable
			break;
		case driver_vendor::DOZEN:
			// This driver is often picked by mistake when the user meant to select something else. Complain loudly.
#ifdef _WIN32
			MessageBox(NULL,
				L"You're attempting to run rpcs3 on Microsoft's Dozen driver that emulates vulkan on top of Direct3D12.\n"
				"This driver is unsupported. You should use your vendor's vulkan driver whenever possible.",
				L"Unsupported Driver",
				MB_ICONWARNING | MB_OK);
#else
			rsx_log.error("Dozen is currently unsupported. How did you even get this to run outside windows?");
#endif
			break;
		case driver_vendor::HONEYKRISP:
			// Needs more testing
			break;
		case driver_vendor::PANVK:
			// Needs more testing
			break;
		case driver_vendor::ARM_MALI:
			// Needs more testing
			break;
		default:
			rsx_log.warning("Unsupported device: %s", gpu_name);
		}

		rsx_log.notice("Vulkan: Renderer initialized on device '%s'", gpu_name);

		{
			// Buffer memory tests, only useful for portability on macOS
			VkBufferUsageFlags types[] =
			{
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
			};

			VkFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			VkBuffer tmp;
			VkMemoryRequirements memory_reqs;

			VkBufferCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			info.size = 4096;
			info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			info.flags = 0;

			for (const auto &usage : types)
			{
				info.usage = usage;
				CHECK_RESULT(vkCreateBuffer(*g_render_device, &info, nullptr, &tmp));

				vkGetBufferMemoryRequirements(*g_render_device, tmp, &memory_reqs);
				if (g_render_device->get_compatible_memory_type(memory_reqs.memoryTypeBits, memory_flags, nullptr))
				{
					g_heap_compatible_buffer_types |= usage;
				}

				vkDestroyBuffer(*g_render_device, tmp, nullptr);
			}
		}

		descriptors::init();
	}

	VkFlags get_heap_compatible_buffer_types()
	{
		return g_heap_compatible_buffer_types;
	}

	driver_vendor get_driver_vendor()
	{
		return g_driver_vendor;
	}

	bool emulate_primitive_restart(rsx::primitive_type type)
	{
		if (g_drv_no_primitive_restart)
		{
			switch (type)
			{
			case rsx::primitive_type::triangle_strip:
			case rsx::primitive_type::quad_strip:
				return true;
			default:
				break;
			}
		}

		return false;
	}

	bool sanitize_fp_values()
	{
		return g_drv_sanitize_fp_values;
	}

	bool fence_reset_disabled()
	{
		return g_drv_disable_fence_reset;
	}

	bool emulate_conditional_rendering()
	{
		return g_drv_emulate_cond_render;
	}

	bool use_strict_query_scopes()
	{
		return g_drv_strict_query_scopes;
	}

	bool force_reuse_query_pools()
	{
		return g_drv_force_reuse_query_pools;
	}

	void raise_status_interrupt(runtime_state status)
	{
		g_runtime_state |= status;
	}

	void clear_status_interrupt(runtime_state status)
	{
		g_runtime_state.clear(status);
	}

	bool test_status_interrupt(runtime_state status)
	{
		return g_runtime_state & status;
	}

	void enter_uninterruptible()
	{
		raise_status_interrupt(runtime_state::uninterruptible);
	}

	void leave_uninterruptible()
	{
		clear_status_interrupt(runtime_state::uninterruptible);
	}

	bool is_uninterruptible()
	{
		return test_status_interrupt(runtime_state::uninterruptible);
	}

	void advance_completed_frame_counter()
	{
		g_num_processed_frames++;
	}

	void advance_frame_counter()
	{
		ensure(g_num_processed_frames <= g_num_total_frames);
		g_num_total_frames++;
	}

	u64 get_current_frame_id()
	{
		return g_num_total_frames;
	}

	u64 get_last_completed_frame_id()
	{
		return (g_num_processed_frames > 0)? g_num_processed_frames - 1: 0;
	}

	void do_query_cleanup(vk::command_buffer& cmd)
	{
		auto renderer = dynamic_cast<VKGSRender*>(rsx::get_current_renderer());
		ensure(renderer);

		renderer->emergency_query_cleanup(&cmd);
	}

	void on_descriptor_pool_fragmentation(bool is_fatal)
	{
		if (auto vkthr = dynamic_cast<VKGSRender*>(rsx::get_current_renderer()))
		{
			vkthr->on_descriptor_pool_fragmentation(is_fatal);
		}
	}
}
