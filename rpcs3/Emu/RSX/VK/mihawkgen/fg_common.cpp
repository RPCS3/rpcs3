/*
 * PS5_FrameGen - the generator's device-free parts: names, checks, measures.
 * Kept apart from fg_vulkan.cpp so the host tests run them without a GPU.
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "framegen.h"

#include <cmath>
#include <limits>

const char *fg_method_name(fg_method method)
{
	switch (method) {
	case FG_METHOD_REPEAT:
		return "repeat";
	case FG_METHOD_BLEND:
		return "blend";
	case FG_METHOD_MVWARP:
		return "mvwarp";
	case FG_METHOD_NEURAL:
		return "neural";
	case FG_METHOD_MVSPLAT:
		return "mvsplat";
	default:
		return "unknown";
	}
}

const char *fg_check_create_info(const fg_create_info *info)
{
	if (!info) {
		return "no create info";
	}
	if (info->device == VK_NULL_HANDLE) {
		return "no device";
	}
	// The PS5's display is 3840x2160; 16384 is Vulkan's guaranteed 2D image limit
	if (info->width == 0 || info->height == 0 || info->width > 16384 || info->height > 16384) {
		return "width and height must be 1 to 16384";
	}
	if (info->slots == 0 || info->slots > 8) {
		return "slots must be 1 to 8";
	}
	if (info->output_format != VK_FORMAT_R8G8B8A8_UNORM) {
		return "output_format must be VK_FORMAT_R8G8B8A8_UNORM (the shaders write rgba8)";
	}
	return nullptr;
}

double fg_psnr(double sum_squared_error, uint64_t samples)
{
	if (samples == 0) {
		return std::numeric_limits<double>::quiet_NaN();
	}
	const double mse = sum_squared_error / static_cast<double>(samples);
	if (mse <= 0.0) {
		return std::numeric_limits<double>::infinity();
	}
	return 10.0 * std::log10(1.0 / mse);
}
