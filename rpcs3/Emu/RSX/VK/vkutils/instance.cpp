#include "stdafx.h"
#include "instance.h"

#include "Emu/system_config.h"

#ifdef __PROSPERO__
#include <array>
#include <cstdlib>
#include <cstring>

// PS5 fork: the Vulkan driver's own memory, counted by its scope (command,
// object, cache, device, instance) for the trace's memory line
// (ps5_frontend.cpp). GTA V ran the title out of memory after 20 minutes
// (build 138): the heap grew to 8.7 GiB, and nothing said whose it was. Given
// to the instance, it is the device's too (a device made without callbacks of
// its own allocates with its instance's)
namespace
{
	atomic_t<s64> g_vk_driver_bytes[5]{};

	struct alignas(16) vk_block
	{
		u64 size;
		u32 scope;
		u32 offset; // from the block malloc returned to the caller's memory
	};

	void* VKAPI_CALL vk_count_alloc(void*, usz size, usz alignment, VkSystemAllocationScope scope)
	{
		const usz align = std::max<usz>(alignment, alignof(vk_block));
		const usz offset = (sizeof(vk_block) + align - 1) / align * align;
		void* base = nullptr;
		if (!size || posix_memalign(&base, align, offset + size) != 0)
		{
			return nullptr;
		}
		u8* const memory = static_cast<u8*>(base) + offset;
		vk_block& block = reinterpret_cast<vk_block*>(memory)[-1];
		const u32 index = std::min<u32>(static_cast<u32>(scope), 4);
		block = {size, index, static_cast<u32>(offset)};
		g_vk_driver_bytes[index] += static_cast<s64>(size);
		return memory;
	}

	void VKAPI_CALL vk_count_free(void*, void* memory)
	{
		if (!memory)
		{
			return;
		}
		const vk_block block = reinterpret_cast<vk_block*>(memory)[-1];
		g_vk_driver_bytes[block.scope] -= static_cast<s64>(block.size);
		std::free(static_cast<u8*>(memory) - block.offset);
	}

	void* VKAPI_CALL vk_count_realloc(void* user, void* original, usz size, usz alignment, VkSystemAllocationScope scope)
	{
		if (!original)
		{
			return vk_count_alloc(user, size, alignment, scope);
		}
		if (!size)
		{
			vk_count_free(user, original);
			return nullptr;
		}
		void* const moved = vk_count_alloc(user, size, alignment, scope);
		if (moved)
		{
			std::memcpy(moved, original, std::min<usz>(size, reinterpret_cast<vk_block*>(original)[-1].size));
			vk_count_free(user, original);
		}
		return moved;
	}

	const VkAllocationCallbacks g_vk_counting_allocator
	{
		.pUserData = nullptr,
		.pfnAllocation = vk_count_alloc,
		.pfnReallocation = vk_count_realloc,
		.pfnFree = vk_count_free,
		.pfnInternalAllocation = nullptr,
		.pfnInternalFree = nullptr,
	};
}

// The bytes the driver holds now, by scope: command, object, cache, device, instance
std::array<s64, 5> ps5_vk_driver_bytes()
{
	std::array<s64, 5> bytes{};
	for (usz i = 0; i < bytes.size(); i++)
	{
		bytes[i] = g_vk_driver_bytes[i].load();
	}
	return bytes;
}
#endif

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

#ifdef __PROSPERO__
		vkDestroyInstance(m_instance, &g_vk_counting_allocator);
#else
		vkDestroyInstance(m_instance, nullptr);
#endif
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
			extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
			extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
			if (support.is_supported(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME))
			{
				extensions.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);

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
			extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
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
#ifdef __PROSPERO__
			// PS5: no window system; the surface is a display plane (swapchain_ps5.hpp)
			if (support.is_supported(VK_KHR_DISPLAY_EXTENSION_NAME))
			{
				extensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
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
#ifdef __APPLE__ 
		// MoltenVK's ICD will not be detected without these extensions enabled.
		else
		{
			extensions_loaded = true;
			extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
			extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		}
#endif

		VkInstanceCreateInfo instance_info = {};
		instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_info.pApplicationInfo = &app;
		instance_info.enabledLayerCount = static_cast<u32>(layers.size());
		instance_info.ppEnabledLayerNames = layers.data();
#ifdef __APPLE__
		instance_info.enabledExtensionCount = static_cast<u32>(extensions.size());
		instance_info.ppEnabledExtensionNames = extensions.data();
#else
		instance_info.enabledExtensionCount = fast ? 0 : static_cast<u32>(extensions.size());
		instance_info.ppEnabledExtensionNames = fast ? nullptr : extensions.data();
#endif
		instance_info.pNext = next_info;
#ifdef __APPLE__
		instance_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

#ifdef __PROSPERO__
		if (VkResult result = vkCreateInstance(&instance_info, &g_vk_counting_allocator, &m_instance); result != VK_SUCCESS)
#else
		if (VkResult result = vkCreateInstance(&instance_info, nullptr, &m_instance); result != VK_SUCCESS)
#endif
		{
			if (result == VK_ERROR_LAYER_NOT_PRESENT)
			{
				rsx_log.fatal("Could not initialize layer VK_LAYER_KHRONOS_validation");
			}

			return false;
		}

#ifdef __PROSPERO__
		// PS5: the commands come through volk (rpcs3/Emu/CMakeLists.txt); load
		// this instance's, the device's among them (one GPU)
		volkLoadInstance(m_instance);
#endif

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

		auto priority = [](const physical_device& dev)
		{
			switch (dev.get_type())
			{
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				return 0;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				return 1;
			default:
				return 2;
			}
		};

		std::stable_sort(gpus.begin(), gpus.end(),
			[&](const physical_device& a, const physical_device& b)
			{
				return priority(a) < priority(b);
			});

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
