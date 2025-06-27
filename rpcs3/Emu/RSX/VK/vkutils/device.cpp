#include "device.h"
#include "instance.h"
#include "util/logs.hpp"
#include "Emu/system_config.h"
#include <vulkan/vulkan_core.h>

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

		VkPhysicalDeviceFeatures2KHR features2;
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.pNext = nullptr;

		VkPhysicalDeviceFloat16Int8FeaturesKHR shader_support_info{};
		VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_info{};
		VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT fbo_loops_info{};
		VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR shader_barycentric_info{};
		VkPhysicalDeviceCustomBorderColorFeaturesEXT custom_border_color_info{};
		VkPhysicalDeviceBorderColorSwizzleFeaturesEXT border_color_swizzle_info{};
		VkPhysicalDeviceFaultFeaturesEXT device_fault_info{};
		VkPhysicalDeviceMultiDrawFeaturesEXT multidraw_info{};

		// Core features
		shader_support_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;
		features2.pNext           = &shader_support_info;

		descriptor_indexing_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		descriptor_indexing_info.pNext = features2.pNext;
		features2.pNext                = &descriptor_indexing_info;
		descriptor_indexing_support    = true;

		// Optional features
		if (device_extensions.is_supported(VK_EXT_ATTACHMENT_FEEDBACK_LOOP_LAYOUT_EXTENSION_NAME))
		{
			fbo_loops_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ATTACHMENT_FEEDBACK_LOOP_LAYOUT_FEATURES_EXT;
			fbo_loops_info.pNext = features2.pNext;
			features2.pNext      = &fbo_loops_info;
		}

		if (device_extensions.is_supported(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME))
		{
			shader_barycentric_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR;
			shader_barycentric_info.pNext = features2.pNext;
			features2.pNext               = &shader_barycentric_info;
		}

		if (device_extensions.is_supported(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME))
		{
			custom_border_color_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
			custom_border_color_info.pNext = features2.pNext;
			features2.pNext                = &custom_border_color_info;
		}

		if (device_extensions.is_supported(VK_EXT_BORDER_COLOR_SWIZZLE_EXTENSION_NAME))
		{
			border_color_swizzle_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BORDER_COLOR_SWIZZLE_FEATURES_EXT;
			border_color_swizzle_info.pNext = features2.pNext;
			features2.pNext                 = &border_color_swizzle_info;
		}

		if (device_extensions.is_supported(VK_EXT_DEVICE_FAULT_EXTENSION_NAME))
		{
			device_fault_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
			device_fault_info.pNext = features2.pNext;
			features2.pNext         = &device_fault_info;
		}

		if (device_extensions.is_supported(VK_EXT_MULTI_DRAW_EXTENSION_NAME))
		{
			multidraw_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT;
			multidraw_info.pNext = features2.pNext;
			features2.pNext      = &multidraw_info;
		}

		vkGetPhysicalDeviceFeatures2(dev, &features2);

		shader_types_support.allow_float64 = !!features2.features.shaderFloat64;
		shader_types_support.allow_float16 = !!shader_support_info.shaderFloat16;
		shader_types_support.allow_int8    = !!shader_support_info.shaderInt8;

		custom_border_color_support.supported = !!custom_border_color_info.customBorderColors && !!custom_border_color_info.customBorderColorWithoutFormat;
		custom_border_color_support.swizzle_extension_supported = border_color_swizzle_info.sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BORDER_COLOR_SWIZZLE_FEATURES_EXT;
		custom_border_color_support.require_border_color_remap = !border_color_swizzle_info.borderColorSwizzleFromImage;

		multidraw_support.supported = !!multidraw_info.multiDraw;
		multidraw_support.max_batch_size = 65536;

		optional_features_support.barycentric_coords  = !!shader_barycentric_info.fragmentShaderBarycentric;
		optional_features_support.framebuffer_loops   = !!fbo_loops_info.attachmentFeedbackLoopLayout;
		optional_features_support.extended_device_fault = !!device_fault_info.deviceFault;

		features = features2.features;

		descriptor_indexing_support.supported = true; // VK_API_VERSION_1_2
#define SET_DESCRIPTOR_BITFLAG(field, bit) if (descriptor_indexing_info.field) descriptor_indexing_support.update_after_bind_mask |= (1ull << bit)
		SET_DESCRIPTOR_BITFLAG(descriptorBindingUniformBufferUpdateAfterBind, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		SET_DESCRIPTOR_BITFLAG(descriptorBindingSampledImageUpdateAfterBind, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		SET_DESCRIPTOR_BITFLAG(descriptorBindingSampledImageUpdateAfterBind, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
		SET_DESCRIPTOR_BITFLAG(descriptorBindingStorageImageUpdateAfterBind, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		SET_DESCRIPTOR_BITFLAG(descriptorBindingStorageBufferUpdateAfterBind, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		SET_DESCRIPTOR_BITFLAG(descriptorBindingUniformTexelBufferUpdateAfterBind, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
		SET_DESCRIPTOR_BITFLAG(descriptorBindingStorageTexelBufferUpdateAfterBind, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
#undef SET_DESCRIPTOR_BITFLAG

		optional_features_support.shader_stencil_export    = device_extensions.is_supported(VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME);
		optional_features_support.conditional_rendering    = device_extensions.is_supported(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);
		optional_features_support.external_memory_host     = device_extensions.is_supported(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME);
		optional_features_support.synchronization_2        = device_extensions.is_supported(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
		optional_features_support.unrestricted_depth_range = device_extensions.is_supported(VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME);

		optional_features_support.debug_utils              = instance_extensions.is_supported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		optional_features_support.surface_capabilities_2   = instance_extensions.is_supported(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);

		// Post-initialization checks
		if (!custom_border_color_support.swizzle_extension_supported)
		{
			// So far only AMD is known to remap image view and border color together. Mark as not required.
			custom_border_color_support.require_border_color_remap = get_driver_vendor() != driver_vendor::AMD;
		}

		// v3dv and PanVK support BC1-BC3 which is all we require, support is reported as false since not all formats are supported
		optional_features_support.texture_compression_bc = features.textureCompressionBC
				|| get_driver_vendor() == driver_vendor::V3DV || get_driver_vendor() == driver_vendor::PANVK;

		// Texel buffer UAB is reported to the trigger for some driver crashes on older NV cards
		if (get_driver_vendor() == driver_vendor::NVIDIA &&
			get_chip_class() >= chip_class::NV_kepler &&
			get_chip_class() <= chip_class::NV_pascal)
		{
			// UBOs are unsupported on these cards anyway, disable texel buffers as well
			descriptor_indexing_support.update_after_bind_mask &= ~(1ull << VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
		}
	}

	void physical_device::get_physical_device_properties_0(bool allow_extensions)
	{
		// Core properties only
		vkGetPhysicalDeviceMemoryProperties(dev, &memory_properties);
		vkGetPhysicalDeviceProperties(dev, &props);

		if (!allow_extensions)
		{
			return;
		}

		VkPhysicalDeviceProperties2KHR properties2;
		properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
		properties2.pNext = nullptr;

		driver_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES_KHR;
		driver_properties.pNext = properties2.pNext;
		properties2.pNext = &driver_properties;
		vkGetPhysicalDeviceProperties2(dev, &properties2);
	}

	void physical_device::get_physical_device_properties_1(bool allow_extensions)
	{
		// Extended properties. Call after checking for features
		if (!allow_extensions)
		{
			return;
		}

		supported_extensions device_extensions(supported_extensions::device, nullptr, dev);

		VkPhysicalDeviceProperties2KHR properties2;
		properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
		properties2.pNext = nullptr;

		VkPhysicalDeviceDescriptorIndexingPropertiesEXT descriptor_indexing_props{};
		VkPhysicalDeviceMultiDrawPropertiesEXT multidraw_props{};

		descriptor_indexing_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT;
		descriptor_indexing_props.pNext = properties2.pNext;
		properties2.pNext = &descriptor_indexing_props;

		if (multidraw_support.supported)
		{
			multidraw_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_PROPERTIES_EXT;
			multidraw_props.pNext = properties2.pNext;
			properties2.pNext = &multidraw_props;
		}

		vkGetPhysicalDeviceProperties2(dev, &properties2);
		props = properties2.properties;

		if (descriptor_indexing_support)
		{
			if (descriptor_indexing_props.maxUpdateAfterBindDescriptorsInAllPools < 800'000)
			{
				rsx_log.error("Physical device does not support enough descriptors for deferred updates to work effectively. Deferred updates are disabled.");
				descriptor_indexing_support.update_after_bind_mask = 0;
			}
			else if (descriptor_indexing_props.maxUpdateAfterBindDescriptorsInAllPools < 2'000'000)
			{
				rsx_log.warning("Physical device reports a low amount of allowed deferred descriptor updates. Draw call threshold will be lowered accordingly.");
				descriptor_max_draw_calls = 8192;
			}
		}

		if (multidraw_support.supported)
		{
			multidraw_support.max_batch_size = multidraw_props.maxMultiDrawCount;

			if (!multidraw_props.maxMultiDrawCount)
			{
				rsx_log.error("Physical device reports 0 support maxMultiDraw count. Multidraw support will be disabled.");
				multidraw_support.supported = false;
			}
		}
	}

	void physical_device::create(VkInstance context, VkPhysicalDevice pdev, bool allow_extensions)
	{
		dev    = pdev;
		parent = context;

		get_physical_device_properties_0(allow_extensions);
		get_physical_device_features(allow_extensions);
		get_physical_device_properties_1(allow_extensions);

		rsx_log.always()("Found Vulkan-compatible GPU: '%s' running on driver %s", get_name(), get_driver_version());

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
			const u32 threshold_version = (452u << 22) | (28 << 14);
#else
			// SPIRV bugs were fixed in 450.56 for linux/BSD
			const u32 threshold_version = (450u << 22) | (56 << 14);
#endif
			const auto current_version = props.driverVersion & ~0x3fffu; // Clear patch and revision fields
			if (current_version < threshold_version)
			{
				rsx_log.error("Your current NVIDIA graphics driver version %s has known issues and is unsupported. Update to the latest NVIDIA driver.", get_driver_version());
			}
		}

		if (get_chip_class() == chip_class::AMD_vega && shader_types_support.allow_float16)
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
#ifdef __APPLE__
		// moltenVK currently returns DRIVER_ID_MOLTENVK (0).
		// For now, assume the vendor is moltenVK on Apple devices.
		return driver_vendor::MVK;
#endif

		if (!driver_properties.driverID)
		{
			const auto gpu_name = get_name();

			if (gpu_name.find("Microsoft Direct3D12") != umax)
			{
				return driver_vendor::DOZEN;
			}

			if (gpu_name.find("RADV") != umax)
			{
				return driver_vendor::RADV;
			}

			if (gpu_name.find("Radeon") != umax)
			{
				return driver_vendor::AMD;
			}

			if (gpu_name.find("NVIDIA") != umax || gpu_name.find("GeForce") != umax || gpu_name.find("Quadro") != umax)
			{
				if (gpu_name.find("NVK") != umax)
				{
					return driver_vendor::NVK;
				}

				return driver_vendor::NVIDIA;
			}

			if (gpu_name.find("Intel") != umax)
			{
#ifdef _WIN32
				return driver_vendor::INTEL;
#else
				return driver_vendor::ANV;
#endif
			}

			if (gpu_name.find("llvmpipe") != umax)
			{
				return driver_vendor::LAVAPIPE;
			}

			if (gpu_name.find("V3D") != umax)
			{
				return driver_vendor::V3DV;
			}

			if (gpu_name.find("Apple") != umax)
			{
				return driver_vendor::HONEYKRISP;
			}

			if (gpu_name.find("Panfrost") != umax)
			{ // e.g. "Mali-G610 (Panfrost)"
				return driver_vendor::PANVK;
			}
			else if (gpu_name.find("Mali") != umax)
			{ // e.g. "Mali-G610", hence "else"
				return driver_vendor::ARM_MALI;
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
				return driver_vendor::INTEL;
			case VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA_KHR:
				return driver_vendor::ANV;
			case VK_DRIVER_ID_MESA_DOZEN:
				return driver_vendor::DOZEN;
			case VK_DRIVER_ID_MESA_LLVMPIPE:
				return driver_vendor::LAVAPIPE;
			case VK_DRIVER_ID_MESA_NVK:
				return driver_vendor::NVK;
			case VK_DRIVER_ID_MESA_V3DV:
				return driver_vendor::V3DV;
			case VK_DRIVER_ID_MESA_HONEYKRISP:
				return driver_vendor::HONEYKRISP;
			case VK_DRIVER_ID_MESA_PANVK:
				return driver_vendor::PANVK;
			case VK_DRIVER_ID_ARM_PROPRIETARY:
				return driver_vendor::ARM_MALI;
			default:
				// Mobile?
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

	const VkQueueFamilyProperties& physical_device::get_queue_properties(u32 queue)
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

	const VkPhysicalDeviceMemoryProperties& physical_device::get_memory_properties() const
	{
		return memory_properties;
	}

	const VkPhysicalDeviceLimits& physical_device::get_limits() const
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
		// 2. Indexable storage buffers
		VkPhysicalDeviceFeatures enabled_features{};
		if (pgpu->custom_border_color_support)
		{
			requested_extensions.push_back(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);
		}

		if (pgpu->multidraw_support)
		{
			requested_extensions.push_back(VK_EXT_MULTI_DRAW_EXTENSION_NAME);
		}

		if (pgpu->optional_features_support.conditional_rendering)
		{
			requested_extensions.push_back(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);
		}

		if (pgpu->optional_features_support.unrestricted_depth_range)
		{
			requested_extensions.push_back(VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME);
		}

		if (pgpu->optional_features_support.external_memory_host)
		{
			requested_extensions.push_back(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME);
		}

		if (pgpu->optional_features_support.shader_stencil_export)
		{
			requested_extensions.push_back(VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME);
		}

		if (pgpu->optional_features_support.framebuffer_loops)
		{
			requested_extensions.push_back(VK_EXT_ATTACHMENT_FEEDBACK_LOOP_LAYOUT_EXTENSION_NAME);
		}

		if (pgpu->optional_features_support.barycentric_coords)
		{
			requested_extensions.push_back(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
		}

		if (pgpu->optional_features_support.synchronization_2)
		{
			requested_extensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME); // VK_API_VERSION_1_3
		}

		if (pgpu->optional_features_support.extended_device_fault)
		{
			requested_extensions.push_back(VK_EXT_DEVICE_FAULT_EXTENSION_NAME);
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
			enabled_features.sampleRateShading = VK_TRUE;
			enabled_features.alphaToOne = VK_TRUE;
			enabled_features.shaderStorageImageMultisample = VK_TRUE;
			// enabled_features.shaderStorageImageReadWithoutFormat = VK_TRUE;  // Unused currently, may be needed soon
			enabled_features.shaderStorageImageWriteWithoutFormat = VK_TRUE;
		}

		if (g_cfg.video.precise_zpass_count)
		{
			enabled_features.occlusionQueryPrecise = VK_TRUE;
		}

		// enabled_features.shaderSampledImageArrayDynamicIndexing = TRUE;  // Unused currently but will be needed soon
		enabled_features.shaderClipDistance = VK_TRUE;
		// enabled_features.shaderCullDistance = VK_TRUE;  // Alt notation of clip distance

		enabled_features.samplerAnisotropy = VK_TRUE;
		enabled_features.textureCompressionBC = pgpu->optional_features_support.texture_compression_bc;
		enabled_features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;

		// Optionally disable unsupported stuff
		if (!pgpu->features.fullDrawIndexUint32)
		{
			// There's really nothing we can do about PS3 draw indices, just pray your GPU doesn't crash.
			rsx_log.error("Your GPU driver does not fully support 32-bit vertex indices. This may result in graphical corruption or crashes in some cases.");
			enabled_features.fullDrawIndexUint32 = VK_FALSE;
		}

		if (!pgpu->features.shaderStorageImageMultisample || !pgpu->features.shaderStorageImageWriteWithoutFormat)
		{
			// Disable MSAA if any of these two features are unsupported
			if (g_cfg.video.antialiasing_level != msaa_level::none)
			{
				rsx_log.error("Your GPU driver does not support some required MSAA features. MSAA will be disabled.");
				g_cfg.video.antialiasing_level.set(msaa_level::none);
			}

			enabled_features.sampleRateShading = VK_FALSE;
			enabled_features.alphaToOne = VK_FALSE;
			enabled_features.shaderStorageImageMultisample = VK_FALSE;
			enabled_features.shaderStorageImageWriteWithoutFormat = VK_FALSE;
		}

		if (!pgpu->features.shaderClipDistance)
		{
			rsx_log.error("Your GPU does not support shader clip distance. Graphics will not render correctly.");
			enabled_features.shaderClipDistance = VK_FALSE;
		}

		if (!pgpu->features.shaderStorageBufferArrayDynamicIndexing)
		{
			rsx_log.error("Your GPU does not support shader storage buffer array dynamic indexing. Graphics will not render correctly.");
			enabled_features.shaderStorageBufferArrayDynamicIndexing = VK_FALSE;
		}

		if (!pgpu->features.samplerAnisotropy)
		{
			rsx_log.error("Your GPU does not support anisotropic filtering. Graphics may not render correctly.");
			enabled_features.samplerAnisotropy = VK_FALSE;
		}

		if (!pgpu->features.shaderFloat64)
		{
			rsx_log.error("Your GPU does not support double precision floats in shaders. Graphics may not render correctly.");
			enabled_features.shaderFloat64 = VK_FALSE;
		}

		if (!pgpu->features.depthBounds)
		{
			rsx_log.error("Your GPU does not support depth bounds testing. Graphics may not render correctly.");
			enabled_features.depthBounds = VK_FALSE;
		}

		if (!pgpu->features.largePoints)
		{
			rsx_log.error("Your GPU does not support large points. Graphics may not render correctly.");
			enabled_features.largePoints = VK_FALSE;
		}

		if (!pgpu->features.wideLines)
		{
			rsx_log.error("Your GPU does not support wide lines. Graphics may not render correctly.");
			enabled_features.wideLines = VK_FALSE;
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

		if (!pgpu->features.occlusionQueryPrecise && enabled_features.occlusionQueryPrecise)
		{
			rsx_log.error("Your GPU does not support precise occlusion queries. Graphics may not render correctly.");
			enabled_features.occlusionQueryPrecise = VK_FALSE;
		}

		if (!pgpu->features.logicOp)
		{
			rsx_log.error("Your GPU does not support framebuffer logical operations. Graphics may not render correctly.");
			enabled_features.logicOp = VK_FALSE;
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
			shader_support_info.pNext = const_cast<void*>(device.pNext);
			device.pNext = &shader_support_info;

			rsx_log.notice("GPU/driver supports float16 data types natively. Using native float16_t variables if possible.");
		}
		else
		{
			rsx_log.notice("GPU/driver lacks support for float16 data types. All float16_t arithmetic will be emulated with float32_t.");
		}

		VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{};
		if (pgpu->descriptor_indexing_support)
		{
#define SET_DESCRIPTOR_BITFLAG(field, bit) if (pgpu->descriptor_indexing_support.update_after_bind_mask & (1ull << bit)) indexing_features.field = VK_TRUE
			SET_DESCRIPTOR_BITFLAG(descriptorBindingUniformBufferUpdateAfterBind, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			SET_DESCRIPTOR_BITFLAG(descriptorBindingSampledImageUpdateAfterBind, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			SET_DESCRIPTOR_BITFLAG(descriptorBindingSampledImageUpdateAfterBind, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
			SET_DESCRIPTOR_BITFLAG(descriptorBindingStorageImageUpdateAfterBind, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
			SET_DESCRIPTOR_BITFLAG(descriptorBindingStorageBufferUpdateAfterBind, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			SET_DESCRIPTOR_BITFLAG(descriptorBindingUniformTexelBufferUpdateAfterBind, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
			SET_DESCRIPTOR_BITFLAG(descriptorBindingStorageTexelBufferUpdateAfterBind, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
#undef SET_DESCRIPTOR_BITFLAG

			indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
			indexing_features.pNext = const_cast<void*>(device.pNext);
			device.pNext = &indexing_features;
		}

		VkPhysicalDeviceCustomBorderColorFeaturesEXT custom_border_color_features{};
		if (pgpu->custom_border_color_support)
		{
			custom_border_color_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
			custom_border_color_features.customBorderColors = VK_TRUE;
			custom_border_color_features.customBorderColorWithoutFormat = VK_TRUE;
			custom_border_color_features.pNext = const_cast<void*>(device.pNext);
			device.pNext = &custom_border_color_features;
		}

		VkPhysicalDeviceMultiDrawFeaturesEXT multidraw_features{};
		if (pgpu->multidraw_support)
		{
			multidraw_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT;
			multidraw_features.multiDraw = VK_TRUE;
			multidraw_features.pNext = const_cast<void*>(device.pNext);
			device.pNext = &multidraw_features;
		}

		VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT fbo_loop_features{};
		if (pgpu->optional_features_support.framebuffer_loops)
		{
			fbo_loop_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ATTACHMENT_FEEDBACK_LOOP_LAYOUT_FEATURES_EXT;
			fbo_loop_features.attachmentFeedbackLoopLayout = VK_TRUE;
			fbo_loop_features.pNext = const_cast<void*>(device.pNext);
			device.pNext = &fbo_loop_features;
		}

		VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2_info{};
		if (pgpu->optional_features_support.synchronization_2)
		{
			synchronization2_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
			synchronization2_info.pNext = const_cast<void*>(device.pNext);
			synchronization2_info.synchronization2 = VK_TRUE;
			device.pNext = &synchronization2_info;
		}

		VkPhysicalDeviceFaultFeaturesEXT device_fault_info{};
		if (pgpu->optional_features_support.extended_device_fault)
		{
			device_fault_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
			device_fault_info.pNext = const_cast<void*>(device.pNext);
			device_fault_info.deviceFault = VK_TRUE;
			device_fault_info.deviceFaultVendorBinary = VK_FALSE;
			device.pNext = &device_fault_info;
		}

		VkPhysicalDeviceConditionalRenderingFeaturesEXT conditional_rendering_info{};
		if (pgpu->optional_features_support.conditional_rendering)
		{
			conditional_rendering_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
			conditional_rendering_info.pNext = const_cast<void*>(device.pNext);
			conditional_rendering_info.conditionalRendering = VK_TRUE;
			device.pNext = &conditional_rendering_info;
		}

		VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR shader_barycentric_info{};
		if (pgpu->optional_features_support.barycentric_coords)
		{
			shader_barycentric_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR;
			shader_barycentric_info.pNext = const_cast<void*>(device.pNext);
			shader_barycentric_info.fragmentShaderBarycentric = VK_TRUE;
			device.pNext = &shader_barycentric_info;
		}

		if (auto error = vkCreateDevice(*pgpu, &device, nullptr, &dev))
		{
			dump_debug_info(requested_extensions, enabled_features);
			vk::die_with_error(error);
		}

		// Dump some diagnostics to the log
		rsx_log.notice("%u extensions loaded:", ::size32(requested_extensions));
		for (const auto& ext : requested_extensions)
		{
			rsx_log.notice("** Using %s", ext);
		}

		// Initialize queues
		vkGetDeviceQueue(dev, graphics_queue_idx, 0, &m_graphics_queue);
		vkGetDeviceQueue(dev, transfer_queue_idx, transfer_queue_sub_index, &m_transfer_queue);

		if (present_queue_idx != umax)
		{
			vkGetDeviceQueue(dev, present_queue_idx, 0, &m_present_queue);
		}

		memory_map = vk::get_memory_mapping(pdev);
		m_formats_support = vk::get_optimal_tiling_supported_formats(pdev);

		if (g_cfg.video.disable_vulkan_mem_allocator)
		{
			m_allocator = std::make_unique<vk::mem_allocator_vk>(*this, pdev);
		}
		else
		{
			m_allocator = std::make_unique<vk::mem_allocator_vma>(*this, pdev, pdev);
		}

		// Useful for debugging different VRAM configurations
		const u64 vram_allocation_limit = g_cfg.video.vk.vram_allocation_limit * 0x100000ull;
		memory_map.device_local_total_bytes = std::min(memory_map.device_local_total_bytes, vram_allocation_limit);
	}

	void render_device::destroy()
	{
		if (g_render_device == this)
		{
			g_render_device = nullptr;
		}

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

	const VkFormatProperties render_device::get_format_properties(VkFormat format) const
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

	void render_device::rebalance_memory_type_usage()
	{
		// Rebalance device local memory types
		memory_map.device_local.rebalance();
	}

	void render_device::dump_debug_info(
		const std::vector<const char*>& requested_extensions,
		const VkPhysicalDeviceFeatures& requested_features) const
	{
		rsx_log.notice("Dumping requested extensions...");
		auto device_extensions = vk::supported_extensions(vk::supported_extensions::enumeration_class::device, nullptr, *pgpu);
		for (const auto& ext : requested_extensions)
		{
			rsx_log.notice("[%s] %s", device_extensions.is_supported(ext) ? "Supported" : "Not supported", ext);
		}

		rsx_log.notice("Dumping requested features...");
		const auto& supported_features = pgpu->features;

#define TEST_VK_FEATURE(name) \
		if (requested_features.name) {\
			if (supported_features.name) \
				rsx_log.notice("[Supported] "#name); \
			else \
				rsx_log.error("[Not supported] "#name); \
		}

		TEST_VK_FEATURE(robustBufferAccess);
		TEST_VK_FEATURE(fullDrawIndexUint32);
		TEST_VK_FEATURE(imageCubeArray);
		TEST_VK_FEATURE(independentBlend);
		TEST_VK_FEATURE(geometryShader);
		TEST_VK_FEATURE(tessellationShader);
		TEST_VK_FEATURE(sampleRateShading);
		TEST_VK_FEATURE(dualSrcBlend);
		TEST_VK_FEATURE(logicOp);
		TEST_VK_FEATURE(multiDrawIndirect);
		TEST_VK_FEATURE(drawIndirectFirstInstance);
		TEST_VK_FEATURE(depthClamp);
		TEST_VK_FEATURE(depthBiasClamp);
		TEST_VK_FEATURE(fillModeNonSolid);
		TEST_VK_FEATURE(depthBounds);
		TEST_VK_FEATURE(wideLines);
		TEST_VK_FEATURE(largePoints);
		TEST_VK_FEATURE(alphaToOne);
		TEST_VK_FEATURE(multiViewport);
		TEST_VK_FEATURE(samplerAnisotropy);
		TEST_VK_FEATURE(textureCompressionETC2);
		TEST_VK_FEATURE(textureCompressionASTC_LDR);
		TEST_VK_FEATURE(textureCompressionBC);
		TEST_VK_FEATURE(occlusionQueryPrecise);
		TEST_VK_FEATURE(pipelineStatisticsQuery);
		TEST_VK_FEATURE(vertexPipelineStoresAndAtomics);
		TEST_VK_FEATURE(fragmentStoresAndAtomics);
		TEST_VK_FEATURE(shaderTessellationAndGeometryPointSize);
		TEST_VK_FEATURE(shaderImageGatherExtended);
		TEST_VK_FEATURE(shaderStorageImageExtendedFormats);
		TEST_VK_FEATURE(shaderStorageImageMultisample);
		TEST_VK_FEATURE(shaderStorageImageReadWithoutFormat);
		TEST_VK_FEATURE(shaderStorageImageWriteWithoutFormat);
		TEST_VK_FEATURE(shaderUniformBufferArrayDynamicIndexing);
		TEST_VK_FEATURE(shaderSampledImageArrayDynamicIndexing);
		TEST_VK_FEATURE(shaderStorageBufferArrayDynamicIndexing);
		TEST_VK_FEATURE(shaderStorageImageArrayDynamicIndexing);
		TEST_VK_FEATURE(shaderClipDistance);
		TEST_VK_FEATURE(shaderCullDistance);
		TEST_VK_FEATURE(shaderFloat64);
		TEST_VK_FEATURE(shaderInt64);
		TEST_VK_FEATURE(shaderInt16);
		TEST_VK_FEATURE(shaderResourceResidency);
		TEST_VK_FEATURE(shaderResourceMinLod);
		TEST_VK_FEATURE(sparseBinding);
		TEST_VK_FEATURE(sparseResidencyBuffer);
		TEST_VK_FEATURE(sparseResidencyImage2D);
		TEST_VK_FEATURE(sparseResidencyImage3D);
		TEST_VK_FEATURE(sparseResidency2Samples);
		TEST_VK_FEATURE(sparseResidency4Samples);
		TEST_VK_FEATURE(sparseResidency8Samples);
		TEST_VK_FEATURE(sparseResidency16Samples);
		TEST_VK_FEATURE(sparseResidencyAliased);
		TEST_VK_FEATURE(variableMultisampleRate);
		TEST_VK_FEATURE(inheritedQueries);

#undef TEST_VK_FEATURE
	}

	// Shared Util
	memory_type_mapping get_memory_mapping(const vk::physical_device& dev)
	{
		VkPhysicalDevice pdev = dev;
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(pdev, &memory_properties);

		memory_type_mapping result;
		result.device_local_total_bytes = 0;
		result.host_visible_total_bytes = 0;
		result.device_bar_total_bytes = 0;

		// Sort the confusingly laid out heap-type map into something easier to scan.
		// Not performance-critical, this method is called once at initialization.
		struct memory_type
		{
			u32 type_index;
			VkFlags flags;
			VkDeviceSize size;
		};

		struct heap_type_map_entry
		{
			VkMemoryHeap heap;
			std::vector<memory_type> types;
		};

		std::vector<heap_type_map_entry> memory_heap_map;
		for (u32 i = 0; i < memory_properties.memoryHeapCount; ++i)
		{
			memory_heap_map.push_back(
			{
				.heap = memory_properties.memoryHeaps[i],
				.types = {}
			});
		}

		for (u32 i = 0; i < memory_properties.memoryTypeCount; i++)
		{
			auto& type_info = memory_properties.memoryTypes[i];
			memory_heap_map[type_info.heapIndex].types.push_back({ i, type_info.propertyFlags, 0 });
		}

		auto find_memory_type_with_property = [&memory_heap_map](VkFlags desired_flags, VkFlags excluded_flags)
		{
			std::vector<memory_type> results;

			for (auto& heap : memory_heap_map)
			{
				for (auto &type : heap.types)
				{
					if (((type.flags & desired_flags) == desired_flags) && !(type.flags & excluded_flags))
					{
						// Match, only once allowed per heap!
						results.push_back({ type.type_index, type.flags, heap.heap.size });
						break;
					}
				}
			}

			return results;
		};

		auto device_local_types = find_memory_type_with_property(
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			(VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD | VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD));
		auto host_coherent_types = find_memory_type_with_property(
			(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT),
			0);
		auto bar_memory_types = find_memory_type_with_property(
			(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
			0);

		if (host_coherent_types.empty())
		{
			rsx_log.warning("[Performance Warning] Could not identify a cached upload heap. Will fall back to uncached transport.");
			host_coherent_types = find_memory_type_with_property(
				(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
				0);
		}

		ensure(!device_local_types.empty());
		ensure(!host_coherent_types.empty());

		// BAR heap, currently parked for future use, I have some plans for it (kd-11)
		for (const auto& type : bar_memory_types)
		{
			result.device_bar.push(type.type_index, type.size);
			result.device_bar_total_bytes += type.size;
		}

		// Generic VRAM access, requires some minor prioritization based on flags
		// Most devices have a 'PURE' device local type, pin that as the first priority
		// Internally, there will be some reshuffling based on memory load later, but this is rare
		if (device_local_types.size() > 1)
		{
			std::sort(device_local_types.begin(), device_local_types.end(), [](const auto& a, const auto& b)
			{
				if (a.flags == b.flags)
				{
					return a.size > b.size;
				}

				return (a.flags == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) || (b.flags != VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT && a.size > b.size);
			});
		}

		for (const auto& type : device_local_types)
		{
			result.device_local.push(type.type_index, type.size);
			result.device_local_total_bytes += type.size;
		}

		// Sort upload heap entries based on size.
		if (host_coherent_types.size() > 1)
		{
			std::sort(host_coherent_types.begin(), host_coherent_types.end(), FN(x.size > y.size));
		}

		for (const auto& type : host_coherent_types)
		{
			result.host_visible_coherent.push(type.type_index, type.size);
			result.host_visible_total_bytes += type.size;
		}

		rsx_log.notice("Detected %llu MB of device local memory", result.device_local_total_bytes / (0x100000));
		rsx_log.notice("Detected %llu MB of host coherent memory", result.host_visible_total_bytes / (0x100000));
		rsx_log.notice("Detected %llu MB of BAR memory", result.device_bar_total_bytes / (0x100000));

		return result;
	}

	gpu_formats_support get_optimal_tiling_supported_formats(const physical_device& dev)
	{
		const auto test_format_features = [&dev](VkFormat format, VkFlags required_features, VkBool32 linear_features) -> bool
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(dev, format, &props);

			const auto supported_features_mask = (linear_features) ? props.linearTilingFeatures : props.optimalTilingFeatures;
			return (supported_features_mask & required_features) == required_features;
		};

		gpu_formats_support result = {};
		const VkFlags required_zbuffer_features = (VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
		const VkFlags required_colorbuffer_features = (VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT);

		// Check supported depth formats
		result.d24_unorm_s8 = test_format_features(VK_FORMAT_D24_UNORM_S8_UINT, required_zbuffer_features, VK_FALSE);
		result.d32_sfloat_s8 = test_format_features(VK_FORMAT_D32_SFLOAT_S8_UINT, required_zbuffer_features, VK_FALSE);

		// Hide d24_s8 if force high precision z buffer is enabled
		if (g_cfg.video.force_high_precision_z_buffer && result.d32_sfloat_s8)
		{
			result.d24_unorm_s8 = false;
		}

		// Checks if linear BGRA8 images can be used for present
		result.bgra8_linear = test_format_features(VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_FEATURE_BLIT_SRC_BIT, VK_TRUE);

		// Check if device supports RGBA8 format for rendering
		if (!test_format_features(VK_FORMAT_R8G8B8A8_UNORM, required_colorbuffer_features, VK_FALSE))
		{
			// Non-fatal. Most games use BGRA layout due to legacy reasons as old GPUs typically supported BGRA and RGBA was emulated.
			rsx_log.error("Your GPU and/or driver does not support RGBA8 format. This can cause problems in some rare games that use this memory layout.");
		}

		// Check if linear RGBA8 images can be used for present
		result.argb8_linear = test_format_features(VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_FEATURE_BLIT_SRC_BIT, VK_TRUE);

		return result;
	}
}
