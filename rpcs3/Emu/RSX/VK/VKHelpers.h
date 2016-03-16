#pragma once

#include "stdafx.h"
#include <exception>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Emu/state.h"
#include "VulkanAPI.h"
#include "../GCM.h"

namespace rsx
{
	class texture;
}

namespace vk
{
#define CHECK_RESULT(expr) { VkResult __res = expr; if(__res != VK_SUCCESS) throw EXCEPTION("Assertion failed! Result is %Xh", __res); }

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

	vk::context *get_current_thread_ctx();
	void set_current_thread_ctx(const vk::context &ctx);

	vk::render_device *get_current_renderer();
	void set_current_renderer(const vk::render_device &device);

	VkComponentMapping default_component_map();
	VkImageSubresource default_image_subresource();
	VkImageSubresourceRange default_image_subresource_range();

	VkSampler null_sampler();
	VkImageView null_image_view();

	void destroy_global_resources();

	void change_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_flags);
	void copy_image(VkCommandBuffer cmd, VkImage &src, VkImage &dst, VkImageLayout srcLayout, VkImageLayout dstLayout, u32 width, u32 height, u32 mipmaps, VkImageAspectFlagBits aspect);
	void copy_scaled_image(VkCommandBuffer cmd, VkImage &src, VkImage &dst, VkImageLayout srcLayout, VkImageLayout dstLayout, u32 src_width, u32 src_height, u32 dst_width, u32 dst_height, u32 mipmaps, VkImageAspectFlagBits aspect);

	VkFormat get_compatible_sampler_format(u32 format, VkComponentMapping& mapping, u8 swizzle_mask=0);
	VkFormat get_compatible_surface_format(rsx::surface_color_format color_format);

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

			if (queue >= queue_props.size()) throw EXCEPTION("Undefined trap");
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
			VkResult err;

			float queue_priorities[1] = { 0.f };
			pgpu = &pdev;

			VkDeviceQueueCreateInfo queue;
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

			if (rpcs3::config.rsx.d3d12.debug_output.value())
				layers.push_back("VK_LAYER_LUNARG_standard_validation");

			VkDeviceCreateInfo device;
			device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			device.pNext = NULL;
			device.queueCreateInfoCount = 1;
			device.pQueueCreateInfos = &queue;
			device.enabledLayerCount = layers.size();
			device.ppEnabledLayerNames = layers.data();
			device.enabledExtensionCount = 1;
			device.ppEnabledExtensionNames = requested_extensions;
			device.pEnabledFeatures = nullptr;

			err = vkCreateDevice(*pgpu, &device, nullptr, &dev);
			if (err != VK_SUCCESS) throw EXCEPTION("Undefined trap");
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
				throw EXCEPTION("Could not find suitable memory type!");

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

	class texture
	{
		VkImageView m_view = nullptr;
		VkSampler m_sampler = nullptr;
		VkImage m_image_contents = nullptr;
		VkMemoryRequirements m_memory_layout;
		VkFormat m_internal_format;
		VkImageUsageFlags m_flags;
		VkImageAspectFlagBits m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageViewType m_view_type = VK_IMAGE_VIEW_TYPE_2D;
		VkImageUsageFlags m_usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		VkImageTiling m_tiling = VK_IMAGE_TILING_LINEAR;

		vk::memory_block_deprecated vram_allocation;
		vk::render_device *owner = nullptr;
		
		u32 m_width;
		u32 m_height;
		u32 m_mipmaps;

		vk::texture *staging_texture = nullptr;
		bool ready = false;

		VkSamplerAddressMode vk_wrap_mode(u32 gcm_wrap_mode);
		float max_aniso(u32 gcm_aniso);
		void sampler_setup(rsx::texture& tex, VkImageViewType type, VkComponentMapping swizzle);

	public:
		texture(vk::swap_chain_image &img);
		texture() {}
		~texture() {}

		void create(vk::render_device &device, VkFormat format, VkImageType image_type, VkImageViewType view_type, VkImageCreateFlags image_flags, VkImageUsageFlags usage, VkImageTiling tiling, u32 width, u32 height, u32 mipmaps, bool gpu_only, VkComponentMapping swizzle);
		void create(vk::render_device &device, VkFormat format, VkImageUsageFlags usage, VkImageTiling tiling, u32 width, u32 height, u32 mipmaps, bool gpu_only, VkComponentMapping swizzle);
		void create(vk::render_device &device, VkFormat format, VkImageUsageFlags usage, u32 width, u32 height, u32 mipmaps = 1, bool gpu_only = false, VkComponentMapping swizzle = default_component_map());
		void destroy();

		void init(rsx::texture &tex, vk::command_buffer &cmd, bool ignore_checks = false);
		void flush(vk::command_buffer & cmd);

		//Fill with debug color 0xFF
		void init_debug();

		void change_layout(vk::command_buffer &cmd, VkImageLayout new_layout);
		VkImageLayout get_layout();

		const u32 width();
		const u32 height();
		const u16 mipmaps();
		const VkFormat get_format();

		operator VkSampler();
		operator VkImageView();
		operator VkImage();
	};

	struct buffer
	{
		VkBuffer value;
		VkBufferCreateInfo info = {};
		std::unique_ptr<vk::memory_block> memory;

		buffer(VkDevice dev, u64 size, uint32_t memory_type_index, VkBufferUsageFlagBits usage, VkBufferCreateFlags flags)
			: m_device(dev)
		{
			info.size = size;
			info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			info.flags = flags;
			info.usage = usage;

			CHECK_RESULT(vkCreateBuffer(m_device, &info, nullptr, &value));

			VkMemoryRequirements memory_reqs;
			//Allocate vram for this buffer
			vkGetBufferMemoryRequirements(m_device, value, &memory_reqs);
			memory.reset(new memory_block(m_device, memory_reqs.size, memory_type_index));
			vkBindBufferMemory(dev, value, memory->memory, 0);
		}

		~buffer()
		{
			vkDestroyBuffer(m_device, value, nullptr);
		}

		void *map(u32 offset, u64 size)
		{
			void *data = nullptr;
			CHECK_RESULT(vkMapMemory(m_device, memory->memory, offset, size, 0, &data));
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

	class framebuffer
	{
		VkFramebuffer m_vk_framebuffer = nullptr;
		vk::render_device *owner = nullptr;

	public:
		framebuffer() {}
		~framebuffer() {}

		void create(vk::render_device &dev, VkRenderPass pass, VkImageView *attachments, u32 nb_attachments, u32 width, u32 height)
		{
			VkFramebufferCreateInfo infos = {};
			infos.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			infos.width = width;
			infos.height = height;
			infos.attachmentCount = nb_attachments;
			infos.pAttachments = attachments;
			infos.renderPass = pass;
			infos.layers = 1;

			vkCreateFramebuffer(dev, &infos, nullptr, &m_vk_framebuffer);
			owner = &dev;
		}

		void destroy()
		{
			if (!owner) return;

			vkDestroyFramebuffer((*owner), m_vk_framebuffer, nullptr);
			owner = nullptr;
		}

		operator VkFramebuffer() const
		{
			return m_vk_framebuffer;
		}
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

		operator vk::texture()
		{
			return vk::texture(*this);
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

		void init_swapchain(u32 width, u32 height)
		{
			VkSwapchainKHR old_swapchain = m_vk_swapchain;

			uint32_t num_modes;
			vk::physical_device& gpu = const_cast<vk::physical_device&>(dev.gpu());
			CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &num_modes, NULL));

			std::vector<VkPresentModeKHR> present_mode_descriptors(num_modes);
			CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &num_modes, present_mode_descriptors.data()));

			VkSurfaceCapabilitiesKHR surface_descriptors;
			CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, m_surface, &surface_descriptors));

			VkExtent2D swapchainExtent;
			
			if (surface_descriptors.currentExtent.width == (uint32_t)-1)
			{
				swapchainExtent.width = width;
				swapchainExtent.height = height;
			}
			else
			{
				swapchainExtent = surface_descriptors.currentExtent;
				width = surface_descriptors.currentExtent.width;
				height = surface_descriptors.currentExtent.height;
			}

			uint32_t nb_available_modes = 0;
			CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &nb_available_modes, nullptr));

			std::vector<VkPresentModeKHR> present_modes(nb_available_modes);
			CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &nb_available_modes, present_modes.data()));

			VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
			
			for (VkPresentModeKHR mode : present_modes)
			{
				if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					//If we can get a mailbox mode, use it
					swapchain_present_mode = mode;
					break;
				}

				//If we can get out of using the FIFO mode, take it. Fifo is very high latency (generic vsync)
				if (swapchain_present_mode == VK_PRESENT_MODE_FIFO_KHR &&
					(mode == VK_PRESENT_MODE_IMMEDIATE_KHR || mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR))
					swapchain_present_mode = mode;
			}
			
			uint32_t nb_swap_images = surface_descriptors.minImageCount + 1;

			if ((surface_descriptors.maxImageCount > 0) && (nb_swap_images > surface_descriptors.maxImageCount))
			{
				// Application must settle for fewer images than desired:
				nb_swap_images = surface_descriptors.maxImageCount;
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

			swap_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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
				destroySwapchainKHR(dev, old_swapchain, nullptr);

			nb_swap_images = 0;
			getSwapchainImagesKHR(dev, m_vk_swapchain, &nb_swap_images, nullptr);
			
			if (!nb_swap_images) throw EXCEPTION("Undefined trap");

			std::vector<VkImage> swap_images;
			swap_images.resize(nb_swap_images);
			getSwapchainImagesKHR(dev, m_vk_swapchain, &nb_swap_images, swap_images.data());

			m_swap_images.resize(nb_swap_images);
			for (u32 i = 0; i < nb_swap_images; ++i)
			{
				m_swap_images[i].create(dev, swap_images[i], m_surface_format);
			}
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
			infos.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
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
		vk::command_pool *pool = nullptr;
		VkCommandBuffer commands = nullptr;

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

		operator VkCommandBuffer()
		{
			return commands;
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

	public:

		context()
		{
			m_instance = nullptr;
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
			PFN_vkDebugReportCallbackEXT callback = vk::dbgFunc;

			createDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");
			destroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT");

			VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
			dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
			dbgCreateInfo.pfnCallback = callback;
			dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

			CHECK_RESULT(createDebugReportCallback(m_instance, &dbgCreateInfo, NULL, &m_debugger));
		}

		uint32_t createInstance(const char *app_name)
		{
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
				"VK_KHR_win32_surface",
				"VK_EXT_debug_report",
			};

			std::vector<const char *> layers;

			if (rpcs3::config.rsx.d3d12.debug_output.value())
				layers.push_back("VK_LAYER_LUNARG_standard_validation");

			VkInstanceCreateInfo instance_info = {};
			instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instance_info.pApplicationInfo = &app;
			instance_info.enabledLayerCount = layers.size();
			instance_info.ppEnabledLayerNames = layers.data();
			instance_info.enabledExtensionCount = 3;
			instance_info.ppEnabledExtensionNames = requested_extensions;

			VkInstance instance;
			VkResult error = vkCreateInstance(&instance_info, nullptr, &instance);

			if (error != VK_SUCCESS) throw EXCEPTION("Undefined trap");

			m_vk_instances.push_back(instance);
			return (u32)m_vk_instances.size();
		}

		void makeCurrentInstance(uint32_t instance_id)
		{
			if (!instance_id || instance_id > m_vk_instances.size())
				throw EXCEPTION("Undefined trap");

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
				throw EXCEPTION("Undefined trap");

			instance_id--;
			return m_vk_instances[instance_id];
		}

		std::vector<physical_device>& enumerateDevices()
		{
			uint32_t num_gpus;
			CHECK_RESULT(vkEnumeratePhysicalDevices(m_instance, &num_gpus, nullptr));

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
			VkResult err = vkCreateWin32SurfaceKHR(m_instance, &createInfo, NULL, &surface);

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
				throw EXCEPTION("Undefined trap");

			if (graphicsQueueNodeIndex != presentQueueNodeIndex)
				throw EXCEPTION("Undefined trap");

			// Get the list of VkFormat's that are supported:
			uint32_t formatCount;
			err = vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, nullptr);
			if (err != VK_SUCCESS) throw EXCEPTION("Undefined trap");

			std::vector<VkSurfaceFormatKHR> surfFormats(formatCount);
			err = vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, surfFormats.data());
			if (err != VK_SUCCESS) throw EXCEPTION("Undefined trap");

			VkFormat format;
			VkColorSpaceKHR color_space;

			if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
			{
				format = VK_FORMAT_B8G8R8A8_UNORM;
			}
			else
			{
				if (!formatCount) throw EXCEPTION("Undefined trap");
				format = surfFormats[0].format;
			}

			color_space = surfFormats[0].colorSpace;

			return new swap_chain(dev, presentQueueNodeIndex, graphicsQueueNodeIndex, format, surface, color_space);
		}
#endif	//if _WIN32

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
			infos.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			infos.maxSets = 2;
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
		enum program_domain
		{
			glsl_vertex_program = 0,
			glsl_fragment_program = 1
		};

		enum program_input_type
		{
			input_type_uniform_buffer = 0,
			input_type_texel_buffer = 1,
			input_type_texture = 2
		};

		struct bound_sampler
		{
			VkImageView image_view = nullptr;
			VkSampler sampler = nullptr;
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
			program_domain domain;
			program_input_type type;
			
			bound_buffer as_buffer;
			bound_sampler as_sampler;

			int location;
			std::string name;
		};

		class program
		{
			struct pipeline_state
			{
				VkGraphicsPipelineCreateInfo pipeline;
				VkPipelineCacheCreateInfo pipeline_cache_desc;
				VkPipelineCache pipeline_cache;
				VkPipelineVertexInputStateCreateInfo vi;
				VkPipelineInputAssemblyStateCreateInfo ia;
				VkPipelineRasterizationStateCreateInfo rs;
				VkPipelineColorBlendStateCreateInfo cb;
				VkPipelineDepthStencilStateCreateInfo ds;
				VkPipelineViewportStateCreateInfo vp;
				VkPipelineMultisampleStateCreateInfo ms;
				VkDynamicState dynamic_state_descriptors[VK_DYNAMIC_STATE_RANGE_SIZE];
				VkPipelineDynamicStateCreateInfo dynamic_state;

				VkPipelineColorBlendAttachmentState att_state[4];

				VkPipelineShaderStageCreateInfo shader_stages[2];
				VkRenderPass render_pass = nullptr;
				VkShaderModule vs, fs;
				VkPipeline pipeline_handle = nullptr;

				VkDescriptorSetLayout descriptor_layouts[2];;
				VkDescriptorSet descriptor_sets[2];
				VkPipelineLayout pipeline_layout;

				int num_targets = 1;

				bool dirty;
				bool in_use;
			}
			pstate;

			bool uniforms_changed = true;

			vk::render_device *device = nullptr;
			std::vector<program_input> uniforms;			
			vk::descriptor_pool descriptor_pool;

			void init_pipeline();

		public:
			program();
			program(const program&) = delete;
			program(program&& other);
			program(vk::render_device &renderer);

			~program();

			program& attach_device(vk::render_device &dev);
			program& attachFragmentProgram(VkShaderModule prog);
			program& attachVertexProgram(VkShaderModule prog);

			void make();
			void destroy();

			//Render state stuff...
			void set_depth_compare_op(VkCompareOp op);
			void set_depth_write_mask(VkBool32 write_enable);
			void set_depth_test_enable(VkBool32 state);
			void set_primitive_topology(VkPrimitiveTopology topology);
			void set_color_mask(int num_targets, u8* targets, VkColorComponentFlags *flags);
			void set_blend_state(int num_targets, u8* targets, VkBool32 *enable);
			void set_blend_state(int num_targets, u8* targets, VkBool32 enable);
			void set_blend_func(int num_targets, u8* targets, VkBlendFactor *src_color, VkBlendFactor *dst_color, VkBlendFactor *src_alpha, VkBlendFactor *dst_alpha);
			void set_blend_func(int num_targets, u8 * targets, VkBlendFactor src_color, VkBlendFactor dst_color, VkBlendFactor src_alpha, VkBlendFactor dst_alpha);
			void set_blend_op(int num_targets, u8* targets, VkBlendOp* color_ops, VkBlendOp* alpha_ops);
			void set_blend_op(int num_targets, u8 * targets, VkBlendOp color_op, VkBlendOp alpha_op);
			void set_primitive_restart(VkBool32 state);
			
			void init_descriptor_layout();
			void update_descriptors();
			void destroy_descriptors();

			void set_draw_buffer_count(u8 draw_buffers);

			program& load_uniforms(program_domain domain, std::vector<program_input>& inputs);

			void use(vk::command_buffer& commands, VkRenderPass pass, u32 subpass);

			bool has_uniform(program_domain domain, std::string uniform_name);
			bool bind_uniform(program_domain domain, std::string uniform_name);
			bool bind_uniform(program_domain domain, std::string uniform_name, vk::texture &_texture);
			bool bind_uniform(program_domain domain, std::string uniform_name, VkBuffer _buffer, VkDeviceSize offset, VkDeviceSize size);
			bool bind_uniform(program_domain domain, std::string uniform_name, VkBuffer _buffer, VkDeviceSize offset, VkDeviceSize size, VkFormat format);

			program& operator = (const program&) = delete;
			program& operator = (program&& other);
		};
	}
}