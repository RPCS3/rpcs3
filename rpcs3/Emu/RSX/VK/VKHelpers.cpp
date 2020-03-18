#include "stdafx.h"
#include "VKHelpers.h"
#include "VKGSRender.h"
#include "VKCompute.h"
#include "VKRenderPass.h"
#include "VKFramebuffer.h"
#include "VKResolveHelper.h"
#include "VKResourceManager.h"
#include "VKDMA.h"
#include "VKCommandStream.h"
#include "VKRenderPass.h"

#include "Utilities/mutex.h"
#include "Utilities/lockless.h"

namespace vk
{
	static chip_family_table s_AMD_family_tree = []()
	{
		chip_family_table table;
		table.default_ = chip_class::AMD_gcn_generic;

		// AMD cards. See https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/amd/amdgpu/amdgpu_drv.c
		table.add(0x67C0, 0x67FF, chip_class::AMD_polaris);
		table.add(0x6FDF, chip_class::AMD_polaris); // RX580 2048SP
		table.add(0x6980, 0x699F, chip_class::AMD_polaris); // Polaris12
		table.add(0x694C, 0x694F, chip_class::AMD_vega); // VegaM
		table.add(0x6860, 0x686F, chip_class::AMD_vega); // VegaPro
		table.add(0x687F, chip_class::AMD_vega); // Vega56/64
		table.add(0x69A0, 0x69AF, chip_class::AMD_vega); // Vega12
		table.add(0x66A0, 0x66AF, chip_class::AMD_vega); // Vega20
		table.add(0x15DD, chip_class::AMD_vega); // Raven Ridge
		table.add(0x15D8, chip_class::AMD_vega); // Raven Ridge
		table.add(0x7310, 0x731F, chip_class::AMD_navi); // Navi10
		table.add(0x7340, 0x7340, chip_class::AMD_navi); // Navi14

		return table;
	}();

	static chip_family_table s_NV_family_tree = []()
	{
		chip_family_table table;
		table.default_ = chip_class::NV_generic;

		// NV cards. See https://envytools.readthedocs.io/en/latest/hw/pciid.html
		// NOTE: Since NV device IDs are linearly incremented per generation, there is no need to carefully check all the ranges
		table.add(0x1180, 0x11fa, chip_class::NV_kepler); // GK104, 106
		table.add(0x0FC0, 0x0FFF, chip_class::NV_kepler); // GK107
		table.add(0x1003, 0x1028, chip_class::NV_kepler); // GK110
		table.add(0x1280, 0x12BA, chip_class::NV_kepler); // GK208
		table.add(0x1381, 0x13B0, chip_class::NV_maxwell); // GM107
		table.add(0x1340, 0x134D, chip_class::NV_maxwell); // GM108
		table.add(0x13C0, 0x13D9, chip_class::NV_maxwell); // GM204
		table.add(0x1401, 0x1427, chip_class::NV_maxwell); // GM206
		table.add(0x15F7, 0x15F9, chip_class::NV_pascal); // GP100 (Tesla P100)
		table.add(0x1B00, 0x1D80, chip_class::NV_pascal);
		table.add(0x1D81, 0x1DBA, chip_class::NV_volta);
		table.add(0x1E02, 0x1F51, chip_class::NV_turing); // RTX 20 series
		table.add(0x2182, chip_class::NV_turing); // TU116
		table.add(0x2184, chip_class::NV_turing); // TU116
		table.add(0x1F82, chip_class::NV_turing); // TU117
		table.add(0x1F91, chip_class::NV_turing); // TU117

		return table;
	}();

	const context* g_current_vulkan_ctx = nullptr;
	const render_device* g_current_renderer;

	std::unique_ptr<image> g_null_texture;
	std::unique_ptr<buffer> g_scratch_buffer;
	std::unordered_map<VkImageViewType, std::unique_ptr<image_view>> g_null_image_views;
	std::unordered_map<u32, std::unique_ptr<image>> g_typeless_textures;
	std::unordered_map<u32, std::unique_ptr<vk::compute_task>> g_compute_tasks;

	// General purpose upload heap
	// TODO: Clean this up and integrate cleanly with VKGSRender
	data_heap g_upload_heap;

	// Garbage collection
	std::vector<std::unique_ptr<image>> g_deleted_typeless_textures;

	VkSampler g_null_sampler = nullptr;

	rsx::atomic_bitmask_t<runtime_state, u64> g_runtime_state;

	// Driver compatibility workarounds
	VkFlags g_heap_compatible_buffer_types = 0;
	driver_vendor g_driver_vendor = driver_vendor::unknown;
	chip_class g_chip_class = chip_class::unknown;
	bool g_drv_no_primitive_restart_flag = false;
	bool g_drv_sanitize_fp_values = false;
	bool g_drv_disable_fence_reset = false;
	bool g_drv_emulate_cond_render = false;

	u64 g_num_processed_frames = 0;
	u64 g_num_total_frames = 0;

	VKAPI_ATTR void* VKAPI_CALL mem_realloc(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
	{
#ifdef _MSC_VER
		return _aligned_realloc(pOriginal, size, alignment);
#elif _WIN32
		return __mingw_aligned_realloc(pOriginal, size, alignment);
#else
		std::abort();
#endif
	}

	VKAPI_ATTR void* VKAPI_CALL mem_alloc(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
	{
#ifdef _MSC_VER
		return _aligned_malloc(size, alignment);
#elif _WIN32
		return __mingw_aligned_malloc(size, alignment);
#else
		std::abort();
#endif
	}

	VKAPI_ATTR void VKAPI_CALL mem_free(void* pUserData, void* pMemory)
	{
#ifdef _MSC_VER
		_aligned_free(pMemory);
#elif _WIN32
		__mingw_aligned_free(pMemory);
#else
		std::abort();
#endif
	}

	bool data_heap::grow(size_t size)
	{
		// Create new heap. All sizes are aligned up by 64M, upto 1GiB
		const size_t size_limit = 1024 * 0x100000;
		const size_t aligned_new_size = align(m_size + size, 64 * 0x100000);

		if (aligned_new_size >= size_limit)
		{
			// Too large
			return false;
		}

		if (shadow)
		{
			// Shadowed. Growing this can be messy as it requires double allocation (macOS only)
			return false;
		}

		// Wait for DMA activity to end
		g_fxo->get<rsx::dma_manager>()->sync();

		if (mapped)
		{
			// Force reset mapping
			unmap(true);
		}

		VkBufferUsageFlags usage = heap->info.usage;

		const auto device = get_current_renderer();
		const auto& memory_map = device->get_memory_mapping();

		VkFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		auto memory_index = memory_map.host_visible_coherent;

		// Update heap information and reset the allocator
		::data_heap::init(aligned_new_size, m_name, m_min_guard_size);

		// Discard old heap and create a new one. Old heap will be garbage collected when no longer needed
		get_resource_manager()->dispose(heap);
		heap = std::make_unique<buffer>(*device, aligned_new_size, memory_index, memory_flags, usage, 0);

		if (notify_on_grow)
		{
			raise_status_interrupt(vk::heap_changed);
		}

		return true;
	}

	memory_type_mapping get_memory_mapping(const vk::physical_device& dev)
	{
		VkPhysicalDevice pdev = dev;
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(pdev, &memory_properties);

		memory_type_mapping result;
		result.device_local = VK_MAX_MEMORY_TYPES;
		result.host_visible_coherent = VK_MAX_MEMORY_TYPES;

		bool host_visible_cached = false;
		VkDeviceSize  host_visible_vram_size = 0;
		VkDeviceSize  device_local_vram_size = 0;

		for (u32 i = 0; i < memory_properties.memoryTypeCount; i++)
		{
			VkMemoryHeap &heap = memory_properties.memoryHeaps[memory_properties.memoryTypes[i].heapIndex];

			bool is_device_local = !!(memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (is_device_local)
			{
				if (device_local_vram_size < heap.size)
				{
					result.device_local = i;
					device_local_vram_size = heap.size;
				}
			}

			bool is_host_visible = !!(memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			bool is_host_coherent = !!(memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			bool is_cached = !!(memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

			if (is_host_coherent && is_host_visible)
			{
				if ((is_cached && !host_visible_cached) ||
					(host_visible_vram_size < heap.size))
				{
					result.host_visible_coherent = i;
					host_visible_vram_size = heap.size;
					host_visible_cached = is_cached;
				}
			}
		}

		if (result.device_local == VK_MAX_MEMORY_TYPES) fmt::throw_exception("GPU doesn't support device local memory" HERE);
		if (result.host_visible_coherent == VK_MAX_MEMORY_TYPES) fmt::throw_exception("GPU doesn't support host coherent device local memory" HERE);
		return result;
	}

	pipeline_binding_table get_pipeline_binding_table(const vk::physical_device& dev)
	{
		pipeline_binding_table result{};

		// Need to check how many samplers are supported by the driver
		const auto usable_samplers = std::min(dev.get_limits().maxPerStageDescriptorSampledImages, 32u);
		result.vertex_textures_first_bind_slot = result.textures_first_bind_slot + usable_samplers;
		result.total_descriptor_bindings = result.vertex_textures_first_bind_slot + 4;
		return result;
	}

	chip_class get_chip_family(uint32_t vendor_id, uint32_t device_id)
	{
		if (vendor_id == 0x10DE)
		{
			return s_NV_family_tree.find(device_id);
		}

		if (vendor_id == 0x1002)
		{
			return s_AMD_family_tree.find(device_id);
		}

		return chip_class::unknown;
	}

	VkAllocationCallbacks default_callbacks()
	{
		VkAllocationCallbacks callbacks;
		callbacks.pfnAllocation = vk::mem_alloc;
		callbacks.pfnFree = vk::mem_free;
		callbacks.pfnReallocation = vk::mem_realloc;

		return callbacks;
	}

	VkSampler null_sampler()
	{
		if (g_null_sampler)
			return g_null_sampler;

		VkSamplerCreateInfo sampler_info = {};

		sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		sampler_info.anisotropyEnable = VK_FALSE;
		sampler_info.compareEnable = VK_FALSE;
		sampler_info.unnormalizedCoordinates = VK_FALSE;
		sampler_info.mipLodBias = 0;
		sampler_info.maxAnisotropy = 0;
		sampler_info.magFilter = VK_FILTER_NEAREST;
		sampler_info.minFilter = VK_FILTER_NEAREST;
		sampler_info.compareOp = VK_COMPARE_OP_NEVER;
		sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		vkCreateSampler(*g_current_renderer, &sampler_info, nullptr, &g_null_sampler);
		return g_null_sampler;
	}

	vk::image_view* null_image_view(vk::command_buffer &cmd, VkImageViewType type)
	{
		if (auto found = g_null_image_views.find(type);
			found != g_null_image_views.end())
		{
			return found->second.get();
		}

		if (!g_null_texture)
		{
			g_null_texture = std::make_unique<image>(*g_current_renderer, g_current_renderer->get_memory_mapping().device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D, VK_FORMAT_B8G8R8A8_UNORM, 4, 4, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0);

			// Initialize memory to transparent black
			VkClearColorValue clear_color = {};
			VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			change_image_layout(cmd, g_null_texture.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
			vkCmdClearColorImage(cmd, g_null_texture->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &range);

			// Prep for shader access
			change_image_layout(cmd, g_null_texture.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);
		}

		auto& ret = g_null_image_views[type] = std::make_unique<image_view>(*g_current_renderer, g_null_texture.get(), type);
		return ret.get();
	}

	vk::image* get_typeless_helper(VkFormat format, u32 requested_width, u32 requested_height)
	{
		auto create_texture = [&]()
		{
			u32 new_width = align(requested_width, 1024u);
			u32 new_height = align(requested_height, 1024u);

			return new vk::image(*g_current_renderer, g_current_renderer->get_memory_mapping().device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D, format, new_width, new_height, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0);
		};

		auto& ptr = g_typeless_textures[+format];
		if (!ptr || ptr->width() < requested_width || ptr->height() < requested_height)
		{
			if (ptr)
			{
				// Safely move to deleted pile
				g_deleted_typeless_textures.emplace_back(std::move(ptr));
			}

			ptr.reset(create_texture());
		}

		return ptr.get();
	}

	vk::buffer* get_scratch_buffer(u32 min_required_size)
	{
		if (g_scratch_buffer && g_scratch_buffer->size() < min_required_size)
		{
			// Scratch heap cannot fit requirements. Discard it and allocate a new one.
			vk::get_resource_manager()->dispose(g_scratch_buffer);
		}

		if (!g_scratch_buffer)
		{
			// Choose optimal size
			const u64 alloc_size = std::max<u64>(64 * 0x100000, align(min_required_size, 0x100000));

			g_scratch_buffer = std::make_unique<vk::buffer>(*g_current_renderer, alloc_size,
				g_current_renderer->get_memory_mapping().device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0);
		}

		return g_scratch_buffer.get();
	}

	data_heap* get_upload_heap()
	{
		if (!g_upload_heap.heap)
		{
			g_upload_heap.create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 64 * 0x100000, "auxilliary upload heap", 0x100000);
		}

		return &g_upload_heap;
	}

	void reset_compute_tasks()
	{
		for (const auto &p : g_compute_tasks)
		{
			p.second->free_resources();
		}
	}

	void reset_global_resources()
	{
		vk::reset_compute_tasks();
		vk::reset_resolve_resources();

		g_upload_heap.reset_allocation_stats();
	}

	void destroy_global_resources()
	{
		VkDevice dev = *g_current_renderer;
		vk::clear_renderpass_cache(dev);
		vk::clear_framebuffer_cache();
		vk::clear_resolve_helpers();
		vk::clear_dma_resources();
		vk::vmm_reset();
		vk::get_resource_manager()->destroy();

		g_null_texture.reset();
		g_null_image_views.clear();
		g_scratch_buffer.reset();
		g_upload_heap.destroy();

		g_typeless_textures.clear();
		g_deleted_typeless_textures.clear();

		if (g_null_sampler)
		{
			vkDestroySampler(dev, g_null_sampler, nullptr);
			g_null_sampler = nullptr;
		}

		for (const auto& p : g_compute_tasks)
		{
			p.second->destroy();
		}

		g_compute_tasks.clear();
	}

	vk::mem_allocator_base* get_current_mem_allocator()
	{
		verify (HERE, g_current_renderer);
		return g_current_renderer->get_allocator();
	}

	void set_current_thread_ctx(const vk::context &ctx)
	{
		g_current_vulkan_ctx = &ctx;
	}

	const context *get_current_thread_ctx()
	{
		return g_current_vulkan_ctx;
	}

	const vk::render_device *get_current_renderer()
	{
		return g_current_renderer;
	}

	void set_current_renderer(const vk::render_device &device)
	{
		g_current_renderer = &device;
		g_runtime_state.clear();
		g_drv_no_primitive_restart_flag = false;
		g_drv_sanitize_fp_values = false;
		g_drv_disable_fence_reset = false;
		g_drv_emulate_cond_render = (g_cfg.video.relaxed_zcull_sync && !g_current_renderer->get_conditional_render_support());
		g_num_processed_frames = 0;
		g_num_total_frames = 0;
		g_heap_compatible_buffer_types = 0;

		const auto& gpu = g_current_renderer->gpu();
		const auto gpu_name = gpu.get_name();

		g_driver_vendor = gpu.get_driver_vendor();
		g_chip_class = gpu.get_chip_class();

		switch (g_driver_vendor)
		{
		case driver_vendor::AMD:
		case driver_vendor::RADV:
			// Previous bugs with fence reset and primitive restart seem to have been fixed with newer drivers
			break;
		case driver_vendor::NVIDIA:
			// Nvidia cards are easily susceptible to NaN poisoning
			g_drv_sanitize_fp_values = true;
			break;
		case driver_vendor::INTEL:
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
				CHECK_RESULT(vkCreateBuffer(*g_current_renderer, &info, nullptr, &tmp));

				vkGetBufferMemoryRequirements(*g_current_renderer, tmp, &memory_reqs);
				if (g_current_renderer->get_compatible_memory_type(memory_reqs.memoryTypeBits, memory_flags, nullptr))
				{
					g_heap_compatible_buffer_types |= usage;
				}

				vkDestroyBuffer(*g_current_renderer, tmp, nullptr);
			}
		}
	}

	VkFlags get_heap_compatible_buffer_types()
	{
		return g_heap_compatible_buffer_types;
	}

	driver_vendor get_driver_vendor()
	{
		return g_driver_vendor;
	}

	chip_class get_chip_family()
	{
		return g_chip_class;
	}

	bool emulate_primitive_restart(rsx::primitive_type type)
	{
		if (g_drv_no_primitive_restart_flag)
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

	void insert_buffer_memory_barrier(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize length, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkAccessFlags src_mask, VkAccessFlags dst_mask)
	{
		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		VkBufferMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.buffer = buffer;
		barrier.offset = offset;
		barrier.size = length;
		barrier.srcAccessMask = src_mask;
		barrier.dstAccessMask = dst_mask;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	}

	void insert_image_memory_barrier(
		VkCommandBuffer cmd, VkImage image,
		VkImageLayout current_layout, VkImageLayout new_layout,
		VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage,
		VkAccessFlags src_mask, VkAccessFlags dst_mask,
		const VkImageSubresourceRange& range)
	{
		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.newLayout = new_layout;
		barrier.oldLayout = current_layout;
		barrier.image = image;
		barrier.srcAccessMask = src_mask;
		barrier.dstAccessMask = dst_mask;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange = range;

		vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void insert_execution_barrier(VkCommandBuffer cmd, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
	{
		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 0, nullptr);
	}

	void change_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout, const VkImageSubresourceRange& range)
	{
		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		//Prepare an image to match the new layout..
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.newLayout = new_layout;
		barrier.oldLayout = current_layout;
		barrier.image = image;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange = range;

		VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		switch (new_layout)
		{
		case VK_IMAGE_LAYOUT_GENERAL:
			// Avoid this layout as it is unoptimized
			barrier.dstAccessMask =
			{
				VK_ACCESS_TRANSFER_READ_BIT |
				VK_ACCESS_TRANSFER_WRITE_BIT |
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
				VK_ACCESS_SHADER_READ_BIT |
				VK_ACCESS_INPUT_ATTACHMENT_READ_BIT
			};
			dst_stage =
			{
				VK_PIPELINE_STAGE_TRANSFER_BIT |
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			};
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
			dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		default:
		case VK_IMAGE_LAYOUT_UNDEFINED:
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			fmt::throw_exception("Attempted to transition to an invalid layout");
		}

		switch (current_layout)
		{
		case VK_IMAGE_LAYOUT_GENERAL:
			// Avoid this layout as it is unoptimized
			if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ||
				new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			{
				if (range.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
				{
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				}
				else
				{
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				}
			}
			else if (new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ||
					 new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				// Finish reading before writing
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else
			{
				barrier.srcAccessMask =
				{
					VK_ACCESS_TRANSFER_READ_BIT |
					VK_ACCESS_TRANSFER_WRITE_BIT |
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
					VK_ACCESS_SHADER_READ_BIT |
					VK_ACCESS_INPUT_ATTACHMENT_READ_BIT
				};
				src_stage =
				{
					VK_PIPELINE_STAGE_TRANSFER_BIT |
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
					VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				};
			}
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			src_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
			src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		default:
			break; //TODO Investigate what happens here
		}

		vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void change_image_layout(VkCommandBuffer cmd, vk::image *image, VkImageLayout new_layout, const VkImageSubresourceRange& range)
	{
		if (image->current_layout == new_layout) return;

		change_image_layout(cmd, image->value, image->current_layout, new_layout, range);
		image->current_layout = new_layout;
	}

	void change_image_layout(VkCommandBuffer cmd, vk::image *image, VkImageLayout new_layout)
	{
		if (image->current_layout == new_layout) return;

		change_image_layout(cmd, image->value, image->current_layout, new_layout, { image->aspect(), 0, 1, 0, 1 });
		image->current_layout = new_layout;
	}

	void insert_texture_barrier(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout, VkImageSubresourceRange range)
	{
		// NOTE: Sampling from an attachment in ATTACHMENT_OPTIMAL layout on some hw ends up with garbage output
		// Transition to GENERAL if this resource is both input and output
		// TODO: This implicitly makes the target incompatible with the renderpass declaration; investigate a proper workaround
		// TODO: This likely throws out hw optimizations on the rest of the renderpass, manage carefully
		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		VkAccessFlags src_access;
		VkPipelineStageFlags src_stage;
		if (range.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
		{
			src_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else
		{
			if (!rsx::method_registers.depth_write_enabled() && current_layout == new_layout)
			{
				// Nothing to do
				return;
			}

			src_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			src_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.newLayout = new_layout;
		barrier.oldLayout = current_layout;
		barrier.image = image;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange = range;
		barrier.srcAccessMask = src_access;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd, src_stage, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void insert_texture_barrier(VkCommandBuffer cmd, vk::image *image, VkImageLayout new_layout)
	{
		if (image->samples() > 1)
		{
			// This barrier is pointless for multisampled images as they require a resolve operation before access anyway
			return;
		}

		insert_texture_barrier(cmd, image->value, image->current_layout, new_layout, { image->aspect(), 0, 1, 0, 1 });
		image->current_layout = new_layout;
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
		verify(HERE), g_num_processed_frames <= g_num_total_frames;
		g_num_total_frames++;
	}

	const u64 get_current_frame_id()
	{
		return g_num_total_frames;
	}

	const u64 get_last_completed_frame_id()
	{
		return (g_num_processed_frames > 0)? g_num_processed_frames - 1: 0;
	}

	void reset_fence(fence *pFence)
	{
		if (g_drv_disable_fence_reset)
		{
			delete pFence;
			pFence = new fence(*g_current_renderer);
		}
		else
		{
			pFence->reset();
		}
	}

	VkResult wait_for_fence(fence* pFence, u64 timeout)
	{
		pFence->wait_flush();

		if (timeout)
		{
			return vkWaitForFences(*g_current_renderer, 1, &pFence->handle, VK_FALSE, timeout * 1000ull);
		}
		else
		{
			while (auto status = vkGetFenceStatus(*g_current_renderer, pFence->handle))
			{
				switch (status)
				{
				case VK_NOT_READY:
					continue;
				default:
					die_with_error(HERE, status);
					return status;
				}
			}

			return VK_SUCCESS;
		}
	}

	VkResult wait_for_event(event* pEvent, u64 timeout)
	{
		u64 t = 0;
		while (true)
		{
			switch (const auto status = pEvent->status())
			{
			case VK_EVENT_SET:
				return VK_SUCCESS;
			case VK_EVENT_RESET:
				break;
			default:
				die_with_error(HERE, status);
				return status;
			}

			if (timeout)
			{
				if (!t)
				{
					t = get_system_time();
					continue;
				}

				if ((get_system_time() - t) > timeout)
				{
					rsx_log.error("[vulkan] vk::wait_for_event has timed out!");
					return VK_TIMEOUT;
				}
			}

			//std::this_thread::yield();
			_mm_pause();
		}
	}

	void do_query_cleanup(vk::command_buffer& cmd)
	{
		auto renderer = dynamic_cast<VKGSRender*>(rsx::get_current_renderer());
		verify(HERE), renderer;

		renderer->emergency_query_cleanup(&cmd);
	}

	void die_with_error(const char* faulting_addr, VkResult error_code)
	{
		std::string error_message;
		int severity = 0; //0 - die, 1 - warn, 2 - nothing

		switch (error_code)
		{
		case VK_SUCCESS:
		case VK_EVENT_SET:
		case VK_EVENT_RESET:
		case VK_INCOMPLETE:
			return;
		case VK_SUBOPTIMAL_KHR:
			error_message = "Present surface is suboptimal (VK_SUBOPTIMAL_KHR)";
			severity = 1;
			break;
		case VK_NOT_READY:
			error_message = "Device or resource busy (VK_NOT_READY)";
			break;
		case VK_TIMEOUT:
			error_message = "Timeout event (VK_TIMEOUT)";
			break;
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			error_message = "Out of host memory (system RAM) (VK_ERROR_OUT_OF_HOST_MEMORY)";
			break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			error_message = "Out of video memory (VRAM) (VK_ERROR_OUT_OF_DEVICE_MEMORY)";
			break;
		case VK_ERROR_INITIALIZATION_FAILED:
			error_message = "Initialization failed (VK_ERROR_INITIALIZATION_FAILED)";
			break;
		case VK_ERROR_DEVICE_LOST:
			error_message = "Device lost (Driver crashed with unspecified error or stopped responding and recovered) (VK_ERROR_DEVICE_LOST)";
			break;
		case VK_ERROR_MEMORY_MAP_FAILED:
			error_message = "Memory map failed (VK_ERROR_MEMORY_MAP_FAILED)";
			break;
		case VK_ERROR_LAYER_NOT_PRESENT:
			error_message = "Requested layer is not available (Try disabling debug output or install vulkan SDK) (VK_ERROR_LAYER_NOT_PRESENT)";
			break;
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			error_message = "Requested extension not available (VK_ERROR_EXTENSION_NOT_PRESENT)";
			break;
		case VK_ERROR_FEATURE_NOT_PRESENT:
			error_message = "Requested feature not available (VK_ERROR_FEATURE_NOT_PRESENT)";
			break;
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			error_message = "Incompatible driver (VK_ERROR_INCOMPATIBLE_DRIVER)";
			break;
		case VK_ERROR_TOO_MANY_OBJECTS:
			error_message = "Too many objects created (Out of handles) (VK_ERROR_TOO_MANY_OBJECTS)";
			break;
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			error_message = "Format not supported (VK_ERROR_FORMAT_NOT_SUPPORTED)";
			break;
		case VK_ERROR_FRAGMENTED_POOL:
			error_message = "Fragmented pool (VK_ERROR_FRAGMENTED_POOL)";
			break;
		case VK_ERROR_SURFACE_LOST_KHR:
			error_message = "Surface lost (VK_ERROR_SURFACE_LOST)";
			break;
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			error_message = "Native window in use (VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)";
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			error_message = "Present surface is out of date (VK_ERROR_OUT_OF_DATE_KHR)";
			severity = 1;
			break;
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			error_message = "Incompatible display (VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)";
			break;
		case VK_ERROR_VALIDATION_FAILED_EXT:
			error_message = "Validation failed (VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)";
			break;
		case VK_ERROR_INVALID_SHADER_NV:
			error_message = "Invalid shader code (VK_ERROR_INVALID_SHADER_NV)";
			break;
		case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
			error_message = "Out of pool memory (VK_ERROR_OUT_OF_POOL_MEMORY_KHR)";
			break;
		case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR:
			error_message = "Invalid external handle (VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR)";
			break;
		default:
			error_message = fmt::format("Unknown Code (%Xh, %d)%s", static_cast<s32>(error_code), static_cast<s32>(error_code), faulting_addr);
			break;
		}

		switch (severity)
		{
		default:
		case 0:
			fmt::throw_exception("Assertion Failed! Vulkan API call failed with unrecoverable error: %s%s", error_message.c_str(), faulting_addr);
		case 1:
			rsx_log.error("Vulkan API call has failed with an error but will continue: %s%s", error_message.c_str(), faulting_addr);
			break;
		case 2:
			break;
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
											uint64_t srcObject, size_t location, int32_t msgCode,
											const char *pLayerPrefix, const char *pMsg, void *pUserData)
	{
		if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		{
			if (strstr(pMsg, "IMAGE_VIEW_TYPE_1D")) return false;

			rsx_log.error("ERROR: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		}
		else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		{
			rsx_log.warning("WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		}
		else
		{
			return false;
		}

		//Let the app crash..
		return false;
	}

	VkBool32 BreakCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
							uint64_t srcObject, size_t location, int32_t msgCode,
							const char *pLayerPrefix, const char *pMsg, void *pUserData)
	{
#ifdef _WIN32
		DebugBreak();
#endif

		return false;
	}
}
