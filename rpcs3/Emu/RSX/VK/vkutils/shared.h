#pragma once

#include "../VulkanAPI.h"
#include <string>

namespace vk
{
#define CHECK_RESULT(expr) { VkResult _res = (expr); if (_res != VK_SUCCESS) vk::die_with_error(_res); }
#define CHECK_RESULT_EX(expr, msg) { VkResult _res = (expr); if (_res != VK_SUCCESS) vk::die_with_error(_res, msg); }

	void die_with_error(VkResult error_code, std::string message = {},
		const char* file = __builtin_FILE(),
		const char* func = __builtin_FUNCTION(),
		u32 line = __builtin_LINE(),
		u32 col = __builtin_COLUMN());

	VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
	                                       u64 srcObject, usz location, s32 msgCode,
	                                       const char *pLayerPrefix, const char *pMsg, void *pUserData);

	VkBool32 BreakCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
		u64 srcObject, usz location, s32 msgCode,
		const char* pLayerPrefix, const char* pMsg,
		void* pUserData);
}
