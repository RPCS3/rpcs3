#pragma once

#include "Utilities/Thread.h"
#include "Input/pad_thread.h"

#include <QDialog>
#include <QPixmap>
#include <QTimer>
#include <QThread>

class qt_camera_handler;
class pad_thread;
template <bool> class ps_move_tracker;

namespace Ui
{
	class ps_move_tracker_dialog;
}

class ps_move_tracker_dialog : public QDialog
{
	Q_OBJECT

public:
	ps_move_tracker_dialog(QWidget* parent = nullptr);
	virtual ~ps_move_tracker_dialog();

private:
	bool eventFilter(QObject* object, QEvent* event) override;

	void update_color(bool update_rgb_sliders, bool update_hue_slider);
	void update_default_brightness(bool update_slider);
	void update_hue(bool update_hue_slider, bool update_rgb_sliders, bool no_signal);
	void update_hue_threshold(bool update_slider = false);
	void update_saturation_threshold(bool update_slider = false);
	void update_min_radius(bool update_slider = false);
	void update_max_radius(bool update_slider = false);

	void reset_camera();
	void process_camera_frame();

	// Blink calibration. process_calibration runs on the tracker thread.
	void process_calibration();
	void abort_calibration();
	void set_led_enabled(bool enabled);

	QPixmap get_histogram(bool ignore_zero, int width) const;

	enum class view_mode
	{
		image,
		grayscale,
		hsv_hue,
		hsv_saturation,
		hsv_value,
		binary,
		contours
	};
	atomic_t<view_mode> m_view_mode = view_mode::contours;

	enum class histo_mode
	{
		unfiltered_hues
	};
	histo_mode m_histo_mode = histo_mode::unfiltered_hues;

	std::unique_ptr<Ui::ps_move_tracker_dialog> ui;
	std::unique_ptr<qt_camera_handler> m_camera_handler;
	std::unique_ptr<ps_move_tracker<true>> m_ps_move_tracker;
	std::vector<u8> m_image_data_frozen;
	std::vector<u8> m_image_data;
	std::array<u32, 360> m_hues{};

	// Measured sphere color of the selected device
	// Measured or calibrated sphere color of the selected device
	struct sphere_color_info
	{
		bool valid = false;
		f32 hue = 0.0f;
		f32 saturation = 0.0f;
		f32 value = 0.0f;
		bool is_calibration = false; // Show the thresholds instead of saturation and value
		f32 hue_threshold = 0.0f;
		s32 min_saturation = 0;
		s32 min_value = 0;
	};
	sphere_color_info m_measured_color{};

	// Blink calibration state. Only used on the tracker thread (or while it is stopped).
	struct blink_calibration
	{
		bool active = false;
		bool led_on = false;
		u32 index = 0;
		u32 frames = 0;
		u8 led_r = 0; // LED color during the calibration
		u8 led_g = 0;
		u8 led_b = 0;
		steady_clock::time_point step_start {};
		std::vector<std::vector<u8>> images_on;
		std::vector<std::vector<u8>> images_off;
	};
	blink_calibration m_calibration{};
	atomic_t<bool> m_calibration_requested = false;
	static constexpr s32 width = 640;
	static constexpr s32 height = 480;
	s32 m_format = 0;
	u64 m_frame_number = 0;
	atomic_t<u32> m_index = 0;
	atomic_t<bool> m_filter_small_contours = true;
	atomic_t<bool> m_freeze_frame = false;
	bool m_frame_frozen = false;
	atomic_t<bool> m_show_all_contours = false;
	atomic_t<bool> m_draw_contours = true;
	atomic_t<bool> m_draw_overlays = true;

	QImage m_image;

	// Thread control
	QTimer* m_update_timer = nullptr;
	std::unique_ptr<QThread> m_tracker_thread;
	atomic_t<u32> m_stop_threads = 0;
	std::mutex m_camera_handler_mutex;
	std::mutex m_image_mutex;
	std::shared_ptr<named_thread<pad_thread>> m_input_thread;
};
