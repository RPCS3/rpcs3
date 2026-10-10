#pragma once

#include "Emu/system_config_types.h"
#include "Utilities/Thread.h"

#include <QCamera>
#include <QDialog>
#include <QMediaCaptureSession>
#include <QVideoFrameInput>

#include <mutex>

#ifdef HAVE_SDL3
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "SDL3/SDL.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
#endif

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

Q_SIGNALS:
	void sdl_frame_ready();

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

#ifdef HAVE_SDL3
	void handle_sdl_camera_change(const QString& name, const QVariant& item_data);
	void handle_sdl_settings_change(const QVariant& item_data);

	void run_sdl();

	SDL_Camera* m_sdl_camera = nullptr;
	SDL_CameraID m_sdl_camera_id = 0;
	QImage m_sdl_image;
	std::mutex m_sdl_image_mutex;
	std::unique_ptr<named_thread<std::function<void()>>> m_sdl_thread;
	std::unique_ptr<QVideoFrameInput> m_video_frame_input;
#endif

	std::unique_ptr<Ui::camera_settings_dialog> ui;
	std::unique_ptr<QCamera> m_camera;
	std::unique_ptr<QMediaCaptureSession> m_media_capture_session;
	camera_handler m_handler = camera_handler::qt;
};
