#include "stdafx.h"
#include "instance.h"

namespace vk
{
	// Supported extensions
	supported_extensions::supported_extensions(enumeration_class _class, const char* layer_name, VkPhysicalDevice pdev)
	{
		u32 count;
		if (_class == enumeration_class::instance)
		{
			if (vkEnumerateInstanceExtensionProperties(layer_name, &count, nullptr) != VK_SUCCESS)
				return;
		}
		else
		{
			ensure(pdev);
			if (vkEnumerateDeviceExtensionProperties(pdev, layer_name, &count, nullptr) != VK_SUCCESS)
				return;
		}

		m_vk_exts.resize(count);
		if (_class == enumeration_class::instance)
		{
			vkEnumerateInstanceExtensionProperties(layer_name, &count, m_vk_exts.data());
		}
		else
		{
			vkEnumerateDeviceExtensionProperties(pdev, layer_name, &count, m_vk_exts.data());
		}
	}

	bool supported_extensions::is_supported(std::string_view ext) const
	{
		return std::any_of(m_vk_exts.cbegin(), m_vk_exts.cend(), [&](const VkExtensionProperties& p) { return p.extensionName == ext; });
	}

	// Instance
	instance::~instance()
	{
		if (m_instance)
		{
			destroy();
		}
	}

	void instance::destroy()
	{
		if (!m_instance) return;

		if (m_debugger)
		{
			_vkDestroyDebugReportCallback(m_instance, m_debugger, nullptr);
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

	void instance::enable_debugging()
	{
		if (!g_cfg.video.debug_output) return;

		PFN_vkDebugReportCallbackEXT callback = vk::dbgFunc;

		_vkCreateDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT"));
		_vkDestroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT"));

		VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
		dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		dbgCreateInfo.pfnCallback = callback;
		dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

		CHECK_RESULT(_vkCreateDebugReportCallback(m_instance, &dbgCreateInfo, NULL, &m_debugger));
	}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
	bool instance::create(const char* app_name, bool fast)
	{
		// Initialize a vulkan instance
		VkApplicationInfo app = {};

		app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app.pApplicationName = app_name;
		app.applicationVersion = 0;
		app.pEngineName = app_name;
		app.engineVersion = 0;
		app.apiVersion = VK_API_VERSION_1_2;

		// Set up instance information

		std::vector<const char*> extensions;
		std::vector<const char*> layers;
		const void* next_info = nullptr;

#ifdef __APPLE__
		// Declare MVK variables here to ensure the lifetime within the entire scope
		const VkBool32 setting_true = VK_TRUE;
		const int32_t setting_fast_math = g_cfg.video.disable_msl_fast_math.get() ? MVK_CONFIG_FAST_MATH_NEVER : MVK_CONFIG_FAST_MATH_ON_DEMAND;

		std::vector<VkLayerSettingEXT> mvk_settings;
		VkLayerSettingsCreateInfoEXT mvk_layer_settings_create_info{};
#endif

		if (!fast)
		{
			extensions_loaded = true;
			supported_extensions support(supported_extensions::instance);

			extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
			if (support.is_supported(VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
			{
				extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			}

#ifdef __APPLE__
			if (support.is_supported(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME))
			{
				extensions.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
				layers.push_back(kMVKMoltenVKDriverLayerName);

				mvk_settings.push_back(VkLayerSettingEXT{ kMVKMoltenVKDriverLayerName, "MVK_CONFIG_RESUME_LOST_DEVICE", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_true });
				mvk_settings.push_back(VkLayerSettingEXT{ kMVKMoltenVKDriverLayerName, "MVK_CONFIG_FAST_MATH_ENABLED", VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &setting_fast_math });

				mvk_layer_settings_create_info.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT;
				mvk_layer_settings_create_info.pNext = next_info;
				mvk_layer_settings_create_info.settingCount = static_cast<uint32_t>(mvk_settings.size());
				mvk_layer_settings_create_info.pSettings = mvk_settings.data();

				next_info = &mvk_layer_settings_create_info;
			}
#endif

			if (support.is_supported(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME))
			{
				extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
			}

			if (g_cfg.video.renderdoc_compatiblity && support.is_supported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
			{
				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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
#ifdef HAVE_WAYLAND
			if (support.is_supported(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME))
			{
				extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
				found_surface_ext = true;
			}
#endif //(WAYLAND)
#ifdef ANDROID
			if (support.is_supported(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME))
			{
				extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
				found_surface_ext = true;
			}
#endif
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
		instance_info.pNext = next_info;

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
	void instance::bind()
	{
		// Register some global states
		if (m_debugger)
		{
			_vkDestroyDebugReportCallback(m_instance, m_debugger, nullptr);
			m_debugger = nullptr;
		}

		enable_debugging();
	}

	std::vector<physical_device>& instance::enumerate_devices()
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

	swapchain_base* instance::create_swapchain(display_handle_t window_handle, vk::physical_device& dev)
	{
		WSI_config surface_config
		{
			.supports_automatic_wm_reports = true
		};
		m_surface = make_WSI_surface(m_instance, window_handle, &surface_config);

		u32 device_queues = dev.get_queue_count();
		std::vector<VkBool32> supports_present(device_queues, VK_FALSE);
		bool present_possible = true;

		for (u32 index = 0; index < device_queues; index++)
		{
			vkGetPhysicalDeviceSurfaceSupportKHR(dev, index, m_surface, &supports_present[index]);
		}

		u32 graphics_queue_idx = -1;
		u32 present_queue_idx = -1;
		u32 transfer_queue_idx = -1;

		auto test_queue_family = [&](u32 index, u32 desired_flags)
			{
				if (const auto flags = dev.get_queue_properties(index).queueFlags;
					(flags & desired_flags) == desired_flags)
				{
					return true;
				}

				return false;
			};

		for (u32 i = 0; i < device_queues; ++i)
		{
			// 1. Test for a present queue possibly one that also supports present
			if (present_queue_idx == umax && supports_present[i])
			{
				present_queue_idx = i;
				if (test_queue_family(i, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
				{
					graphics_queue_idx = i;
				}
			}
			// 2. Check for graphics support
			else if (graphics_queue_idx == umax && test_queue_family(i, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
			{
				graphics_queue_idx = i;
				if (supports_present[i])
				{
					present_queue_idx = i;
				}
			}
			// 3. Check if transfer + compute is available
			else if (transfer_queue_idx == umax && test_queue_family(i, VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))
			{
				transfer_queue_idx = i;
			}
		}

		if (graphics_queue_idx == umax)
		{
			rsx_log.fatal("Failed to find a suitable graphics queue");
			return nullptr;
		}

		if (graphics_queue_idx != present_queue_idx)
		{
			// Separate graphics and present, use headless fallback
			present_possible = false;
		}

		if (!present_possible)
		{
			//Native(sw) swapchain
			rsx_log.error("It is not possible for the currently selected GPU to present to the window (Likely caused by NVIDIA driver running the current display)");
			rsx_log.warning("Falling back to software present support (native windowing API)");
			auto swapchain = new swapchain_NATIVE(dev, -1, graphics_queue_idx, transfer_queue_idx);
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
			for (auto& surface_format : surfFormats)
			{
				if (surface_format.format == VK_FORMAT_B8G8R8A8_UNORM)
				{
					format = VK_FORMAT_B8G8R8A8_UNORM;
					break;
				}
			}
		}

		color_space = surfFormats[0].colorSpace;

		return new swapchain_WSI(dev, present_queue_idx, graphics_queue_idx, transfer_queue_idx, format, m_surface, color_space, !surface_config.supports_automatic_wm_reports);
	}
}
