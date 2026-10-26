#pragma once

#include "Emu/Cell/Modules/cellGem.h"
#include "blob_detector.h"

#include <chrono>

struct ps_move_info
{
	bool valid = false;     // The tracking result
	f32 radius = 0.0f;      // Radius of the sphere in pixels
	f32 distance_mm = 0.0f; // Distance from sphere to camera in mm
	f32 x_pos = 0.0f;       // X position (0-1)
	f32 y_pos = 0.0f;       // Y position (0-1)
};

// The color of the sphere as seen by the camera. It is measured while the sphere is tracked.
struct ps_move_sphere_color
{
	bool valid = false;
	f32 hue = 0.0f;        // Degrees (0-360)
	f32 saturation = 0.0f; // 0-255
	f32 value = 0.0f;      // 0-255
};

// The result of a calibration: the color of the sphere as seen by the camera and the thresholds that include most of the sphere
struct ps_move_tracker_calibration
{
	ps_move_sphere_color color {};
	f32 hue_threshold = 0.0f;    // Allowed hue difference in degrees
	u8 min_saturation = 0;       // 0-255
	u8 min_value = 0;            // 0-255
	f32 background_ratio = 0.0f; // Fraction of the background pixels that match the thresholds (0-1)
};

enum class ps_move_calibration_result
{
	ok,
	invalid_images, // No images or images with the wrong size
	not_found,      // Nothing blinked
	not_round,      // The blinking area is not round (e.g. the controller moved)
	no_color,       // The sphere has almost no color (e.g. it is overexposed)
};

// Calibrates the sphere from RGBA images that were taken while the LED was blinking.
// images_on[i] and images_off[i] are images with the LED turned on and off. Only pixels that changed in every pair are used.
ps_move_calibration_result ps_move_calibrate_sphere(const std::vector<std::vector<u8>>& images_on, const std::vector<std::vector<u8>>& images_off, u32 width, u32 height, ps_move_tracker_calibration& calibration);

template <bool DiagnosticsEnabled = false>
class ps_move_tracker
{
public:
	ps_move_tracker();
	virtual ~ps_move_tracker();

	void set_image_data(const void* buf, u64 size, u32 width, u32 height, s32 format);

	void init_worker(u32 index);
	void join_worker(u32 index);
	void process_image();
	void convert_image(s32 output_format);
	void process_hues();
	void process_contours(ps_move_info& info, u32 index);
	
	void set_active(u32 index, bool active);
	void set_hue(u32 index, u16 hue);
	void set_hue_threshold(u32 index, u8 threshold);
	void set_saturation_threshold(u32 index, u8 threshold);
	void set_value_threshold(u32 index, u8 threshold);

	void set_min_radius(f32 radius) { m_min_radius = radius; }
	void set_max_radius(f32 radius) { m_max_radius = radius; }
	void set_filter_small_contours(bool enabled = false) { m_filter_small_contours = enabled; }
	void set_show_all_contours(bool enabled = false) { m_show_all_contours = enabled; }
	void set_draw_contours(bool enabled = false) { m_draw_contours = enabled; }
	void set_draw_overlays(bool enabled = false) { m_draw_overlays = enabled; }

	const std::array<ps_move_info, CELL_GEM_MAX_NUM>& info() { return m_info; }
	const ps_move_sphere_color& sphere_color(u32 index) const { return ::at32(m_sphere_colors, index); }
	const std::array<u32, 360>& hues() { return m_hues; }
	const std::vector<u8>& rgba() { return m_image_rgba; }
	const std::vector<u8>& rgba_contours() { return m_image_rgba_contours; }
	const std::vector<u8>& hsv() { return m_image_hsv; }
	const std::vector<u8>& gray() { return m_image_gray; }
	const std::vector<u8>& binary(u32 index) { return ::at32(m_image_binary, index); }

	static std::tuple<u8, u8, u8> hsv_to_rgb(u16 hue, f32 saturation, f32 value);
	static std::tuple<s16, f32, f32> rgb_to_hsv(f32 r, f32 g, f32 b);

	// Allowed hue difference to the measured sphere color in degrees
	static constexpr f32 sphere_hue_range = 16.0f;

private:
	struct ps_move_config
	{
		bool active = false;

		u16 hue = 0;
		u8 hue_threshold = 0;
		u8 saturation_threshold_u8 = 0;
		u8 value_threshold_u8 = 150;

		s16 min_hue = 0;
		s16 max_hue = 0;

		void calculate_values();
	};

	void set_valid(ps_move_info& info, u32 index, bool valid);

	// Creates the binary image of all pixels that match the hue, saturation and value of the given device.
	// If use_sphere_color is true, the pixels also have to be similar to the measured sphere color.
	// Also creates a mask of overexposed pixels that don't match. Returns true if there are any.
	bool filter_image(u32 index, bool use_sphere_color);

	// Measures the color of the sphere and blends it into the sphere color of the given device
	void update_sphere_color(u32 index, const blob_detector::blob& b);

	void draw_sphere_size_range(f32 result_radius);

	CellGemVideoConvertAttribute m_vc_attr {};

	u32 m_width = 0;
	u32 m_height = 0;
	s32 m_format = 0;
	f32 m_min_radius = 0.0f;
	f32 m_max_radius = 1.0f;

	bool m_filter_small_contours = true;
	bool m_show_all_contours = false;
	bool m_draw_contours = false;
	bool m_draw_overlays = false;

	std::mutex m_mutex;

	std::vector<u8> m_image_data;
	std::vector<u8> m_image_rgba;
	std::vector<u8> m_image_rgba_contours;
	std::vector<u8> m_image_hsv;
	std::vector<u8> m_image_gray;

	std::array<std::vector<u8>, CELL_GEM_MAX_NUM> m_image_hue_filtered{};
	std::array<std::vector<u8>, CELL_GEM_MAX_NUM> m_image_binary{};
	std::array<std::vector<u8>, CELL_GEM_MAX_NUM> m_image_overexposed{}; // Overexposed pixels that didn't pass the filter
	std::array<std::vector<u8>, CELL_GEM_MAX_NUM> m_image_enclosed{};    // Pixels that are enclosed by pixels that passed the filter
	std::array<u32, CELL_GEM_MAX_NUM> m_fail_count{};
	std::array<u32, CELL_GEM_MAX_NUM> m_shape_fail_count{};

	std::array<u32, 360> m_hues{};
	std::array<ps_move_info, CELL_GEM_MAX_NUM> m_info{};
	std::array<ps_move_config, CELL_GEM_MAX_NUM> m_config{};
	std::array<ps_move_sphere_color, CELL_GEM_MAX_NUM> m_sphere_colors{};
	std::array<std::chrono::steady_clock::time_point, CELL_GEM_MAX_NUM> m_sphere_color_updates{};
	std::array<blob_detector, CELL_GEM_MAX_NUM> m_blob_detectors{};

	std::array<std::unique_ptr<named_thread<std::function<void()>>>, CELL_GEM_MAX_NUM> m_workers{};
	std::array<atomic_t<u32>, CELL_GEM_MAX_NUM> m_wake_up_workers{};
	std::array<atomic_t<u32>, CELL_GEM_MAX_NUM> m_workers_finished{};
};
