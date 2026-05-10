#pragma once

#include "Emu/Io/camera_handler_base.h"
#include "qt_camera_video_sink.h"
#include "util/types.hpp"

#include <QCamera>
#include <QMediaCaptureSession>
#include <QObject>
#include <QVideoSink>

class qt_camera_handler final : public QObject, public camera_handler_base
{
	Q_OBJECT

public:
	qt_camera_handler();
	virtual ~qt_camera_handler();

	void open_camera() override;
	void close_camera() override;
	void start_camera() override;
	void stop_camera() override;
	void set_format(s32 format, u32 bytesize) override;
	void set_frame_rate(u32 frame_rate) override;
	void set_resolution(u32 width, u32 height) override;
	void set_mirrored(bool mirrored) override;
	u64 frame_number() const override;
	camera_handler_state get_image(u8* buf, u64 size, u32& width, u32& height, u64& frame_number, u64& bytes_read) override;

private:
	void set_camera(const QCameraDevice& camera_info);
	void reset();
	void update_camera_settings();

	std::string m_camera_id;
	std::unique_ptr<QCamera> m_camera;
	std::unique_ptr<QMediaCaptureSession> m_media_capture_session;
	std::unique_ptr<qt_camera_video_sink> m_video_sink;

private Q_SLOTS:
	void handle_camera_active(bool is_active);
	void handle_camera_error(QCamera::Error error, const QString& errorString);
};
