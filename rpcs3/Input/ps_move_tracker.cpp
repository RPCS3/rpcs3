#include "stdafx.h"
#include "Emu/Cell/Modules/cellCamera.h"
#include "Emu/Cell/Modules/cellGem.h"
#include "ps_move_tracker.h"

#include <cmath>

#ifdef HAVE_OPENCV
#include <opencv2/photo.hpp>
#endif

LOG_CHANNEL(ps_move);

namespace gem
{
	extern bool convert_image_format(CellCameraFormat input_format, CellGemVideoConvertFormatEnum output_format,
	                                 const std::vector<u8>& video_data_in, u32 width, u32 height,
	                                 u8* video_data_out, u32 video_data_out_size, std::string_view caller);
}

template <bool DiagnosticsEnabled>
ps_move_tracker<DiagnosticsEnabled>::ps_move_tracker()
{
	init_workers();
}

template <bool DiagnosticsEnabled>
ps_move_tracker<DiagnosticsEnabled>::~ps_move_tracker()
{
	for (u32 index = 0; index < CELL_GEM_MAX_NUM; index++)
	{
		if (auto& worker = m_workers[index])
		{
			auto& thread = *worker;
			thread = thread_state::aborting;
			m_wake_up_workers[index].release(1);
			m_wake_up_workers[index].notify_one();
			thread();
		}
	}
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::set_valid(ps_move_info& info, u32 index, bool valid)
{
	u32& fail_count = ::at32(m_fail_count, index);

	if (info.valid && !valid)
	{
		// Ignore a couple of untracked frames. This reduces noise.
		if (++fail_count >= 3)
		{
			info.valid = valid;
		}

		return;
	}

	info.valid = valid;
	fail_count = 0; // Reset fail count
};

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::set_image_data(const void* buf, u64 size, u32 width, u32 height, s32 format)
{
	if (!buf || !size || !width || !height || !format)
	{
		ps_move.error("ps_move_tracker got unexpected input: buf=*0x%x, size=%d, width=%d, height=%d, format=%d", buf, size, width, height, format);
		return;
	}

	m_width = width;
	m_height = height;
	m_format = format;
	m_image_data.resize(size);

	std::memcpy(m_image_data.data(), buf, size);
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::set_active(u32 index, bool active)
{
	ps_move_config& config = ::at32(m_config, index);
	config.active = active;
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::ps_move_config::calculate_values()
{
	// The hue is a "circle", so we use modulo 360.
	max_hue = (hue + hue_threshold) % 360;
	min_hue = hue - hue_threshold;

	if (min_hue < 0)
	{
		min_hue += 360;
	}
	else
	{
		min_hue %= 360;
	}

	saturation_threshold = saturation_threshold_u8 / 255.0f;
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::set_hue(u32 index, u16 hue)
{
	ps_move_config& config = ::at32(m_config, index);
	config.hue = hue;
	config.calculate_values();
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::set_hue_threshold(u32 index, u16 threshold)
{
	ps_move_config& config = ::at32(m_config, index);
	config.hue_threshold = threshold;
	config.calculate_values();
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::set_saturation_threshold(u32 index, u16 threshold)
{
	ps_move_config& config = ::at32(m_config, index);
	config.saturation_threshold_u8 = threshold;
	config.calculate_values();
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::init_workers()
{
	for (u32 index = 0; index < CELL_GEM_MAX_NUM; index++)
	{
		if (m_workers[index])
		{
			continue;
		}

		m_workers[index] = std::make_unique<named_thread<std::function<void()>>>(fmt::format("PS Move Worker %d", index), [this, index]()
		{
			while (thread_ctrl::state() != thread_state::aborting)
			{
				// Notify that all work is done
				m_workers_finished[index].release(1);
				m_workers_finished[index].notify_one();

				// Wait for work
				m_wake_up_workers[index].wait(0);
				m_wake_up_workers[index].release(0);

				if (thread_ctrl::state() == thread_state::aborting)
				{
					break;
				}

				// Find contours
				ps_move_info& info = m_info[index];
				ps_move_info new_info = info;
				process_contours(new_info, index);

				if (new_info.valid)
				{
					info = std::move(new_info);
				}
				else
				{
					info.valid = false;
				}
			}

			// Notify one last time that all work is done
			m_workers_finished[index].release(1);
			m_workers_finished[index].notify_one();
		});
	}
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::process_image()
{
	// Convert image to RGBA
	convert_image(CELL_GEM_RGBA_640x480);

	// Calculate hues
	process_hues();

	// Get active devices
	std::vector<u32> active_devices;
	for (u32 index = 0; index < CELL_GEM_MAX_NUM; index++)
	{
		ps_move_config& config = m_config[index];

		if (config.active)
		{
			active_devices.push_back(index);
		}
		else
		{
			ps_move_info& info = m_info[index];
			info.valid = false;
			m_fail_count[index] = 0;
		}
	}

	// Find contours on worker threads
	for (u32 index : active_devices)
	{
		// Clear old state
		m_workers_finished[index].release(0);

		// Wake up worker
		m_wake_up_workers[index].release(1);
		m_wake_up_workers[index].notify_one();
	}

	// Wait for worker threads (a couple of seconds so we don't deadlock)
	for (u32 index : active_devices)
	{
		// Wait for worker
		m_workers_finished[index].wait(0, atomic_wait_timeout{5000000000});
		m_workers_finished[index].release(0);
	}
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::convert_image(s32 output_format)
{
	const u32 width = m_width;
	const u32 height = m_height;
	const u32 size = height * width;

	m_image_rgba.resize(size * 4);
	m_image_hsv.resize(size * 3);

	for (u32 index = 0; index < CELL_GEM_MAX_NUM; index++)
	{
		m_image_binary[index].resize(size);
	}

	if (gem::convert_image_format(CellCameraFormat{m_format}, CellGemVideoConvertFormatEnum{output_format}, m_image_data, width, height, m_image_rgba.data(), ::size32(m_image_rgba), "gemTracker"))
	{
		ps_move.trace("Converted video frame of format %s to %s", CellCameraFormat{m_format}, CellGemVideoConvertFormatEnum{output_format});
	}

	if constexpr (DiagnosticsEnabled)
	{
		m_image_gray.resize(size);
		m_image_rgba_contours.resize(size * 4);

		std::memcpy(m_image_rgba_contours.data(), m_image_rgba.data(), m_image_rgba.size());
	}
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::process_hues()
{
	const u32 width = m_width;
	const u32 height = m_height;

	if constexpr (DiagnosticsEnabled)
	{
		std::fill(m_hues.begin(), m_hues.end(), 0);
	}

	u8* gray = nullptr;

	for (u32 y = 0; y < height; y++)
	{
		const u8* rgba = &m_image_rgba[y * width * 4];
		u8* hsv = &m_image_hsv[y * width * 3];

		if constexpr (DiagnosticsEnabled)
		{
			gray = &m_image_gray[y * width];
		}

		for (u32 x = 0; x < width; x++, rgba += 4, hsv += 3)
		{
			const f32 r = rgba[0] / 255.0f;
			const f32 g = rgba[1] / 255.0f;
			const f32 b = rgba[2] / 255.0f;
			const auto [hue, saturation, value] = rgb_to_hsv(r, g, b);

			hsv[0] = static_cast<u8>(hue / 2);
			hsv[1] = static_cast<u8>(saturation * 255.0f);
			hsv[2] = static_cast<u8>(value * 255.0f);

			if constexpr (DiagnosticsEnabled)
			{
				*gray++ = static_cast<u8>(std::clamp((0.299f * r + 0.587f * g + 0.114f * b) * 255.0f, 0.0f, 255.0f));
				++m_hues[hue];
			}
		}
	}
}

#ifdef HAVE_OPENCV
static bool is_circular_contour(const std::vector<cv::Point>& contour, f32& area)
{
	std::vector<cv::Point> approx;
	cv::approxPolyDP(contour, approx, 0.01 * cv::arcLength(contour, true), true);
	if (approx.size() < 8ULL) return false;

	area = static_cast<f32>(cv::contourArea(contour));
	if (area < 30.0f) return false;

	cv::Point2f center;
	f32 radius;
	cv::minEnclosingCircle(contour, center, radius);
	if (radius < 5.0f) return false;

	return true;
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::draw_sphere_size_range(f32 result_radius)
{
	if constexpr (!DiagnosticsEnabled) return;
	if (!m_draw_overlays) return;

	// Map memory
	cv::Mat rgba(cv::Size(m_width, m_height), CV_8UC4, m_image_rgba_contours.data(), 0);

	// Draw result, min and max radius
	const f32 min_radius = m_min_radius * m_width;
	const f32 max_radius = m_max_radius * m_width;
	const f32 min_radius_clamped = std::max(0.0f, std::min(min_radius, max_radius));
	const cv::Point2f center = cv::Point2f(m_width - 1 - max_radius, max_radius);
	if (result_radius > 0.0f)
	{
		cv::circle(rgba, center, static_cast<int>(result_radius), cv::Scalar(255, 0, 0, 255), cv::FILLED);
	}
	if (min_radius_clamped > 0.0f && min_radius_clamped <= max_radius)
	{
		cv::circle(rgba, center, static_cast<int>(min_radius_clamped), cv::Scalar(0, 0, 0, 255), cv::FILLED);
	}
	if (max_radius > min_radius_clamped)
	{
		cv::circle(rgba, center, static_cast<int>(max_radius), cv::Scalar(0, 0, 0, 255), 1);
	}
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::process_contours(ps_move_info& info, u32 index)
{
	const ps_move_config& config = ::at32(m_config, index);
	const std::vector<u8>& image_hsv = m_image_hsv;
	std::vector<u8>& image_binary = ::at32(m_image_binary, index);

	const u32 width = m_width;
	const u32 height = m_height;
	const bool wrapped_hue = config.min_hue > config.max_hue; // e.g. min=355, max=5 (red)

	info.x_max = width;
	info.y_max = height;

	// Map memory
	cv::Mat binary(cv::Size(width, height), CV_8UC1, image_binary.data(), 0);

	// Filter image
	for (u32 y = 0; y < height; y++)
	{
		const u8* src = &image_hsv[y * width * 3];
		u8* dst = &image_binary[y * width];

		for (u32 x = 0; x < width; x++, src += 3)
		{
			const u16 hue = src[0] * 2;
			const u8 saturation = src[1];
			const u8 value = src[2];

			// Simply drop dark and colorless pixels as well as pixels that don't match our hue
			if ((wrapped_hue ? (hue < config.min_hue && hue > config.max_hue) : (hue < config.min_hue || hue > config.max_hue)) ||
				saturation < config.saturation_threshold_u8 || saturation > 200 ||
				value < 150 || value > 255)
			{
				dst[x] = 0;
			}
			else
			{
				dst[x] = 255;
			}
		}
	}

	// Remove all small outer contours
	if (m_filter_small_contours)
	{
		std::vector<std::vector<cv::Point>> contours;
		cv::findContours(binary, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
		for (auto it = contours.begin(); it != contours.end();)
		{
			f32 area;
			if (is_circular_contour(*it, area))
			{
				it = contours.erase(it);
				continue;
			}
			it++;
		}
		if (!contours.empty())
		{
			cv::drawContours(binary, contours, -1, 0, cv::FILLED);
		}
	}

	// Find best contour
	std::vector<std::vector<cv::Point>> all_contours;
	cv::findContours(binary, all_contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	if (all_contours.empty())
	{
		set_valid(info, index, false);

		if constexpr (DiagnosticsEnabled)
		{
			draw_sphere_size_range(0.0f);
		}
		return;
	}

	std::vector<std::vector<cv::Point>> contours;
	contours.reserve(all_contours.size());

	std::vector<cv::Point2f> centers;
	centers.reserve(all_contours.size());

	std::vector<f32> radii;
	radii.reserve(all_contours.size());

	const f32 min_radius = m_min_radius * width;
	const f32 max_radius = m_max_radius * width;

	usz best_index = umax;
	f32 best_area = 0.0f;
	for (usz i = 0; i < all_contours.size(); i++)
	{
		const std::vector<cv::Point>& contour = all_contours[i];
		f32 area;
		if (!is_circular_contour(contour, area))
			continue;

		// Get center and radius
		cv::Point2f center;
		f32 radius;
		cv::minEnclosingCircle(contour, center, radius);

		// Filter radius
		if (radius < min_radius || radius > max_radius)
			continue;

		contours.push_back(std::move(all_contours[i]));
		centers.push_back(std::move(center));
		radii.push_back(std::move(radius));

		if (area > best_area)
		{
			best_area = area;
			best_index = contours.size() - 1;
		}
	}

	if (best_index == umax)
	{
		set_valid(info, index, false);

		if constexpr (DiagnosticsEnabled)
		{
			draw_sphere_size_range(0.0f);
		}
		return;
	}

	// Calculate distance from sphere to camera
	const f32 sphere_radius_pixels = radii[best_index];
	constexpr f32 focal_length_mm = 3.5f; // Based on common webcam specs
	constexpr f32 sensor_width_mm = 3.6f; // Based on common webcam specs
	const f32 image_width_pixels = static_cast<f32>(width);
	const f32 focal_length_pixels = (focal_length_mm * image_width_pixels) / sensor_width_mm;
	const f32 distance_mm = (focal_length_pixels * CELL_GEM_SPHERE_RADIUS_MM) / sphere_radius_pixels;

	// Set results
	set_valid(info, index, true);

	const u32 x_pos = std::clamp(static_cast<u32>(centers[best_index].x), 0u, width);
	const u32 y_pos = std::clamp(static_cast<u32>(centers[best_index].y), 0u, height);

	// Only set new values if the new shape and position are relatively similar to the old ones.
	const auto distance_travelled = [](int x1, int y1, int x2, int y2)
	{
		return std::sqrt(std::pow(x2 - x1, 2) + pow(y2 - y1, 2));
	};
	const bool shape_matches = std::abs(info.radius - sphere_radius_pixels) < (info.radius * 2) &&
	                           distance_travelled(info.x_pos, info.y_pos, x_pos, y_pos) < (info.radius * 8);

	if (shape_matches || ++m_shape_fail_count[index] >= 3)
	{
		info.distance_mm = distance_mm;
		info.radius = sphere_radius_pixels;
		info.x_pos = x_pos;
		info.y_pos = y_pos;

		m_shape_fail_count[index] = 0; // Reset fail count
	}

	if constexpr (!DiagnosticsEnabled)
		return;

	if (!m_draw_contours && !m_draw_overlays) [[likely]]
		return;

	draw_sphere_size_range(info.radius);

	// Map memory
	cv::Mat rgba(cv::Size(width, height), CV_8UC4, m_image_rgba_contours.data(), 0);

	if (!m_show_all_contours) [[likely]]
	{
		std::vector<cv::Point> contour = std::move(contours[best_index]);
		contours = { std::move(contour) };
		centers = { centers[best_index] };
		radii = { radii[best_index] };
	}

	static const cv::Scalar contour_color(255, 0, 0, 255);
	static const cv::Scalar circle_color(0, 255, 0, 255);
	static const cv::Scalar center_color(0, 0, 255, 255);

	// Draw contours
	if (m_draw_contours)
	{
		cv::drawContours(rgba, contours, -1, contour_color, cv::FILLED);
	}

	// Draw overlays
	if (m_draw_overlays)
	{
		for (usz i = 0; i < centers.size(); i++)
		{
			const cv::Point2f& center = centers[i];
			const f32 radius = radii[i];

			cv::circle(rgba, center, static_cast<int>(radius), circle_color, 1);
			cv::line(rgba, center + cv::Point2f(-2.0f, 0.0f), center + cv::Point2f(2.0f, 0.0f), center_color, 1);
			cv::line(rgba, center + cv::Point2f(0.0f, -2.0f), center + cv::Point2f(0.0f, 2.0f), center_color, 1);
		}
	}
}
#else
template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::process_contours(ps_move_info& info, u32 index)
{
	ensure(index < m_config.size());

	const u32 width = m_width;
	const u32 height = m_height;

	info.valid = false;
	info.x_max = width;
	info.y_max = height;

	ps_move.error("The tracker is not implemented for this operating system.");
}
#endif

template <bool DiagnosticsEnabled>
std::tuple<u8, u8, u8> ps_move_tracker<DiagnosticsEnabled>::hsv_to_rgb(u16 hue, f32 saturation, f32 value)
{
	const f32 h = hue / 60.0f;
	const f32 chroma = value * saturation;
	const f32 x = chroma * (1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f));
	const f32 m = value - chroma;

	f32 r = 0.0f;
	f32 g = 0.0f;
	f32 b = 0.0f;

	switch (static_cast<int>(std::ceil(h)))
	{
	case 0:
	case 1:
		r = chroma + m;
		g = x + m;
		b = 0 + m;
		break;
	case 2:
		r = x + m;
		g = chroma + m;
		b = 0 + m;
		break;
	case 3:
		r = 0 + m;
		g = chroma + m;
		b = x + m;
		break;
	case 4:
		r = 0 + m;
		g = x + m;
		b = chroma + m;
		break;
	case 5:
		r = x + m;
		g = 0 + m;
		b = chroma + m;
		break;
	case 6:
		r = chroma + m;
		g = 0 + m;
		b = x + m;
		break;
	default:
		break;
	}

	const u8 red   = static_cast<u8>(std::clamp(std::round(r * 255.0f), 0.0f, 255.0f));
	const u8 green = static_cast<u8>(std::clamp(std::round(g * 255.0f), 0.0f, 255.0f));
	const u8 blue  = static_cast<u8>(std::clamp(std::round(b * 255.0f), 0.0f, 255.0f));

	return { red, green, blue };
}

template <bool DiagnosticsEnabled>
std::tuple<s16, f32, f32> ps_move_tracker<DiagnosticsEnabled>::rgb_to_hsv(f32 r, f32 g, f32 b)
{
	const f32 cmax = std::max({r, g, b}); // V (of HSV)
	const f32 cmin = std::min({r, g, b});
	const f32 delta = cmax - cmin;
	const f32 saturation = cmax ? (delta / cmax) : 0.0f; // S (of HSV)

	s16 hue; // H (of HSV)

	if (!delta)
	{
		hue = 0;
	}
	else if (cmax == r)
	{
		hue = (static_cast<s16>(60.0f * (g - b) / delta) + 360) % 360;
	}
	else if (cmax == g)
	{
		hue = (static_cast<s16>(60.0f * (b - r) / delta) + 120 + 360) % 360;
	}
	else
	{
		hue = (static_cast<s16>(60.0f * (r - g) / delta) + 240 + 360) % 360;
	}

	return { hue, saturation, cmax };
}

template class ps_move_tracker<false>;
template class ps_move_tracker<true>;
