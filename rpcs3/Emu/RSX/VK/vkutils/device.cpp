#include "device.h"
#include "instance.hpp"
#include "util/logs.hpp"
#include "Emu/system_config.h"

namespace vk
{
	// Global shared render device
	const render_device* g_render_device = nullptr;

	void physical_device::get_physical_device_features(bool allow_extensions)
	{
		if (!allow_extensions)
		{
			vkGetPhysicalDeviceFeatures(dev, &features);
			return;
		}

		supported_extensions instance_extensions(supported_extensions::instance);
		supported_extensions device_extensions(supported_extensions::device, nullptr, dev);

		if (!instance_extensions.is_supported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			vkGetPhysicalDeviceFeatures(dev, &features);
		}
		else
		{
			VkPhysicalDeviceFeatures2KHR features2;
			features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			features2.pNext = nullptr;

			VkPhysicalDeviceFloat16Int8FeaturesKHR shader_support_info{};

			if (device_extensions.is_supported(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME))
			{
				shader_support_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR;
				features2.pNext           = &shader_support_info;
			}

			if (device_extensions.is_supported(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME))
			{
				driver_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES_KHR;
				driver_properties.pNext = features2.pNext;
				features2.pNext         = &driver_properties;
			}

			auto _vkGetPhysicalDeviceFeatures2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>(vkGetInstanceProcAddr(parent, "vkGetPhysicalDeviceFeatures2KHR"));
			ensure(_vkGetPhysicalDeviceFeatures2KHR); // "vkGetInstanceProcAddress failed to find entry point!"
			_vkGetPhysicalDeviceFeatures2KHR(dev, &features2);

			shader_types_support.allow_float64 = !!features2.features.shaderFloat64;
			shader_types_support.allow_float16 = !!shader_support_info.shaderFloat16;
			shader_types_support.allow_int8    = !!shader_support_info.shaderInt8;
			features                           = features2.features;
		}

		stencil_export_support           = device_extensions.is_supported(VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME);
		conditional_render_support       = device_extensions.is_supported(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);
		external_memory_host_support     = device_extensions.is_supported(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME);
		unrestricted_depth_range_support = device_extensions.is_supported(VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME);
		surface_capabilities_2_support   = instance_extensions.is_supported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	}

	void physical_device::create(VkInstance context, VkPhysicalDevice pdev, bool allow_extensions)
	{
		dev    = pdev;
		parent = context;
		vkGetPhysicalDeviceProperties(pdev, &props);
		vkGetPhysicalDeviceMemoryProperties(pdev, &memory_properties);
		get_physical_device_features(allow_extensions);

		rsx_log.always("Found vulkan-compatible GPU: '%s' running on driver %s", get_name(), get_driver_version());

		if (get_driver_vendor() == driver_vendor::RADV && get_name().find("LLVM 8.0.0") != umax)
		{
			// Serious driver bug causing black screens
			// See https://bugs.freedesktop.org/show_bug.cgi?id=110970
			rsx_log.fatal("RADV drivers have a major driver bug with LLVM 8.0.0 resulting in no visual output. Upgrade to LLVM version 8.0.1 or greater to avoid this issue.");
		}
		else if (get_driver_vendor() == driver_vendor::NVIDIA)
		{
#ifdef _WIN32
			// SPIRV bugs were fixed in 452.28 for windows
			const u32 threshold_version = (452u >> 22) | (28 >> 14);
#else
			// SPIRV bugs were fixed in 450.56 for linux/BSD
			const u32 threshold_version = (450u >> 22) | (56 >> 14);
#endif
			const auto current_version = props.driverVersion & ~0x3fffu; // Clear patch and revision fields
			if (current_version < threshold_version)
			{
				rsx_log.error("Your current NVIDIA graphics driver version %s has known issues and is unsupported. Update to the latest NVIDIA driver.", get_driver_version());
			}
		}

		if (get_chip_class() == chip_class::AMD_vega)
		{
			// Disable fp16 if driver uses LLVM emitter. It does fine with AMD proprietary drivers though.
			shader_types_support.allow_float16 = (driver_properties.driverID == VK_DRIVER_ID_AMD_PROPRIETARY_KHR);
		}
	}

	std::string physical_device::get_name() const
	{
		return props.deviceName;
	}

	driver_vendor physical_device::get_driver_vendor() const
	{
		if (!driver_properties.driverID)
		{
			const auto gpu_name = get_name();
			if (gpu_name.find("Radeon") != umax)
			{
				return driver_vendor::AMD;
			}

			if (gpu_name.find("NVIDIA") != umax || gpu_name.find("GeForce") != umax || gpu_name.find("Quadro") != umax)
			{
				return driver_vendor::NVIDIA;
			}

			if (gpu_name.find("RADV") != umax)
			{
				return driver_vendor::RADV;
			}

			if (gpu_name.find("Intel") != umax)
			{
				return driver_vendor::INTEL;
			}

			return driver_vendor::unknown;
		}
		else
		{
			switch (driver_properties.driverID)
			{
			case VK_DRIVER_ID_AMD_PROPRIETARY_KHR:
			case VK_DRIVER_ID_AMD_OPEN_SOURCE_KHR:
				return driver_vendor::AMD;
			case VK_DRIVER_ID_MESA_RADV_KHR:
				return driver_vendor::RADV;
			case VK_DRIVER_ID_NVIDIA_PROPRIETARY_KHR:
				return driver_vendor::NVIDIA;
			case VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS_KHR:
			case VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA_KHR:
				return driver_vendor::INTEL;
			default:
				// Mobile
				return driver_vendor::unknown;
			}
		}
	}

	std::string physical_device::get_driver_version() const
	{
		switch (get_driver_vendor())
		{
		case driver_vendor::NVIDIA:
		{
			// 10 + 8 + 8 + 6
			const auto major_version = props.driverVersion >> 22;
			const auto minor_version = (props.driverVersion >> 14) & 0xff;
			const auto patch         = (props.driverVersion >> 6) & 0xff;
			const auto revision      = (props.driverVersion & 0x3f);

			return fmt::format("%u.%u.%u.%u", major_version, minor_version, patch, revision);
		}
		default:
		{
			// 10 + 10 + 12 (standard vulkan encoding created with VK_MAKE_VERSION)
			return fmt::format("%u.%u.%u", (props.driverVersion >> 22), (props.driverVersion >> 12) & 0x3ff, (props.driverVersion) & 0x3ff);
		}
		}
	}

	chip_class physical_device::get_chip_class() const
	{
		return get_chip_family(props.vendorID, props.deviceID);
	}

	u32 physical_device::get_queue_count() const
	{
		if (!queue_props.empty())
			return ::size32(queue_props);

		u32 count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);

		return count;
	}

	VkQueueFamilyProperties physical_device::get_queue_properties(u32 queue)
	{
		if (queue_props.empty())
		{
			u32 count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);

			queue_props.resize(count);
			vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, queue_props.data());
		}

		if (queue >= queue_props.size())
			fmt::throw_exception("Bad queue index passed to get_queue_properties (%u)", queue);
		return queue_props[queue];
	}

	VkPhysicalDeviceMemoryProperties physical_device::get_memory_properties() const
	{
		return memory_properties;
	}

	VkPhysicalDeviceLimits physical_device::get_limits() const
	{
		return props.limits;
	}

	physical_device::operator VkPhysicalDevice() const
	{
		return dev;
	}

	physical_device::operator VkInstance() const
	{
		return parent;
	}

	// Render Device - The actual usable device
	void render_device::create(vk::physical_device& pdev, u32 graphics_queue_idx, u32 present_queue_idx, u32 transfer_queue_idx)
	{
		std::string message_on_error;
		float queue_priorities[1] = { 0.f };
		pgpu = &pdev;

		ensure(graphics_queue_idx == present_queue_idx || present_queue_idx == umax); // TODO
		std::vector<VkDeviceQueueCreateInfo> device_queues;

		auto& graphics_queue = device_queues.emplace_back();
		graphics_queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		graphics_queue.pNext = NULL;
		graphics_queue.flags = 0;
		graphics_queue.queueFamilyIndex = graphics_queue_idx;
		graphics_queue.queueCount = 1;
		graphics_queue.pQueuePriorities = queue_priorities;

		u32 transfer_queue_sub_index = 0;
		if (transfer_queue_idx == umax)
		{
			// Transfer queue must be a valid device queue
			rsx_log.warning("Dedicated transfer+compute queue was not found on this GPU. Will use graphics queue instead.");
			transfer_queue_idx = graphics_queue_idx;

			// Check if we can at least get a second graphics queue
			if (pdev.get_queue_properties(graphics_queue_idx).queueCount > 1)
			{
				rsx_log.notice("Will use a spare graphics queue to push transfer operations.");
				graphics_queue.queueCount++;
				transfer_queue_sub_index = 1;
			}
		}

		m_graphics_queue_family = graphics_queue_idx;
		m_present_queue_family = present_queue_idx;
		m_transfer_queue_family = transfer_queue_idx;

		if (graphics_queue_idx != transfer_queue_idx)
		{
			auto& transfer_queue = device_queues.emplace_back();
			transfer_queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			transfer_queue.pNext = NULL;
			transfer_queue.flags = 0;
			transfer_queue.queueFamilyIndex = transfer_queue_idx;
			transfer_queue.queueCount = 1;
			transfer_queue.pQueuePriorities = queue_priorities;
		}

		// Set up instance information
		std::vector<const char*> requested_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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

		if (pgpu->external_memory_host_support)
		{
			requested_extensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
			requested_extensions.push_back(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME);
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
			if (!pgpu->features.shaderStorageImageMultisample || !pgpu->features.shaderStorageImageWriteWithoutFormat)
			{
				// TODO: Slow fallback to emulate this
				// Just warn and let the driver decide whether to crash or not
				rsx_log.fatal("Your GPU driver does not support some required MSAA features. Expect problems.");
				message_on_error += "Your GPU driver does not support some required MSAA features.\nTry updating your GPU driver or disable Anti-Aliasing in the settings.";
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

		// If we're on lavapipe / llvmpipe, disable unimplemented features:
		// - samplerAnisotropy
		// - shaderStorageBufferArrayDynamicIndexing
		// - wideLines
		// as of mesa 21.1.0-dev (aea36ee05e9, 2020-02-10)
		// Several games work even if we disable these, testing purpose only
		if (pgpu->get_name().find("llvmpipe") != umax)
		{
			if (!pgpu->features.samplerAnisotropy)
			{
				rsx_log.error("Running lavapipe without support for samplerAnisotropy");
				enabled_features.samplerAnisotropy = VK_FALSE;
			}
			if (!pgpu->features.shaderStorageBufferArrayDynamicIndexing)
			{
				rsx_log.error("Running lavapipe without support for shaderStorageBufferArrayDynamicIndexing");
				enabled_features.shaderStorageBufferArrayDynamicIndexing = VK_FALSE;
			}
			if (!pgpu->features.wideLines)
			{
				rsx_log.error("Running lavapipe without support for wideLines");
				enabled_features.wideLines = VK_FALSE;
			}
		}

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
		device.queueCreateInfoCount = ::size32(device_queues);
		device.pQueueCreateInfos = device_queues.data();
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

		CHECK_RESULT_EX(vkCreateDevice(*pgpu, &device, nullptr, &dev), message_on_error);

		// Initialize queues
		vkGetDeviceQueue(dev, graphics_queue_idx, 0, &m_graphics_queue);
		vkGetDeviceQueue(dev, transfer_queue_idx, transfer_queue_sub_index, &m_transfer_queue);

		if (present_queue_idx != UINT32_MAX)
		{
			vkGetDeviceQueue(dev, present_queue_idx, 0, &m_present_queue);
		}

		// Import optional function endpoints
		if (pgpu->conditional_render_support)
		{
			_vkCmdBeginConditionalRenderingEXT = reinterpret_cast<PFN_vkCmdBeginConditionalRenderingEXT>(vkGetDeviceProcAddr(dev, "vkCmdBeginConditionalRenderingEXT"));
			_vkCmdEndConditionalRenderingEXT = reinterpret_cast<PFN_vkCmdEndConditionalRenderingEXT>(vkGetDeviceProcAddr(dev, "vkCmdEndConditionalRenderingEXT"));
		}

		memory_map = vk::get_memory_mapping(pdev);
		m_formats_support = vk::get_optimal_tiling_supported_formats(pdev);
		m_pipeline_binding_table = vk::get_pipeline_binding_table(pdev);

		if (pgpu->external_memory_host_support)
		{
			memory_map._vkGetMemoryHostPointerPropertiesEXT = reinterpret_cast<PFN_vkGetMemoryHostPointerPropertiesEXT>(vkGetDeviceProcAddr(dev, "vkGetMemoryHostPointerPropertiesEXT"));
		}

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
			dev = nullptr;
			memory_map = {};
			m_formats_support = {};
		}
	}

	VkQueue render_device::get_present_queue() const
	{
		return m_present_queue;
	}

	VkQueue render_device::get_graphics_queue() const
	{
		return m_graphics_queue;
	}

	VkQueue render_device::get_transfer_queue() const
	{
		return m_transfer_queue;
	}

	u32 render_device::get_graphics_queue_family() const
	{
		return m_graphics_queue_family;
	}

	u32 render_device::get_present_queue_family() const
	{
		return m_graphics_queue_family;
	}

	u32 render_device::get_transfer_queue_family() const
	{
		return m_transfer_queue_family;
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

	bool render_device::get_external_memory_host_support() const
	{
		return pgpu->external_memory_host_support;
	}

	bool render_device::get_surface_capabilities_2_support() const
	{
		return pgpu->surface_capabilities_2_support;
	}

	mem_allocator_base* render_device::get_allocator() const
	{
		return m_allocator.get();
	}

	render_device::operator VkDevice() const
	{
		return dev;
	}

	// Shared Util
	memory_type_mapping get_memory_mapping(const vk::physical_device& dev)
	{
		VkPhysicalDevice pdev = dev;
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(pdev, &memory_properties);

		memory_type_mapping result;
		result.device_local = VK_MAX_MEMORY_TYPES;
		result.host_visible_coherent = VK_MAX_MEMORY_TYPES;

		bool host_visible_cached = false;
		VkDeviceSize host_visible_vram_size = 0;
		VkDeviceSize device_local_vram_size = 0;

		for (u32 i = 0; i < memory_properties.memoryTypeCount; i++)
		{
			VkMemoryHeap& heap = memory_properties.memoryHeaps[memory_properties.memoryTypes[i].heapIndex];

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
				if ((is_cached && !host_visible_cached) || (host_visible_vram_size < heap.size))
				{
					result.host_visible_coherent = i;
					host_visible_vram_size = heap.size;
					host_visible_cached = is_cached;
				}
			}
		}

		if (result.device_local == VK_MAX_MEMORY_TYPES)
			fmt::throw_exception("GPU doesn't support device local memory");
		if (result.host_visible_coherent == VK_MAX_MEMORY_TYPES)
			fmt::throw_exception("GPU doesn't support host coherent device local memory");
		return result;
	}

	gpu_formats_support get_optimal_tiling_supported_formats(const physical_device& dev)
	{
		gpu_formats_support result = {};

		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(dev, VK_FORMAT_D24_UNORM_S8_UINT, &props);

		result.d24_unorm_s8 = !!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) && !!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) &&
			!!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) && !!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

		vkGetPhysicalDeviceFormatProperties(dev, VK_FORMAT_D32_SFLOAT_S8_UINT, &props);
		result.d32_sfloat_s8 = !!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) && !!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) &&
			!!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);

		// Hide d24_s8 if force high precision z buffer is enabled
		if (g_cfg.video.force_high_precision_z_buffer && result.d32_sfloat_s8)
			result.d24_unorm_s8 = false;

		// Checks if BGRA8 images can be used for blitting
		vkGetPhysicalDeviceFormatProperties(dev, VK_FORMAT_B8G8R8A8_UNORM, &props);
		result.bgra8_linear = !!(props.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);

		// Check if device supports RGBA8 format
		vkGetPhysicalDeviceFormatProperties(dev, VK_FORMAT_R8G8B8A8_UNORM, &props);
		if (!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) || !(props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) ||
			!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
		{
			// Non-fatal. Most games use BGRA layout due to legacy reasons as old GPUs typically supported BGRA and RGBA was emulated.
			rsx_log.error("Your GPU and/or driver does not support RGBA8 format. This can cause problems in some rare games that use this memory layout.");
		}

		result.argb8_linear = !!(props.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
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
}
