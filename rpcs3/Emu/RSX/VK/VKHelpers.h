#pragma once

#include "stdafx.h"
#include <exception>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>

#ifdef __linux__
#include <X11/Xlib.h>
#endif

#include "Emu/System.h"
#include "VulkanAPI.h"
#include "../GCM.h"
#include "../Common/TextureUtils.h"
#include "../Common/ring_buffer_helper.h"
#include "../Common/GLSLCommon.h"
#include "../rsx_cache.h"

#define DESCRIPTOR_MAX_DRAW_CALLS 4096

#define VERTEX_BUFFERS_FIRST_BIND_SLOT 3
#define FRAGMENT_CONSTANT_BUFFERS_BIND_SLOT 2
#define VERTEX_CONSTANT_BUFFERS_BIND_SLOT 1
#define SCALE_OFFSET_BIND_SLOT 0
#define TEXTURES_FIRST_BIND_SLOT 19
#define VERTEX_TEXTURES_FIRST_BIND_SLOT 35 //19+16

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

	class context;
	class render_device;
	class swap_chain_image;
	class physical_device;
	class command_buffer;
	struct image;

	vk::context *get_current_thread_ctx();
	void set_current_thread_ctx(const vk::context &ctx);

	vk::render_device *get_current_renderer();
	void set_current_renderer(const vk::render_device &device);

	bool emulate_primitive_restart();

	VkComponentMapping default_component_map();
	VkImageSubresource default_image_subresource();
	VkImageSubresourceRange get_image_subresource_range(uint32_t base_layer, uint32_t base_mip, uint32_t layer_count, uint32_t level_count, VkImageAspectFlags aspect);

	VkSampler null_sampler();
	VkImageView null_image_view(vk::command_buffer&);

	void destroy_global_resources();

	void change_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout, VkImageSubresourceRange range);
	void change_image_layout(VkCommandBuffer cmd, vk::image *image, VkImageLayout new_layout, VkImageSubresourceRange range);
	void copy_image(VkCommandBuffer cmd, VkImage &src, VkImage &dst, VkImageLayout srcLayout, VkImageLayout dstLayout, u32 width, u32 height, u32 mipmaps, VkImageAspectFlagBits aspect);
	void copy_scaled_image(VkCommandBuffer cmd, VkImage &src, VkImage &dst, VkImageLayout srcLayout, VkImageLayout dstLayout, u32 src_x_offset, u32 src_y_offset, u32 src_width, u32 src_height, u32 dst_x_offset, u32 dst_y_offset, u32 dst_width, u32 dst_height, u32 mipmaps, VkImageAspectFlagBits aspect);

	VkFormat get_compatible_sampler_format(u32 format);
	u8 get_format_texel_width(const VkFormat format);
	std::pair<VkFormat, VkComponentMapping> get_compatible_surface_format(rsx::surface_color_format color_format);
	size_t get_render_pass_location(VkFormat color_surface_format, VkFormat depth_stencil_format, u8 color_surface_count);

	void enter_uninterruptible();
	void leave_uninterruptible();
	bool is_uninterruptible();

	void advance_completed_frame_counter();
	void advance_frame_counter();
	const u64 get_current_frame_id();
	const u64 get_last_completed_frame_id();

	void die_with_error(const char* faulting_addr, VkResult error_code);

	struct memory_type_mapping
	{
		uint32_t host_visible_coherent;
		uint32_t device_local;
	};

	memory_type_mapping get_memory_mapping(VkPhysicalDevice pdev);

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
		}

		std::string name()
		{
			return props.deviceName;
		}

		uint32_t get_queue_count()
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

		VkPhysicalDeviceMemoryProperties get_memory_properties()
		{
			return memory_properties;
		}

		operator VkPhysicalDevice()
		{
			return dev;
		}
	};

	class render_device
	{
		vk::physical_device *pgpu;
		VkDevice dev;

	public:

		render_device()
		{
			dev = nullptr;
			pgpu = nullptr;
		}

		render_device(vk::physical_device &pdev, uint32_t graphics_queue_idx)
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
				"VK_KHR_swapchain"
			};

			std::vector<const char *> layers;

			if (g_cfg.video.debug_output)
				layers.push_back("VK_LAYER_LUNARG_standard_validation");

			//Enable hardware features manually
			//Currently we require:
			//1. Anisotropic sampling
			//2. DXT support
			VkPhysicalDeviceFeatures available_features;
			vkGetPhysicalDeviceFeatures(*pgpu, &available_features);

			available_features.samplerAnisotropy = VK_TRUE;
			available_features.textureCompressionBC = VK_TRUE;

			VkDeviceCreateInfo device = {};
			device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			device.pNext = NULL;
			device.queueCreateInfoCount = 1;
			device.pQueueCreateInfos = &queue;
			device.enabledLayerCount = static_cast<uint32_t>(layers.size());
			device.ppEnabledLayerNames = layers.data();
			device.enabledExtensionCount = 1;
			device.ppEnabledExtensionNames = requested_extensions;
			device.pEnabledFeatures = &available_features;

			CHECK_RESULT(vkCreateDevice(*pgpu, &device, nullptr, &dev));
		}

		~render_device()
		{
		}

		void destroy()
		{
			if (dev && pgpu)
			{
				vkDestroyDevice(dev, nullptr);
				dev = nullptr;
			}
		}

		bool get_compatible_memory_type(u32 typeBits, u32 desired_mask, u32 *type_index)
		{
			VkPhysicalDeviceMemoryProperties mem_infos = pgpu->get_memory_properties();

			for (uint32_t i = 0; i < 32; i++)
			{
				if ((typeBits & 1) == 1)
				{
					if ((mem_infos.memoryTypes[i].propertyFlags & desired_mask) == desired_mask)
					{
						*type_index = i;
						return true;
					}
				}

				typeBits >>= 1;
			}

			return false;
		}

		vk::physical_device& gpu()
		{
			return *pgpu;
		}

		operator VkDevice()
		{
			return dev;
		}
	};

	struct memory_block
	{
		VkMemoryAllocateInfo info = {};
		VkDeviceMemory memory;

		memory_block(VkDevice dev, u64 block_sz, uint32_t memory_type_index) : m_device(dev)
		{
			info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			info.allocationSize = block_sz;
			info.memoryTypeIndex = memory_type_index;

			CHECK_RESULT(vkAllocateMemory(m_device, &info, nullptr, &memory));
		}

		~memory_block()
		{
			vkFreeMemory(m_device, memory, nullptr);
		}

		memory_block(const memory_block&) = delete;
		memory_block(memory_block&&) = delete;

	private:
		VkDevice m_device;
	};

	class memory_block_deprecated
	{
		VkDeviceMemory vram = nullptr;
		vk::render_device *owner = nullptr;
		u64 vram_block_sz = 0;
		bool mappable = false;

	public:
		memory_block_deprecated() {}
		~memory_block_deprecated() {}

		void allocate_from_pool(vk::render_device &device, u64 block_sz, bool host_visible, u32 typeBits)
		{
			if (vram)
				destroy();

			u32 typeIndex = 0;

			owner = (vk::render_device*)&device;
			VkDevice dev = (VkDevice)(*owner);

			u32 access_mask = 0;
			
			if (host_visible)
				access_mask |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

			if (!owner->get_compatible_memory_type(typeBits, access_mask, &typeIndex))
				fmt::throw_exception("Could not find suitable memory type!" HERE);

			VkMemoryAllocateInfo infos;
			infos.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			infos.pNext = nullptr;
			infos.allocationSize = block_sz;
			infos.memoryTypeIndex = typeIndex;

			CHECK_RESULT(vkAllocateMemory(dev, &infos, nullptr, &vram));
			vram_block_sz = block_sz;
			mappable = host_visible;
		}

		void allocate_from_pool(vk::render_device &device, u64 block_sz, u32 typeBits)
		{
			allocate_from_pool(device, block_sz, true, typeBits);
		}

		void destroy()
		{
			VkDevice dev = (VkDevice)(*owner);
			vkFreeMemory(dev, vram, nullptr);

			owner = nullptr;
			vram = nullptr;
			vram_block_sz = 0;
		}

		bool is_mappable()
		{
			return mappable;
		}

		vk::render_device& get_owner()
		{
			return (*owner);
		}

		operator VkDeviceMemory()
		{
			return vram;
		}
	};

	struct image
	{
		VkImage value;
		VkComponentMapping native_component_map = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
		VkImageLayout current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageCreateInfo info = {};
		std::shared_ptr<vk::memory_block> memory;

		image(vk::render_device &dev,
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
			: m_device(dev)
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

			memory = std::make_shared<vk::memory_block>(m_device, memory_req.size, memory_type_index);
			CHECK_RESULT(vkBindImageMemory(m_device, value, memory->memory, 0));
		}

		// TODO: Ctor that uses a provided memory heap

		~image()
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
		VkImageView value;
		VkImageViewCreateInfo info = {};

		image_view(VkDevice dev, VkImage image, VkImageViewType view_type, VkFormat format, VkComponentMapping mapping, VkImageSubresourceRange range)
			: m_device(dev)
		{
			info.format = format;
			info.image = image;
			info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			info.viewType = view_type;
			info.components = mapping;
			info.subresourceRange = range;

			CHECK_RESULT(vkCreateImageView(m_device, &info, nullptr, &value));
		}

		image_view(VkDevice dev, VkImageViewCreateInfo create_info)
			: m_device(dev), info(create_info)
		{
			CHECK_RESULT(vkCreateImageView(m_device, &info, nullptr, &value));
		}

		~image_view()
		{
			vkDestroyImageView(m_device, value, nullptr);
		}

		image_view(const image_view&) = delete;
		image_view(image_view&&) = delete;
	private:
		VkDevice m_device;
	};

	struct buffer
	{
		VkBuffer value;
		VkBufferCreateInfo info = {};
		std::unique_ptr<vk::memory_block> memory;

		buffer(vk::render_device& dev, u64 size, uint32_t memory_type_index, uint32_t access_flags, VkBufferUsageFlagBits usage, VkBufferCreateFlags flags)
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

			memory.reset(new memory_block(m_device, memory_reqs.size, memory_type_index));
			vkBindBufferMemory(dev, value, memory->memory, 0);
		}

		~buffer()
		{
			vkDestroyBuffer(m_device, value, nullptr);
		}

		void *map(u64 offset, u64 size)
		{
			void *data = nullptr;
			CHECK_RESULT(vkMapMemory(m_device, memory->memory, offset, std::max<u64>(size, 1u), 0, &data));
			return data;
		}

		void unmap()
		{
			vkUnmapMemory(m_device, memory->memory);
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

	class swap_chain_image
	{
		VkImageView view = nullptr;
		VkImage image = nullptr;
		VkFormat internal_format;
		vk::render_device *owner = nullptr;

	public:
		swap_chain_image() {}

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

		operator VkImage()
		{
			return image;
		}

		operator VkImageView()
		{
			return view;
		}
	};

	class swap_chain
	{
		vk::render_device dev;

		uint32_t m_present_queue = 0xFFFF;
		uint32_t m_graphics_queue = 0xFFFF;

		VkQueue vk_graphics_queue = nullptr;
		VkQueue vk_present_queue = nullptr;

		/* WSI surface information */
		VkSurfaceKHR m_surface = nullptr;
		VkFormat m_surface_format;
		VkColorSpaceKHR m_color_space;

		VkSwapchainKHR m_vk_swapchain = nullptr;
		std::vector<vk::swap_chain_image> m_swap_images;

	public:

		PFN_vkCreateSwapchainKHR createSwapchainKHR;
		PFN_vkDestroySwapchainKHR destroySwapchainKHR;
		PFN_vkGetSwapchainImagesKHR getSwapchainImagesKHR;
		PFN_vkAcquireNextImageKHR acquireNextImageKHR;
		PFN_vkQueuePresentKHR queuePresentKHR;

		swap_chain(vk::physical_device &gpu, uint32_t _present_queue, uint32_t _graphics_queue, VkFormat format, VkSurfaceKHR surface, VkColorSpaceKHR color_space)
		{
			dev = render_device(gpu, _graphics_queue);

			createSwapchainKHR = (PFN_vkCreateSwapchainKHR)vkGetDeviceProcAddr(dev, "vkCreateSwapchainKHR");
			destroySwapchainKHR = (PFN_vkDestroySwapchainKHR)vkGetDeviceProcAddr(dev, "vkDestroySwapchainKHR");
			getSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)vkGetDeviceProcAddr(dev, "vkGetSwapchainImagesKHR");
			acquireNextImageKHR = (PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr(dev, "vkAcquireNextImageKHR");
			queuePresentKHR = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(dev, "vkQueuePresentKHR");

			vkGetDeviceQueue(dev, _graphics_queue, 0, &vk_graphics_queue);
			vkGetDeviceQueue(dev, _present_queue, 0, &vk_present_queue);

			m_present_queue = _present_queue;
			m_graphics_queue = _graphics_queue;
			m_surface = surface;
			m_color_space = color_space;
			m_surface_format = format;
		}

		~swap_chain()
		{
		}

		void destroy()
		{
			if (VkDevice pdev = (VkDevice)dev)
			{
				if (m_vk_swapchain)
				{
					if (m_swap_images.size())
					{
						for (vk::swap_chain_image &img : m_swap_images)
							img.discard(dev);
					}

					destroySwapchainKHR(pdev, m_vk_swapchain, nullptr);
				}

				dev.destroy();
			}
		}

		bool init_swapchain(u32 width, u32 height)
		{
			VkSwapchainKHR old_swapchain = m_vk_swapchain;
			vk::physical_device& gpu = const_cast<vk::physical_device&>(dev.gpu());

			VkSurfaceCapabilitiesKHR surface_descriptors = {};
			CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, m_surface, &surface_descriptors));

			if (surface_descriptors.maxImageExtent.width < width ||
				surface_descriptors.maxImageExtent.height < height)
			{
				LOG_ERROR(RSX, "Swapchain: Swapchain creation failed because dimensions cannot fit. Max = %d, %d, Requested = %d, %d",
					surface_descriptors.maxImageExtent.width, surface_descriptors.maxImageExtent.height, width, height);

				return false;
			}

			VkExtent2D swapchainExtent;
			if (surface_descriptors.currentExtent.width == (uint32_t)-1)
			{
				swapchainExtent.width = width;
				swapchainExtent.height = height;
			}
			else
			{
				if (surface_descriptors.currentExtent.width == 0 || surface_descriptors.currentExtent.height == 0)
				{
					LOG_WARNING(RSX, "Swapchain: Current surface extent is a null region. Is the window minimized?");
					return false;
				}

				swapchainExtent = surface_descriptors.currentExtent;
				width = surface_descriptors.currentExtent.width;
				height = surface_descriptors.currentExtent.height;
			}

			uint32_t nb_available_modes = 0;
			CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &nb_available_modes, nullptr));

			std::vector<VkPresentModeKHR> present_modes(nb_available_modes);
			CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &nb_available_modes, present_modes.data()));

			VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
			std::vector<VkPresentModeKHR> preferred_modes;

			//List of preferred modes in decreasing desirability
			if (g_cfg.video.vsync)
				preferred_modes = { VK_PRESENT_MODE_MAILBOX_KHR };
			else
				preferred_modes = { VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_MAILBOX_KHR };

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

			swap_info.imageExtent.width = width;
			swap_info.imageExtent.height = height;

			createSwapchainKHR(dev, &swap_info, nullptr, &m_vk_swapchain);

			if (old_swapchain)
			{
				if (m_swap_images.size())
				{
					for (auto &img : m_swap_images)
						img.discard(dev);

					m_swap_images.resize(0);
				}

				destroySwapchainKHR(dev, old_swapchain, nullptr);
			}

			nb_swap_images = 0;
			getSwapchainImagesKHR(dev, m_vk_swapchain, &nb_swap_images, nullptr);
			
			if (!nb_swap_images) fmt::throw_exception("Driver returned 0 images for swapchain" HERE);

			std::vector<VkImage> swap_images;
			swap_images.resize(nb_swap_images);
			getSwapchainImagesKHR(dev, m_vk_swapchain, &nb_swap_images, swap_images.data());

			m_swap_images.resize(nb_swap_images);
			for (u32 i = 0; i < nb_swap_images; ++i)
			{
				m_swap_images[i].create(dev, swap_images[i], m_surface_format);
			}

			return true;
		}

		u32 get_swap_image_count()
		{
			return (u32)m_swap_images.size();
		}

		vk::swap_chain_image& get_swap_chain_image(const int index)
		{
			return m_swap_images[index];
		}

		const vk::render_device& get_device()
		{
			return dev;
		}

		const VkQueue& get_present_queue()
		{
			return vk_graphics_queue;
		}

		const VkFormat get_surface_format()
		{
			return m_surface_format;
		}

		operator const VkSwapchainKHR()
		{
			return m_vk_swapchain;
		}
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

	protected:
		vk::command_pool *pool = nullptr;
		VkCommandBuffer commands = nullptr;

	public:
		enum access_type_hint
		{
			flush_only, //Only to be submitted/opened/closed via command flush
			all         //Auxilliary, can be sumitted/opened/closed at any time
		}
		access_hint = flush_only;

	public:
		command_buffer() {}
		~command_buffer() {}

		void create(vk::command_pool &cmd_pool)
		{
			VkCommandBufferAllocateInfo infos = {};
			infos.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			infos.commandBufferCount = 1;
			infos.commandPool = (VkCommandPool)cmd_pool;
			infos.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			CHECK_RESULT(vkAllocateCommandBuffers(cmd_pool.get_owner(), &infos, &commands));
			pool = &cmd_pool;
		}

		void destroy()
		{
			vkFreeCommandBuffers(pool->get_owner(), (*pool), 1, &commands);
		}

		vk::command_pool& get_command_pool() const
		{
			return *pool;
		}

		operator VkCommandBuffer()
		{
			return commands;
		}

		void begin()
		{
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

			VkSubmitInfo infos = {};
			infos.commandBufferCount = 1;
			infos.pCommandBuffers = &commands;
			infos.pWaitDstStageMask = &pipeline_stage_flags;
			infos.pWaitSemaphores = semaphores.data();
			infos.waitSemaphoreCount = static_cast<uint32_t>(semaphores.size());
			infos.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			CHECK_RESULT(vkQueueSubmit(queue, 1, &infos, fence));
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

			//Check that some critical entry-points have been loaded into memory indicating prescence of a loader
			loader_exists = (&vkCreateInstance != nullptr);
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
			const char *requested_extensions[] =
			{
				"VK_KHR_surface",
#ifdef _WIN32
				"VK_KHR_win32_surface",
#else
				"VK_KHR_xlib_surface",
#endif
				"VK_EXT_debug_report",
			};

			std::vector<const char *> layers;

			if (!fast && g_cfg.video.debug_output)
				layers.push_back("VK_LAYER_LUNARG_standard_validation");

			VkInstanceCreateInfo instance_info = {};
			instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instance_info.pApplicationInfo = &app;
			instance_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
			instance_info.ppEnabledLayerNames = layers.data();
			instance_info.enabledExtensionCount = fast? 0: 3;
			instance_info.ppEnabledExtensionNames = fast? nullptr: requested_extensions;

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

#ifdef _WIN32
		
		vk::swap_chain* createSwapChain(HINSTANCE hInstance, HWND hWnd, vk::physical_device &dev)
		{
			VkWin32SurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.hinstance = hInstance;
			createInfo.hwnd = hWnd;

			VkSurfaceKHR surface;
			CHECK_RESULT(vkCreateWin32SurfaceKHR(m_instance, &createInfo, NULL, &surface));
#elif HAVE_VULKAN
		
		vk::swap_chain* createSwapChain(Display *display, Window window, vk::physical_device &dev)
		{
			VkXlibSurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
			createInfo.dpy = display;
			createInfo.window = window;
			
			VkSurfaceKHR surface;
			CHECK_RESULT(vkCreateXlibSurfaceKHR(m_instance, &createInfo, nullptr, &surface));
#endif

			uint32_t device_queues = dev.get_queue_count();
			std::vector<VkBool32> supportsPresent(device_queues);

			for (u32 index = 0; index < device_queues; index++)
			{
				vkGetPhysicalDeviceSurfaceSupportKHR(dev, index, surface, &supportsPresent[index]);
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

			// Generate error if could not find both a graphics and a present queue
			if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
				fmt::throw_exception("Failed to find a suitable graphics/compute queue" HERE);

			if (graphicsQueueNodeIndex != presentQueueNodeIndex)
				fmt::throw_exception("Separate graphics and present queues not supported" HERE);

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

			return new swap_chain(dev, presentQueueNodeIndex, graphicsQueueNodeIndex, format, surface, color_space);
		}
	};

	class descriptor_pool
	{
		VkDescriptorPool pool = nullptr;
		vk::render_device *owner = nullptr;

	public:
		descriptor_pool() {}
		~descriptor_pool() {}

		void create(vk::render_device &dev, VkDescriptorPoolSize *sizes, u32 size_descriptors_count)
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

	namespace glsl
	{
		enum program_input_type
		{
			input_type_uniform_buffer = 0,
			input_type_texel_buffer = 1,
			input_type_texture = 2
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

			int location;
			std::string name;
		};

		class program
		{
			std::vector<program_input> uniforms;
			VkDevice m_device;
		public:
			VkPipeline pipeline;
			u64 attribute_location_mask;
			u64 vertex_attributes_mask;

			program(VkDevice dev, VkPipeline p, const std::vector<program_input> &vertex_input, const std::vector<program_input>& fragment_inputs);
			program(const program&) = delete;
			program(program&& other) = delete;
			~program();

			program& load_uniforms(::glsl::program_domain domain, const std::vector<program_input>& inputs);

			bool has_uniform(std::string uniform_name);
			void bind_uniform(VkDescriptorImageInfo image_descriptor, std::string uniform_name, VkDescriptorSet &descriptor_set);
			void bind_uniform(VkDescriptorBufferInfo buffer_descriptor, uint32_t binding_point, VkDescriptorSet &descriptor_set);
			void bind_uniform(const VkBufferView &buffer_view, const std::string &binding_name, VkDescriptorSet &descriptor_set);

			u64 get_vertex_input_attributes_mask();
		};
	}

	struct vk_data_heap : public data_heap
	{
		std::unique_ptr<vk::buffer> heap;
		bool mapped = false;

		void* map(size_t offset, size_t size)
		{
			mapped = true;
			return heap->map(offset, size);
		}

		void unmap()
		{
			mapped = false;
			heap->unmap();
		}
	};

	/**
	* Allocate enough space in upload_buffer and write all mipmap/layer data into the subbuffer.
	* Then copy all layers into dst_image.
	* dst_image must be in TRANSFER_DST_OPTIMAL layout and upload_buffer have TRANSFER_SRC_BIT usage flag.
	*/
	void copy_mipmaped_image_using_buffer(VkCommandBuffer cmd, VkImage dst_image,
		const std::vector<rsx_subresource_layout>& subresource_layout, int format, bool is_swizzled, u16 mipmap_count,
		VkImageAspectFlags flags, vk::vk_data_heap &upload_heap, vk::buffer* upload_buffer);
}
