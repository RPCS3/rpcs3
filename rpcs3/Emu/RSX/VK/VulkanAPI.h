#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_MACOS_MVK
#elif HAVE_X11
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4005 )
#endif

#include <vulkan/vulkan.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <util/types.hpp>

// Requires SDK ver 230 which is not supported by CI currently
#ifndef VK_EXT_device_fault

#define VK_EXT_device_fault 1
#define VK_EXT_DEVICE_FAULT_EXTENSION_NAME "VK_EXT_device_fault"
#define VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT static_cast<VkStructureType>(1000341002)
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT static_cast<VkStructureType>(1000341000)

typedef enum VkDeviceFaultAddressTypeEXT {
	VK_DEVICE_FAULT_ADDRESS_TYPE_NONE_EXT = 0,
	VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT = 1,
	VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT = 2,
	VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_EXT = 3,
	VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_EXT = 4,
	VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_EXT = 5,
	VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT = 6,
} VkDeviceFaultAddressTypeEXT;

typedef struct VkPhysicalDeviceFaultFeaturesEXT {
	VkStructureType    sType;
	void*              pNext;
	VkBool32           deviceFault;
	VkBool32           deviceFaultVendorBinary;
} VkPhysicalDeviceFaultFeaturesEXT;

typedef struct VkDeviceFaultCountsEXT {
	VkStructureType    sType;
	void*              pNext;
	uint32_t           addressInfoCount;
	uint32_t           vendorInfoCount;
	VkDeviceSize       vendorBinarySize;
} VkDeviceFaultCountsEXT;

typedef struct VkDeviceFaultAddressInfoEXT {
	VkDeviceFaultAddressTypeEXT    addressType;
	VkDeviceAddress                reportedAddress;
	VkDeviceSize                   addressPrecision;
} VkDeviceFaultAddressInfoEXT;

typedef struct VkDeviceFaultVendorInfoEXT {
	char        description[VK_MAX_DESCRIPTION_SIZE];
	uint64_t    vendorFaultCode;
	uint64_t    vendorFaultData;
} VkDeviceFaultVendorInfoEXT;

typedef struct VkDeviceFaultInfoEXT {
	VkStructureType                 sType;
	void*                           pNext;
	char                            description[VK_MAX_DESCRIPTION_SIZE];
	VkDeviceFaultAddressInfoEXT*    pAddressInfos;
	VkDeviceFaultVendorInfoEXT*     pVendorInfos;
	void*                           pVendorBinaryData;
} VkDeviceFaultInfoEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkGetDeviceFaultInfoEXT(
	VkDevice                                    device,
	VkDeviceFaultCountsEXT*                     pFaultCounts,
	VkDeviceFaultInfoEXT*                       pFaultInfo);

typedef VkResult (VKAPI_PTR* PFN_vkGetDeviceFaultInfoEXT)(VkDevice device, VkDeviceFaultCountsEXT* pFaultCounts, VkDeviceFaultInfoEXT* pFaultInfo);

#endif
