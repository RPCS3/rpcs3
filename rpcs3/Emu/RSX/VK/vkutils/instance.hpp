#pragma once

#include "../VulkanAPI.h"
#include "swapchain.hpp"

#include <algorithm>
#include <vector>

namespace vk
{
	class supported_extensions
	{
	private:
		std::vector<VkExtensionProperties> m_vk_exts;

	public:
		enum enumeration_class
		{
			instance = 0,
			device   = 1
		};

		supported_extensions(enumeration_class _class, const char* layer_name = nullptr, VkPhysicalDevice pdev = VK_NULL_HANDLE)
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

		bool is_supported(const char* ext)
		{
			return std::any_of(m_vk_exts.cbegin(), m_vk_exts.cend(), [&](const VkExtensionProperties& p) { return std::strcmp(p.extensionName, ext) == 0; });
		}
	};

	class instance
	{
	private:
		std::vector<physical_device> gpus;
		VkInstance m_instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;

		PFN_vkDestroyDebugReportCallbackEXT _vkDestroyDebugReportCallback = nullptr;
		PFN_vkCreateDebugReportCallbackEXT _vkCreateDebugReportCallback = nullptr;
		VkDebugReportCallbackEXT m_debugger = nullptr;

		bool extensions_loaded = false;

	public:

		instance() = default;

		~instance()
		{
			if (m_instance)
			{
				destroy();
			}
		}

		void destroy()
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

		void enable_debugging()
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
		bool create(const char* app_name, bool fast = false)
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

			std::vector<const char*> extensions;
			std::vector<const char*> layers;

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

				if (support.is_supported(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME))
				{
					extensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
				}

				if (support.is_supported(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME))
				{
					extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
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
		void bind()
		{
			// Register some global states
			if (m_debugger)
			{
				_vkDestroyDebugReportCallback(m_instance, m_debugger, nullptr);
				m_debugger = nullptr;
			}

			enable_debugging();
		}

		std::vector<physical_device>& enumerate_devices()
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

		swapchain_base* create_swapchain(display_handle_t window_handle, vk::physical_device& dev)
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
					createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
					createInfo.dpy = p.first;
					createInfo.window = p.second;
					CHECK_RESULT(vkCreateXlibSurfaceKHR(this->m_instance, &createInfo, nullptr, &m_surface));
				}
				else
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
					if constexpr (std::is_same_v<T, std::pair<wl_display*, wl_surface*>>)
					{
						VkWaylandSurfaceCreateInfoKHR createInfo = {};
						createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
						createInfo.display = p.first;
						createInfo.surface = p.second;
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
			std::vector<VkBool32> supports_present(device_queues, VK_FALSE);
			bool present_possible = true;

			for (u32 index = 0; index < device_queues; index++)
			{
				vkGetPhysicalDeviceSurfaceSupportKHR(dev, index, m_surface, &supports_present[index]);
			}

			u32 graphics_queue_idx = UINT32_MAX;
			u32 present_queue_idx = UINT32_MAX;
			u32 transfer_queue_idx = UINT32_MAX;

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
				if (present_queue_idx == UINT32_MAX && supports_present[i])
				{
					present_queue_idx = i;
					if (test_queue_family(i, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
					{
						graphics_queue_idx = i;
					}
				}
				// 2. Check for graphics support
				else if (graphics_queue_idx == UINT32_MAX && test_queue_family(i, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
				{
					graphics_queue_idx = i;
					if (supports_present[i])
					{
						present_queue_idx = i;
					}
				}
				// 3. Check if transfer + compute is available
				else if (transfer_queue_idx == UINT32_MAX && test_queue_family(i, VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))
				{
					transfer_queue_idx = i;
				}
			}

			if (graphics_queue_idx == UINT32_MAX)
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
				auto swapchain = new swapchain_NATIVE(dev, UINT32_MAX, graphics_queue_idx, transfer_queue_idx);
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

			return new swapchain_WSI(dev, present_queue_idx, graphics_queue_idx, transfer_queue_idx, format, m_surface, color_space, force_wm_reporting_off);
		}
	};
}
