#include "stdafx.h"
#include "Emu/Cell/Modules/cellCamera.h"
#include "Emu/Cell/Modules/cellGem.h"
#include "ps_move_tracker.h"

#include <cmath>
#include <numbers>

LOG_CHANNEL(ps_move);

namespace gem
{
	extern bool convert_image_format(CellCameraFormat input_format, const CellGemVideoConvertAttribute& vc,
	                                 const std::vector<u8>& video_data_in, u32 width, u32 height,
	                                 u8* video_data_out, u32 video_data_out_size, u8* buffer_memory,
	                                 std::string_view caller);
}

template <bool DiagnosticsEnabled>
ps_move_tracker<DiagnosticsEnabled>::ps_move_tracker()
{
	m_vc_attr.alpha = 255;
	m_vc_attr.gain = 1.0f;
	m_vc_attr.red_gain = 1.0f;
	m_vc_attr.green_gain = 1.0f;
	m_vc_attr.blue_gain = 1.0f;
}

template <bool DiagnosticsEnabled>
ps_move_tracker<DiagnosticsEnabled>::~ps_move_tracker()
{
	for (u32 index = 0; index < CELL_GEM_MAX_NUM; index++)
	{
		join_worker(index);
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
}

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
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::set_hue(u32 index, u16 hue)
{
	ps_move_config& config = ::at32(m_config, index);
	config.hue = hue;
	config.calculate_values();
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::set_hue_threshold(u32 index, u8 threshold)
{
	ps_move_config& config = ::at32(m_config, index);
	config.hue_threshold = threshold;
	config.calculate_values();
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::set_saturation_threshold(u32 index, u8 threshold)
{
	ps_move_config& config = ::at32(m_config, index);
	config.saturation_threshold_u8 = threshold;
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::init_worker(u32 index)
{
	auto& worker = ::at32(m_workers, index);
	if (worker) return;

	worker = std::make_unique<named_thread<std::function<void()>>>(fmt::format("PS Move Worker %d", index), [this, index]()
	{
		while (thread_ctrl::state() != thread_state::aborting)
		{
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

			// Notify that all work is done
			m_workers_finished[index].release(1);
			m_workers_finished[index].notify_one();
		}

		// Notify one last time that all work is done
		m_workers_finished[index].release(1);
		m_workers_finished[index].notify_one();
	});
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::join_worker(u32 index)
{
	auto& worker = ::at32(m_workers, index);
	if (!worker) return;

	auto& thread = *worker;
	thread = thread_state::aborting;
	m_wake_up_workers[index].release(1);
	m_wake_up_workers[index].notify_one();
	thread();
	worker.reset();
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
			init_worker(index);
			active_devices.push_back(index);
		}
		else
		{
			join_worker(index);
			ps_move_info& info = m_info[index];
			info.valid = false;
			m_fail_count[index] = 0;
			m_shape_fail_count[index] = 0;
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

	m_vc_attr.output_format = CellGemVideoConvertFormatEnum{output_format};

	if (gem::convert_image_format(CellCameraFormat{m_format}, m_vc_attr, m_image_data, width, height, m_image_rgba.data(), ::size32(m_image_rgba), nullptr, "gemTracker"))
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

static constexpr u32 min_blob_area = 30;
static constexpr f32 min_sphere_radius = 5.0f;

static bool is_circular_blob(const blob_detector::blob& b)
{
	// A disk has a fill ratio of about 1. Squares (0.64), half disks and 2:1 ellipses (0.5) are rejected.
	constexpr f32 min_fill_ratio = 0.7f;

	if (b.area < min_blob_area) return false;
	if (b.radius < min_sphere_radius) return false;
	if (b.fill_ratio() < min_fill_ratio) return false;

	return true;
}

// Checks if the last position is close to the line of movement of a sphere that was blurred into a capsule.
// The sphere usually moves further between two frames than during the exposure, so the last position may be behind the end of the capsule.
static bool is_on_movement_path(const blob_detector::capsule& capsule, f32 last_x, f32 last_y, f32 last_radius)
{
	const f32 dx = last_x - capsule.center_x;
	const f32 dy = last_y - capsule.center_y;
	const f32 axis_distance = std::abs(dx * capsule.dir_x + dy * capsule.dir_y);
	const f32 side_distance = std::abs(dx * capsule.dir_y - dy * capsule.dir_x);

	return side_distance <= last_radius * 2.0f && axis_distance <= capsule.half_length * 4.0f + last_radius * 2.0f;
}

struct sphere_candidate
{
	const blob_detector::blob* blob = nullptr;
	f32 x = 0.0f;
	f32 y = 0.0f;
	f32 radius = 0.0f;
	f32 score = 0.0f;
};

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::draw_sphere_size_range(f32 result_radius)
{
	if constexpr (!DiagnosticsEnabled) return;
	if (!m_draw_overlays) return;

	// Protect the destination image from race conditions
	std::lock_guard lock(m_mutex);

	// Draw result, min and max radius
	const f32 min_radius = m_min_radius * m_width;
	const f32 max_radius = m_max_radius * m_width;
	const f32 min_radius_clamped = std::max(0.0f, std::min(min_radius, max_radius));
	const f32 center_x = m_width - 1 - max_radius;
	const f32 center_y = max_radius;
	if (result_radius > 0.0f)
	{
		rgba_draw::circle(m_image_rgba_contours, m_width, m_height, center_x, center_y, static_cast<s32>(result_radius), { 255, 0, 0, 255 }, true);
	}
	if (min_radius_clamped > 0.0f && min_radius_clamped <= max_radius)
	{
		rgba_draw::circle(m_image_rgba_contours, m_width, m_height, center_x, center_y, static_cast<s32>(min_radius_clamped), { 0, 0, 0, 255 }, true);
	}
	if (max_radius > min_radius_clamped)
	{
		rgba_draw::circle(m_image_rgba_contours, m_width, m_height, center_x, center_y, static_cast<s32>(max_radius), { 0, 0, 0, 255 }, false);
	}
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::filter_image(u32 index)
{
	const u32 width = m_width;
	const u32 height = m_height;

	if (width == 0 || height == 0)
	{
		return;
	}

	const ps_move_config& config = ::at32(m_config, index);
	const std::vector<u8>& image_hsv = m_image_hsv;
	std::vector<u8>& image_binary = ::at32(m_image_binary, index);

	const bool wrapped_hue = config.min_hue > config.max_hue; // e.g. min=355, max=5 (red)
	const bool has_hue_threshold = config.hue_threshold < 180; // with hue threshold 180 min and max are the same value

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
			if ((has_hue_threshold && (wrapped_hue ? (hue < config.min_hue && hue > config.max_hue) : (hue < config.min_hue || hue > config.max_hue))) ||
				saturation < config.saturation_threshold_u8 ||
				value < 150)
			{
				dst[x] = 0;
			}
			else
			{
				dst[x] = 255;
			}
		}
	}
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::process_contours(ps_move_info& info, u32 index)
{
	const u32 width = m_width;
	const u32 height = m_height;

	if (width == 0 || height == 0)
	{
		set_valid(info, index, false);
		return;
	}

	// Filter image
	filter_image(index);

	std::vector<u8>& image_binary = ::at32(m_image_binary, index);
	blob_detector& detector = ::at32(m_blob_detectors, index);

	// Remove noise and thin lines. This also separates the sphere from background objects that only touch it with a thin bridge.
	detector.open(image_binary, width, height);

	// Find all blobs. Holes are filled, so overexposed sphere centers don't split the sphere into a ring.
	detector.detect(image_binary, width, height, true, min_blob_area);

	const f32 min_radius = m_min_radius * width;
	const f32 max_radius = m_max_radius * width;

	// Prefer candidates near the last known position
	const bool has_last_position = info.valid && info.radius > 0.0f;
	const f32 last_x = info.x_pos * width;
	const f32 last_y = info.y_pos * height;
	const f32 search_radius = info.radius * 2.0f;

	// The inscribed circle has to be similar to the last sphere
	const f32 min_inscribed_radius = std::max(min_sphere_radius, info.radius * 0.5f);
	const f32 max_inscribed_radius = info.radius * 1.5f;

	std::vector<sphere_candidate> candidates;
	sphere_candidate best_candidate {};

	for (const blob_detector::blob& b : detector.blobs())
	{
		sphere_candidate candidate { .blob = &b };
		f32 weight = 1.0f;

		if (is_circular_blob(b))
		{
			candidate.x = b.center_x;
			candidate.y = b.center_y;
			candidate.radius = b.radius;
		}
		else if (has_last_position &&
		         b.area >= min_blob_area &&
		         b.max_x - b.min_x + 1 >= 2.0f * min_inscribed_radius &&
		         b.max_y - b.min_y + 1 >= 2.0f * min_inscribed_radius &&
		         b.area >= std::numbers::pi_v<f32> * min_inscribed_radius * min_inscribed_radius)
		{
			// The blob is not round. This is only used to keep tracking a sphere of similar size. A new sphere has to be found as a real circle.
			// The cheap checks above make sure that we only calculate the inscribed circle if the blob is large enough to contain it.
			const blob_detector::inscribed_circle circle = detector.find_inscribed_circle(b);
			const bool similar_size = circle.radius >= min_inscribed_radius && circle.radius <= max_inscribed_radius;
			blob_detector::capsule capsule {};

			if (similar_size && blob_detector::fit_capsule(b, circle, capsule) && is_on_movement_path(capsule, last_x, last_y, info.radius))
			{
				// The sphere is blurred by fast movement. The center of the capsule is the average position during the exposure.
				candidate.x = capsule.center_x;
				candidate.y = capsule.center_y;
				candidate.radius = capsule.radius;
				weight = 0.75f;
			}
			else if (similar_size && detector.is_round_bulge(b, circle))
			{
				// The blob contains a round bulge. This happens if the sphere touches a background object with a similar color.
				// Use the largest circle that fits inside the blob, but prefer real circles.
				candidate.x = circle.center_x;
				candidate.y = circle.center_y;
				candidate.radius = circle.radius;
				weight = 0.5f;
			}
		}

		if (candidate.radius <= 0.0f)
		{
			// Remove all non-circular blobs from the binary image
			if constexpr (DiagnosticsEnabled)
			{
				if (m_filter_small_contours)
				{
					detector.fill_blob(image_binary, b, 0);
				}
			}
			continue;
		}

		// Filter radius
		if (candidate.radius < min_radius || candidate.radius > max_radius)
			continue;

		// Larger spheres are more likely to be the real one, but the distance to the last position is more important
		candidate.score = weight * candidate.radius * candidate.radius;

		if (has_last_position)
		{
			const f32 dx = (candidate.x - last_x) / search_radius;
			const f32 dy = (candidate.y - last_y) / search_radius;
			candidate.score /= 1.0f + dx * dx + dy * dy;
		}

		if constexpr (DiagnosticsEnabled)
		{
			candidates.push_back(candidate);
		}

		if (!best_candidate.blob || candidate.score > best_candidate.score)
		{
			best_candidate = candidate;
		}
	}

	if (!best_candidate.blob)
	{
		set_valid(info, index, false);

		if constexpr (DiagnosticsEnabled)
		{
			draw_sphere_size_range(0.0f);
		}
		return;
	}

	// Calculate distance from sphere to camera
	const f32 sphere_radius_pixels = best_candidate.radius;
	constexpr f32 focal_length_mm = 3.5f; // Based on common webcam specs
	constexpr f32 sensor_width_mm = 3.6f; // Based on common webcam specs
	const f32 image_width_pixels = static_cast<f32>(width);
	const f32 focal_length_pixels = (focal_length_mm * image_width_pixels) / sensor_width_mm;
	const f32 distance_mm = (focal_length_pixels * CELL_GEM_SPHERE_RADIUS_MM) / sphere_radius_pixels;

	const u32 x_pos = std::clamp(static_cast<u32>(best_candidate.x), 0u, width);
	const u32 y_pos = std::clamp(static_cast<u32>(best_candidate.y), 0u, height);

	// Only set new values if the new shape and position are relatively similar to the old ones.
	const auto distance_squared = [](int x1, int y1, int x2, int y2)
	{
		const int dx = x2 - x1;
		const int dy = y2 - y1;
		return dx * dx + dy * dy;
	};
	const f32 max_distance = info.radius * 8.0f;
	const f32 max_distance_squared = max_distance * max_distance;
	const bool shape_matches = std::abs(info.radius - sphere_radius_pixels) < (info.radius * 2) &&
	                           distance_squared(static_cast<s32>(info.x_pos * width), static_cast<s32>(info.y_pos * height), x_pos, y_pos) < max_distance_squared;
	const bool reacquired = !info.valid || info.radius <= 0.0f;

	if (reacquired || shape_matches || ++m_shape_fail_count[index] >= 3)
	{
		info.distance_mm = distance_mm;
		info.radius = sphere_radius_pixels;
		info.x_pos = std::clamp(x_pos / static_cast<f32>(width), 0.0f, 1.0f);
		info.y_pos = std::clamp(y_pos / static_cast<f32>(height), 0.0f, 1.0f);

		m_shape_fail_count[index] = 0; // Reset fail count
	}

	set_valid(info, index, true);

	if constexpr (!DiagnosticsEnabled)
		return;

	if (!m_draw_contours && !m_draw_overlays) [[likely]]
		return;

	draw_sphere_size_range(info.radius);

	// Protect the destination image from race conditions
	std::lock_guard lock(m_mutex);

	if (!m_show_all_contours) [[likely]]
	{
		candidates = { best_candidate };
	}

	static constexpr std::array<u8, 4> contour_color = { 255, 0, 0, 255 };
	static constexpr std::array<u8, 4> circle_color = { 0, 255, 0, 255 };
	static constexpr std::array<u8, 4> center_color = { 0, 0, 255, 255 };

	// Draw contours
	if (m_draw_contours)
	{
		for (const sphere_candidate& candidate : candidates)
		{
			detector.fill_blob_rgba(m_image_rgba_contours, *candidate.blob, contour_color);
		}
	}

	// Draw overlays
	if (m_draw_overlays)
	{
		for (const sphere_candidate& candidate : candidates)
		{
			const s32 x = static_cast<s32>(std::lround(candidate.x));
			const s32 y = static_cast<s32>(std::lround(candidate.y));

			rgba_draw::circle(m_image_rgba_contours, width, height, candidate.x, candidate.y, static_cast<s32>(candidate.radius), circle_color, false);
			rgba_draw::line(m_image_rgba_contours, width, height, x - 2, y, x + 2, y, center_color);
			rgba_draw::line(m_image_rgba_contours, width, height, x, y - 2, x, y + 2, center_color);
		}
	}
}

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
