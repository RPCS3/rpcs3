#pragma once

#include "ui_pad_motion_settings_dialog.h"
#include "pad_device_info.h"
#include "Emu/Io/PadHandler.h"
#include "Utilities/Thread.h"

#include <QDialog>
#include <QTimer>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>

#include <unordered_map>
#include <mutex>

namespace Ui
{
	class pad_motion_settings_dialog;
}

class pad_motion_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit pad_motion_settings_dialog(QDialog* parent, std::shared_ptr<PadHandlerBase> handler, cfg_player* cfg);
	~pad_motion_settings_dialog();

private Q_SLOTS:
	void change_device(int index);

private:
	void switch_buddy_pad_info(int index, pad_device_info info, bool is_connected);
	void start_input_thread();
	void pause_input_thread();

	std::unique_ptr<Ui::pad_motion_settings_dialog> ui;
	std::shared_ptr<PadHandlerBase> m_handler;
	std::string m_device_name;
	cfg_player* m_cfg = nullptr;
	std::mutex m_config_mutex;
	std::unordered_map<u32, std::string> m_motion_axis_list;

	// Input thread. Its Callback handles the input
	std::unique_ptr<named_thread<std::function<void()>>> m_input_thread;
	enum class input_thread_state { paused, pausing, active };
	atomic_t<input_thread_state> m_input_thread_state{input_thread_state::paused};
	struct motion_callback_data
	{
		bool success = false;
		bool has_new_data = false;
		std::string pad_name;
		std::array<u16, 4> preview_values{{ DEFAULT_MOTION_X, DEFAULT_MOTION_Y, DEFAULT_MOTION_Z, DEFAULT_MOTION_G}};;
	} m_motion_callback_data;
	QTimer m_timer_input;
	std::mutex m_input_mutex;

	std::array<QSlider*, 4> m_preview_sliders;
	std::array<QLabel*, 4> m_preview_labels;
	std::array<QComboBox*, 4> m_axis_names;
	std::array<QCheckBox*, 4> m_mirrors;
	std::array<QSpinBox*, 4> m_shifts;
	std::array<cfg_sensor*, 4> m_config_entries;

	const QString Disconnected_suffix = tr(" (disconnected)");
};
