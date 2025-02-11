#pragma once

#include "Emu/Cell/Modules/cellGem.h"

#ifdef HAVE_OPENCV
	constexpr bool g_ps_move_tracking_supported = true;
#else
	constexpr bool g_ps_move_tracking_supported = false;
#endif

struct ps_move_info
{
	bool valid = false;     // The tracking result
	f32 radius = 0.0f;      // Radius of the sphere in pixels
	f32 distance_mm = 0.0f; // Distance from sphere to camera in mm
	u32 x_pos = 0;          // X position in pixels
	u32 y_pos = 0;          // Y position in pixels
	u32 x_max = 0;          // Maximum X position in pixels
	u32 y_max = 0;          // Maximum Y position in pixels
};

template <bool DiagnosticsEnabled = false>
class ps_move_tracker
{
public:
	ps_move_tracker();
	virtual ~ps_move_tracker();

	void set_image_data(const void* buf, u64 size, u32 width, u32 height, s32 format);

	void init_workers();
	void process_image();
	void convert_image(s32 output_format);
	void process_hues();
	void process_contours(ps_move_info& info, u32 index);
	
	void set_active(u32 index, bool active);
	void set_hue(u32 index, u16 hue);
	void set_hue_threshold(u32 index, u16 threshold);
	void set_saturation_threshold(u32 index, u16 threshold);

	void set_min_radius(f32 radius) { m_min_radius = radius; }
	void set_max_radius(f32 radius) { m_max_radius = radius; }
	void set_filter_small_contours(bool enabled = false) { m_filter_small_contours = enabled; }
	void set_show_all_contours(bool enabled = false) { m_show_all_contours = enabled; }
	void set_draw_contours(bool enabled = false) { m_draw_contours = enabled; }
	void set_draw_overlays(bool enabled = false) { m_draw_overlays = enabled; }

	const std::array<ps_move_info, CELL_GEM_MAX_NUM>& info() { return m_info; }
	const std::array<u32, 360>& hues() { return m_hues; }
	const std::vector<u8>& rgba() { return m_image_rgba; }
	const std::vector<u8>& rgba_contours() { return m_image_rgba_contours; }
	const std::vector<u8>& hsv() { return m_image_hsv; }
	const std::vector<u8>& gray() { return m_image_gray; }
	const std::vector<u8>& binary(u32 index) { return ::at32(m_image_binary, index); }

	static std::tuple<u8, u8, u8> hsv_to_rgb(u16 hue, f32 saturation, f32 value);
	static std::tuple<s16, f32, f32> rgb_to_hsv(f32 r, f32 g, f32 b);

private:
	struct ps_move_config
	{
		bool active = false;

		u16 hue = 0;
		u16 hue_threshold = 0;
		u16 saturation_threshold_u8 = 0;

		s16 min_hue = 0;
		s16 max_hue = 0;
		f32 saturation_threshold = 0.0f;

		void calculate_values();
	};

	void set_valid(ps_move_info& info, u32 index, bool valid);

	void draw_sphere_size_range(f32 result_radius);

	u32 m_width = 0;
	u32 m_height = 0;
	s32 m_format = 0;
	f32 m_min_radius = 0.0f;
	f32 m_max_radius = 1.0f;

	bool m_filter_small_contours = true;
	bool m_show_all_contours = false;
	bool m_draw_contours = false;
	bool m_draw_overlays = false;

	std::vector<u8> m_image_data;
	std::vector<u8> m_image_rgba;
	std::vector<u8> m_image_rgba_contours;
	std::vector<u8> m_image_hsv;
	std::vector<u8> m_image_gray;

	std::array<std::vector<u8>, CELL_GEM_MAX_NUM> m_image_hue_filtered{};
	std::array<std::vector<u8>, CELL_GEM_MAX_NUM> m_image_binary{};
	std::array<u32, CELL_GEM_MAX_NUM> m_fail_count{};
	std::array<u32, CELL_GEM_MAX_NUM> m_shape_fail_count{};

	std::array<u32, 360> m_hues{};
	std::array<ps_move_info, CELL_GEM_MAX_NUM> m_info{};
	std::array<ps_move_config, CELL_GEM_MAX_NUM> m_config{};

	std::array<std::unique_ptr<named_thread<std::function<void()>>>, CELL_GEM_MAX_NUM> m_workers{};
	std::array<atomic_t<u32>, CELL_GEM_MAX_NUM> m_wake_up_workers{};
	std::array<atomic_t<u32>, CELL_GEM_MAX_NUM> m_workers_finished{};
};
