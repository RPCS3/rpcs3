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
	void update_color(bool update_sliders = false);
	void update_hue(bool update_slider = false);
	void update_hue_threshold(bool update_slider = false);
	void update_saturation_threshold(bool update_slider = false);
	void update_min_radius(bool update_slider = false);
	void update_max_radius(bool update_slider = false);

	void reset_camera();
	void process_camera_frame();

	QPixmap get_histogram(const std::array<u32, 360>& hues, bool ignore_zero) const;

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
	view_mode m_view_mode = view_mode::contours;

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
	static constexpr s32 width = 640;
	static constexpr s32 height = 480;
	s32 m_format = 0;
	u64 m_frame_number = 0;
	u32 m_index = 0;
	bool m_filter_small_contours = true;
	bool m_freeze_frame = false;
	bool m_frame_frozen = false;
	bool m_show_all_contours = false;
	bool m_draw_contours = true;
	bool m_draw_overlays = true;

	QPixmap m_image;
	QPixmap m_histogram;

	// Thread control
	QTimer* m_update_timer = nullptr;
	std::unique_ptr<QThread> m_tracker_thread;
	atomic_t<u32> m_stop_threads = 0;
	std::mutex m_camera_handler_mutex;
	std::mutex m_image_mutex;
	std::shared_ptr<named_thread<pad_thread>> m_input_thread;
};
