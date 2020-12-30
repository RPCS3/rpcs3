#include "shared.h"
#include "util/logs.hpp"

namespace vk
{
	void die_with_error(VkResult error_code,
		const char* file,
		const char* func,
		u32 line,
		u32 col)
	{
		std::string error_message;
		int severity = 0; // 0 - die, 1 - warn, 2 - nothing

		switch (error_code)
		{
		case VK_SUCCESS:
		case VK_EVENT_SET:
		case VK_EVENT_RESET:
		case VK_INCOMPLETE:
			return;
		case VK_SUBOPTIMAL_KHR:
			error_message = "Present surface is suboptimal (VK_SUBOPTIMAL_KHR)";
			severity = 1;
			break;
		case VK_NOT_READY:
			error_message = "Device or resource busy (VK_NOT_READY)";
			break;
		case VK_TIMEOUT:
			error_message = "Timeout event (VK_TIMEOUT)";
			break;
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			error_message = "Out of host memory (system RAM) (VK_ERROR_OUT_OF_HOST_MEMORY)";
			break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			error_message = "Out of video memory (VRAM) (VK_ERROR_OUT_OF_DEVICE_MEMORY)";
			break;
		case VK_ERROR_INITIALIZATION_FAILED:
			error_message = "Initialization failed (VK_ERROR_INITIALIZATION_FAILED)";
			break;
		case VK_ERROR_DEVICE_LOST:
			error_message = "Device lost (Driver crashed with unspecified error or stopped responding and recovered) (VK_ERROR_DEVICE_LOST)";
			break;
		case VK_ERROR_MEMORY_MAP_FAILED:
			error_message = "Memory map failed (VK_ERROR_MEMORY_MAP_FAILED)";
			break;
		case VK_ERROR_LAYER_NOT_PRESENT:
			error_message = "Requested layer is not available (Try disabling debug output or install vulkan SDK) (VK_ERROR_LAYER_NOT_PRESENT)";
			break;
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			error_message = "Requested extension not available (VK_ERROR_EXTENSION_NOT_PRESENT)";
			break;
		case VK_ERROR_FEATURE_NOT_PRESENT:
			error_message = "Requested feature not available (VK_ERROR_FEATURE_NOT_PRESENT)";
			break;
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			error_message = "Incompatible driver (VK_ERROR_INCOMPATIBLE_DRIVER)";
			break;
		case VK_ERROR_TOO_MANY_OBJECTS:
			error_message = "Too many objects created (Out of handles) (VK_ERROR_TOO_MANY_OBJECTS)";
			break;
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			error_message = "Format not supported (VK_ERROR_FORMAT_NOT_SUPPORTED)";
			break;
		case VK_ERROR_FRAGMENTED_POOL:
			error_message = "Fragmented pool (VK_ERROR_FRAGMENTED_POOL)";
			break;
		case VK_ERROR_SURFACE_LOST_KHR:
			error_message = "Surface lost (VK_ERROR_SURFACE_LOST)";
			break;
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			error_message = "Native window in use (VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)";
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			error_message = "Present surface is out of date (VK_ERROR_OUT_OF_DATE_KHR)";
			severity = 1;
			break;
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			error_message = "Incompatible display (VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)";
			break;
		case VK_ERROR_VALIDATION_FAILED_EXT:
			error_message = "Validation failed (VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)";
			break;
		case VK_ERROR_INVALID_SHADER_NV:
			error_message = "Invalid shader code (VK_ERROR_INVALID_SHADER_NV)";
			break;
		case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
			error_message = "Out of pool memory (VK_ERROR_OUT_OF_POOL_MEMORY_KHR)";
			break;
		case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR:
			error_message = "Invalid external handle (VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR)";
			break;
		default:
			error_message = fmt::format("Unknown Code (%Xh, %d)%s", static_cast<s32>(error_code), static_cast<s32>(error_code), src_loc{line, col, file, func});
			break;
		}

		switch (severity)
		{
		default:
		case 0:
			fmt::throw_exception("Assertion Failed! Vulkan API call failed with unrecoverable error: %s%s", error_message, src_loc{line, col, file, func});
		case 1:
			rsx_log.error("Vulkan API call has failed with an error but will continue: %s%s", error_message, src_loc{line, col, file, func});
			break;
		case 2:
			break;
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
	                                       u64 srcObject, usz location, s32 msgCode,
	                                       const char* pLayerPrefix, const char* pMsg, void* pUserData)
	{
		if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		{
			if (strstr(pMsg, "IMAGE_VIEW_TYPE_1D"))
				return false;

			rsx_log.error("ERROR: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		}
		else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		{
			rsx_log.warning("WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		}
		else
		{
			return false;
		}

		// Let the app crash..
		return false;
	}
}
