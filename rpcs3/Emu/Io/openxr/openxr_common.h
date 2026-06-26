#pragma once

// Common includes and helpers of the OpenXR session, input and rendering code

#ifndef HAVE_VULKAN
#error "OpenXR requires Vulkan"
#endif

#include "Emu/RSX/VK/VulkanAPI.h"

// Enables the Vulkan types in openxr_platform.h. Has to be included after vulkan.h.
#ifndef XR_USE_GRAPHICS_API_VULKAN
#define XR_USE_GRAPHICS_API_VULKAN
#endif

#include "openxr/openxr.h"
#include "openxr/openxr_platform.h"

#include "util/types.hpp"
#include "util/logs.hpp"

#include <set>
#include <string>

LOG_CHANNEL(openxr_log, "openxr");

namespace openxr
{
	// Extension functions are not exported by the loader and have to be queried
	template <typename T>
	bool get_instance_proc(XrInstance instance, const char* name, T& func)
	{
		PFN_xrVoidFunction void_func = nullptr;

		if (XR_FAILED(xrGetInstanceProcAddr(instance, name, &void_func)) || !void_func)
		{
			openxr_log.error("xrGetInstanceProcAddr failed for '%s'", name);
			return false;
		}

		func = reinterpret_cast<T>(void_func);
		return true;
	}
}
