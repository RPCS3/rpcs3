#pragma once

#include "util/types.hpp"
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include <stack>
#include <deque>

#ifdef HAVE_X11
#include <X11/Xutil.h>
#endif

#include "VulkanAPI.h"
#include "VKCommonDecompiler.h"
#include "../GCM.h"
#include "../Common/ring_buffer_helper.h"
#include "../Common/TextureUtils.h"
#include "../display.h"
#include "../rsx_utils.h"
#include "vkutils/chip_class.h"
#include "vkutils/fence.h"
#include "vkutils/mem_allocator.h"
#include "vkutils/memory_block.h"
#include "vkutils/physical_device.h"
#include "vkutils/pipeline_binding_table.h"
#include "vkutils/shared.h"
#include "vkutils/supported_extensions.h"

#ifdef __APPLE__
#define VK_DISABLE_COMPONENT_SWIZZLE 1
#else
#define VK_DISABLE_COMPONENT_SWIZZLE 0
#endif

#define DESCRIPTOR_MAX_DRAW_CALLS 16384
#define OCCLUSION_MAX_POOL_SIZE   DESCRIPTOR_MAX_DRAW_CALLS

#define FRAME_PRESENT_TIMEOUT 10000000ull // 10 seconds
#define GENERAL_WAIT_TIMEOUT  2000000ull  // 2 seconds

//using enum rsx::format_class;
using namespace ::rsx::format_class_;

namespace rsx
{
	class fragment_texture;
}

namespace vk
{
	VKAPI_ATTR void *VKAPI_CALL mem_realloc(void *pUserData, void *pOriginal, usz size, usz alignment, VkSystemAllocationScope allocationScope);
	VKAPI_ATTR void *VKAPI_CALL mem_alloc(void *pUserData, usz size, usz alignment, VkSystemAllocationScope allocationScope);
	VKAPI_ATTR void VKAPI_CALL mem_free(void *pUserData, void *pMemory);

	VkBool32 BreakCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
							u64 srcObject, usz location, s32 msgCode,
							const char *pLayerPrefix, const char *pMsg,
							void *pUserData);

	//VkAllocationCallbacks default_callbacks();
	enum runtime_state
	{
		uninterruptible = 1,
		heap_dirty = 2,
		heap_changed = 3
	};

	enum : u32// special remap_encoding enums
	{
		VK_REMAP_IDENTITY = 0xCAFEBABE,             // Special view encoding to return an identity image view
		VK_REMAP_VIEW_MULTISAMPLED = 0xDEADBEEF     // Special encoding for multisampled images; returns a multisampled image view
	};

	class context;
	class render_device;
	class swap_chain_image;
	class command_buffer;
	class image;
	struct image_view;
	struct buffer;
	class data_heap;
	class event;

	const vk::context *get_current_thread_ctx();
	void set_current_thread_ctx(const vk::context &ctx);

	const vk::render_device *get_current_renderer();
	void set_current_renderer(const vk::render_device &device);

	//Compatibility workarounds
	bool emulate_primitive_restart(rsx::primitive_type type);
	bool sanitize_fp_values();
	bool fence_reset_disabled();
	bool emulate_conditional_rendering();
	VkFlags get_heap_compatible_buffer_types();
	driver_vendor get_driver_vendor();

	VkComponentMapping default_component_map();
	VkComponentMapping apply_swizzle_remap(const std::array<VkComponentSwizzle, 4>& base_remap, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap_vector);
	VkImageSubresource default_image_subresource();
	VkImageSubresourceRange get_image_subresource_range(u32 base_layer, u32 base_mip, u32 layer_count, u32 level_count, VkImageAspectFlags aspect);
	VkImageAspectFlags get_aspect_flags(VkFormat format);

	VkSampler null_sampler();
	image_view* null_image_view(vk::command_buffer&, VkImageViewType type);
	image* get_typeless_helper(VkFormat format, rsx::format_class format_class, u32 requested_width, u32 requested_height);
	buffer* get_scratch_buffer(u32 min_required_size = 0);
	data_heap* get_upload_heap();

	// Sync helpers around vkQueueSubmit
	void acquire_global_submit_lock();
	void release_global_submit_lock();
	void queue_submit(VkQueue queue, const VkSubmitInfo* info, fence* pfence, VkBool32 flush = VK_FALSE);

	bool is_renderpass_open(VkCommandBuffer cmd);
	void end_renderpass(VkCommandBuffer cmd);

	template<class T>
	T* get_compute_task();
	void reset_compute_tasks();

	void destroy_global_resources();
	void reset_global_resources();

	/**
	* Allocate enough space in upload_buffer and write all mipmap/layer data into the subbuffer.
	* Then copy all layers into dst_image.
	* dst_image must be in TRANSFER_DST_OPTIMAL layout and upload_buffer have TRANSFER_SRC_BIT usage flag.
	*/
	void copy_mipmaped_image_using_buffer(VkCommandBuffer cmd, vk::image* dst_image,
		const std::vector<rsx::subresource_layout>& subresource_layout, int format, bool is_swizzled, u16 mipmap_count,
		VkImageAspectFlags flags, vk::data_heap &upload_heap, u32 heap_align = 0);

	//Other texture management helpers
	void change_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout, const VkImageSubresourceRange& range);
	void change_image_layout(VkCommandBuffer cmd, vk::image *image, VkImageLayout new_layout, const VkImageSubresourceRange& range);
	void change_image_layout(VkCommandBuffer cmd, vk::image *image, VkImageLayout new_layout);

	void copy_image_to_buffer(VkCommandBuffer cmd, const vk::image* src, const vk::buffer* dst, const VkBufferImageCopy& region, bool swap_bytes = false);
	void copy_buffer_to_image(VkCommandBuffer cmd, const vk::buffer* src, const vk::image* dst, const VkBufferImageCopy& region);

	void copy_image_typeless(const command_buffer &cmd, image *src, image *dst, const areai& src_rect, const areai& dst_rect,
		u32 mipmaps, VkImageAspectFlags src_transfer_mask = 0xFF, VkImageAspectFlags dst_transfer_mask = 0xFF);

	void copy_image(const vk::command_buffer& cmd, vk::image* src, vk::image* dst,
			const areai& src_rect, const areai& dst_rect, u32 mipmaps,
			VkImageAspectFlags src_transfer_mask = 0xFF, VkImageAspectFlags dst_transfer_mask = 0xFF);

	void copy_scaled_image(const vk::command_buffer& cmd, vk::image* src, vk::image* dst,
			const areai& src_rect, const areai& dst_rect, u32 mipmaps,
			bool compatible_formats, VkFilter filter = VK_FILTER_LINEAR);

	std::pair<VkFormat, VkComponentMapping> get_compatible_surface_format(rsx::surface_color_format color_format);

	//Texture barrier applies to a texture to ensure writes to it are finished before any reads are attempted to avoid RAW hazards
	void insert_texture_barrier(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout, VkImageSubresourceRange range);
	void insert_texture_barrier(VkCommandBuffer cmd, vk::image *image, VkImageLayout new_layout);

	void insert_buffer_memory_barrier(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize length,
			VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkAccessFlags src_mask, VkAccessFlags dst_mask);

	void insert_image_memory_barrier(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout,
		VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkAccessFlags src_mask, VkAccessFlags dst_mask,
		const VkImageSubresourceRange& range);

	void insert_execution_barrier(VkCommandBuffer cmd,
		VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	void raise_status_interrupt(runtime_state status);
	void clear_status_interrupt(runtime_state status);
	bool test_status_interrupt(runtime_state status);
	void enter_uninterruptible();
	void leave_uninterruptible();
	bool is_uninterruptible();

	void advance_completed_frame_counter();
	void advance_frame_counter();
	const u64 get_current_frame_id();
	const u64 get_last_completed_frame_id();

	// Fence reset with driver workarounds in place
	void reset_fence(fence* pFence);
	VkResult wait_for_fence(fence* pFence, u64 timeout = 0ull);
	VkResult wait_for_event(event* pEvent, u64 timeout = 0ull);

	// Handle unexpected submit with dangling occlusion query
	// TODO: Move queries out of the renderer!
	void do_query_cleanup(vk::command_buffer& cmd);

	class render_device
	{
		physical_device *pgpu = nullptr;
		memory_type_mapping memory_map{};
		gpu_formats_support m_formats_support{};
		pipeline_binding_table m_pipeline_binding_table{};
		std::unique_ptr<mem_allocator_base> m_allocator;
		VkDevice dev = VK_NULL_HANDLE;

	public:
		// Exported device endpoints
		PFN_vkCmdBeginConditionalRenderingEXT cmdBeginConditionalRenderingEXT = nullptr;
		PFN_vkCmdEndConditionalRenderingEXT cmdEndConditionalRenderingEXT = nullptr;

	public:
		render_device() = default;
		~render_device() = default;

		void create(vk::physical_device &pdev, u32 graphics_queue_idx)
		{
			float queue_priorities[1] = { 0.f };
			pgpu = &pdev;

			VkDeviceQueueCreateInfo queue = {};
			queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue.pNext = NULL;
			queue.queueFamilyIndex = graphics_queue_idx;
			queue.queueCount = 1;
			queue.pQueuePriorities = queue_priorities;

			// Set up instance information
			std::vector<const char *>requested_extensions =
			{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};

			// Enable hardware features manually
			// Currently we require:
			// 1. Anisotropic sampling
			// 2. DXT support
			// 3. Indexable storage buffers
			VkPhysicalDeviceFeatures enabled_features{};
			if (pgpu->shader_types_support.allow_float16)
			{
				requested_extensions.push_back(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);
			}

			if (pgpu->conditional_render_support)
			{
				requested_extensions.push_back(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);
			}

			if (pgpu->unrestricted_depth_range_support)
			{
				requested_extensions.push_back(VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME);
			}

			enabled_features.robustBufferAccess = VK_TRUE;
			enabled_features.fullDrawIndexUint32 = VK_TRUE;
			enabled_features.independentBlend = VK_TRUE;
			enabled_features.logicOp = VK_TRUE;
			enabled_features.depthClamp = VK_TRUE;
			enabled_features.depthBounds = VK_TRUE;
			enabled_features.wideLines = VK_TRUE;
			enabled_features.largePoints = VK_TRUE;
			enabled_features.shaderFloat64 = VK_TRUE;

			if (g_cfg.video.antialiasing_level != msaa_level::none)
			{
				// MSAA features
				if (!pgpu->features.shaderStorageImageMultisample ||
					!pgpu->features.shaderStorageImageWriteWithoutFormat)
				{
					// TODO: Slow fallback to emulate this
					// Just warn and let the driver decide whether to crash or not
					rsx_log.fatal("Your GPU driver does not support some required MSAA features. Expect problems.");
				}

				enabled_features.sampleRateShading = VK_TRUE;
				enabled_features.alphaToOne = VK_TRUE;
				enabled_features.shaderStorageImageMultisample = VK_TRUE;
				// enabled_features.shaderStorageImageReadWithoutFormat = VK_TRUE;  // Unused currently, may be needed soon
				enabled_features.shaderStorageImageWriteWithoutFormat = VK_TRUE;
			}

			// enabled_features.shaderSampledImageArrayDynamicIndexing = TRUE;  // Unused currently but will be needed soon
			enabled_features.shaderClipDistance = VK_TRUE;
			// enabled_features.shaderCullDistance = VK_TRUE;  // Alt notation of clip distance

			enabled_features.samplerAnisotropy = VK_TRUE;
			enabled_features.textureCompressionBC = VK_TRUE;
			enabled_features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;

			// Optionally disable unsupported stuff
			if (!pgpu->features.shaderFloat64)
			{
				rsx_log.error("Your GPU does not support double precision floats in shaders. Graphics may not work correctly.");
				enabled_features.shaderFloat64 = VK_FALSE;
			}

			if (!pgpu->features.depthBounds)
			{
				rsx_log.error("Your GPU does not support depth bounds testing. Graphics may not work correctly.");
				enabled_features.depthBounds = VK_FALSE;
			}

			if (!pgpu->features.sampleRateShading && enabled_features.sampleRateShading)
			{
				rsx_log.error("Your GPU does not support sample rate shading for multisampling. Graphics may be inaccurate when MSAA is enabled.");
				enabled_features.sampleRateShading = VK_FALSE;
			}

			if (!pgpu->features.alphaToOne && enabled_features.alphaToOne)
			{
				// AMD proprietary drivers do not expose alphaToOne support
				rsx_log.error("Your GPU does not support alpha-to-one for multisampling. Graphics may be inaccurate when MSAA is enabled.");
				enabled_features.alphaToOne = VK_FALSE;
			}

			VkDeviceCreateInfo device = {};
			device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			device.pNext = nullptr;
			device.queueCreateInfoCount = 1;
			device.pQueueCreateInfos = &queue;
			device.enabledLayerCount = 0;
			device.ppEnabledLayerNames = nullptr; // Deprecated
			device.enabledExtensionCount = ::size32(requested_extensions);
			device.ppEnabledExtensionNames = requested_extensions.data();
			device.pEnabledFeatures = &enabled_features;

			VkPhysicalDeviceFloat16Int8FeaturesKHR shader_support_info{};
			if (pgpu->shader_types_support.allow_float16)
			{
				// Allow use of f16 type in shaders if possible
				shader_support_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR;
				shader_support_info.shaderFloat16 = VK_TRUE;
				device.pNext = &shader_support_info;

				rsx_log.notice("GPU/driver supports float16 data types natively. Using native float16_t variables if possible.");
			}
			else
			{
				rsx_log.notice("GPU/driver lacks support for float16 data types. All float16_t arithmetic will be emulated with float32_t.");
			}

			CHECK_RESULT(vkCreateDevice(*pgpu, &device, nullptr, &dev));

			// Import optional function endpoints
			if (pgpu->conditional_render_support)
			{
				cmdBeginConditionalRenderingEXT = reinterpret_cast<PFN_vkCmdBeginConditionalRenderingEXT>(vkGetDeviceProcAddr(dev, "vkCmdBeginConditionalRenderingEXT"));
				cmdEndConditionalRenderingEXT = reinterpret_cast<PFN_vkCmdEndConditionalRenderingEXT>(vkGetDeviceProcAddr(dev, "vkCmdEndConditionalRenderingEXT"));
			}

			memory_map = vk::get_memory_mapping(pdev);
			m_formats_support = vk::get_optimal_tiling_supported_formats(pdev);
			m_pipeline_binding_table = vk::get_pipeline_binding_table(pdev);

			if (g_cfg.video.disable_vulkan_mem_allocator)
				m_allocator = std::make_unique<vk::mem_allocator_vk>(dev, pdev);
			else
				m_allocator = std::make_unique<vk::mem_allocator_vma>(dev, pdev);
		}

		void destroy()
		{
			if (dev && pgpu)
			{
				if (m_allocator)
				{
					m_allocator->destroy();
					m_allocator.reset();
				}

				vkDestroyDevice(dev, nullptr);
				dev = nullptr;
				memory_map = {};
				m_formats_support = {};
			}
		}

		const VkFormatProperties get_format_properties(VkFormat format)
		{
			auto found = pgpu->format_properties.find(format);
			if (found != pgpu->format_properties.end())
			{
				return found->second;
			}

			auto& props = pgpu->format_properties[format];
			vkGetPhysicalDeviceFormatProperties(*pgpu, format, &props);
			return props;
		}

		bool get_compatible_memory_type(u32 typeBits, u32 desired_mask, u32 *type_index) const
		{
			VkPhysicalDeviceMemoryProperties mem_infos = pgpu->get_memory_properties();

			for (u32 i = 0; i < 32; i++)
			{
				if ((typeBits & 1) == 1)
				{
					if ((mem_infos.memoryTypes[i].propertyFlags & desired_mask) == desired_mask)
					{
						if (type_index)
						{
							*type_index = i;
						}

						return true;
					}
				}

				typeBits >>= 1;
			}

			return false;
		}

		const physical_device& gpu() const
		{
			return *pgpu;
		}

		const memory_type_mapping& get_memory_mapping() const
		{
			return memory_map;
		}

		const gpu_formats_support& get_formats_support() const
		{
			return m_formats_support;
		}

		const pipeline_binding_table& get_pipeline_binding_table() const
		{
			return m_pipeline_binding_table;
		}

		const gpu_shader_types_support& get_shader_types_support() const
		{
			return pgpu->shader_types_support;
		}

		bool get_shader_stencil_export_support() const
		{
			return pgpu->stencil_export_support;
		}

		bool get_depth_bounds_support() const
		{
			return pgpu->features.depthBounds != VK_FALSE;
		}

		bool get_alpha_to_one_support() const
		{
			return pgpu->features.alphaToOne != VK_FALSE;
		}

		bool get_conditional_render_support() const
		{
			return pgpu->conditional_render_support;
		}

		bool get_unrestricted_depth_range_support() const
		{
			return pgpu->unrestricted_depth_range_support;
		}

		mem_allocator_base* get_allocator() const
		{
			return m_allocator.get();
		}

		operator VkDevice() const
		{
			return dev;
		}
	};

	class command_pool
	{
		vk::render_device *owner = nullptr;
		VkCommandPool pool = nullptr;

	public:
		command_pool() = default;
		~command_pool() = default;

		void create(vk::render_device &dev)
		{
			owner = &dev;
			VkCommandPoolCreateInfo infos = {};
			infos.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			infos.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

			CHECK_RESULT(vkCreateCommandPool(dev, &infos, nullptr, &pool));
		}

		void destroy()
		{
			if (!pool)
				return;

			vkDestroyCommandPool((*owner), pool, nullptr);
			pool = nullptr;
		}

		vk::render_device& get_owner()
		{
			return (*owner);
		}

		operator VkCommandPool()
		{
			return pool;
		}
	};

	class command_buffer
	{
	private:
		bool is_open = false;
		bool is_pending = false;
		fence* m_submit_fence = nullptr;

	protected:
		vk::command_pool *pool = nullptr;
		VkCommandBuffer commands = nullptr;

	public:
		enum access_type_hint
		{
			flush_only, //Only to be submitted/opened/closed via command flush
			all         //Auxiliary, can be submitted/opened/closed at any time
		}
		access_hint = flush_only;

		enum command_buffer_data_flag : u32
		{
			cb_has_occlusion_task = 1,
			cb_has_blit_transfer = 2,
			cb_has_dma_transfer = 4,
			cb_has_open_query = 8,
			cb_load_occluson_task = 16,
			cb_has_conditional_render = 32
		};
		u32 flags = 0;

	public:
		command_buffer() = default;
		~command_buffer() = default;

		void create(vk::command_pool &cmd_pool, bool auto_reset = false)
		{
			VkCommandBufferAllocateInfo infos = {};
			infos.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			infos.commandBufferCount = 1;
			infos.commandPool = +cmd_pool;
			infos.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			CHECK_RESULT(vkAllocateCommandBuffers(cmd_pool.get_owner(), &infos, &commands));

			if (auto_reset)
			{
				m_submit_fence = new fence(cmd_pool.get_owner());
			}

			pool = &cmd_pool;
		}

		void destroy()
		{
			vkFreeCommandBuffers(pool->get_owner(), (*pool), 1, &commands);

			if (m_submit_fence)
			{
				//vkDestroyFence(pool->get_owner(), m_submit_fence, nullptr);
				delete m_submit_fence;
				m_submit_fence = nullptr;
			}
		}

		vk::command_pool& get_command_pool() const
		{
			return *pool;
		}

		void clear_flags()
		{
			flags = 0;
		}

		void set_flag(command_buffer_data_flag flag)
		{
			flags |= flag;
		}

		operator VkCommandBuffer() const
		{
			return commands;
		}

		bool is_recording() const
		{
			return is_open;
		}

		void begin()
		{
			if (m_submit_fence && is_pending)
			{
				wait_for_fence(m_submit_fence);
				is_pending = false;

				//CHECK_RESULT(vkResetFences(pool->get_owner(), 1, &m_submit_fence));
				reset_fence(m_submit_fence);
				CHECK_RESULT(vkResetCommandBuffer(commands, 0));
			}

			if (is_open)
				return;

			VkCommandBufferInheritanceInfo inheritance_info = {};
			inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

			VkCommandBufferBeginInfo begin_infos = {};
			begin_infos.pInheritanceInfo = &inheritance_info;
			begin_infos.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_infos.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			CHECK_RESULT(vkBeginCommandBuffer(commands, &begin_infos));
			is_open = true;
		}

		void end()
		{
			if (!is_open)
			{
				rsx_log.error("commandbuffer->end was called but commandbuffer is not in a recording state");
				return;
			}

			CHECK_RESULT(vkEndCommandBuffer(commands));
			is_open = false;
		}

		void submit(VkQueue queue, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore, fence* pfence, VkPipelineStageFlags pipeline_stage_flags, VkBool32 flush = VK_FALSE)
		{
			if (is_open)
			{
				rsx_log.error("commandbuffer->submit was called whilst the command buffer is in a recording state");
				return;
			}

			// Check for hanging queries to avoid driver hang
			ensure((flags & cb_has_open_query) == 0); // "close and submit of commandbuffer with a hanging query!"

			if (!pfence)
			{
				pfence = m_submit_fence;
				is_pending = bool(pfence);
			}

			VkSubmitInfo infos = {};
			infos.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			infos.commandBufferCount = 1;
			infos.pCommandBuffers = &commands;
			infos.pWaitDstStageMask = &pipeline_stage_flags;

			if (wait_semaphore)
			{
				infos.waitSemaphoreCount = 1;
				infos.pWaitSemaphores = &wait_semaphore;
			}

			if (signal_semaphore)
			{
				infos.signalSemaphoreCount = 1;
				infos.pSignalSemaphores = &signal_semaphore;
			}

			queue_submit(queue, &infos, pfence, flush);
			clear_flags();
		}
	};

	class image
	{
		std::stack<VkImageLayout> m_layout_stack;
		VkImageAspectFlags m_storage_aspect = 0;

		rsx::format_class m_format_class = RSX_FORMAT_CLASS_UNDEFINED;

		void validate(const vk::render_device& dev, const VkImageCreateInfo& info) const
		{
			const auto gpu_limits = dev.gpu().get_limits();
			u32 longest_dim, dim_limit;

			switch (info.imageType)
			{
			case VK_IMAGE_TYPE_1D:
				longest_dim = info.extent.width;
				dim_limit = gpu_limits.maxImageDimension1D;
				break;
			case VK_IMAGE_TYPE_2D:
				longest_dim = std::max(info.extent.width, info.extent.height);
				dim_limit = (info.flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)? gpu_limits.maxImageDimensionCube : gpu_limits.maxImageDimension2D;
				break;
			case VK_IMAGE_TYPE_3D:
				longest_dim = std::max({ info.extent.width, info.extent.height, info.extent.depth });
				dim_limit = gpu_limits.maxImageDimension3D;
				break;
			default:
				fmt::throw_exception("Unreachable");
			}

			if (longest_dim > dim_limit)
			{
				// Longest dimension exceeds the limit. Can happen when using MSAA + very high resolution scaling
				// Just kill the application at this point.
				fmt::throw_exception(
					"The renderer requested an image larger than the limit allowed for by your GPU hardware. "
					"Turn down your resolution scale and/or disable MSAA to fit within the image budget.");
			}
		}

	public:
		VkImage value = VK_NULL_HANDLE;
		VkComponentMapping native_component_map = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
		VkImageLayout current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageCreateInfo info = {};
		std::shared_ptr<vk::memory_block> memory;

		image(const vk::render_device &dev,
			u32 memory_type_index,
			u32 access_flags,
			VkImageType image_type,
			VkFormat format,
			u32 width, u32 height, u32 depth,
			u32 mipmaps, u32 layers,
			VkSampleCountFlagBits samples,
			VkImageLayout initial_layout,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkImageCreateFlags image_flags,
			rsx::format_class format_class = RSX_FORMAT_CLASS_UNDEFINED)
			: current_layout(initial_layout)
			, m_device(dev)
		{
			info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			info.imageType = image_type;
			info.format = format;
			info.extent = { width, height, depth };
			info.mipLevels = mipmaps;
			info.arrayLayers = layers;
			info.samples = samples;
			info.tiling = tiling;
			info.usage = usage;
			info.flags = image_flags;
			info.initialLayout = initial_layout;
			info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			validate(dev, info);
			CHECK_RESULT(vkCreateImage(m_device, &info, nullptr, &value));

			VkMemoryRequirements memory_req;
			vkGetImageMemoryRequirements(m_device, value, &memory_req);

			if (!(memory_req.memoryTypeBits & (1 << memory_type_index)))
			{
				//Suggested memory type is incompatible with this memory type.
				//Go through the bitset and test for requested props.
				if (!dev.get_compatible_memory_type(memory_req.memoryTypeBits, access_flags, &memory_type_index))
					fmt::throw_exception("No compatible memory type was found!");
			}

			memory = std::make_shared<vk::memory_block>(m_device, memory_req.size, memory_req.alignment, memory_type_index);
			CHECK_RESULT(vkBindImageMemory(m_device, value, memory->get_vk_device_memory(), memory->get_vk_device_memory_offset()));

			m_storage_aspect = get_aspect_flags(format);

			if (format_class == RSX_FORMAT_CLASS_UNDEFINED)
			{
				if (m_storage_aspect != VK_IMAGE_ASPECT_COLOR_BIT)
				{
					rsx_log.error("Depth/stencil textures must have format class explicitly declared");
				}
				else
				{
					format_class = RSX_FORMAT_CLASS_COLOR;
				}
			}

			m_format_class = format_class;
		}

		// TODO: Ctor that uses a provided memory heap

		virtual ~image()
		{
			vkDestroyImage(m_device, value, nullptr);
		}

		image(const image&) = delete;
		image(image&&) = delete;

		u32 width() const
		{
			return info.extent.width;
		}

		u32 height() const
		{
			return info.extent.height;
		}

		u32 depth() const
		{
			return info.extent.depth;
		}

		u32 mipmaps() const
		{
			return info.mipLevels;
		}

		u32 layers() const
		{
			return info.arrayLayers;
		}

		u8 samples() const
		{
			return u8(info.samples);
		}

		VkFormat format() const
		{
			return info.format;
		}

		VkImageAspectFlags aspect() const
		{
			return m_storage_aspect;
		}

		rsx::format_class format_class() const
		{
			return m_format_class;
		}

		void push_layout(VkCommandBuffer cmd, VkImageLayout layout)
		{
			m_layout_stack.push(current_layout);
			change_image_layout(cmd, this, layout);
		}

		void push_barrier(VkCommandBuffer cmd, VkImageLayout layout)
		{
			m_layout_stack.push(current_layout);
			insert_texture_barrier(cmd, this, layout);
		}

		void pop_layout(VkCommandBuffer cmd)
		{
			ensure(!m_layout_stack.empty());

			auto layout = m_layout_stack.top();
			m_layout_stack.pop();
			change_image_layout(cmd, this, layout);
		}

		void change_layout(command_buffer& cmd, VkImageLayout new_layout)
		{
			if (current_layout == new_layout)
				return;

			ensure(m_layout_stack.empty());
			change_image_layout(cmd, this, new_layout);
		}

	private:
		VkDevice m_device;
	};

	struct image_view
	{
		VkImageView value = VK_NULL_HANDLE;
		VkImageViewCreateInfo info = {};

		image_view(VkDevice dev, VkImage image, VkImageViewType view_type, VkFormat format, VkComponentMapping mapping, VkImageSubresourceRange range)
			: m_device(dev)
		{
			info.format = format;
			info.image = image;
			info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			info.components = mapping;
			info.viewType = view_type;
			info.subresourceRange = range;

			create_impl();
		}

		image_view(VkDevice dev, VkImageViewCreateInfo create_info)
			: info(create_info)
			, m_device(dev)
		{
			create_impl();
		}

		image_view(VkDevice dev, vk::image* resource,
			VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_MAX_ENUM,
			const VkComponentMapping& mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
			const VkImageSubresourceRange& range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
			: m_device(dev), m_resource(resource)
		{
			info.format = resource->info.format;
			info.image = resource->value;
			info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			info.components = mapping;
			info.subresourceRange = range;

			if (view_type == VK_IMAGE_VIEW_TYPE_MAX_ENUM)
			{
				switch (resource->info.imageType)
				{
				case VK_IMAGE_TYPE_1D:
					info.viewType = VK_IMAGE_VIEW_TYPE_1D;
					break;
				case VK_IMAGE_TYPE_2D:
					if (resource->info.flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
						info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
					else if (resource->info.arrayLayers == 1)
						info.viewType = VK_IMAGE_VIEW_TYPE_2D;
					else
						info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
					break;
				case VK_IMAGE_TYPE_3D:
					info.viewType = VK_IMAGE_VIEW_TYPE_3D;
					break;
				default:
					fmt::throw_exception("Unreachable");
				}

				info.subresourceRange.layerCount = resource->info.arrayLayers;
			}
			else
			{
				info.viewType = view_type;
			}

			create_impl();
		}

		~image_view()
		{
			vkDestroyImageView(m_device, value, nullptr);
		}

		u32 encoded_component_map() const
		{
#if	(VK_DISABLE_COMPONENT_SWIZZLE)
			u32 result = static_cast<u32>(info.components.a) - 1;
			result |= (static_cast<u32>(info.components.r) - 1) << 3;
			result |= (static_cast<u32>(info.components.g) - 1) << 6;
			result |= (static_cast<u32>(info.components.b) - 1) << 9;

			return result;
#else
			return 0;
#endif
		}

		vk::image* image() const
		{
			return m_resource;
		}

		image_view(const image_view&) = delete;
		image_view(image_view&&) = delete;

	private:
		VkDevice m_device;
		vk::image* m_resource = nullptr;

		void create_impl()
		{
#if (VK_DISABLE_COMPONENT_SWIZZLE)
			// Force identity
			const auto mapping = info.components;
			info.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
#endif

			CHECK_RESULT(vkCreateImageView(m_device, &info, nullptr, &value));

#if (VK_DISABLE_COMPONENT_SWIZZLE)
			// Restore requested mapping
			info.components = mapping;
#endif
		}
	};

	class viewable_image : public image
	{
	private:
		std::unordered_multimap<u32, std::unique_ptr<vk::image_view>> views;

	public:
		using image::image;

		virtual image_view* get_view(u32 remap_encoding, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap,
			VkImageAspectFlags mask = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT)
		{
			if (remap_encoding == VK_REMAP_IDENTITY)
			{
				if (native_component_map.a == VK_COMPONENT_SWIZZLE_A &&
					native_component_map.r == VK_COMPONENT_SWIZZLE_R &&
					native_component_map.g == VK_COMPONENT_SWIZZLE_G &&
					native_component_map.b == VK_COMPONENT_SWIZZLE_B)
				{
					remap_encoding = 0xAAE4;
				}
			}

			auto found = views.equal_range(remap_encoding);
			for (auto It = found.first; It != found.second; ++It)
			{
				if (It->second->info.subresourceRange.aspectMask & mask)
				{
					return It->second.get();
				}
			}

			VkComponentMapping real_mapping;
			switch (remap_encoding)
			{
			case VK_REMAP_IDENTITY:
				real_mapping = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
				break;
			case 0xAAE4:
				real_mapping = native_component_map;
				break;
			default:
				real_mapping = vk::apply_swizzle_remap
				(
					{ native_component_map.a, native_component_map.r, native_component_map.g, native_component_map.b },
					remap
				);
				break;
			}

			const auto range = vk::get_image_subresource_range(0, 0, info.arrayLayers, info.mipLevels, aspect() & mask);

			ensure(range.aspectMask);
			auto view = std::make_unique<vk::image_view>(*get_current_renderer(), this, VK_IMAGE_VIEW_TYPE_MAX_ENUM, real_mapping, range);

			auto result = view.get();
			views.emplace(remap_encoding, std::move(view));
			return result;
		}

		void set_native_component_layout(VkComponentMapping new_layout)
		{
			if (new_layout.r != native_component_map.r ||
				new_layout.g != native_component_map.g ||
				new_layout.b != native_component_map.b ||
				new_layout.a != native_component_map.a)
			{
				native_component_map = new_layout;
				views.clear();
			}
		}
	};

	struct buffer
	{
		VkBuffer value;
		VkBufferCreateInfo info = {};
		std::unique_ptr<vk::memory_block> memory;

		buffer(const vk::render_device& dev, u64 size, u32 memory_type_index, u32 access_flags, VkBufferUsageFlags usage, VkBufferCreateFlags flags)
			: m_device(dev)
		{
			info.size = size;
			info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			info.flags = flags;
			info.usage = usage;

			CHECK_RESULT(vkCreateBuffer(m_device, &info, nullptr, &value));

			//Allocate vram for this buffer
			VkMemoryRequirements memory_reqs;
			vkGetBufferMemoryRequirements(m_device, value, &memory_reqs);

			if (!(memory_reqs.memoryTypeBits & (1 << memory_type_index)))
			{
				//Suggested memory type is incompatible with this memory type.
				//Go through the bitset and test for requested props.
				if (!dev.get_compatible_memory_type(memory_reqs.memoryTypeBits, access_flags, &memory_type_index))
					fmt::throw_exception("No compatible memory type was found!");
			}

			memory = std::make_unique<memory_block>(m_device, memory_reqs.size, memory_reqs.alignment, memory_type_index);
			vkBindBufferMemory(dev, value, memory->get_vk_device_memory(), memory->get_vk_device_memory_offset());
		}

		~buffer()
		{
			vkDestroyBuffer(m_device, value, nullptr);
		}

		void *map(u64 offset, u64 size)
		{
			return memory->map(offset, size);
		}

		void unmap()
		{
			memory->unmap();
		}

		u32 size() const
		{
			return static_cast<u32>(info.size);
		}

		buffer(const buffer&) = delete;
		buffer(buffer&&) = delete;

	private:
		VkDevice m_device;
	};

	class event
	{
		VkDevice m_device = VK_NULL_HANDLE;
		VkEvent m_vk_event = VK_NULL_HANDLE;

		std::unique_ptr<buffer> m_buffer;
		volatile u32* m_value = nullptr;

	public:
		event(const render_device& dev)
		{
			m_device = dev;
			if (dev.gpu().get_driver_vendor() != driver_vendor::AMD)
			{
				VkEventCreateInfo info
				{
					.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0
				};
				vkCreateEvent(dev, &info, nullptr, &m_vk_event);
			}
			else
			{
				// Work around AMD's broken event signals
				m_buffer = std::make_unique<buffer>
				(
					dev,
					4,
					dev.get_memory_mapping().host_visible_coherent,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
					VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					0
				);

				m_value = reinterpret_cast<u32*>(m_buffer->map(0, 4));
				*m_value = 0xCAFEBABE;
			}
		}

		~event()
		{
			if (m_vk_event) [[likely]]
			{
				vkDestroyEvent(m_device, m_vk_event, nullptr);
			}
			else
			{
				m_buffer->unmap();
				m_buffer.reset();
				m_value = nullptr;
			}
		}

		void signal(const command_buffer& cmd, VkPipelineStageFlags stages)
		{
			if (m_vk_event) [[likely]]
			{
				vkCmdSetEvent(cmd, m_vk_event, stages);
			}
			else
			{
				insert_execution_barrier(cmd, stages, VK_PIPELINE_STAGE_TRANSFER_BIT);
				vkCmdFillBuffer(cmd, m_buffer->value, 0, 4, 0xDEADBEEF);
			}
		}

		VkResult status() const
		{
			if (m_vk_event) [[likely]]
			{
				return vkGetEventStatus(m_device, m_vk_event);
			}
			else
			{
				return (*m_value == 0xDEADBEEF)? VK_EVENT_SET : VK_EVENT_RESET;
			}
		}
	};

	struct framebuffer
	{
		VkFramebuffer value;
		VkFramebufferCreateInfo info = {};
		std::vector<std::unique_ptr<vk::image_view>> attachments;
		u32 m_width = 0;
		u32 m_height = 0;

	public:
		framebuffer(VkDevice dev, VkRenderPass pass, u32 width, u32 height, std::vector<std::unique_ptr<vk::image_view>> &&atts)
			: attachments(std::move(atts))
			, m_device(dev)
		{
			std::vector<VkImageView> image_view_array(attachments.size());
			usz i = 0;
			for (const auto &att : attachments)
			{
				image_view_array[i++] = att->value;
			}

			info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			info.width = width;
			info.height = height;
			info.attachmentCount = static_cast<u32>(image_view_array.size());
			info.pAttachments = image_view_array.data();
			info.renderPass = pass;
			info.layers = 1;

			m_width = width;
			m_height = height;

			CHECK_RESULT(vkCreateFramebuffer(dev, &info, nullptr, &value));
		}

		~framebuffer()
		{
			vkDestroyFramebuffer(m_device, value, nullptr);
		}

		u32 width()
		{
			return m_width;
		}

		u32 height()
		{
			return m_height;
		}

		bool matches(std::vector<vk::image*> fbo_images, u32 width, u32 height)
		{
			if (m_width != width || m_height != height)
				return false;

			if (fbo_images.size() != attachments.size())
				return false;

			for (uint n = 0; n < fbo_images.size(); ++n)
			{
				if (attachments[n]->info.image != fbo_images[n]->value ||
					attachments[n]->info.format != fbo_images[n]->info.format)
					return false;
			}

			return true;
		}

		framebuffer(const framebuffer&) = delete;
		framebuffer(framebuffer&&) = delete;

	private:
		VkDevice m_device;
	};

	struct swapchain_image_WSI
	{
		VkImage value = VK_NULL_HANDLE;
	};

	class swapchain_image_RPCS3 : public image
	{
		std::unique_ptr<buffer> m_dma_buffer;
		u32 m_width = 0;
		u32 m_height = 0;

public:
		swapchain_image_RPCS3(render_device &dev, const memory_type_mapping& memory_map, u32 width, u32 height)
		:image(dev, memory_map.device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_TYPE_2D, VK_FORMAT_B8G8R8A8_UNORM, width, height, 1, 1, 1,
			VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 0)
		{
			m_width = width;
			m_height = height;
			current_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			m_dma_buffer = std::make_unique<buffer>(dev, m_width * m_height * 4, memory_map.host_visible_coherent,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0);
		}

		void do_dma_transfer(command_buffer& cmd)
		{
			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = m_width;
			copyRegion.bufferImageHeight = m_height;
			copyRegion.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
			copyRegion.imageOffset = {};
			copyRegion.imageExtent = {m_width, m_height, 1};

			VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			change_image_layout(cmd, this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresource_range);
			vkCmdCopyImageToBuffer(cmd, value, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_dma_buffer->value, 1, &copyRegion);
			change_image_layout(cmd, this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);
		}

		u32 get_required_memory_size() const
		{
			return m_width * m_height * 4;
		}

		void* get_pixels()
		{
			return m_dma_buffer->map(0, VK_WHOLE_SIZE);
		}

		void free_pixels()
		{
			m_dma_buffer->unmap();
		}
	};

	class swapchain_base
	{
	protected:
		render_device dev;

		u32 m_present_queue = UINT32_MAX;
		u32 m_graphics_queue = UINT32_MAX;
		VkQueue vk_graphics_queue = VK_NULL_HANDLE;
		VkQueue vk_present_queue = VK_NULL_HANDLE;

		display_handle_t window_handle{};
		u32 m_width = 0;
		u32 m_height = 0;
		VkFormat m_surface_format = VK_FORMAT_B8G8R8A8_UNORM;

		virtual void init_swapchain_images(render_device& dev, u32 count) = 0;

	public:
		swapchain_base(physical_device &gpu, u32 _present_queue, u32 _graphics_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
		{
			dev.create(gpu, _graphics_queue);

			if (_graphics_queue < UINT32_MAX) vkGetDeviceQueue(dev, _graphics_queue, 0, &vk_graphics_queue);
			if (_present_queue < UINT32_MAX) vkGetDeviceQueue(dev, _present_queue, 0, &vk_present_queue);

			m_present_queue = _present_queue;
			m_graphics_queue = _graphics_queue;
			m_surface_format = format;
		}

		virtual ~swapchain_base() = default;

		virtual void create(display_handle_t& handle) = 0;
		virtual void destroy(bool full = true) = 0;
		virtual bool init() = 0;

		virtual u32 get_swap_image_count() const = 0;
		virtual VkImage get_image(u32 index) = 0;
		virtual VkResult acquire_next_swapchain_image(VkSemaphore semaphore, u64 timeout, u32* result) = 0;
		virtual void end_frame(command_buffer& cmd, u32 index) = 0;
		virtual VkResult present(VkSemaphore semaphore, u32 index) = 0;
		virtual VkImageLayout get_optimal_present_layout() = 0;

		virtual bool supports_automatic_wm_reports() const
		{
			return false;
		}

		virtual bool init(u32 w, u32 h)
		{
			m_width = w;
			m_height = h;
			return init();
		}

		const vk::render_device& get_device()
		{
			return dev;
		}

		const VkQueue& get_present_queue()
		{
			return vk_present_queue;
		}

		const VkQueue& get_graphics_queue()
		{
			return vk_graphics_queue;
		}

		const VkFormat get_surface_format()
		{
			return m_surface_format;
		}

		const bool is_headless() const
		{
			return (vk_present_queue == VK_NULL_HANDLE);
		}
	};

	template<typename T>
	class abstract_swapchain_impl : public swapchain_base
	{
	protected:
		std::vector<T> swapchain_images;

	public:
		abstract_swapchain_impl(physical_device &gpu, u32 _present_queue, u32 _graphics_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
		: swapchain_base(gpu, _present_queue, _graphics_queue, format)
		{}

		~abstract_swapchain_impl() override = default;

		u32 get_swap_image_count() const override
		{
			return ::size32(swapchain_images);
		}

		using swapchain_base::init;
	};

	using native_swapchain_base = abstract_swapchain_impl<std::pair<bool, std::unique_ptr<swapchain_image_RPCS3>>>;
	using WSI_swapchain_base = abstract_swapchain_impl<swapchain_image_WSI>;

#ifdef _WIN32

	class swapchain_WIN32 : public native_swapchain_base
	{
		HDC hDstDC = NULL;
		HDC hSrcDC = NULL;
		HBITMAP hDIB = NULL;
		LPVOID hPtr = NULL;

	public:
		swapchain_WIN32(physical_device &gpu, u32 _present_queue, u32 _graphics_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
		: native_swapchain_base(gpu, _present_queue, _graphics_queue, format)
		{}

		~swapchain_WIN32(){}

		bool init() override
		{
			if (hDIB || hSrcDC)
				destroy(false);

			RECT rect;
			GetClientRect(window_handle, &rect);
			m_width = rect.right - rect.left;
			m_height = rect.bottom - rect.top;

			if (m_width == 0 || m_height == 0)
			{
				rsx_log.error("Invalid window dimensions %d x %d", m_width, m_height);
				return false;
			}

			BITMAPINFO bitmap = {};
			bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bitmap.bmiHeader.biWidth = m_width;
			bitmap.bmiHeader.biHeight = m_height * -1;
			bitmap.bmiHeader.biPlanes = 1;
			bitmap.bmiHeader.biBitCount = 32;
			bitmap.bmiHeader.biCompression = BI_RGB;

			hSrcDC = CreateCompatibleDC(hDstDC);
			hDIB = CreateDIBSection(hSrcDC, &bitmap, DIB_RGB_COLORS, &hPtr, NULL, 0);
			SelectObject(hSrcDC, hDIB);
			init_swapchain_images(dev, 3);
			return true;
		}

		void create(display_handle_t& handle) override
		{
			window_handle = handle;
			hDstDC = GetDC(handle);
		}

		void destroy(bool full=true) override
		{
			DeleteObject(hDIB);
			DeleteDC(hSrcDC);
			hDIB = NULL;
			hSrcDC = NULL;

			swapchain_images.clear();

			if (full)
			{
				ReleaseDC(window_handle, hDstDC);
				hDstDC = NULL;

				dev.destroy();
			}
		}

		VkResult present(VkSemaphore /*semaphore*/, u32 image) override
		{
			auto& src = swapchain_images[image];
			GdiFlush();

			if (hSrcDC)
			{
				memcpy(hPtr, src.second->get_pixels(), src.second->get_required_memory_size());
				BitBlt(hDstDC, 0, 0, m_width, m_height, hSrcDC, 0, 0, SRCCOPY);
				src.second->free_pixels();
			}

			src.first = false;
			return VK_SUCCESS;
		}
#elif defined(__APPLE__)

	class swapchain_MacOS : public native_swapchain_base
	{
		void* nsView = nullptr;

	public:
		swapchain_MacOS(physical_device &gpu, u32 _present_queue, u32 _graphics_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
		: native_swapchain_base(gpu, _present_queue, _graphics_queue, format)
		{}

		~swapchain_MacOS(){}

		bool init() override
		{
			//TODO: get from `nsView`
			m_width = 0;
			m_height = 0;

			if (m_width == 0 || m_height == 0)
			{
				rsx_log.error("Invalid window dimensions %d x %d", m_width, m_height);
				return false;
			}

			init_swapchain_images(dev, 3);
			return true;
		}

		void create(display_handle_t& window_handle) override
		{
			nsView = window_handle;
		}

		void destroy(bool full=true) override
		{
			swapchain_images.clear();

			if (full)
				dev.destroy();
		}

		VkResult present(VkSemaphore /*semaphore*/, u32 index) override
		{
			fmt::throw_exception("Native macOS swapchain is not implemented yet!");
		}
#elif defined(HAVE_X11)

	class swapchain_X11 : public native_swapchain_base
	{
		Display* display = nullptr;
		Window window = 0;
		XImage* pixmap = nullptr;
		GC gc = nullptr;
		int bit_depth = 24;

	public:
		swapchain_X11(physical_device &gpu, u32 _present_queue, u32 _graphics_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
		: native_swapchain_base(gpu, _present_queue, _graphics_queue, format)
		{}

		~swapchain_X11() override = default;

		bool init() override
		{
			if (pixmap)
				destroy(false);

			Window root;
			int x, y;
			u32 w = 0, h = 0, border, depth;

			if (XGetGeometry(display, window, &root, &x, &y, &w, & h, &border, &depth))
			{
				m_width = w;
				m_height = h;
				bit_depth = depth;
			}

			if (m_width == 0 || m_height == 0)
			{
				rsx_log.error("Invalid window dimensions %d x %d", m_width, m_height);
				return false;
			}

			XVisualInfo visual{};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
			if (!XMatchVisualInfo(display, DefaultScreen(display), bit_depth, TrueColor, &visual))
#pragma GCC diagnostic pop
			{
				rsx_log.error("Could not find matching visual info!");
				return false;
			}

			pixmap = XCreateImage(display, visual.visual, visual.depth, ZPixmap, 0, nullptr, m_width, m_height, 32, 0);
			init_swapchain_images(dev, 3);
			return true;
		}

		void create(display_handle_t& window_handle) override
		{
			std::visit([&](auto&& p)
			{
				using T = std::decay_t<decltype(p)>;
				if constexpr (std::is_same_v<T, std::pair<Display*, Window>>)
				{
					display = p.first;
					window = p.second;
				}
			}, window_handle);

			if (display == NULL)
			{
				rsx_log.fatal("Could not create virtual display on this window protocol (Wayland?)");
				return;
			}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
			gc = DefaultGC(display, DefaultScreen(display));
#pragma GCC diagnostic pop
		}

		void destroy(bool full=true) override
		{
			pixmap->data = nullptr;
			XDestroyImage(pixmap);
			pixmap = NULL;

			swapchain_images.clear();

			if (full)
				dev.destroy();
		}

		VkResult present(VkSemaphore /*semaphore*/, u32 index) override
		{
			auto& src = swapchain_images[index];
			if (pixmap)
			{
				pixmap->data = static_cast<char*>(src.second->get_pixels());

				XPutImage(display, window, gc, pixmap, 0, 0, 0, 0, m_width, m_height);
				XFlush(display);

				src.second->free_pixels();
			}

			//Release reference
			src.first = false;
			return VK_SUCCESS;
		}
#else

	class swapchain_Wayland : public native_swapchain_base
	{

	public:
		swapchain_Wayland(physical_device &gpu, u32 _present_queue, u32 _graphics_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
		: native_swapchain_base(gpu, _present_queue, _graphics_queue, format)
		{}

		~swapchain_Wayland(){}

		bool init() override
		{
			fmt::throw_exception("Native Wayland swapchain is not implemented yet!");
		}

		void create(display_handle_t& window_handle) override
		{
			fmt::throw_exception("Native Wayland swapchain is not implemented yet!");
		}

		void destroy(bool full=true) override
		{
			fmt::throw_exception("Native Wayland swapchain is not implemented yet!");
		}

		VkResult present(VkSemaphore /*semaphore*/, u32 index) override
		{
			fmt::throw_exception("Native Wayland swapchain is not implemented yet!");
		}
#endif

		VkResult acquire_next_swapchain_image(VkSemaphore /*semaphore*/, u64 /*timeout*/, u32* result) override
		{
			u32 index = 0;
			for (auto &p : swapchain_images)
			{
				if (!p.first)
				{
					p.first = true;
					*result = index;
					return VK_SUCCESS;
				}

				++index;
			}

			return VK_NOT_READY;
		}

		void end_frame(command_buffer& cmd, u32 index) override
		{
			swapchain_images[index].second->do_dma_transfer(cmd);
		}

		VkImage get_image(u32 index) override
		{
			return swapchain_images[index].second->value;
		}

		VkImageLayout get_optimal_present_layout() override
		{
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		}

	protected:
		void init_swapchain_images(render_device& dev, u32 preferred_count) override
		{
			swapchain_images.resize(preferred_count);
			for (auto &img : swapchain_images)
			{
				img.second = std::make_unique<swapchain_image_RPCS3>(dev, dev.get_memory_mapping(), m_width, m_height);
				img.first = false;
			}
		}
	};

	class swapchain_WSI : public WSI_swapchain_base
	{
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VkColorSpaceKHR m_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		VkSwapchainKHR m_vk_swapchain = nullptr;

		PFN_vkCreateSwapchainKHR createSwapchainKHR = nullptr;
		PFN_vkDestroySwapchainKHR destroySwapchainKHR = nullptr;
		PFN_vkGetSwapchainImagesKHR getSwapchainImagesKHR = nullptr;
		PFN_vkAcquireNextImageKHR acquireNextImageKHR = nullptr;
		PFN_vkQueuePresentKHR queuePresentKHR = nullptr;

		bool m_wm_reports_flag = false;

	protected:
		void init_swapchain_images(render_device& dev, u32 /*preferred_count*/ = 0) override
		{
			u32 nb_swap_images = 0;
			getSwapchainImagesKHR(dev, m_vk_swapchain, &nb_swap_images, nullptr);

			if (!nb_swap_images) fmt::throw_exception("Driver returned 0 images for swapchain");

			std::vector<VkImage> vk_images;
			vk_images.resize(nb_swap_images);
			getSwapchainImagesKHR(dev, m_vk_swapchain, &nb_swap_images, vk_images.data());

			swapchain_images.resize(nb_swap_images);
			for (u32 i = 0; i < nb_swap_images; ++i)
			{
				swapchain_images[i].value = vk_images[i];
			}
		}

	public:
		swapchain_WSI(vk::physical_device &gpu, u32 _present_queue, u32 _graphics_queue, VkFormat format, VkSurfaceKHR surface, VkColorSpaceKHR color_space, bool force_wm_reporting_off)
			: WSI_swapchain_base(gpu, _present_queue, _graphics_queue, format)
		{
			createSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(vkGetDeviceProcAddr(dev, "vkCreateSwapchainKHR"));
			destroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(vkGetDeviceProcAddr(dev, "vkDestroySwapchainKHR"));
			getSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(vkGetDeviceProcAddr(dev, "vkGetSwapchainImagesKHR"));
			acquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(vkGetDeviceProcAddr(dev, "vkAcquireNextImageKHR"));
			queuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(vkGetDeviceProcAddr(dev, "vkQueuePresentKHR"));

			m_surface = surface;
			m_color_space = color_space;

			if (!force_wm_reporting_off)
			{
				switch (gpu.get_driver_vendor())
				{
				case driver_vendor::AMD:
					break;
				case driver_vendor::INTEL:
#ifdef _WIN32
				break;
#endif
				case driver_vendor::NVIDIA:
				case driver_vendor::RADV:
					m_wm_reports_flag = true;
					break;
				default:
					break;
				}
			}
		}

		~swapchain_WSI() override = default;

		void create(display_handle_t&) override
		{}

		void destroy(bool = true) override
		{
			if (VkDevice pdev = dev)
			{
				if (m_vk_swapchain)
				{
					destroySwapchainKHR(pdev, m_vk_swapchain, nullptr);
				}

				dev.destroy();
			}
		}

		using WSI_swapchain_base::init;
		bool init() override
		{
			if (vk_present_queue == VK_NULL_HANDLE)
			{
				rsx_log.error("Cannot create WSI swapchain without a present queue");
				return false;
			}

			VkSwapchainKHR old_swapchain = m_vk_swapchain;
			vk::physical_device& gpu = const_cast<vk::physical_device&>(dev.gpu());

			VkSurfaceCapabilitiesKHR surface_descriptors = {};
			CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, m_surface, &surface_descriptors));

			if (surface_descriptors.maxImageExtent.width < m_width ||
				surface_descriptors.maxImageExtent.height < m_height)
			{
				rsx_log.error("Swapchain: Swapchain creation failed because dimensions cannot fit. Max = %d, %d, Requested = %d, %d",
					surface_descriptors.maxImageExtent.width, surface_descriptors.maxImageExtent.height, m_width, m_height);

				return false;
			}

			VkExtent2D swapchainExtent;
			if (surface_descriptors.currentExtent.width == UINT32_MAX)
			{
				swapchainExtent.width = m_width;
				swapchainExtent.height = m_height;
			}
			else
			{
				if (surface_descriptors.currentExtent.width == 0 || surface_descriptors.currentExtent.height == 0)
				{
					rsx_log.warning("Swapchain: Current surface extent is a null region. Is the window minimized?");
					return false;
				}

				swapchainExtent = surface_descriptors.currentExtent;
				m_width = surface_descriptors.currentExtent.width;
				m_height = surface_descriptors.currentExtent.height;
			}

			u32 nb_available_modes = 0;
			CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &nb_available_modes, nullptr));

			std::vector<VkPresentModeKHR> present_modes(nb_available_modes);
			CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &nb_available_modes, present_modes.data()));

			VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
			std::vector<VkPresentModeKHR> preferred_modes;

			if (!g_cfg.video.vk.force_fifo)
			{
				// List of preferred modes in decreasing desirability
				// NOTE: Always picks "triple-buffered vsync" types if possible
				if (!g_cfg.video.vsync)
				{
					preferred_modes = { VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR };
				}
			}

			bool mode_found = false;
			for (VkPresentModeKHR preferred_mode : preferred_modes)
			{
				//Search for this mode in supported modes
				for (VkPresentModeKHR mode : present_modes)
				{
					if (mode == preferred_mode)
					{
						swapchain_present_mode = mode;
						mode_found = true;
						break;
					}
				}

				if (mode_found)
					break;
			}

			rsx_log.notice("Swapchain: present mode %d in use.", static_cast<int>(swapchain_present_mode));

			u32 nb_swap_images = surface_descriptors.minImageCount + 1;
			if (surface_descriptors.maxImageCount > 0)
			{
				//Try to negotiate for a triple buffer setup
				//In cases where the front-buffer isnt available for present, its better to have a spare surface
				nb_swap_images = std::max(surface_descriptors.minImageCount + 2u, 3u);

				if (nb_swap_images > surface_descriptors.maxImageCount)
				{
					// Application must settle for fewer images than desired:
					nb_swap_images = surface_descriptors.maxImageCount;
				}
			}

			VkSurfaceTransformFlagBitsKHR pre_transform = surface_descriptors.currentTransform;
			if (surface_descriptors.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
				pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

			VkSwapchainCreateInfoKHR swap_info = {};
			swap_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swap_info.surface = m_surface;
			swap_info.minImageCount = nb_swap_images;
			swap_info.imageFormat = m_surface_format;
			swap_info.imageColorSpace = m_color_space;

			swap_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			swap_info.preTransform = pre_transform;
			swap_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swap_info.imageArrayLayers = 1;
			swap_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swap_info.presentMode = swapchain_present_mode;
			swap_info.oldSwapchain = old_swapchain;
			swap_info.clipped = true;

			swap_info.imageExtent.width = std::max(m_width, surface_descriptors.minImageExtent.width);
			swap_info.imageExtent.height = std::max(m_height, surface_descriptors.minImageExtent.height);

			createSwapchainKHR(dev, &swap_info, nullptr, &m_vk_swapchain);

			if (old_swapchain)
			{
				if (!swapchain_images.empty())
				{
					swapchain_images.clear();
				}

				destroySwapchainKHR(dev, old_swapchain, nullptr);
			}

			init_swapchain_images(dev);
			return true;
		}

		bool supports_automatic_wm_reports() const override
		{
			return m_wm_reports_flag;
		}

		VkResult acquire_next_swapchain_image(VkSemaphore semaphore, u64 timeout, u32* result) override
		{
			return vkAcquireNextImageKHR(dev, m_vk_swapchain, timeout, semaphore, VK_NULL_HANDLE, result);
		}

		void end_frame(command_buffer& /*cmd*/, u32 /*index*/) override
		{
		}

		VkResult present(VkSemaphore semaphore, u32 image) override
		{
			VkPresentInfoKHR present = {};
			present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			present.pNext = nullptr;
			present.swapchainCount = 1;
			present.pSwapchains = &m_vk_swapchain;
			present.pImageIndices = &image;
			present.waitSemaphoreCount = 1;
			present.pWaitSemaphores = &semaphore;

			return queuePresentKHR(vk_present_queue, &present);
		}

		VkImage get_image(u32 index) override
		{
			return swapchain_images[index].value;
		}

		VkImageLayout get_optimal_present_layout() override
		{
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}
	};

	class context
	{
	private:
		std::vector<physical_device> gpus;
		VkInstance m_instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;

		PFN_vkDestroyDebugReportCallbackEXT destroyDebugReportCallback = nullptr;
		PFN_vkCreateDebugReportCallbackEXT createDebugReportCallback = nullptr;
		VkDebugReportCallbackEXT m_debugger = nullptr;

		bool extensions_loaded = false;

	public:

		context() = default;

		~context()
		{
			if (m_instance)
				close();
		}

		void close()
		{
			if (!m_instance) return;

			if (m_debugger)
			{
				destroyDebugReportCallback(m_instance, m_debugger, nullptr);
				m_debugger = nullptr;
			}

			if (m_surface)
			{
				vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
				m_surface = VK_NULL_HANDLE;
			}

			vkDestroyInstance(m_instance, nullptr);
			m_instance = VK_NULL_HANDLE;
		}

		void enable_debugging()
		{
			if (!g_cfg.video.debug_output) return;

			PFN_vkDebugReportCallbackEXT callback = vk::dbgFunc;

			createDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT"));
			destroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT"));

			VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
			dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
			dbgCreateInfo.pfnCallback = callback;
			dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

			CHECK_RESULT(createDebugReportCallback(m_instance, &dbgCreateInfo, NULL, &m_debugger));
		}
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
		bool createInstance(const char *app_name, bool fast = false)
		{
			// Initialize a vulkan instance
			VkApplicationInfo app = {};

			app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			app.pApplicationName = app_name;
			app.applicationVersion = 0;
			app.pEngineName = app_name;
			app.engineVersion = 0;
			app.apiVersion = VK_API_VERSION_1_0;

			// Set up instance information

			std::vector<const char *> extensions;
			std::vector<const char *> layers;

			if (!fast)
			{
				extensions_loaded = true;
				supported_extensions support(supported_extensions::instance);

				extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
				if (support.is_supported(VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
				{
					extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
				}

				if (support.is_supported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
				{
					extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
				}
#ifdef _WIN32
				extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(__APPLE__)
				extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#else
				bool found_surface_ext = false;
#ifdef HAVE_X11
				if (support.is_supported(VK_KHR_XLIB_SURFACE_EXTENSION_NAME))
				{
					extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
					found_surface_ext = true;
				}
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
				if (support.is_supported(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME))
				{
					extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
					found_surface_ext = true;
				}
#endif //(WAYLAND)
				if (!found_surface_ext)
				{
					rsx_log.error("Could not find a supported Vulkan surface extension");
					return 0;
				}
#endif //(WIN32, __APPLE__)
				if (g_cfg.video.debug_output)
					layers.push_back("VK_LAYER_KHRONOS_validation");
			}

			VkInstanceCreateInfo instance_info = {};
			instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instance_info.pApplicationInfo = &app;
			instance_info.enabledLayerCount = static_cast<u32>(layers.size());
			instance_info.ppEnabledLayerNames = layers.data();
			instance_info.enabledExtensionCount = fast ? 0 : static_cast<u32>(extensions.size());
			instance_info.ppEnabledExtensionNames = fast ? nullptr : extensions.data();

			if (VkResult result = vkCreateInstance(&instance_info, nullptr, &m_instance); result != VK_SUCCESS)
			{
				if (result == VK_ERROR_LAYER_NOT_PRESENT)
				{
					rsx_log.fatal("Could not initialize layer VK_LAYER_KHRONOS_validation");
				}

				return false;
			}

			return true;
		}
#ifdef __clang__
#pragma clang diagnostic pop
#endif
		void makeCurrentInstance()
		{
			// Register some global states
			if (m_debugger)
			{
				destroyDebugReportCallback(m_instance, m_debugger, nullptr);
				m_debugger = nullptr;
			}

			enable_debugging();
		}

		VkInstance getCurrentInstance()
		{
			return m_instance;
		}

		std::vector<physical_device>& enumerateDevices()
		{
			u32 num_gpus;
			// This may fail on unsupported drivers, so just assume no devices
			if (vkEnumeratePhysicalDevices(m_instance, &num_gpus, nullptr) != VK_SUCCESS)
				return gpus;

			if (gpus.size() != num_gpus)
			{
				std::vector<VkPhysicalDevice> pdevs(num_gpus);
				gpus.resize(num_gpus);

				CHECK_RESULT(vkEnumeratePhysicalDevices(m_instance, &num_gpus, pdevs.data()));

				for (u32 i = 0; i < num_gpus; ++i)
					gpus[i].create(m_instance, pdevs[i], extensions_loaded);
			}

			return gpus;
		}

		swapchain_base* createSwapChain(display_handle_t window_handle, vk::physical_device &dev)
		{
			bool force_wm_reporting_off = false;
#ifdef _WIN32
			using swapchain_NATIVE = swapchain_WIN32;
			HINSTANCE hInstance = NULL;

			VkWin32SurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.hinstance = hInstance;
			createInfo.hwnd = window_handle;

			CHECK_RESULT(vkCreateWin32SurfaceKHR(m_instance, &createInfo, NULL, &m_surface));

#elif defined(__APPLE__)
			using swapchain_NATIVE = swapchain_MacOS;
			VkMacOSSurfaceCreateInfoMVK createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
			createInfo.pView = window_handle;

			CHECK_RESULT(vkCreateMacOSSurfaceMVK(m_instance, &createInfo, NULL, &m_surface));
#else
#ifdef HAVE_X11
			using swapchain_NATIVE = swapchain_X11;
#else
			using swapchain_NATIVE = swapchain_Wayland;
#endif

			std::visit([&](auto&& p)
			{
				using T = std::decay_t<decltype(p)>;

#ifdef HAVE_X11
				if constexpr (std::is_same_v<T, std::pair<Display*, Window>>)
				{
					VkXlibSurfaceCreateInfoKHR createInfo = {};
					createInfo.sType                      = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
					createInfo.dpy                        = p.first;
					createInfo.window                     = p.second;
					CHECK_RESULT(vkCreateXlibSurfaceKHR(this->m_instance, &createInfo, nullptr, &m_surface));
				}
				else
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
				if constexpr (std::is_same_v<T, std::pair<wl_display*, wl_surface*>>)
				{
					VkWaylandSurfaceCreateInfoKHR createInfo = {};
					createInfo.sType                         = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
					createInfo.display                       = p.first;
					createInfo.surface                       = p.second;
					CHECK_RESULT(vkCreateWaylandSurfaceKHR(this->m_instance, &createInfo, nullptr, &m_surface));
					force_wm_reporting_off = true;
				}
				else
#endif
				{
					static_assert(std::conditional_t<true, std::false_type, T>::value, "Unhandled window_handle type in std::variant");
				}
			}, window_handle);
#endif

			u32 device_queues = dev.get_queue_count();
			std::vector<VkBool32> supportsPresent(device_queues, VK_FALSE);
			bool present_possible = false;

			for (u32 index = 0; index < device_queues; index++)
			{
				vkGetPhysicalDeviceSurfaceSupportKHR(dev, index, m_surface, &supportsPresent[index]);
			}

			for (const auto &value : supportsPresent)
			{
				if (value)
				{
					present_possible = true;
					break;
				}
			}

			if (!present_possible)
			{
				rsx_log.error("It is not possible for the currently selected GPU to present to the window (Likely caused by NVIDIA driver running the current display)");
			}

			// Search for a graphics and a present queue in the array of queue
			// families, try to find one that supports both
			u32 graphicsQueueNodeIndex = UINT32_MAX;
			u32 presentQueueNodeIndex = UINT32_MAX;

			for (u32 i = 0; i < device_queues; i++)
			{
				if ((dev.get_queue_properties(i).queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
				{
					if (graphicsQueueNodeIndex == UINT32_MAX)
						graphicsQueueNodeIndex = i;

					if (supportsPresent[i] == VK_TRUE)
					{
						graphicsQueueNodeIndex = i;
						presentQueueNodeIndex = i;

						break;
					}
				}
			}

			if (presentQueueNodeIndex == UINT32_MAX)
			{
				// If didn't find a queue that supports both graphics and present, then
				// find a separate present queue.
				for (u32 i = 0; i < device_queues; ++i)
				{
					if (supportsPresent[i] == VK_TRUE)
					{
						presentQueueNodeIndex = i;
						break;
					}
				}
			}

			if (graphicsQueueNodeIndex == UINT32_MAX)
			{
				rsx_log.fatal("Failed to find a suitable graphics queue");
				return nullptr;
			}

			if (graphicsQueueNodeIndex != presentQueueNodeIndex)
			{
				//Separate graphics and present, use headless fallback
				present_possible = false;
			}

			if (!present_possible)
			{
				//Native(sw) swapchain
				rsx_log.warning("Falling back to software present support (native windowing API)");
				auto swapchain = new swapchain_NATIVE(dev, UINT32_MAX, graphicsQueueNodeIndex);
				swapchain->create(window_handle);
				return swapchain;
			}

			// Get the list of VkFormat's that are supported:
			u32 formatCount;
			CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(dev, m_surface, &formatCount, nullptr));

			std::vector<VkSurfaceFormatKHR> surfFormats(formatCount);
			CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(dev, m_surface, &formatCount, surfFormats.data()));

			VkFormat format;
			VkColorSpaceKHR color_space;

			if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				format = VK_FORMAT_B8G8R8A8_UNORM;
			}
			else
			{
				if (!formatCount) fmt::throw_exception("Format count is zero!");
				format = surfFormats[0].format;

				//Prefer BGRA8_UNORM to avoid sRGB compression (RADV)
				for (auto& surface_format: surfFormats)
				{
					if (surface_format.format == VK_FORMAT_B8G8R8A8_UNORM)
					{
						format = VK_FORMAT_B8G8R8A8_UNORM;
						break;
					}
				}
			}

			color_space = surfFormats[0].colorSpace;

			return new swapchain_WSI(dev, presentQueueNodeIndex, graphicsQueueNodeIndex, format, m_surface, color_space, force_wm_reporting_off);
		}
	};

	class descriptor_pool
	{
		const vk::render_device *m_owner = nullptr;

		std::vector<VkDescriptorPool> m_device_pools;
		VkDescriptorPool m_current_pool_handle = VK_NULL_HANDLE;
		u32 m_current_pool_index = 0;

	public:
		descriptor_pool() = default;
		~descriptor_pool() = default;

		void create(const vk::render_device &dev, VkDescriptorPoolSize *sizes, u32 size_descriptors_count, u32 max_sets, u8 subpool_count)
		{
			ensure(subpool_count);

			VkDescriptorPoolCreateInfo infos = {};
			infos.flags = 0;
			infos.maxSets = max_sets;
			infos.poolSizeCount = size_descriptors_count;
			infos.pPoolSizes = sizes;
			infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

			m_owner = &dev;
			m_device_pools.resize(subpool_count);

			for (auto &pool : m_device_pools)
			{
				CHECK_RESULT(vkCreateDescriptorPool(dev, &infos, nullptr, &pool));
			}

			m_current_pool_handle = m_device_pools[0];
		}

		void destroy()
		{
			if (m_device_pools.empty()) return;

			for (auto &pool : m_device_pools)
			{
				vkDestroyDescriptorPool((*m_owner), pool, nullptr);
				pool = VK_NULL_HANDLE;
			}

			m_owner = nullptr;
		}

		bool valid()
		{
			return (!m_device_pools.empty());
		}

		operator VkDescriptorPool()
		{
			return m_current_pool_handle;
		}

		void reset(VkDescriptorPoolResetFlags flags)
		{
			m_current_pool_index = (m_current_pool_index + 1) % u32(m_device_pools.size());
			m_current_pool_handle = m_device_pools[m_current_pool_index];
			CHECK_RESULT(vkResetDescriptorPool(*m_owner, m_current_pool_handle, flags));
		}
	};

	class data_heap : public ::data_heap
	{
	private:
		usz initial_size = 0;
		bool mapped = false;
		void *_ptr = nullptr;

		bool notify_on_grow = false;

		std::unique_ptr<buffer> shadow;
		std::vector<VkBufferCopy> dirty_ranges;

	protected:
		bool grow(usz size) override;

	public:
		std::unique_ptr<buffer> heap;

		// NOTE: Some drivers (RADV) use heavyweight OS map/unmap routines that are insanely slow
		// Avoid mapping/unmapping to keep these drivers from stalling
		// NOTE2: HOST_CACHED flag does not keep the mapped ptr around in the driver either

		void create(VkBufferUsageFlags usage, usz size, const char *name, usz guard = 0x10000, VkBool32 notify = VK_FALSE)
		{
			::data_heap::init(size, name, guard);

			const auto device = get_current_renderer();
			const auto& memory_map = device->get_memory_mapping();

			VkFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			auto memory_index = memory_map.host_visible_coherent;

			if (!(get_heap_compatible_buffer_types() & usage))
			{
				rsx_log.warning("Buffer usage %u is not heap-compatible using this driver, explicit staging buffer in use", usage);

				shadow = std::make_unique<buffer>(*device, size, memory_index, memory_flags, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0);
				usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				memory_index = memory_map.device_local;
			}

			heap = std::make_unique<buffer>(*device, size, memory_index, memory_flags, usage, 0);

			initial_size = size;
			notify_on_grow = bool(notify);
		}

		void destroy()
		{
			if (mapped)
			{
				unmap(true);
			}

			heap.reset();
			shadow.reset();
		}

		void* map(usz offset, usz size)
		{
			if (!_ptr)
			{
				if (shadow)
					_ptr = shadow->map(0, shadow->size());
				else
					_ptr = heap->map(0, heap->size());

				mapped = true;
			}

			if (shadow)
			{
				dirty_ranges.push_back({offset, offset, size});
				raise_status_interrupt(runtime_state::heap_dirty);
			}

			return static_cast<u8*>(_ptr) + offset;
		}

		void unmap(bool force = false)
		{
			if (force)
			{
				if (shadow)
					shadow->unmap();
				else
					heap->unmap();

				mapped = false;
				_ptr = nullptr;
			}
		}

		bool dirty()
		{
			return !dirty_ranges.empty();
		}

		void sync(const vk::command_buffer& cmd)
		{
			if (!dirty_ranges.empty())
			{
				ensure(shadow);
				ensure(heap);
				vkCmdCopyBuffer(cmd, shadow->value, heap->value, ::size32(dirty_ranges), dirty_ranges.data());
				dirty_ranges.clear();

				insert_buffer_memory_barrier(cmd, heap->value, 0, heap->size(),
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
						VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
			}
		}

		bool is_critical() const override
		{
			if (!::data_heap::is_critical())
				return false;

			// By default, allow the size to grow upto 8x larger
			// This value is arbitrary, theoretically it is possible to allow infinite stretching to improve performance
			const usz soft_limit = initial_size * 8;
			if ((m_size + m_min_guard_size) < soft_limit)
				return false;

			return true;
		}
	};

	struct blitter
	{
		void scale_image(vk::command_buffer& cmd, vk::image* src, vk::image* dst, areai src_area, areai dst_area, bool interpolate, const rsx::typeless_xfer& xfer_info);
	};
}
