#pragma once

#include "../VulkanAPI.h"
#include <string>
#include <source_location>

namespace vk
{
#define CHECK_RESULT(expr) { VkResult _res = (expr); if (_res != VK_SUCCESS) vk::die_with_error(_res); }
#define CHECK_RESULT_EX(expr, msg) { VkResult _res = (expr); if (_res != VK_SUCCESS) vk::die_with_error(_res, msg); }

	void die_with_error(VkResult error_code, std::string message = {}, const std::source_location src_loc = std::source_location::current());

	VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
	                                       u64 srcObject, usz location, s32 msgCode,
	                                       const char *pLayerPrefix, const char *pMsg, void *pUserData);

	VkBool32 BreakCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
		u64 srcObject, usz location, s32 msgCode,
		const char* pLayerPrefix, const char* pMsg,
		void* pUserData);
}
