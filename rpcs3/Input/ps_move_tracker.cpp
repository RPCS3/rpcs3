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

	if (config.hue == hue)
		return;

	::at32(m_sphere_colors, index).valid = false;
	config.hue = hue;
	config.calculate_values();
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::set_hue_threshold(u32 index, u8 threshold)
{
	ps_move_config& config = ::at32(m_config, index);

	if (config.hue_threshold == threshold)
		return;

	::at32(m_sphere_colors, index).valid = false;
	config.hue_threshold = threshold;
	config.calculate_values();
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::set_saturation_threshold(u32 index, u8 threshold)
{
	ps_move_config& config = ::at32(m_config, index);

	if (config.saturation_threshold_u8 == threshold)
		return;

	::at32(m_sphere_colors, index).valid = false;
	config.saturation_threshold_u8 = threshold;
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::set_value_threshold(u32 index, u8 threshold)
{
	ps_move_config& config = ::at32(m_config, index);

	if (config.value_threshold_u8 == threshold)
		return;

	::at32(m_sphere_colors, index).valid = false;
	config.value_threshold_u8 = threshold;
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
			m_sphere_colors[index].valid = false;
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
		m_image_overexposed[index].resize(size);
		m_image_enclosed[index].resize(size);
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

// Pixels that are at least this bright are considered overexposed. The center of a bright sphere is white, so it has no usable color.
static constexpr u8 min_overexposed_value = 230;

// Round blobs that are stretched at least this much (major / minor axis) are considered blurred
static constexpr f32 min_blurred_elongation = 1.15f;

// Allowed differences to the measured sphere color (see also sphere_hue_range)
static constexpr s32 sphere_saturation_range = 85;
static constexpr s32 sphere_value_range = 85;

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

// Why a blob was or wasn't chosen. Only used for diagnostics. Higher values are drawn on top.
enum class candidate_status
{
	rejected_shape,  // Not round and no fallback matched
	rejected_radius, // Round, but outside of the configured radius range
	candidate,       // Valid candidate, but another one had a better score
	best,            // The chosen sphere
};

struct sphere_candidate
{
	const blob_detector::blob* blob = nullptr;
	f32 x = 0.0f;
	f32 y = 0.0f;
	f32 radius = 0.0f;
	f32 score = 0.0f;
	bool is_circle = false;  // The blob itself is round
	bool is_blurred = false; // The sphere is blurred by movement, so its radius is not reliable
	candidate_status status = candidate_status::rejected_shape;
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
bool ps_move_tracker<DiagnosticsEnabled>::filter_image(u32 index, bool use_sphere_color)
{
	const u32 width = m_width;
	const u32 height = m_height;

	if (width == 0 || height == 0)
	{
		return false;
	}

	const ps_move_config& config = ::at32(m_config, index);
	const ps_move_sphere_color& color = ::at32(m_sphere_colors, index);
	const std::vector<u8>& image_hsv = m_image_hsv;
	std::vector<u8>& image_binary = ::at32(m_image_binary, index);

	const bool wrapped_hue = config.min_hue > config.max_hue; // e.g. min=355, max=5 (red)
	const bool has_hue_threshold = config.hue_threshold < 180; // with hue threshold 180 min and max are the same value

	// Precalculate which hues are allowed. The HSV image stores the hue divided by 2.
	thread_local std::array<bool, 256> hue_allowed {};

	for (u32 i = 0; i < hue_allowed.size(); i++)
	{
		const u16 hue = static_cast<u16>(i * 2);
		bool allowed = true;

		if (!has_hue_threshold)
		{
			// Any hue is allowed
		}
		else
		{
			allowed = wrapped_hue ? (hue >= config.min_hue || hue <= config.max_hue) : (hue >= config.min_hue && hue <= config.max_hue);
		}

		if (allowed && use_sphere_color)
		{
			allowed = std::abs(std::remainder(hue - color.hue, 360.0f)) <= sphere_hue_range;
		}

		hue_allowed[i] = allowed;
	}

	// Drop dark and colorless pixels
	s32 min_saturation = config.saturation_threshold_u8;
	s32 max_saturation = 255;
	s32 min_value = config.value_threshold_u8;
	s32 max_value = 255;

	// The measured sphere color can only make the ranges smaller
	if (use_sphere_color)
	{
		min_saturation = std::max(min_saturation, static_cast<s32>(color.saturation) - sphere_saturation_range);
		max_saturation = std::min(max_saturation, static_cast<s32>(color.saturation) + sphere_saturation_range);
		min_value = std::max(min_value, static_cast<s32>(color.value) - sphere_value_range);
		max_value = std::min(max_value, static_cast<s32>(color.value) + sphere_value_range);
	}

	std::vector<u8>& image_overexposed = ::at32(m_image_overexposed, index);
	bool has_overexposed_pixels = false;

	for (u32 y = 0; y < height; y++)
	{
		const u8* src = &image_hsv[y * width * 3];
		u8* dst = &image_binary[y * width];
		u8* overexposed = &image_overexposed[y * width];

		for (u32 x = 0; x < width; x++, src += 3)
		{
			const u8 hue = src[0];
			const u8 saturation = src[1];
			const u8 value = src[2];

			const bool matches = hue_allowed[hue] &&
				saturation >= min_saturation && saturation <= max_saturation &&
				value >= min_value && value <= max_value;

			const bool is_overexposed = !matches && value >= min_overexposed_value;

			dst[x] = matches ? 255 : 0;
			overexposed[x] = is_overexposed ? 255 : 0;
			has_overexposed_pixels |= is_overexposed;
		}
	}

	return has_overexposed_pixels;
}

template <bool DiagnosticsEnabled>
void ps_move_tracker<DiagnosticsEnabled>::update_sphere_color(u32 index, const blob_detector::blob& b)
{
	ps_move_sphere_color& color = ::at32(m_sphere_colors, index);
	std::chrono::steady_clock::time_point& last_update = ::at32(m_sphere_color_updates, index);

	// Don't update the color too often. This prevents it from slowly drifting towards the background.
	const auto now = std::chrono::steady_clock::now();
	if (color.valid && now - last_update < std::chrono::seconds(1))
	{
		return;
	}

	const blob_detector& detector = ::at32(m_blob_detectors, index);
	const std::vector<u32>& labels = detector.labels();
	const std::vector<u8>& image_binary = ::at32(m_image_binary, index);
	const std::vector<u8>& image_overexposed = ::at32(m_image_overexposed, index);
	const u32 width = m_width;

	if (labels.size() != image_binary.size() || image_overexposed.size() != image_binary.size() || m_image_hsv.size() != image_binary.size() * 3 || b.max_x >= width || b.max_y >= m_height)
	{
		return;
	}

	// Average the color of all pixels that passed the filter. Filled holes and overexposed pixels (e.g. white centers) are ignored.
	// The hue is circular, so we average it as a direction.
	f64 sum_cos = 0.0;
	f64 sum_sin = 0.0;
	f64 sum_saturation = 0.0;
	f64 sum_value = 0.0;
	u32 count = 0;

	for (u32 y = b.min_y; y <= b.max_y; y++)
	{
		for (u32 x = b.min_x; x <= b.max_x; x++)
		{
			const u32 pixel = y * width + x;

			if (labels[pixel] != b.label || !image_binary[pixel] || image_overexposed[pixel])
			{
				continue;
			}

			const u8* hsv = &m_image_hsv[pixel * 3];
			const f64 angle = hsv[0] * 2.0 * std::numbers::pi / 180.0;

			sum_cos += std::cos(angle);
			sum_sin += std::sin(angle);
			sum_saturation += hsv[1];
			sum_value += hsv[2];
			count++;
		}
	}

	if (count < min_blob_area)
	{
		return;
	}

	f32 hue = static_cast<f32>(std::atan2(sum_sin, sum_cos) * 180.0 / std::numbers::pi);
	if (hue < 0.0f) hue += 360.0f;

	const f32 saturation = static_cast<f32>(sum_saturation / count);
	const f32 value = static_cast<f32>(sum_value / count);

	if (!color.valid)
	{
		color.hue = hue;
		color.saturation = saturation;
		color.value = value;
		color.valid = true;
	}
	else
	{
		// Blend the new color into the old one. Use the shortest way around the hue circle.
		color.hue += std::remainder(hue - color.hue, 360.0f) * 0.5f;
		if (color.hue < 0.0f) color.hue += 360.0f;
		if (color.hue >= 360.0f) color.hue -= 360.0f;

		color.saturation = (color.saturation + saturation) * 0.5f;
		color.value = (color.value + value) * 0.5f;
	}

	last_update = now;
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

	// Only use the measured sphere color while the sphere is tracked. A lost sphere has to be found with the configured color.
	ps_move_sphere_color& color = ::at32(m_sphere_colors, index);

	if (!info.valid)
	{
		color.valid = false;
	}

	// Filter image
	const bool has_overexposed_pixels = filter_image(index, color.valid);

	std::vector<u8>& image_binary = ::at32(m_image_binary, index);
	blob_detector& detector = ::at32(m_blob_detectors, index);

	// The center of a bright sphere is overexposed (white), so it doesn't match the color and only a colored ring is left.
	// Add the overexposed pixels that are enclosed by colored pixels. Overexposed areas that are not enclosed (e.g. windows or monitors) are ignored.
	// The colored pixels are grown by one pixel first, so small gaps in the ring don't connect the center to the outside.
	if (has_overexposed_pixels)
	{
		const std::vector<u8>& image_overexposed = ::at32(m_image_overexposed, index);
		std::vector<u8>& image_enclosed = ::at32(m_image_enclosed, index);

		image_enclosed = image_binary;
		detector.dilate(image_enclosed, width, height);
		detector.find_enclosed(image_enclosed, width, height);

		for (usz i = 0; i < image_binary.size(); i++)
		{
			if (image_overexposed[i] && image_enclosed[i])
			{
				image_binary[i] = 255;
			}
		}
	}

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
	const f32 last_radius = info.radius;
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
			candidate.is_circle = true;

			// A slightly blurred sphere can still be round enough. Its enclosing circle includes a part of the blur.
			candidate.is_blurred = has_last_position && b.minor_axis > 0.0f && b.major_axis / b.minor_axis >= min_blurred_elongation;
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
				candidate.is_blurred = true;
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
			if constexpr (DiagnosticsEnabled)
			{
				// Remove all non-circular blobs from the binary image
				if (m_filter_small_contours)
				{
					detector.fill_blob(image_binary, b, 0);
				}

				candidates.push_back(candidate);
			}
			continue;
		}

		// Filter radius
		if (candidate.radius < min_radius || candidate.radius > max_radius)
		{
			if constexpr (DiagnosticsEnabled)
			{
				candidate.status = candidate_status::rejected_radius;
				candidates.push_back(candidate);
			}
			continue;
		}

		candidate.status = candidate_status::candidate;

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

	// Draws the blobs and the circles of the candidates into the diagnostics image
	const auto draw_diagnostics = [&](f32 result_radius)
	{
		if constexpr (DiagnosticsEnabled)
		{
			if (!m_draw_contours && !m_draw_overlays) [[likely]]
				return;

			draw_sphere_size_range(result_radius);

			// Protect the destination image from race conditions
			std::lock_guard lock(m_mutex);

			if (!m_show_all_contours) [[likely]]
			{
				candidates.clear();

				if (best_candidate.blob)
				{
					candidates.push_back(best_candidate);
				}
			}

			for (sphere_candidate& candidate : candidates)
			{
				if (candidate.blob == best_candidate.blob)
				{
					candidate.status = candidate_status::best;
				}
			}

			// Draw the best candidate last, so it is on top
			std::stable_sort(candidates.begin(), candidates.end(), [](const sphere_candidate& l, const sphere_candidate& r) { return l.status < r.status; });

			struct status_colors
			{
				std::array<u8, 4> contour;
				std::array<u8, 4> circle;
			};

			const auto get_colors = [](candidate_status status) -> status_colors
			{
				switch (status)
				{
				case candidate_status::rejected_shape: return { { 128, 128, 128, 255 }, { 128, 128, 128, 255 } };
				case candidate_status::rejected_radius: return { { 255, 255, 0, 255 }, { 255, 255, 128, 255 } };
				case candidate_status::candidate: return { { 255, 140, 0, 255 }, { 255, 200, 100, 255 } };
				case candidate_status::best: break;
				}
				return { { 255, 0, 0, 255 }, { 0, 255, 0, 255 } };
			};

			static constexpr std::array<u8, 4> center_color = { 0, 0, 255, 255 };

			// Draw contours
			if (m_draw_contours)
			{
				for (const sphere_candidate& candidate : candidates)
				{
					detector.fill_blob_rgba(m_image_rgba_contours, *candidate.blob, get_colors(candidate.status).contour);
				}
			}

			// Draw overlays. Blobs that were rejected because of their shape have no circle.
			if (m_draw_overlays)
			{
				for (const sphere_candidate& candidate : candidates)
				{
					if (candidate.radius <= 0.0f)
						continue;

					const s32 x = static_cast<s32>(std::lround(candidate.x));
					const s32 y = static_cast<s32>(std::lround(candidate.y));

					rgba_draw::circle(m_image_rgba_contours, width, height, candidate.x, candidate.y, static_cast<s32>(candidate.radius), get_colors(candidate.status).circle, false);
					rgba_draw::line(m_image_rgba_contours, width, height, x - 2, y, x + 2, y, center_color);
					rgba_draw::line(m_image_rgba_contours, width, height, x, y - 2, x, y + 2, center_color);
				}
			}
		}
	};

	if (!best_candidate.blob)
	{
		set_valid(info, index, false);
		draw_diagnostics(0.0f);
		return;
	}

	f32 sphere_radius_pixels = best_candidate.radius;
	f32 x_pos = std::clamp(best_candidate.x, 0.0f, static_cast<f32>(width));

	// The radius of a blurred sphere is not reliable. The edges of the blur are darker, so the blur looks thinner than the sphere,
	// and the enclosing circle of a slightly blurred sphere includes a part of the blur. Keep the last radius (and distance) instead.
	if (best_candidate.is_blurred && has_last_position)
	{
		sphere_radius_pixels = last_radius;
	}
	f32 y_pos = std::clamp(best_candidate.y, 0.0f, static_cast<f32>(height));

	// Only set new values if the new shape and position are relatively similar to the old ones.
	const f32 move_x = x_pos - last_x;
	const f32 move_y = y_pos - last_y;
	const f32 move_distance = std::sqrt(move_x * move_x + move_y * move_y);
	const bool shape_matches = std::abs(last_radius - sphere_radius_pixels) < (last_radius * 2) &&
	                           move_distance < last_radius * 8.0f;
	const bool reacquired = !info.valid || info.radius <= 0.0f;

	if (reacquired || shape_matches || ++m_shape_fail_count[index] >= 3)
	{
		if (!reacquired)
		{
			// Adaptive smoothing: small changes are smoothed strongly to reduce jitter, large changes are applied almost immediately.
			const f32 radius_factor = std::min(std::abs(sphere_radius_pixels - last_radius) / 4.0f + 0.15f, 1.0f);
			sphere_radius_pixels = last_radius + (sphere_radius_pixels - last_radius) * radius_factor;

			const f32 position_factor = std::min(move_distance / 7.0f + 0.15f, 1.0f);
			x_pos = last_x + move_x * position_factor;
			y_pos = last_y + move_y * position_factor;
		}

		// Calculate distance from sphere to camera
		constexpr f32 focal_length_mm = 3.5f; // Based on common webcam specs
		constexpr f32 sensor_width_mm = 3.6f; // Based on common webcam specs
		const f32 image_width_pixels = static_cast<f32>(width);
		const f32 focal_length_pixels = (focal_length_mm * image_width_pixels) / sensor_width_mm;

		info.distance_mm = (focal_length_pixels * CELL_GEM_SPHERE_RADIUS_MM) / sphere_radius_pixels;
		info.radius = sphere_radius_pixels;
		info.x_pos = std::clamp(x_pos / static_cast<f32>(width), 0.0f, 1.0f);
		info.y_pos = std::clamp(y_pos / static_cast<f32>(height), 0.0f, 1.0f);

		m_shape_fail_count[index] = 0; // Reset fail count
	}

	// Measure the color of the sphere, but only in clean frames: a complete round sphere with a stable size
	if (has_last_position && best_candidate.is_circle && !best_candidate.is_blurred &&
	    best_candidate.blob->fill_ratio() > 0.8f &&
	    best_candidate.radius > 6.0f &&
	    std::abs(best_candidate.radius - last_radius) < last_radius * 0.2f)
	{
		update_sphere_color(index, *best_candidate.blob);
	}

	set_valid(info, index, true);
	draw_diagnostics(info.radius);
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

ps_move_calibration_result ps_move_calibrate_sphere(const std::vector<std::vector<u8>>& images_on, const std::vector<std::vector<u8>>& images_off, u32 width, u32 height, ps_move_tracker_calibration& calibration)
{
	calibration = {};
	ps_move_sphere_color& color = calibration.color;

	const usz size = usz{width} * height;

	if (!size || images_on.empty() || images_on.size() != images_off.size())
	{
		return ps_move_calibration_result::invalid_images;
	}

	for (usz i = 0; i < images_on.size(); i++)
	{
		if (images_on[i].size() != size * 4 || images_off[i].size() != size * 4)
		{
			return ps_move_calibration_result::invalid_images;
		}
	}

	// Only keep pixels that changed in every blink. This removes the background and things that only moved once.
	constexpr s32 min_difference = 30;
	std::vector<u8> mask(size, 255);

	for (usz i = 0; i < images_on.size(); i++)
	{
		const u8* on = images_on[i].data();
		const u8* off = images_off[i].data();

		for (usz pixel = 0; pixel < size; pixel++, on += 4, off += 4)
		{
			if (!mask[pixel])
				continue;

			const s32 difference = std::max({ std::abs(on[0] - off[0]), std::abs(on[1] - off[1]), std::abs(on[2] - off[2]) });

			if (difference < min_difference)
			{
				mask[pixel] = 0;
			}
		}
	}

	// Find the largest blinking area
	blob_detector detector;
	detector.open(mask, width, height);
	detector.detect(mask, width, height, true);

	const blob_detector::blob* sphere = nullptr;

	for (const blob_detector::blob& b : detector.blobs())
	{
		if (!sphere || b.area > sphere->area)
		{
			sphere = &b;
		}
	}

	constexpr u32 min_sphere_area = 50;

	if (!sphere || sphere->area < min_sphere_area)
	{
		return ps_move_calibration_result::not_found;
	}

	if (sphere->fill_ratio() < 0.5f)
	{
		return ps_move_calibration_result::not_round;
	}

	// Average the color of the sphere while the LED was on.
	// The hue is circular, so we average it as a direction. It is weighted by the saturation, so overexposed (white) pixels don't distort it.
	const std::vector<u32>& labels = detector.labels();
	f64 sum_cos = 0.0;
	f64 sum_sin = 0.0;
	f64 sum_saturation = 0.0;
	f64 sum_value = 0.0;
	u64 count = 0;

	for (const std::vector<u8>& image : images_on)
	{
		for (u32 y = sphere->min_y; y <= sphere->max_y; y++)
		{
			for (u32 x = sphere->min_x; x <= sphere->max_x; x++)
			{
				const u32 pixel = y * width + x;

				// Ignore filled holes
				if (labels[pixel] != sphere->label || !mask[pixel])
					continue;

				const u8* rgba = &image[pixel * 4];
				const auto [hue, saturation, value] = ps_move_tracker<false>::rgb_to_hsv(rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f);
				const f64 angle = hue * std::numbers::pi / 180.0;

				sum_cos += saturation * std::cos(angle);
				sum_sin += saturation * std::sin(angle);
				sum_saturation += saturation;
				sum_value += value;
				count++;
			}
		}
	}

	if (!count || (sum_cos == 0.0 && sum_sin == 0.0))
	{
		return ps_move_calibration_result::not_found;
	}

	f32 hue = static_cast<f32>(std::atan2(sum_sin, sum_cos) * 180.0 / std::numbers::pi);
	if (hue < 0.0f) hue += 360.0f;

	// Measure how much the colored part of the sphere varies, so the thresholds include most of it.
	// Pixels with little color (e.g. an overexposed center) are ignored. The tracker fills those holes anyway.
	constexpr u8 min_colored_saturation = 64;
	std::array<u64, 181> hue_distances {};
	std::array<u64, 256> saturations {};
	std::array<u64, 256> values {};
	u64 colored_count = 0;

	for (const std::vector<u8>& image : images_on)
	{
		for (u32 y = sphere->min_y; y <= sphere->max_y; y++)
		{
			for (u32 x = sphere->min_x; x <= sphere->max_x; x++)
			{
				const u32 pixel = y * width + x;

				if (labels[pixel] != sphere->label || !mask[pixel])
					continue;

				const u8* rgba = &image[pixel * 4];
				const auto [pixel_hue, pixel_saturation, pixel_value] = ps_move_tracker<false>::rgb_to_hsv(rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f);
				const u8 saturation = static_cast<u8>(std::clamp(pixel_saturation * 255.0f, 0.0f, 255.0f));

				if (saturation < min_colored_saturation)
					continue;

				const u32 hue_distance = static_cast<u32>(std::lround(std::abs(std::remainder(pixel_hue - hue, 360.0f))));

				hue_distances[std::min(hue_distance, 180u)]++;
				saturations[saturation]++;
				values[static_cast<u8>(std::clamp(pixel_value * 255.0f, 0.0f, 255.0f))]++;
				colored_count++;
			}
		}
	}

	if (colored_count < min_sphere_area)
	{
		return ps_move_calibration_result::no_color;
	}

	// Returns the smallest value that is larger than or equal to the given fraction of all samples
	const auto percentile = [colored_count](const auto& histogram, f64 fraction) -> s32
	{
		const u64 target = static_cast<u64>(std::ceil(colored_count * fraction));
		u64 sum = 0;

		for (usz i = 0; i < histogram.size(); i++)
		{
			sum += histogram[i];

			if (sum >= target)
			{
				return static_cast<s32>(i);
			}
		}

		return static_cast<s32>(histogram.size() - 1);
	};

	// Include 90% of the colored sphere pixels plus a small margin
	calibration.hue_threshold = std::clamp(percentile(hue_distances, 0.9) + 4.0f, 8.0f, 60.0f);
	calibration.min_saturation = static_cast<u8>(std::clamp(percentile(saturations, 0.1) - 10, s32{min_colored_saturation} / 2, 255));
	calibration.min_value = static_cast<u8>(std::clamp(percentile(values, 0.1) - 10, 64, 255));

	color.hue = hue;
	color.saturation = static_cast<f32>(sum_saturation / count * 255.0);
	color.value = static_cast<f32>(sum_value / count * 255.0);
	color.valid = true;

	// Check how much of the background would also match. Ignore the area around the sphere, because it may be lit by the sphere.
	const u32 margin = std::max(sphere->max_x - sphere->min_x, sphere->max_y - sphere->min_y) / 2 + 1;
	const u32 ignore_min_x = sphere->min_x > margin ? sphere->min_x - margin : 0;
	const u32 ignore_min_y = sphere->min_y > margin ? sphere->min_y - margin : 0;
	const u32 ignore_max_x = sphere->max_x + margin;
	const u32 ignore_max_y = sphere->max_y + margin;

	u64 background_count = 0;
	u64 background_matches = 0;

	for (const std::vector<u8>& image : images_on)
	{
		for (u32 y = 0; y < height; y++)
		{
			for (u32 x = 0; x < width; x++)
			{
				if (x >= ignore_min_x && x <= ignore_max_x && y >= ignore_min_y && y <= ignore_max_y)
					continue;

				const u8* rgba = &image[(y * width + x) * 4];
				const auto [pixel_hue, pixel_saturation, pixel_value] = ps_move_tracker<false>::rgb_to_hsv(rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f);

				background_count++;

				if (pixel_saturation * 255.0f >= calibration.min_saturation &&
				    pixel_value * 255.0f >= calibration.min_value &&
				    std::abs(std::remainder(pixel_hue - hue, 360.0f)) <= calibration.hue_threshold)
				{
					background_matches++;
				}
			}
		}
	}

	calibration.background_ratio = background_count ? static_cast<f32>(static_cast<f64>(background_matches) / background_count) : 0.0f;

	return ps_move_calibration_result::ok;
}

template class ps_move_tracker<false>;
template class ps_move_tracker<true>;
