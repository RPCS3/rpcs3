#pragma once

// PS5: the console has no window system. A title owns the one display, through
// VK_KHR_display on RADV (as PS5_VulkanTemplate's foundation drives it): the
// surface is a display plane, created here, and RPCS3's WSI swapchain presents
// to it as it would to a window.

#include "swapchain_core.h"
#include "util/logs.hpp"
#include "Utilities/File.h"
#include "Utilities/StrFmt.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

namespace vk
{
#if defined(__PROSPERO__)
	// The present queue is RADV's graphics queue, so the software-present
	// fallback (instance::create_swapchain) never applies; as on Android, it
	// is the base class, which reports that it is not implemented.
	using swapchain_NATIVE = native_swapchain_base;

	// The display's refresh rate in mHz, as its mode reports it: frame
	// generation's frames go up one a refresh (VKPresent.cpp)
	inline u32 g_ps5_refresh_millihertz = 59940;

	[[maybe_unused]] static
	VkSurfaceKHR make_WSI_surface(VkInstance vk_instance, display_handle_t /*window_handle*/, WSI_config* /*config*/)
	{
		VkSurfaceKHR result = VK_NULL_HANDLE;

		// The console has one GPU, and the display hangs off it
		u32 gpu_count = 1;
		VkPhysicalDevice gpu = VK_NULL_HANDLE;
		if (vkEnumeratePhysicalDevices(vk_instance, &gpu_count, &gpu) < 0 || gpu == VK_NULL_HANDLE)
		{
			rsx_log.fatal("PS5: no GPU to present with");
			return result;
		}

		u32 display_count = 0;
		vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &display_count, nullptr);
		std::vector<VkDisplayPropertiesKHR> displays(display_count);
		vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &display_count, displays.data());
		if (displays.empty())
		{
			rsx_log.fatal("PS5: VK_KHR_display reports no display");
			return result;
		}
		const VkDisplayKHR display = displays[0].display;

		// The largest mode, then the fastest refresh at that size (3840x2160 at
		// 119.88 Hz where the title asks for high frame rates, else 59.94 Hz)
		u32 mode_count = 0;
		vkGetDisplayModePropertiesKHR(gpu, display, &mode_count, nullptr);
		std::vector<VkDisplayModePropertiesKHR> modes(mode_count);
		vkGetDisplayModePropertiesKHR(gpu, display, &mode_count, modes.data());
		const VkDisplayModePropertiesKHR* best = nullptr;
		for (const auto& mode : modes)
		{
			const auto& p = mode.parameters;
			if (!best)
			{
				best = &mode;
				continue;
			}
			const auto& b = best->parameters;
			const u64 area = u64{p.visibleRegion.width} * p.visibleRegion.height;
			const u64 best_area = u64{b.visibleRegion.width} * b.visibleRegion.height;
			if (area > best_area || (area == best_area && p.refreshRate > b.refreshRate))
			{
				best = &mode;
			}
		}
		if (!best)
		{
			rsx_log.fatal("PS5: the display reports no mode");
			return result;
		}

		// The first plane that can show this display
		u32 plane_count = 0;
		vkGetPhysicalDeviceDisplayPlanePropertiesKHR(gpu, &plane_count, nullptr);
		std::vector<VkDisplayPlanePropertiesKHR> planes(plane_count);
		vkGetPhysicalDeviceDisplayPlanePropertiesKHR(gpu, &plane_count, planes.data());
		u32 plane_index = umax;
		for (u32 i = 0; i < plane_count && plane_index == umax; i++)
		{
			u32 count = 0;
			vkGetDisplayPlaneSupportedDisplaysKHR(gpu, i, &count, nullptr);
			std::vector<VkDisplayKHR> supported(count);
			vkGetDisplayPlaneSupportedDisplaysKHR(gpu, i, &count, supported.data());
			for (VkDisplayKHR d : supported)
			{
				if (d == display)
				{
					plane_index = i;
					break;
				}
			}
		}
		if (plane_index == umax)
		{
			rsx_log.fatal("PS5: no display plane for the display");
			return result;
		}

		VkDisplayPlaneCapabilitiesKHR caps{};
		vkGetDisplayPlaneCapabilitiesKHR(gpu, best->displayMode, plane_index, &caps);
		VkDisplayPlaneAlphaFlagBitsKHR alpha = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
		for (auto mode : { VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR, VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR,
			VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR, VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR })
		{
			if (caps.supportedAlpha & mode)
			{
				alpha = mode;
				break;
			}
		}

		VkDisplaySurfaceCreateInfoKHR create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
		create_info.displayMode = best->displayMode;
		create_info.planeIndex = plane_index;
		create_info.planeStackIndex = planes[plane_index].currentStackIndex;
		create_info.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		create_info.globalAlpha = 1.0f;
		create_info.alphaMode = alpha;
		create_info.imageExtent = best->parameters.visibleRegion;

		rsx_log.notice("PS5: presenting to display plane %u at %ux%u, %u mHz", plane_index,
			create_info.imageExtent.width, create_info.imageExtent.height, best->parameters.refreshRate);

		// Why not 119.88 Hz, when it is not: the driver offers that mode only
		// where the title declares it (param.json's attribute3 0x40), VideoOut
		// takes its high-frame-rate output and the display then refreshes at it
		// (PS5_Mesa's wsi_common_videoout.c, whose own line in klog says which
		// step failed)
		std::string offered;
		u32 fastest = 0;
		for (const auto& mode : modes)
		{
			const auto& p = mode.parameters;
			offered += fmt::format("%s%ux%u at %u mHz", offered.empty() ? "" : ", ", p.visibleRegion.width, p.visibleRegion.height, p.refreshRate);
			fastest = std::max(fastest, p.refreshRate);
		}
		rsx_log.notice("PS5: the display's modes: %s", offered);
		if (fastest < 100'000)
		{
			bool declared = false;
			if (fs::file param{"/app0/sce_sys/param.json"})
			{
				const std::string text = param.to_string();
				if (const usz at = text.find("\"attribute3\""); at != umax)
				{
					if (const usz colon = text.find(':', at); colon != umax)
					{
						declared = (std::strtoul(text.c_str() + colon + 1, nullptr, 0) & 0x40) != 0;
					}
				}
			}
			rsx_log.warning("PS5: no 119.88 Hz mode offered: %s", declared
				? "the title declares it (attribute3 0x40), so VideoOut refused it or the display kept refreshing at 60 Hz. The console's 120 Hz output setting (Screen and Video > Video Output) and the display decide it; klog's wsi/videoout line says which"
				: "the title's param.json does not declare high-frame-rate output (attribute3 0x40)");
		}
		if (best->parameters.refreshRate)
		{
			g_ps5_refresh_millihertz = best->parameters.refreshRate;
		}

		CHECK_RESULT(vkCreateDisplayPlaneSurfaceKHR(vk_instance, &create_info, nullptr, &result));
		return result;
	}
#endif
}
