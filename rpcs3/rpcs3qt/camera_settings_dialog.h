#pragma once

#include "Emu/system_config_types.h"

#include <QCamera>
#include <QDialog>
#include <QMediaCaptureSession>

namespace Ui
{
	class camera_settings_dialog;
}

class camera_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	camera_settings_dialog(QWidget* parent = nullptr);
	virtual ~camera_settings_dialog();

private Q_SLOTS:
	void handle_handler_change(int index);
	void handle_camera_change(int index);
	void handle_settings_change(int index);

private:
	void enable_combos();
	void reset_cameras();

	void load_config();
	void save_config();

	void handle_qt_camera_change(const QVariant& item_data);
	void handle_qt_settings_change(const QVariant& item_data);

	std::unique_ptr<Ui::camera_settings_dialog> ui;
	std::unique_ptr<QCamera> m_camera;
	std::unique_ptr<QMediaCaptureSession> m_media_capture_session;
	camera_handler m_handler = camera_handler::qt;
};
