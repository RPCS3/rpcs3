#pragma once

#include "stdafx.h"
#include <exception>
#include <string>
#include <cstring>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <X11/Xutil.h>
#endif

#include "Emu/RSX/GSRender.h"
#include "Emu/System.h"
#include "VulkanAPI.h"
#include "VKCommonDecompiler.h"
#include "../GCM.h"
#include "../Common/TextureUtils.h"
#include "../Common/ring_buffer_helper.h"
#include "../rsx_cache.h"

#include "3rdparty/GPUOpen/include/vk_mem_alloc.h"

#ifdef __APPLE__
#define VK_DISABLE_COMPONENT_SWIZZLE 1
#else
#define VK_DISABLE_COMPONENT_SWIZZLE 0
#endif

#define DESCRIPTOR_MAX_DRAW_CALLS 4096
#define OCCLUSION_MAX_POOL_SIZE 8192

#define VERTEX_PARAMS_BIND_SLOT 0
#define VERTEX_LAYOUT_BIND_SLOT 1
#define VERTEX_CONSTANT_BUFFERS_BIND_SLOT 2
#define FRAGMENT_CONSTANT_BUFFERS_BIND_SLOT 3
#define FRAGMENT_STATE_BIND_SLOT 4
#define FRAGMENT_TEXTURE_PARAMS_BIND_SLOT 5
#define VERTEX_BUFFERS_FIRST_BIND_SLOT 6
#define TEXTURES_FIRST_BIND_SLOT 8
#define VERTEX_TEXTURES_FIRST_BIND_SLOT 24 //8+16

#define VK_NUM_DESCRIPTOR_BINDINGS (VERTEX_TEXTURES_FIRST_BIND_SLOT + 4)

namespace rsx
{
	class fragment_texture;
}

namespace vk
{
#define CHECK_RESULT(expr) { VkResult _res = (expr); if (_res != VK_SUCCESS) vk::die_with_error(HERE, _res); }

	VKAPI_ATTR void *VKAPI_CALL mem_realloc(void *pUserData, void *pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
	VKAPI_ATTR void *VKAPI_CALL mem_alloc(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
	VKAPI_ATTR void VKAPI_CALL mem_free(void *pUserData, void *pMemory);

	VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
											uint64_t srcObject, size_t location, int32_t msgCode,
											const char *pLayerPrefix, const char *pMsg, void *pUserData);

	VkBool32 BreakCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
							uint64_t srcObject, size_t location, int32_t msgCode,
							const char *pLayerPrefix, const char *pMsg,
							void *pUserData);

	//VkAllocationCallbacks default_callbacks();

	enum driver_vendor
	{
		unknown,
		AMD,
		NVIDIA,
		RADV
	};

	class context;
	class render_device;
	class swap_chain_image;
	class physical_device;
	class command_buffer;
	struct image;
	struct buffer;
	struct data_heap;
	class mem_allocator_base;
	struct memory_type_mapping;
	struct gpu_formats_support;

	const vk::context *get_current_thread_ctx();
	void set_current_thread_ctx(const vk::context &ctx);

	const vk::render_device *get_current_renderer();
	void set_current_renderer(const vk::render_device &device);

	mem_allocator_base *get_current_mem_allocator();

	//Compatibility workarounds
	bool emulate_primitive_restart(rsx::primitive_type type);
	bool sanitize_fp_values();
	bool fence_reset_disabled();
	VkFlags get_heap_compatible_buffer_types();
	driver_vendor get_driver_vendor();

	VkComponentMapping default_component_map();
	VkComponentMapping apply_swizzle_remap(const std::array<VkComponentSwizzle, 4>& base_remap, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap_vector);
	VkImageSubresource default_image_subresource();
	VkImageSubresourceRange get_image_subresource_range(uint32_t base_layer, uint32_t base_mip, uint32_t layer_count, uint32_t level_count, VkImageAspectFlags aspect);
	VkImageAspectFlags get_aspect_flags(VkFormat format);

	VkSampler null_sampler();
	VkImageView null_image_view(vk::command_buffer&);
	image* get_typeless_helper(VkFormat format);
	buffer* get_scratch_buffer();

	memory_type_mapping get_memory_mapping(const physical_device& dev);
	gpu_formats_support get_optimal_tiling_supported_formats(const physical_device& dev);

	//Sync helpers around vkQueueSubmit
	void acquire_global_submit_lock();
	void release_global_submit_lock();

	template<class T>
	T* get_compute_task();
	void reset_compute_tasks();

	void destroy_global_resources();

	/**
	* Allocate enough space in upload_buffer and write all mipmap/layer data into the subbuffer.
	* Then copy all layers into dst_image.
	* dst_image must be in TRANSFER_DST_OPTIMAL layout and upload_buffer have TRANSFER_SRC_BIT usage flag.
	*/
	void copy_mipmaped_image_using_buffer(VkCommandBuffer cmd, vk::image* dst_image,
		const std::vector<rsx_subresource_layout>& subresource_layout, int format, bool is_swizzled, u16 mipmap_count,
		VkImageAspectFlags flags, vk::data_heap &upload_heap);

	//Other texture management helpers
	void change_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout, VkImageSubresourceRange range);
	void change_image_layout(VkCommandBuffer cmd, vk::image *image, VkImageLayout new_layout, VkImageSubresourceRange range);
	void change_image_layout(VkCommandBuffer cmd, vk::image *image, VkImageLayout new_layout);

	void copy_image_typeless(const command_buffer &cmd, const image *src, const image *dst, const areai& src_rect, const areai& dst_rect,
		u32 mipmaps, VkImageAspectFlags src_aspect, VkImageAspectFlags dst_aspect,
		VkImageAspectFlags src_transfer_mask = 0xFF, VkImageAspectFlags dst_transfer_mask = 0xFF);

	void copy_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkImageLayout srcLayout, VkImageLayout dstLayout,
			const areai& src_rect, const areai& dst_rect, u32 mipmaps, VkImageAspectFlags src_aspect, VkImageAspectFlags dst_aspect,
			VkImageAspectFlags src_transfer_mask = 0xFF, VkImageAspectFlags dst_transfer_mask = 0xFF);

	void copy_scaled_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkImageLayout srcLayout, VkImageLayout dstLayout,
			u32 src_x_offset, u32 src_y_offset, u32 src_width, u32 src_height, u32 dst_x_offset, u32 dst_y_offset, u32 dst_width, u32 dst_height, u32 mipmaps,
			VkImageAspectFlags aspect, bool compatible_formats, VkFilter filter = VK_FILTER_LINEAR, VkFormat src_format = VK_FORMAT_UNDEFINED, VkFormat dst_format = VK_FORMAT_UNDEFINED);

	std::pair<VkFormat, VkComponentMapping> get_compatible_surface_format(rsx::surface_color_format color_format);
	size_t get_render_pass_location(VkFormat color_surface_format, VkFormat depth_stencil_format, u8 color_surface_count);

	//Texture barrier applies to a texture to ensure writes to it are finished before any reads are attempted to avoid RAW hazards
	void insert_texture_barrier(VkCommandBuffer cmd, VkImage image, VkImageLayout layout, VkImageSubresourceRange range);
	void insert_texture_barrier(VkCommandBuffer cmd, vk::image *image);

	void insert_buffer_memory_barrier(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize length,
			VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkAccessFlags src_mask, VkAccessFlags dst_mask);

	//Manage 'uininterruptible' state where secondary operations (e.g violation handlers) will have to wait
	void enter_uninterruptible();
	void leave_uninterruptible();
	bool is_uninterruptible();

	void advance_completed_frame_counter();
	void advance_frame_counter();
	const u64 get_current_frame_id();
	const u64 get_last_completed_frame_id();

	//Fence reset with driver workarounds in place
	void reset_fence(VkFence *pFence);
	void wait_for_fence(VkFence pFence);

	void die_with_error(const char* faulting_addr, VkResult error_code);

	struct memory_type_mapping
	{
		uint32_t host_visible_coherent;
		uint32_t device_local;
	};

	struct gpu_formats_support
	{
		bool d24_unorm_s8;
		bool d32_sfloat_s8;
		bool bgra8_linear;
	};

	// Memory Allocator - base class

	class mem_allocator_base
	{
	public:
		using mem_handle_t = void *;

		mem_allocator_base(VkDevice dev, VkPhysicalDevice /*pdev*/) : m_device(dev) {}
		virtual ~mem_allocator_base() {}

		virtual void destroy() = 0;

		virtual mem_handle_t alloc(u64 block_sz, u64 alignment, uint32_t memory_type_index) = 0;
		virtual void free(mem_handle_t mem_handle) = 0;
		virtual void *map(mem_handle_t mem_handle, u64 offset, u64 size) = 0;
		virtual void unmap(mem_handle_t mem_handle) = 0;
		virtual VkDeviceMemory get_vk_device_memory(mem_handle_t mem_handle) = 0;
		virtual u64 get_vk_device_memory_offset(mem_handle_t mem_handle) = 0;

	protected:
		VkDevice m_device;
	private:
	};

	// Memory Allocator - Vulkan Memory Allocator 
	// https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator

	class mem_allocator_vma : public mem_allocator_base
	{
	public:
		mem_allocator_vma(VkDevice dev, VkPhysicalDevice pdev) : mem_allocator_base(dev, pdev)
		{
			VmaAllocatorCreateInfo allocatorInfo = {};
			allocatorInfo.physicalDevice = pdev;
			allocatorInfo.device = dev;

			vmaCreateAllocator(&allocatorInfo, &m_allocator);
		}

		~mem_allocator_vma() {};

		void destroy() override
		{
			vmaDestroyAllocator(m_allocator);
		}

		mem_handle_t alloc(u64 block_sz, u64 alignment, uint32_t memory_type_index) override
		{
			VmaAllocation vma_alloc;
			VkMemoryRequirements mem_req = {};
			VmaAllocationCreateInfo create_info = {};

			mem_req.memoryTypeBits = 1u << memory_type_index;
			mem_req.size = block_sz;
			mem_req.alignment = alignment;
			create_info.memoryTypeBits = 1u << memory_type_index;
			CHECK_RESULT(vmaAllocateMemory(m_allocator, &mem_req, &create_info, &vma_alloc, nullptr));
			return vma_alloc;
		}

		void free(mem_handle_t mem_handle) override
		{
			vmaFreeMemory(m_allocator, static_cast<VmaAllocation>(mem_handle));
		}

		void *map(mem_handle_t mem_handle, u64 offset, u64 /*size*/) override
		{
			void *data = nullptr;

			CHECK_RESULT(vmaMapMemory(m_allocator, static_cast<VmaAllocation>(mem_handle), &data));

			// Add offset
			data = static_cast<u8 *>(data) + offset;
			return data;
		}

		void unmap(mem_handle_t mem_handle) override
		{
			vmaUnmapMemory(m_allocator, static_cast<VmaAllocation>(mem_handle));
		}

		VkDeviceMemory get_vk_device_memory(mem_handle_t mem_handle) override
		{
			VmaAllocationInfo alloc_info;

			vmaGetAllocationInfo(m_allocator, static_cast<VmaAllocation>(mem_handle), &alloc_info);
			return alloc_info.deviceMemory;
		}

		u64 get_vk_device_memory_offset(mem_handle_t mem_handle) override
		{
			VmaAllocationInfo alloc_info;

			vmaGetAllocationInfo(m_allocator, static_cast<VmaAllocation>(mem_handle), &alloc_info);
			return alloc_info.offset;
		}

	private:
		VmaAllocator m_allocator;
	};

	// Memory Allocator - built-in Vulkan device memory allocate/free

	class mem_allocator_vk : public mem_allocator_base
	{
	public:
		mem_allocator_vk(VkDevice dev, VkPhysicalDevice pdev) : mem_allocator_base(dev, pdev) {};
		~mem_allocator_vk() {};

		void destroy() override {};

		mem_handle_t alloc(u64 block_sz, u64 /*alignment*/, uint32_t memory_type_index) override
		{
			VkDeviceMemory memory;
			VkMemoryAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			info.allocationSize = block_sz;
			info.memoryTypeIndex = memory_type_index;

			CHECK_RESULT(vkAllocateMemory(m_device, &info, nullptr, &memory));
			return memory;
		}

		void free(mem_handle_t mem_handle) override
		{
			vkFreeMemory(m_device, (VkDeviceMemory)mem_handle, nullptr);
		}

		void *map(mem_handle_t mem_handle, u64 offset, u64 size) override
		{
			void *data = nullptr;
			CHECK_RESULT(vkMapMemory(m_device, (VkDeviceMemory)mem_handle, offset, std::max<u64>(size, 1u), 0, &data));
			return data;
		}

		void unmap(mem_handle_t mem_handle) override
		{
			vkUnmapMemory(m_device, (VkDeviceMemory)mem_handle);
		}

		VkDeviceMemory get_vk_device_memory(mem_handle_t mem_handle) override
		{
			return (VkDeviceMemory)mem_handle;
		}

		u64 get_vk_device_memory_offset(mem_handle_t /*mem_handle*/) override
		{
			return 0;
		}

	private:
	};

	struct memory_block
	{
		memory_block(VkDevice dev, u64 block_sz, u64 alignment, uint32_t memory_type_index) : m_device(dev)
		{
			m_mem_allocator = get_current_mem_allocator();
			m_mem_handle = m_mem_allocator->alloc(block_sz, alignment, memory_type_index);
		}

		~memory_block()
		{
			m_mem_allocator->free(m_mem_handle);
		}

		VkDeviceMemory get_vk_device_memory()
		{
			return m_mem_allocator->get_vk_device_memory(m_mem_handle);
		}

		u64 get_vk_device_memory_offset()
		{
			return m_mem_allocator->get_vk_device_memory_offset(m_mem_handle);
		}

		void *map(u64 offset, u64 size)
		{
			return m_mem_allocator->map(m_mem_handle, offset, size);
		}

		void unmap()
		{
			m_mem_allocator->unmap(m_mem_handle);
		}

		memory_block(const memory_block&) = delete;
		memory_block(memory_block&&) = delete;

	private:
		VkDevice m_device;
		vk::mem_allocator_base* m_mem_allocator;
		mem_allocator_base::mem_handle_t m_mem_handle;
	};

	class physical_device
	{
		VkPhysicalDevice dev = nullptr;
		VkPhysicalDeviceProperties props;
		VkPhysicalDeviceMemoryProperties memory_properties;
		std::vector<VkQueueFamilyProperties> queue_props;

	public:

		physical_device() {}
		~physical_device() {}

		void set_device(VkPhysicalDevice pdev)
		{
			dev = pdev;
			vkGetPhysicalDeviceProperties(pdev, &props);
			vkGetPhysicalDeviceMemoryProperties(pdev, &memory_properties);

			LOG_NOTICE(RSX, "Found vulkan-compatible GPU: '%s' running on driver %s", get_name(), get_driver_version());
		}

		std::string get_name() const
		{
			return props.deviceName;
		}

		driver_vendor get_driver_vendor() const
		{
			const auto gpu_name = get_name();
			if (gpu_name.find("Radeon") != std::string::npos)
			{
				return driver_vendor::AMD;
			}

			if (gpu_name.find("NVIDIA") != std::string::npos || gpu_name.find("GeForce") != std::string::npos)
			{
				return driver_vendor::NVIDIA;
			}

			if (gpu_name.find("RADV") != std::string::npos)
			{
				return driver_vendor::RADV;
			}

			return driver_vendor::unknown;
		}

		std::string get_driver_version() const
		{
			switch (get_driver_vendor())
			{
			case driver_vendor::NVIDIA:
			{
				// 10 + 8 + 8 + 6
				const auto major_version = VK_VERSION_MAJOR(props.driverVersion);
				const auto minor_version = (props.driverVersion >> 14) & 0xff;
				const auto patch = (props.driverVersion >> 6) & 0xff;
				const auto revision = (props.driverVersion & 0x3f);

				return fmt::format("%u.%u.%u.%u", major_version, minor_version, patch, revision);
			}
			default:
			{
				// 10 + 10 + 12 (standard vulkan encoding created with VK_MAKE_VERSION)
				return fmt::format("%u.%u.%u",
					VK_VERSION_MAJOR(props.driverVersion),
					VK_VERSION_MINOR(props.driverVersion),
					VK_VERSION_PATCH(props.driverVersion));
			}
			}
		}

		uint32_t get_queue_count() const
		{
			if (queue_props.size())
				return (u32)queue_props.size();

			uint32_t count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);

			return count;
		}

		VkQueueFamilyProperties get_queue_properties(uint32_t queue)
		{
			if (!queue_props.size())
			{
				uint32_t count = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);

				queue_props.resize(count);
				vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, queue_props.data());
			}

			if (queue >= queue_props.size()) fmt::throw_exception("Bad queue index passed to get_queue_properties (%u)" HERE, queue);
			return queue_props[queue];
		}

		VkPhysicalDeviceMemoryProperties get_memory_properties() const
		{
			return memory_properties;
		}

		VkPhysicalDeviceLimits get_limits() const
		{
			return props.limits;
		}

		operator VkPhysicalDevice() const
		{
			return dev;
		}
	};

	class render_device
	{
		physical_device *pgpu = nullptr;
		memory_type_mapping memory_map{};
		gpu_formats_support m_formats_support{};
		std::unique_ptr<mem_allocator_base> m_allocator;
		VkDevice dev = VK_NULL_HANDLE;

	public:
		render_device()
		{}

		~render_device()
		{}

		void create(vk::physical_device &pdev, uint32_t graphics_queue_idx)
		{
			float queue_priorities[1] = { 0.f };
			pgpu = &pdev;

			VkDeviceQueueCreateInfo queue = {};
			queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue.pNext = NULL;
			queue.queueFamilyIndex = graphics_queue_idx;
			queue.queueCount = 1;
			queue.pQueuePriorities = queue_priorities;

			//Set up instance information
			const char *requested_extensions[] =
			{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};

			//Enable hardware features manually
			//Currently we require:
			//1. Anisotropic sampling
			//2. DXT support
			//3. Indexable storage buffers
			VkPhysicalDeviceFeatures available_features;
			vkGetPhysicalDeviceFeatures(*pgpu, &available_features);

			available_features.samplerAnisotropy = VK_TRUE;
			available_features.textureCompressionBC = VK_TRUE;
			available_features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;

			VkDeviceCreateInfo device = {};
			device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			device.pNext = NULL;
			device.queueCreateInfoCount = 1;
			device.pQueueCreateInfos = &queue;
			device.enabledLayerCount = 0;
			device.ppEnabledLayerNames = nullptr; // Deprecated
			device.enabledExtensionCount = 1;
			device.ppEnabledExtensionNames = requested_extensions;
			device.pEnabledFeatures = &available_features;

			CHECK_RESULT(vkCreateDevice(*pgpu, &device, nullptr, &dev));

			memory_map = vk::get_memory_mapping(pdev);
			m_formats_support = vk::get_optimal_tiling_supported_formats(pdev);

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

		bool get_compatible_memory_type(u32 typeBits, u32 desired_mask, u32 *type_index) const
		{
			VkPhysicalDeviceMemoryProperties mem_infos = pgpu->get_memory_properties();

			for (uint32_t i = 0; i < 32; i++)
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

		mem_allocator_base* get_allocator() const
		{
			return m_allocator.get();
		}

		operator VkDevice() const
		{
			return dev;
		}
	};

	struct image
	{
		VkImage value = VK_NULL_HANDLE;
		VkComponentMapping native_component_map = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
		VkImageLayout current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageCreateInfo info = {};
		std::shared_ptr<vk::memory_block> memory;

		image(const vk::render_device &dev,
			uint32_t memory_type_index,
			uint32_t access_flags,
			VkImageType image_type,
			VkFormat format,
			uint32_t width, uint32_t height, uint32_t depth,
			uint32_t mipmaps, uint32_t layers,
			VkSampleCountFlagBits samples,
			VkImageLayout initial_layout,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkImageCreateFlags image_flags)
			: m_device(dev), current_layout(initial_layout)
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

			CHECK_RESULT(vkCreateImage(m_device, &info, nullptr, &value));

			VkMemoryRequirements memory_req;
			vkGetImageMemoryRequirements(m_device, value, &memory_req);

			if (!(memory_req.memoryTypeBits & (1 << memory_type_index)))
			{
				//Suggested memory type is incompatible with this memory type.
				//Go through the bitset and test for requested props.
				if (!dev.get_compatible_memory_type(memory_req.memoryTypeBits, access_flags, &memory_type_index))
					fmt::throw_exception("No compatible memory type was found!" HERE);
			}

			memory = std::make_shared<vk::memory_block>(m_device, memory_req.size, memory_req.alignment, memory_type_index);
			CHECK_RESULT(vkBindImageMemory(m_device, value, memory->get_vk_device_memory(), memory->get_vk_device_memory_offset()));
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
			: m_device(dev), info(create_info)
		{
			create_impl();
		}

		image_view(VkDevice dev, vk::image* resource,
			const VkComponentMapping mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
			const VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})
			: m_device(dev), m_resource(resource)
		{
			info.format = resource->info.format;
			info.image = resource->value;
			info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			info.components = mapping;
			info.subresourceRange = range;

			switch (resource->info.imageType)
			{
			case VK_IMAGE_TYPE_1D:
				info.viewType = VK_IMAGE_VIEW_TYPE_1D;
				break;
			case VK_IMAGE_TYPE_2D:
				if (resource->info.flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
					info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
				else
					info.viewType = VK_IMAGE_VIEW_TYPE_2D;
				break;
			case VK_IMAGE_TYPE_3D:
				info.viewType = VK_IMAGE_VIEW_TYPE_3D;
				break;
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
			u32 result = (u32)info.components.a - 1;
			result |= ((u32)info.components.r - 1) << 3;
			result |= ((u32)info.components.g - 1) << 6;
			result |= ((u32)info.components.b - 1) << 9;

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

		image_view* get_view(u32 remap_encoding, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap,
			VkImageAspectFlags mask = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT)
		{
			auto found = views.equal_range(remap_encoding);
			for (auto It = found.first; It != found.second; ++It)
			{
				if (It->second->info.subresourceRange.aspectMask & mask)
				{
					return It->second.get();
				}
			}

			VkComponentMapping real_mapping = vk::apply_swizzle_remap
			(
				{native_component_map.a, native_component_map.r, native_component_map.g, native_component_map.b },
				remap
			);

			const auto range = vk::get_image_subresource_range(0, 0, info.arrayLayers, info.mipLevels, get_aspect_flags(info.format) & mask);
			auto view = std::make_unique<vk::image_view>(*get_current_renderer(), this, real_mapping, range);

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

		buffer(const vk::render_device& dev, u64 size, uint32_t memory_type_index, uint32_t access_flags, VkBufferUsageFlags usage, VkBufferCreateFlags flags)
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
					fmt::throw_exception("No compatible memory type was found!" HERE);
			}

			memory.reset(new memory_block(m_device, memory_reqs.size, memory_reqs.alignment, memory_type_index));
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
			return (u32)info.size;
		}

		buffer(const buffer&) = delete;
		buffer(buffer&&) = delete;

	private:
		VkDevice m_device;
	};

	struct buffer_view
	{
		VkBufferView value;
		VkBufferViewCreateInfo info = {};

		buffer_view(VkDevice dev, VkBuffer buffer, VkFormat format, VkDeviceSize offset, VkDeviceSize size)
			: m_device(dev)
		{
			info.buffer = buffer;
			info.format = format;
			info.offset = offset;
			info.range = size;
			info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
			CHECK_RESULT(vkCreateBufferView(m_device, &info, nullptr, &value));
		}

		~buffer_view()
		{
			vkDestroyBufferView(m_device, value, nullptr);
		}

		buffer_view(const buffer_view&) = delete;
		buffer_view(buffer_view&&) = delete;

		bool in_range(u32 address, u32 size, u32& offset) const
		{
			if (address < info.offset)
				return false;

			const u32 _offset = address - (u32)info.offset;
			if (info.range < _offset)
				return false;

			const auto remaining = info.range - _offset;
			if (size <= remaining)
			{
				offset = _offset;
				return true;
			}

			return false;
		}

	private:
		VkDevice m_device;
	};

	struct sampler
	{
		VkSampler value;
		VkSamplerCreateInfo info = {};

		sampler(VkDevice dev, VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
			VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
			VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, VkBorderColor border_color,
			VkBool32 depth_compare = false, VkCompareOp depth_compare_mode = VK_COMPARE_OP_NEVER)
			: m_device(dev)
		{
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.addressModeU = clamp_u;
			info.addressModeV = clamp_v;
			info.addressModeW = clamp_w;
			info.anisotropyEnable = VK_TRUE;
			info.compareEnable = depth_compare;
			info.unnormalizedCoordinates = unnormalized_coordinates;
			info.mipLodBias = mipLodBias;
			info.maxAnisotropy = max_anisotropy;
			info.maxLod = max_lod;
			info.minLod = min_lod;
			info.magFilter = mag_filter;
			info.minFilter = min_filter;
			info.mipmapMode = mipmap_mode;
			info.compareOp = depth_compare_mode;
			info.borderColor = border_color;

			CHECK_RESULT(vkCreateSampler(m_device, &info, nullptr, &value));
		}

		~sampler()
		{
			vkDestroySampler(m_device, value, nullptr);
		}

		bool matches(VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
			VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
			VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, VkBorderColor border_color,
			VkBool32 depth_compare = false, VkCompareOp depth_compare_mode = VK_COMPARE_OP_NEVER)
		{
			if (info.magFilter != mag_filter || info.minFilter != min_filter || info.mipmapMode != mipmap_mode ||
				info.addressModeU != clamp_u || info.addressModeV != clamp_v || info.addressModeW != clamp_w ||
				info.compareEnable != depth_compare || info.unnormalizedCoordinates != unnormalized_coordinates ||
				info.mipLodBias != mipLodBias || info.maxAnisotropy != max_anisotropy || info.maxLod != max_lod ||
				info.minLod != min_lod || info.compareOp != depth_compare_mode || info.borderColor != border_color)
				return false;

			return true;
		}

		sampler(const sampler&) = delete;
		sampler(sampler&&) = delete;
	private:
		VkDevice m_device;
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
			: m_device(dev), attachments(std::move(atts))
		{
			std::vector<VkImageView> image_view_array(attachments.size());
			size_t i = 0;
			for (const auto &att : attachments)
			{
				image_view_array[i++] = att->value;
			}

			info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			info.width = width;
			info.height = height;
			info.attachmentCount = static_cast<uint32_t>(image_view_array.size());
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

			for (int n = 0; n < fbo_images.size(); ++n)
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

	class command_pool
	{
		vk::render_device *owner = nullptr;
		VkCommandPool pool = nullptr;

	public:
		command_pool() {}
		~command_pool() {}

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
		VkFence m_submit_fence = VK_NULL_HANDLE;

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

	public:
		command_buffer() {}
		~command_buffer() {}

		void create(vk::command_pool &cmd_pool, bool auto_reset = false)
		{
			VkCommandBufferAllocateInfo infos = {};
			infos.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			infos.commandBufferCount = 1;
			infos.commandPool = (VkCommandPool)cmd_pool;
			infos.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			CHECK_RESULT(vkAllocateCommandBuffers(cmd_pool.get_owner(), &infos, &commands));

			if (auto_reset)
			{
				VkFenceCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				CHECK_RESULT(vkCreateFence(cmd_pool.get_owner(), &info, nullptr, &m_submit_fence));
			}

			pool = &cmd_pool;
		}

		void destroy()
		{
			vkFreeCommandBuffers(pool->get_owner(), (*pool), 1, &commands);

			if (m_submit_fence)
			{
				vkDestroyFence(pool->get_owner(), m_submit_fence, nullptr);
			}
		}

		vk::command_pool& get_command_pool() const
		{
			return *pool;
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

				CHECK_RESULT(vkResetFences(pool->get_owner(), 1, &m_submit_fence));
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
				LOG_ERROR(RSX, "commandbuffer->end was called but commandbuffer is not in a recording state");
				return;
			}

			CHECK_RESULT(vkEndCommandBuffer(commands));
			is_open = false;
		}

		void submit(VkQueue queue, const std::vector<VkSemaphore> &semaphores, VkFence fence, VkPipelineStageFlags pipeline_stage_flags)
		{
			if (is_open)
			{
				LOG_ERROR(RSX, "commandbuffer->submit was called whilst the command buffer is in a recording state");
				return;
			}

			if (fence == VK_NULL_HANDLE)
			{
				fence = m_submit_fence;
				is_pending = (fence != VK_NULL_HANDLE);
			}

			VkSubmitInfo infos = {};
			infos.commandBufferCount = 1;
			infos.pCommandBuffers = &commands;
			infos.pWaitDstStageMask = &pipeline_stage_flags;
			infos.pWaitSemaphores = semaphores.data();
			infos.waitSemaphoreCount = static_cast<uint32_t>(semaphores.size());
			infos.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			acquire_global_submit_lock();
			CHECK_RESULT(vkQueueSubmit(queue, 1, &infos, fence));
			release_global_submit_lock();
		}
	};

	class swapchain_image_WSI
	{
		VkImageView view = nullptr;
		VkImage image = nullptr;
		VkFormat internal_format;
		vk::render_device *owner = nullptr;

	public:
		swapchain_image_WSI() {}

		void create(vk::render_device &dev, VkImage &swap_image, VkFormat format)
		{
			VkImageViewCreateInfo color_image_view = {};

			color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			color_image_view.format = format;

			color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
			color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
			color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
			color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;

			color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			color_image_view.subresourceRange.baseMipLevel = 0;
			color_image_view.subresourceRange.levelCount = 1;
			color_image_view.subresourceRange.baseArrayLayer = 0;
			color_image_view.subresourceRange.layerCount = 1;

			color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;

			color_image_view.image = swap_image;
			vkCreateImageView(dev, &color_image_view, nullptr, &view);

			image = swap_image;
			internal_format = format;
			owner = &dev;
		}

		void discard(vk::render_device &dev)
		{
			vkDestroyImageView(dev, view, nullptr);
		}

		operator VkImage&()
		{
			return image;
		}

		operator VkImageView&()
		{
			return view;
		}
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

		uint32_t m_present_queue = UINT32_MAX;
		uint32_t m_graphics_queue = UINT32_MAX;
		VkQueue vk_graphics_queue = VK_NULL_HANDLE;
		VkQueue vk_present_queue = VK_NULL_HANDLE;

		display_handle_t window_handle{};
		u32 m_width = 0;
		u32 m_height = 0;
		VkFormat m_surface_format = VK_FORMAT_B8G8R8A8_UNORM;

		virtual void init_swapchain_images(render_device& dev, u32 count) = 0;

	public:
		swapchain_base(physical_device &gpu, uint32_t _present_queue, uint32_t _graphics_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
		{
			dev.create(gpu, _graphics_queue);

			if (_graphics_queue < UINT32_MAX) vkGetDeviceQueue(dev, _graphics_queue, 0, &vk_graphics_queue);
			if (_present_queue < UINT32_MAX) vkGetDeviceQueue(dev, _present_queue, 0, &vk_present_queue);

			m_present_queue = _present_queue;
			m_graphics_queue = _graphics_queue;
			m_surface_format = format;
		}

		virtual ~swapchain_base() {}

		virtual void create(display_handle_t& handle) = 0;
		virtual void destroy(bool full = true) = 0;
		virtual bool init() = 0;

		virtual u32 get_swap_image_count() const = 0;
		virtual VkImage& get_image(u32 index) = 0;
		virtual VkResult acquire_next_swapchain_image(VkSemaphore semaphore, u64 timeout, u32* result) = 0;
		virtual void end_frame(command_buffer& cmd, u32 index) = 0;
		virtual VkResult present(u32 index) = 0;
		virtual VkImageLayout get_optimal_present_layout() = 0;

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
		abstract_swapchain_impl(physical_device &gpu, uint32_t _present_queue, uint32_t _graphics_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
		: swapchain_base(gpu, _present_queue, _graphics_queue, format)
		{}

		~abstract_swapchain_impl()
		{}

		u32 get_swap_image_count() const override
		{
			return (u32)swapchain_images.size();
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
		swapchain_WIN32(physical_device &gpu, uint32_t _present_queue, uint32_t _graphics_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
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
				LOG_ERROR(RSX, "Invalid window dimensions %d x %d", m_width, m_height);
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
				dev.destroy();
		}

		VkResult present(u32 image) override
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
		void* nsView = NULL;

	public:
		swapchain_MacOS(physical_device &gpu, uint32_t _present_queue, uint32_t _graphics_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
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
				LOG_ERROR(RSX, "Invalid window dimensions %d x %d", m_width, m_height);
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

		VkResult present(u32 index) override
		{
			fmt::throw_exception("Native macOS swapchain is not implemented yet!");
		}
#else

	class swapchain_X11 : public native_swapchain_base
	{
		Display *display = NULL;
		Window window = (Window)NULL;
		XImage* pixmap = NULL;
		GC gc = NULL;
		int bit_depth = 24;

	public:
		swapchain_X11(physical_device &gpu, uint32_t _present_queue, uint32_t _graphics_queue, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM)
		: native_swapchain_base(gpu, _present_queue, _graphics_queue, format)
		{}

		~swapchain_X11(){}

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
				LOG_ERROR(RSX, "Invalid window dimensions %d x %d", m_width, m_height);
				return false;
			}

			XVisualInfo visual{};
			if (!XMatchVisualInfo(display, DefaultScreen(display), bit_depth, TrueColor, &visual))
			{
				LOG_ERROR(RSX, "Could not find matching visual info!" HERE);
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
				LOG_FATAL(RSX, "Could not create virtual display on this window protocol (Wayland?)");
				return;
			}

			gc = DefaultGC(display, DefaultScreen(display));
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

		VkResult present(u32 index) override
		{
			auto& src = swapchain_images[index];
			if (pixmap)
			{
				pixmap->data = (char*)src.second->get_pixels();

				XPutImage(display, window, gc, pixmap, 0, 0, 0, 0, m_width, m_height);
				XFlush(display);

				src.second->free_pixels();
			}

			//Release reference
			src.first = false;
			return VK_SUCCESS;
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

		VkImage& get_image(u32 index) override
		{
			return (VkImage&)(*swapchain_images[index].second.get());
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

	protected:
		void init_swapchain_images(render_device& dev, u32 /*preferred_count*/ = 0) override
		{
			u32 nb_swap_images = 0;
			getSwapchainImagesKHR(dev, m_vk_swapchain, &nb_swap_images, nullptr);

			if (!nb_swap_images) fmt::throw_exception("Driver returned 0 images for swapchain" HERE);

			std::vector<VkImage> vk_images;
			vk_images.resize(nb_swap_images);
			getSwapchainImagesKHR(dev, m_vk_swapchain, &nb_swap_images, vk_images.data());

			swapchain_images.resize(nb_swap_images);
			for (u32 i = 0; i < nb_swap_images; ++i)
			{
				swapchain_images[i].create(dev, vk_images[i], m_surface_format);
			}
		}

	public:
		swapchain_WSI(vk::physical_device &gpu, uint32_t _present_queue, uint32_t _graphics_queue, VkFormat format, VkSurfaceKHR surface, VkColorSpaceKHR color_space)
			: WSI_swapchain_base(gpu, _present_queue, _graphics_queue, format)
		{
			createSwapchainKHR = (PFN_vkCreateSwapchainKHR)vkGetDeviceProcAddr(dev, "vkCreateSwapchainKHR");
			destroySwapchainKHR = (PFN_vkDestroySwapchainKHR)vkGetDeviceProcAddr(dev, "vkDestroySwapchainKHR");
			getSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)vkGetDeviceProcAddr(dev, "vkGetSwapchainImagesKHR");
			acquireNextImageKHR = (PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr(dev, "vkAcquireNextImageKHR");
			queuePresentKHR = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(dev, "vkQueuePresentKHR");

			m_surface = surface;
			m_color_space = color_space;
		}

		~swapchain_WSI()
		{}

		void create(display_handle_t&) override
		{}

		void destroy(bool=true) override
		{
			if (VkDevice pdev = (VkDevice)dev)
			{
				if (m_vk_swapchain)
				{
					for (auto &img : swapchain_images)
						img.discard(dev);

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
				LOG_ERROR(RSX, "Cannot create WSI swapchain without a present queue");
				return false;
			}

			VkSwapchainKHR old_swapchain = m_vk_swapchain;
			vk::physical_device& gpu = const_cast<vk::physical_device&>(dev.gpu());

			VkSurfaceCapabilitiesKHR surface_descriptors = {};
			CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, m_surface, &surface_descriptors));

			if (surface_descriptors.maxImageExtent.width < m_width ||
				surface_descriptors.maxImageExtent.height < m_height)
			{
				LOG_ERROR(RSX, "Swapchain: Swapchain creation failed because dimensions cannot fit. Max = %d, %d, Requested = %d, %d",
					surface_descriptors.maxImageExtent.width, surface_descriptors.maxImageExtent.height, m_width, m_height);

				return false;
			}

			VkExtent2D swapchainExtent;
			if (surface_descriptors.currentExtent.width == (uint32_t)-1)
			{
				swapchainExtent.width = m_width;
				swapchainExtent.height = m_height;
			}
			else
			{
				if (surface_descriptors.currentExtent.width == 0 || surface_descriptors.currentExtent.height == 0)
				{
					LOG_WARNING(RSX, "Swapchain: Current surface extent is a null region. Is the window minimized?");
					return false;
				}

				swapchainExtent = surface_descriptors.currentExtent;
				m_width = surface_descriptors.currentExtent.width;
				m_height = surface_descriptors.currentExtent.height;
			}

			uint32_t nb_available_modes = 0;
			CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &nb_available_modes, nullptr));

			std::vector<VkPresentModeKHR> present_modes(nb_available_modes);
			CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &nb_available_modes, present_modes.data()));

			VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
			std::vector<VkPresentModeKHR> preferred_modes;

			if (!g_cfg.video.vk.force_fifo)
			{
				// List of preferred modes in decreasing desirability
				// NOTE: Always picks "triple-buffered vsync" types if possible
				if (g_cfg.video.vsync)
				{
					preferred_modes = { VK_PRESENT_MODE_MAILBOX_KHR };
				}
				else
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

			LOG_NOTICE(RSX, "Swapchain: present mode %d in use.", (s32&)swapchain_present_mode);

			uint32_t nb_swap_images = surface_descriptors.minImageCount + 1;
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

			swap_info.imageExtent.width = m_width;
			swap_info.imageExtent.height = m_height;

			createSwapchainKHR(dev, &swap_info, nullptr, &m_vk_swapchain);

			if (old_swapchain)
			{
				if (swapchain_images.size())
				{
					for (auto &img : swapchain_images)
						img.discard(dev);

					swapchain_images.resize(0);
				}

				destroySwapchainKHR(dev, old_swapchain, nullptr);
			}

			init_swapchain_images(dev);
			return true;
		}

		VkResult acquire_next_swapchain_image(VkSemaphore semaphore, u64 timeout, u32* result) override
		{
			return vkAcquireNextImageKHR(dev, m_vk_swapchain, timeout, semaphore, VK_NULL_HANDLE, result);
		}

		void end_frame(command_buffer& /*cmd*/, u32 /*index*/) override
		{
		}

		VkResult present(u32 image) override
		{
			VkPresentInfoKHR present = {};
			present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			present.pNext = nullptr;
			present.swapchainCount = 1;
			present.pSwapchains = &m_vk_swapchain;
			present.pImageIndices = &image;

			return queuePresentKHR(vk_present_queue, &present);
		}

		VkImage& get_image(u32 index) override
		{
			return (VkImage&)swapchain_images[index];
		}

		VkImageLayout get_optimal_present_layout() override
		{
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}
	};

	class supported_extensions
	{
	private:
		std::vector<VkExtensionProperties> m_vk_exts;

	public:
		supported_extensions()
		{
			uint32_t count;
			if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS)
				return;

			m_vk_exts.resize(count);
			vkEnumerateInstanceExtensionProperties(nullptr, &count, m_vk_exts.data());
		}

		bool is_supported(const char *ext)
		{
			return std::any_of(m_vk_exts.cbegin(), m_vk_exts.cend(),
				[&](const VkExtensionProperties& p) { return std::strcmp(p.extensionName, ext) == 0; });
		}
	};

	class context
	{
	private:
		std::vector<physical_device> gpus;

		std::vector<VkInstance> m_vk_instances;
		VkInstance m_instance;

		PFN_vkDestroyDebugReportCallbackEXT destroyDebugReportCallback = nullptr;
		PFN_vkCreateDebugReportCallbackEXT createDebugReportCallback = nullptr;
		VkDebugReportCallbackEXT m_debugger = nullptr;

		bool loader_exists = false;

	public:

		context()
		{
			m_instance = nullptr;

			//Check that some critical entry-points have been loaded into memory indicating presence of a loader
			loader_exists = (vkCreateInstance != nullptr);
		}

		~context()
		{
			if (m_instance || m_vk_instances.size())
				close();
		}

		void close()
		{
			if (!m_vk_instances.size()) return;

			if (m_debugger)
			{
				destroyDebugReportCallback(m_instance, m_debugger, nullptr);
				m_debugger = nullptr;
			}

			for (VkInstance &inst : m_vk_instances)
			{
				vkDestroyInstance(inst, nullptr);
			}

			m_instance = nullptr;
			m_vk_instances.resize(0);
		}

		void enable_debugging()
		{
			if (!g_cfg.video.debug_output) return;

			PFN_vkDebugReportCallbackEXT callback = vk::dbgFunc;

			createDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");
			destroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT");

			VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
			dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
			dbgCreateInfo.pfnCallback = callback;
			dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

			CHECK_RESULT(createDebugReportCallback(m_instance, &dbgCreateInfo, NULL, &m_debugger));
		}

		uint32_t createInstance(const char *app_name, bool fast = false)
		{
			if (!loader_exists) return 0;

			//Initialize a vulkan instance
			VkApplicationInfo app = {};

			app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			app.pApplicationName = app_name;
			app.applicationVersion = 0;
			app.pEngineName = app_name;
			app.engineVersion = 0;
			app.apiVersion = VK_MAKE_VERSION(1, 0, 0);

			//Set up instance information

			std::vector<const char *> extensions;
			std::vector<const char *> layers;

			if (!fast)
			{
				supported_extensions support;

				extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
				if (support.is_supported(VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
				{
					extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
				}
#ifdef _WIN32
				extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(__APPLE__)
				extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#else
				bool found_surface_ext = false;
				if (support.is_supported(VK_KHR_XLIB_SURFACE_EXTENSION_NAME))
				{
					extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
					found_surface_ext = true;
				}
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
				if (support.is_supported(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME))
				{
					extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
					found_surface_ext = true;
				}
#endif //(WAYLAND)
				if (!found_surface_ext)
				{
					LOG_ERROR(RSX, "Could not find a supported Vulkan surface extension");
					return 0;
				}
#endif //(WIN32, __APPLE__)
				if (g_cfg.video.debug_output)
					layers.push_back("VK_LAYER_LUNARG_standard_validation");
			}

			VkInstanceCreateInfo instance_info = {};
			instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instance_info.pApplicationInfo = &app;
			instance_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
			instance_info.ppEnabledLayerNames = layers.data();
			instance_info.enabledExtensionCount = fast ? 0 : static_cast<uint32_t>(extensions.size());
			instance_info.ppEnabledExtensionNames = fast ? nullptr : extensions.data();

			VkInstance instance;
			if (vkCreateInstance(&instance_info, nullptr, &instance) != VK_SUCCESS)
				return 0;

			m_vk_instances.push_back(instance);
			return (u32)m_vk_instances.size();
		}

		void makeCurrentInstance(uint32_t instance_id)
		{
			if (!instance_id || instance_id > m_vk_instances.size())
				fmt::throw_exception("Invalid instance passed to makeCurrentInstance (%u)" HERE, instance_id);

			if (m_debugger)
			{
				destroyDebugReportCallback(m_instance, m_debugger, nullptr);
				m_debugger = nullptr;
			}

			instance_id--;
			m_instance = m_vk_instances[instance_id];
		}

		VkInstance getCurrentInstance()
		{
			return m_instance;
		}

		VkInstance getInstanceById(uint32_t instance_id)
		{
			if (!instance_id || instance_id > m_vk_instances.size())
				fmt::throw_exception("Invalid instance passed to getInstanceById (%u)" HERE, instance_id);

			instance_id--;
			return m_vk_instances[instance_id];
		}

		std::vector<physical_device>& enumerateDevices()
		{
			if (!loader_exists)
				return gpus;

			uint32_t num_gpus;
			// This may fail on unsupported drivers, so just assume no devices
			if (vkEnumeratePhysicalDevices(m_instance, &num_gpus, nullptr) != VK_SUCCESS)
				return gpus;

			if (gpus.size() != num_gpus)
			{
				std::vector<VkPhysicalDevice> pdevs(num_gpus);
				gpus.resize(num_gpus);

				CHECK_RESULT(vkEnumeratePhysicalDevices(m_instance, &num_gpus, pdevs.data()));

				for (u32 i = 0; i < num_gpus; ++i)
					gpus[i].set_device(pdevs[i]);
			}

			return gpus;
		}

		swapchain_base* createSwapChain(display_handle_t window_handle, vk::physical_device &dev)
		{
			VkSurfaceKHR surface;
#ifdef _WIN32
			using swapchain_NATIVE = swapchain_WIN32;
			HINSTANCE hInstance = NULL;

			VkWin32SurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.hinstance = hInstance;
			createInfo.hwnd = window_handle;

			CHECK_RESULT(vkCreateWin32SurfaceKHR(m_instance, &createInfo, NULL, &surface));

#elif defined(__APPLE__)
			using swapchain_NATIVE = swapchain_MacOS;
			VkMacOSSurfaceCreateInfoMVK createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
			createInfo.pView = window_handle;

			CHECK_RESULT(vkCreateMacOSSurfaceMVK(m_instance, &createInfo, NULL, &surface));
#else
			using swapchain_NATIVE = swapchain_X11;

			std::visit([&](auto&& p)
			{
				using T = std::decay_t<decltype(p)>;

				if constexpr (std::is_same_v<T, std::pair<Display*, Window>>)
				{
					VkXlibSurfaceCreateInfoKHR createInfo = {};
					createInfo.sType                      = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
					createInfo.dpy                        = p.first;
					createInfo.window                     = p.second;
					CHECK_RESULT(vkCreateXlibSurfaceKHR(this->m_instance, &createInfo, nullptr, &surface));
				}
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
				else if constexpr (std::is_same_v<T, std::pair<wl_display*, wl_surface*>>)
				{
					VkWaylandSurfaceCreateInfoKHR createInfo = {};
					createInfo.sType                         = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
					createInfo.display                       = p.first;
					createInfo.surface                       = p.second;
					CHECK_RESULT(vkCreateWaylandSurfaceKHR(this->m_instance, &createInfo, nullptr, &surface));
				}
				else
				{
					static_assert(std::conditional_t<true, std::false_type, T>::value, "Unhandled window_handle type in std::variant");
				}
#endif
			}, window_handle);
#endif

			uint32_t device_queues = dev.get_queue_count();
			std::vector<VkBool32> supportsPresent(device_queues, VK_FALSE);
			bool present_possible = false;

			for (u32 index = 0; index < device_queues; index++)
			{
				vkGetPhysicalDeviceSurfaceSupportKHR(dev, index, surface, &supportsPresent[index]);
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
				LOG_ERROR(RSX, "It is not possible for the currently selected GPU to present to the window (Likely caused by NVIDIA driver running the current display)");
			}

			// Search for a graphics and a present queue in the array of queue
			// families, try to find one that supports both
			uint32_t graphicsQueueNodeIndex = UINT32_MAX;
			uint32_t presentQueueNodeIndex = UINT32_MAX;

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
				for (uint32_t i = 0; i < device_queues; ++i)
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
				LOG_FATAL(RSX, "Failed to find a suitable graphics queue" HERE);
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
				LOG_WARNING(RSX, "Falling back to software present support (native windowing API)");
				auto swapchain = new swapchain_NATIVE(dev, UINT32_MAX, graphicsQueueNodeIndex);
				swapchain->create(window_handle);
				return swapchain;
			}

			// Get the list of VkFormat's that are supported:
			uint32_t formatCount;
			CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, nullptr));

			std::vector<VkSurfaceFormatKHR> surfFormats(formatCount);
			CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, surfFormats.data()));

			VkFormat format;
			VkColorSpaceKHR color_space;

			if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				format = VK_FORMAT_B8G8R8A8_UNORM;
			}
			else
			{
				if (!formatCount) fmt::throw_exception("Format count is zero!" HERE);
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

			return new swapchain_WSI(dev, presentQueueNodeIndex, graphicsQueueNodeIndex, format, surface, color_space);
		}
	};

	class descriptor_pool
	{
		VkDescriptorPool pool = nullptr;
		const vk::render_device *owner = nullptr;

	public:
		descriptor_pool() {}
		~descriptor_pool() {}

		void create(const vk::render_device &dev, VkDescriptorPoolSize *sizes, u32 size_descriptors_count)
		{
			VkDescriptorPoolCreateInfo infos = {};
			infos.flags = 0;
			infos.maxSets = DESCRIPTOR_MAX_DRAW_CALLS;
			infos.poolSizeCount = size_descriptors_count;
			infos.pPoolSizes = sizes;
			infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

			owner = &dev;
			CHECK_RESULT(vkCreateDescriptorPool(dev, &infos, nullptr, &pool));
		}

		void destroy()
		{
			if (!pool) return;

			vkDestroyDescriptorPool((*owner), pool, nullptr);
			owner = nullptr;
			pool = nullptr;
		}

		bool valid()
		{
			return (pool != nullptr);
		}

		operator VkDescriptorPool()
		{
			return pool;
		}
	};

	class occlusion_query_pool
	{
		VkQueryPool query_pool = VK_NULL_HANDLE;
		vk::render_device* owner = nullptr;

		std::deque<u32> available_slots;
		std::vector<bool> query_active_status;
	public:

		void create(vk::render_device &dev, u32 num_entries)
		{
			VkQueryPoolCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			info.queryType = VK_QUERY_TYPE_OCCLUSION;
			info.queryCount = num_entries;

			CHECK_RESULT(vkCreateQueryPool(dev, &info, nullptr, &query_pool));
			owner = &dev;

			query_active_status.resize(num_entries, false);
			available_slots.resize(num_entries);

			for (u32 n = 0; n < num_entries; ++n)
			{
				available_slots[n] = n;
			}
		}

		void destroy()
		{
			if (query_pool)
			{
				vkDestroyQueryPool(*owner, query_pool, nullptr);

				owner = nullptr;
				query_pool = VK_NULL_HANDLE;
			}
		}

		void begin_query(vk::command_buffer &cmd, u32 index)
		{
			if (query_active_status[index])
			{
				//Synchronization must be done externally
				vkCmdResetQueryPool(cmd, query_pool, index, 1);
			}

			vkCmdBeginQuery(cmd, query_pool, index, 0);//VK_QUERY_CONTROL_PRECISE_BIT);
			query_active_status[index] = true;
		}

		void end_query(vk::command_buffer &cmd, u32 index)
		{
			vkCmdEndQuery(cmd, query_pool, index);
		}

		bool check_query_status(u32 index)
		{
			u32 result[2] = {0, 0};
			switch (VkResult status = vkGetQueryPoolResults(*owner, query_pool, index, 1, 8, result, 8, VK_QUERY_RESULT_WITH_AVAILABILITY_BIT))
			{
			case VK_SUCCESS:
				break;
			case VK_NOT_READY:
				return false;
			default:
				vk::die_with_error(HERE, status);
			}

			return result[1] != 0;
		}

		u32 get_query_result(u32 index)
		{
			u32 result = 0;
			CHECK_RESULT(vkGetQueryPoolResults(*owner, query_pool, index, 1, 4, &result, 4, VK_QUERY_RESULT_WAIT_BIT));

			return result == 0u? 0u: 1u;
		}

		void reset_query(vk::command_buffer &cmd, u32 index)
		{
			if (query_active_status[index])
			{
				vkCmdResetQueryPool(cmd, query_pool, index, 1);

				query_active_status[index] = false;
				available_slots.push_back(index);
			}
		}

		template<template<class> class _List>
		void reset_queries(vk::command_buffer &cmd, _List<u32> &list)
		{
			for (const auto index : list)
				reset_query(cmd, index);
		}

		void reset_all(vk::command_buffer &cmd)
		{
			for (u32 n = 0; n < query_active_status.size(); n++)
			{
				if (query_active_status[n])
					reset_query(cmd, n);
			}
		}

		u32 find_free_slot()
		{
			if (available_slots.empty())
			{
				return ~0u;
			}

			u32 result = available_slots.front();
			available_slots.pop_front();

			verify(HERE), !query_active_status[result];
			return result;
		}
	};

	class graphics_pipeline_state
	{
	public:
		VkPipelineInputAssemblyStateCreateInfo ia;
		VkPipelineDepthStencilStateCreateInfo ds;
		VkPipelineColorBlendAttachmentState att_state[4];
		VkPipelineColorBlendStateCreateInfo cs;
		VkPipelineRasterizationStateCreateInfo rs;

		graphics_pipeline_state()
		{
			// NOTE: Vk** structs have padding bytes
			memset(this, 0, sizeof(graphics_pipeline_state));

			ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			cs.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

			rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rs.polygonMode = VK_POLYGON_MODE_FILL;
			rs.cullMode = VK_CULL_MODE_NONE;
			rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rs.lineWidth = 1.f;
		}

		graphics_pipeline_state(const graphics_pipeline_state& other)
		{
			// NOTE: Vk** structs have padding bytes
			memcpy(this, &other, sizeof(graphics_pipeline_state));

			if (other.cs.pAttachments == other.att_state)
			{
				// Rebase pointer
				cs.pAttachments = att_state;
			}
		}

		~graphics_pipeline_state()
		{}

		graphics_pipeline_state& operator = (const graphics_pipeline_state& other)
		{
			if (this != &other)
			{
				// NOTE: Vk** structs have padding bytes
				memcpy(this, &other, sizeof(graphics_pipeline_state));

				if (other.cs.pAttachments == other.att_state)
				{
					// Rebase pointer
					cs.pAttachments = att_state;
				}
			}

			return *this;
		}

		void set_primitive_type(VkPrimitiveTopology type)
		{
			ia.topology = type;
		}

		void enable_primitive_restart(bool enable = true)
		{
			ia.primitiveRestartEnable = enable? VK_TRUE : VK_FALSE;
		}

		void set_color_mask(bool r, bool g, bool b, bool a)
		{
			VkColorComponentFlags mask = 0;
			if (a) mask |= VK_COLOR_COMPONENT_A_BIT;
			if (b) mask |= VK_COLOR_COMPONENT_B_BIT;
			if (g) mask |= VK_COLOR_COMPONENT_G_BIT;
			if (r) mask |= VK_COLOR_COMPONENT_R_BIT;

			att_state[0].colorWriteMask = mask;
			att_state[1].colorWriteMask = mask;
			att_state[2].colorWriteMask = mask;
			att_state[3].colorWriteMask = mask;
		}

		void set_depth_mask(bool enable)
		{
			ds.depthWriteEnable = enable ? VK_TRUE : VK_FALSE;
		}

		void set_stencil_mask(u32 mask)
		{
			ds.front.writeMask = mask;
			ds.back.writeMask = mask;
		}

		void set_stencil_mask_separate(int face, u32 mask)
		{
			if (!face)
				ds.front.writeMask = mask;
			else
				ds.back.writeMask = mask;
		}

		void enable_depth_test(VkCompareOp op)
		{
			ds.depthTestEnable = VK_TRUE;
			ds.depthCompareOp = op;
		}

		void enable_depth_clamp(bool enable = true)
		{
			rs.depthClampEnable = enable ? VK_TRUE : VK_FALSE;
		}

		void enable_depth_bias(bool enable = true)
		{
			rs.depthBiasEnable = enable ? VK_TRUE : VK_FALSE;
		}

		void enable_depth_bounds_test(bool enable = true)
		{
			ds.depthBoundsTestEnable = enable? VK_TRUE : VK_FALSE;
		}

		void enable_blend(int mrt_index, VkBlendFactor src_factor_rgb, VkBlendFactor src_factor_a,
			VkBlendFactor dst_factor_rgb, VkBlendFactor dst_factor_a,
			VkBlendOp equation_rgb, VkBlendOp equation_a)
		{
			att_state[mrt_index].srcColorBlendFactor = src_factor_rgb;
			att_state[mrt_index].srcAlphaBlendFactor = src_factor_a;
			att_state[mrt_index].dstColorBlendFactor = dst_factor_rgb;
			att_state[mrt_index].dstAlphaBlendFactor = dst_factor_a;
			att_state[mrt_index].colorBlendOp = equation_rgb;
			att_state[mrt_index].alphaBlendOp = equation_a;
			att_state[mrt_index].blendEnable = VK_TRUE;
		}

		void enable_stencil_test(VkStencilOp fail, VkStencilOp zfail, VkStencilOp pass,
			VkCompareOp func, u32 func_mask, u32 ref)
		{
			ds.front.failOp = fail;
			ds.front.passOp = pass;
			ds.front.depthFailOp = zfail;
			ds.front.compareOp = func;
			ds.front.compareMask = func_mask;
			ds.front.reference = ref;
			ds.back = ds.front;

			ds.stencilTestEnable = VK_TRUE;
		}

		void enable_stencil_test_separate(int face, VkStencilOp fail, VkStencilOp zfail, VkStencilOp pass,
			VkCompareOp func, u32 func_mask, u32 ref)
		{
			auto& face_props = (face ? ds.back : ds.front);
			face_props.failOp = fail;
			face_props.passOp = pass;
			face_props.depthFailOp = zfail;
			face_props.compareOp = func;
			face_props.compareMask = func_mask;
			face_props.reference = ref;

			ds.stencilTestEnable = VK_TRUE;
		}

		void enable_logic_op(VkLogicOp op)
		{
			cs.logicOpEnable = VK_TRUE;
			cs.logicOp = op;
		}

		void enable_cull_face(VkCullModeFlags cull_mode)
		{
			rs.cullMode = cull_mode;
		}

		void set_front_face(VkFrontFace face)
		{
			rs.frontFace = face;
		}

		void set_attachment_count(u32 count)
		{
			cs.attachmentCount = count;
			cs.pAttachments = att_state;
		}
	};

	namespace glsl
	{
		enum program_input_type : u32
		{
			input_type_uniform_buffer = 0,
			input_type_texel_buffer = 1,
			input_type_texture = 2,
			input_type_storage_buffer = 3,

			input_type_max_enum = 4
		};

		struct bound_sampler
		{
			VkFormat format;
			VkImage image;
			VkComponentMapping mapping;
		};

		struct bound_buffer
		{
			VkFormat format = VK_FORMAT_UNDEFINED;
			VkBuffer buffer = nullptr;
			u64 offset = 0;
			u64 size = 0;
		};

		struct program_input
		{
			::glsl::program_domain domain;
			program_input_type type;

			bound_buffer as_buffer;
			bound_sampler as_sampler;

			u32 location;
			std::string name;
		};

		class shader
		{
			::glsl::program_domain type = ::glsl::program_domain::glsl_vertex_program;
			VkShaderModule m_handle = VK_NULL_HANDLE;
			std::string m_source;
			std::vector<u32> m_compiled;

		public:
			shader()
			{}

			~shader()
			{}

			void create(::glsl::program_domain domain, const std::string& source)
			{
				type = domain;
				m_source = source;
			}

			VkShaderModule compile()
			{
				verify(HERE), m_handle == VK_NULL_HANDLE;

				if (!vk::compile_glsl_to_spv(m_source, type, m_compiled))
				{
					std::string shader_type = type == ::glsl::program_domain::glsl_vertex_program ? "vertex" :
						type == ::glsl::program_domain::glsl_fragment_program ? "fragment" : "compute";

					fmt::throw_exception("Failed to compile %s shader" HERE, shader_type);
				}

				VkShaderModuleCreateInfo vs_info;
				vs_info.codeSize = m_compiled.size() * sizeof(u32);
				vs_info.pNext = nullptr;
				vs_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				vs_info.pCode = (uint32_t*)m_compiled.data();
				vs_info.flags = 0;

				VkDevice dev = (VkDevice)*vk::get_current_renderer();
				vkCreateShaderModule(dev, &vs_info, nullptr, &m_handle);

				return m_handle;
			}

			void destroy()
			{
				m_source.clear();
				m_compiled.clear();

				if (m_handle)
				{
					VkDevice dev = (VkDevice)*vk::get_current_renderer();
					vkDestroyShaderModule(dev, m_handle, nullptr);
					m_handle = nullptr;
				}
			}

			const std::string& get_source() const
			{
				return m_source;
			}

			const std::vector<u32> get_compiled() const
			{
				return m_compiled;
			}

			VkShaderModule get_handle() const
			{
				return m_handle;
			}
		};

		class program
		{
			std::array<std::vector<program_input>, input_type_max_enum> uniforms;
			VkDevice m_device;

			std::array<u32, 16> fs_texture_bindings;
			std::array<u32, 16> fs_texture_mirror_bindings;
			std::array<u32, 4>  vs_texture_bindings;
			bool linked;

		public:
			VkPipeline pipeline;
			u64 attribute_location_mask;
			u64 vertex_attributes_mask;

			program(VkDevice dev, VkPipeline p, const std::vector<program_input> &vertex_input, const std::vector<program_input>& fragment_inputs);
			program(const program&) = delete;
			program(program&& other) = delete;
			~program();

			program& load_uniforms(::glsl::program_domain domain, const std::vector<program_input>& inputs);
			program& link();

			bool has_uniform(program_input_type type, const std::string &uniform_name);
			void bind_uniform(const VkDescriptorImageInfo &image_descriptor, const std::string &uniform_name, VkDescriptorSet &descriptor_set);
			void bind_uniform(const VkDescriptorImageInfo &image_descriptor, int texture_unit, ::glsl::program_domain domain, VkDescriptorSet &descriptor_set, bool is_stencil_mirror = false);
			void bind_uniform(const VkDescriptorBufferInfo &buffer_descriptor, uint32_t binding_point, VkDescriptorSet &descriptor_set);
			void bind_uniform(const VkBufferView &buffer_view, program_input_type type, const std::string &binding_name, VkDescriptorSet &descriptor_set);

			void bind_buffer(const VkDescriptorBufferInfo &buffer_descriptor, uint32_t binding_point, VkDescriptorType type, VkDescriptorSet &descriptor_set);

			u64 get_vertex_input_attributes_mask();
		};
	}

	struct data_heap : public ::data_heap
	{
		std::unique_ptr<buffer> heap;
		bool mapped = false;
		void *_ptr = nullptr;

		std::unique_ptr<buffer> shadow;
		std::vector<VkBufferCopy> dirty_ranges;

		// NOTE: Some drivers (RADV) use heavyweight OS map/unmap routines that are insanely slow
		// Avoid mapping/unmapping to keep these drivers from stalling
		// NOTE2: HOST_CACHED flag does not keep the mapped ptr around in the driver either

		void create(VkBufferUsageFlags usage, size_t size, const char *name = "unnamed", size_t guard = 0x10000)
		{
			::data_heap::init(size, name, guard);

			const auto device = get_current_renderer();
			const auto memory_map = device->get_memory_mapping();

			VkFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			auto memory_index = memory_map.host_visible_coherent;

			if (!(get_heap_compatible_buffer_types() & usage))
			{
				LOG_WARNING(RSX, "Buffer usage %u is not heap-compatible using this driver, explicit staging buffer in use", (u32)usage);

				shadow.reset(new buffer(*device, size, memory_index, memory_flags, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0));
				usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				memory_index = memory_map.device_local;
			}

			heap.reset(new buffer(*device, size, memory_index, memory_flags, usage, 0));
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

		void* map(size_t offset, size_t size)
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
			}

			return (u8*)_ptr + offset;
		}

		void unmap(bool force = false)
		{
			if (force || g_cfg.video.disable_vulkan_mem_allocator)
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
				verify (HERE), shadow, heap;
				vkCmdCopyBuffer(cmd, shadow->value, heap->value, (u32)dirty_ranges.size(), dirty_ranges.data());
				dirty_ranges.resize(0);

				insert_buffer_memory_barrier(cmd, heap->value, 0, heap->size(),
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
						VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
			}
		}
	};
}
