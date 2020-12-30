#include "render_device.h"
#include "mem_allocator.h"
#include "shared.h"

#include "Emu/system_config.h"

namespace vk
{
	void render_device::create(vk::physical_device& pdev, u32 graphics_queue_idx)
	{
		float queue_priorities[1] = {0.f};
		pgpu                      = &pdev;

		VkDeviceQueueCreateInfo queue = {};
		queue.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue.pNext                   = NULL;
		queue.queueFamilyIndex        = graphics_queue_idx;
		queue.queueCount              = 1;
		queue.pQueuePriorities        = queue_priorities;

		// Set up instance information
		std::vector<const char*> requested_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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

		enabled_features.robustBufferAccess  = VK_TRUE;
		enabled_features.fullDrawIndexUint32 = VK_TRUE;
		enabled_features.independentBlend    = VK_TRUE;
		enabled_features.logicOp             = VK_TRUE;
		enabled_features.depthClamp          = VK_TRUE;
		enabled_features.depthBounds         = VK_TRUE;
		enabled_features.wideLines           = VK_TRUE;
		enabled_features.largePoints         = VK_TRUE;
		enabled_features.shaderFloat64       = VK_TRUE;

		if (g_cfg.video.antialiasing_level != msaa_level::none)
		{
			// MSAA features
			if (!pgpu->features.shaderStorageImageMultisample || !pgpu->features.shaderStorageImageWriteWithoutFormat)
			{
				// TODO: Slow fallback to emulate this
				// Just warn and let the driver decide whether to crash or not
				rsx_log.fatal("Your GPU driver does not support some required MSAA features. Expect problems.");
			}

			enabled_features.sampleRateShading             = VK_TRUE;
			enabled_features.alphaToOne                    = VK_TRUE;
			enabled_features.shaderStorageImageMultisample = VK_TRUE;
			// enabled_features.shaderStorageImageReadWithoutFormat = VK_TRUE;  // Unused currently, may be needed soon
			enabled_features.shaderStorageImageWriteWithoutFormat = VK_TRUE;
		}

		// enabled_features.shaderSampledImageArrayDynamicIndexing = TRUE;  // Unused currently but will be needed soon
		enabled_features.shaderClipDistance = VK_TRUE;
		// enabled_features.shaderCullDistance = VK_TRUE;  // Alt notation of clip distance

		enabled_features.samplerAnisotropy                       = VK_TRUE;
		enabled_features.textureCompressionBC                    = VK_TRUE;
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

		VkDeviceCreateInfo device      = {};
		device.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device.pNext                   = nullptr;
		device.queueCreateInfoCount    = 1;
		device.pQueueCreateInfos       = &queue;
		device.enabledLayerCount       = 0;
		device.ppEnabledLayerNames     = nullptr; // Deprecated
		device.enabledExtensionCount   = ::size32(requested_extensions);
		device.ppEnabledExtensionNames = requested_extensions.data();
		device.pEnabledFeatures        = &enabled_features;

		VkPhysicalDeviceFloat16Int8FeaturesKHR shader_support_info{};
		if (pgpu->shader_types_support.allow_float16)
		{
			// Allow use of f16 type in shaders if possible
			shader_support_info.sType         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR;
			shader_support_info.shaderFloat16 = VK_TRUE;
			device.pNext                      = &shader_support_info;

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
			cmdEndConditionalRenderingEXT   = reinterpret_cast<PFN_vkCmdEndConditionalRenderingEXT>(vkGetDeviceProcAddr(dev, "vkCmdEndConditionalRenderingEXT"));
		}

		memory_map               = vk::get_memory_mapping(pdev);
		m_formats_support        = vk::get_optimal_tiling_supported_formats(pdev);
		m_pipeline_binding_table = vk::get_pipeline_binding_table(pdev);

		if (g_cfg.video.disable_vulkan_mem_allocator)
			m_allocator = std::make_unique<vk::mem_allocator_vk>(dev, pdev);
		else
			m_allocator = std::make_unique<vk::mem_allocator_vma>(dev, pdev);
	}

	void render_device::destroy()
	{
		if (dev && pgpu)
		{
			if (m_allocator)
			{
				m_allocator->destroy();
				m_allocator.reset();
			}

			vkDestroyDevice(dev, nullptr);
			dev               = nullptr;
			memory_map        = {};
			m_formats_support = {};
		}
	}

	const VkFormatProperties render_device::get_format_properties(VkFormat format)
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

	bool render_device::get_compatible_memory_type(u32 typeBits, u32 desired_mask, u32* type_index) const
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

	const physical_device& render_device::gpu() const
	{
		return *pgpu;
	}

	const memory_type_mapping& render_device::get_memory_mapping() const
	{
		return memory_map;
	}

	const gpu_formats_support& render_device::get_formats_support() const
	{
		return m_formats_support;
	}

	const pipeline_binding_table& render_device::get_pipeline_binding_table() const
	{
		return m_pipeline_binding_table;
	}

	const gpu_shader_types_support& render_device::get_shader_types_support() const
	{
		return pgpu->shader_types_support;
	}

	bool render_device::get_shader_stencil_export_support() const
	{
		return pgpu->stencil_export_support;
	}

	bool render_device::get_depth_bounds_support() const
	{
		return pgpu->features.depthBounds != VK_FALSE;
	}

	bool render_device::get_alpha_to_one_support() const
	{
		return pgpu->features.alphaToOne != VK_FALSE;
	}

	bool render_device::get_conditional_render_support() const
	{
		return pgpu->conditional_render_support;
	}

	bool render_device::get_unrestricted_depth_range_support() const
	{
		return pgpu->unrestricted_depth_range_support;
	}

	mem_allocator_base* render_device::get_allocator() const
	{
		return m_allocator.get();
	}

	render_device::operator VkDevice() const
	{
		return dev;
	}
}
