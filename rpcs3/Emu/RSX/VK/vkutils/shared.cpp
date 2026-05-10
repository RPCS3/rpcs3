#include "device.h"
#include "shared.h"
#include "util/logs.hpp"

#ifndef _WIN32
#include <signal.h>
#endif

namespace vk
{
	extern void print_debug_markers();

	std::string retrieve_device_fault_info()
	{
		if (!g_render_device || !g_render_device->get_extended_device_fault_support())
		{
			return "Extended fault info is not available. Extension 'VK_EXT_device_fault' is probably not supported by your driver.";
		}

		ensure(_vkGetDeviceFaultInfoEXT);

		VkDeviceFaultCountsEXT fault_counts
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT
		};

		std::vector<VkDeviceFaultAddressInfoEXT> address_info;
		std::vector<VkDeviceFaultVendorInfoEXT> vendor_info;
		std::vector<u8> vendor_binary_data;
		std::string fault_description;

		// Retrieve sizes
		_vkGetDeviceFaultInfoEXT(*g_render_device, &fault_counts, nullptr);

		// Resize arrays and fill
		address_info.resize(fault_counts.addressInfoCount);
		vendor_info.resize(fault_counts.vendorInfoCount);
		vendor_binary_data.resize(fault_counts.vendorBinarySize);

		VkDeviceFaultInfoEXT fault_info
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT,
			.pAddressInfos = address_info.data(),
			.pVendorInfos = vendor_info.data(),
			.pVendorBinaryData = vendor_binary_data.data()
		};
		_vkGetDeviceFaultInfoEXT(*g_render_device, &fault_counts, &fault_info);

		fault_description = fault_info.description;
		std::string fault_message = fmt::format(
			"Device Fault Information:\n"
			"Fault Summary:\n"
			"  %s\n\n",
			fault_description);

		if (!address_info.empty())
		{
			fmt::append(fault_message, "  Address Fault Information:\n", fault_description);

			for (const auto& fault : address_info)
			{
				std::string access_type = "access_unknown";
				switch (fault.addressType)
				{
				case VK_DEVICE_FAULT_ADDRESS_TYPE_NONE_EXT:
					access_type = "access_none";
					break;
				case VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT:
					access_type = "access_read"; break;
				case VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT:
					access_type = "access_write"; break;
				case VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_EXT:
					access_type = "access_execute"; break;
				case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_EXT:
					access_type = "instruction_pointer_unknown"; break;
				case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_EXT:
					access_type = "instruction_pointer_invalid"; break;
				case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT:
					access_type = "instruction_pointer_fault"; break;
				default:
					break;
				}

				fmt::append(fault_message, "  - Fault at address 0x%llx caused by %s\n", fault.reportedAddress, access_type);
			}
		}

		if (!vendor_info.empty())
		{
			fmt::append(fault_message, "  Vendor Fault Information:\n", fault_description);

			for (const auto& fault : vendor_info)
			{
				fmt::append(fault_message, "  - [0x%llx, 0x%llx] %s\n", fault.vendorFaultCode, fault.vendorFaultData, fault.description);
			}
		}

		return fault_message;
	}

	void die_with_error(VkResult error_code, std::string message, std::source_location src_loc)
	{
		std::string error_message, extra_info;
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
			extra_info = retrieve_device_fault_info();
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
		case VK_ERROR_FRAGMENTATION_EXT:
			error_message = "Descriptor pool creation failed (VK_ERROR_FRAGMENTATION)";
			break;
		default:
			error_message = fmt::format("Unknown Code (%Xh, %d)%s", static_cast<s32>(error_code), static_cast<s32>(error_code), src_loc);
			break;
		}

		if (!extra_info.empty())
		{
			error_message = fmt::format("%s\n---------------- EXTRA INFORMATION --------------------\n%s", error_message, extra_info);
		}

		switch (severity)
		{
		default:
		case 0:
			print_debug_markers();

			if (!message.empty()) message += "\n\n";
			fmt::throw_exception("%sAssertion Failed! Vulkan API call failed with unrecoverable error: %s%s", message, error_message, src_loc);
		case 1:
			rsx_log.error("Vulkan API call has failed with an error but will continue: %s%s", error_message, src_loc);
			break;
		case 2:
			break;
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT /*objType*/,
	                                       u64 /*srcObject*/, usz /*location*/, s32 msgCode,
	                                       const char* pLayerPrefix, const char* pMsg, void* /*pUserData*/)
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

	VkBool32 BreakCallback(VkFlags /*msgFlags*/, VkDebugReportObjectTypeEXT /*objType*/,
		u64 /*srcObject*/, usz /*location*/, s32 /*msgCode*/,
		const char* /*pLayerPrefix*/, const char* /*pMsg*/, void* /*pUserData*/)
	{
#ifdef _WIN32
		DebugBreak();
#else
		raise(SIGTRAP);
#endif

		return false;
	}
}
