#pragma once

#include "Emu/Io/camera_handler_base.h"
#include "qt_camera_video_surface.h"

#include <QCamera>
#include <QCameraImageCapture>
#include <QAbstractVideoSurface>

class video_surface;

class qt_camera_handler final : public camera_handler_base, public QObject
{
public:
	qt_camera_handler();
	virtual ~qt_camera_handler();

	void set_camera(const QCameraInfo &cameraInfo);

	void open_camera() override;
	void close_camera() override;
	void start_camera() override;
	void stop_camera() override;
	void set_format(s32 format, u32 bytes_per_pixel) override;
	void set_frame_rate(u32 frame_rate) override;
	void set_resolution(u32 width, u32 height) override;
	void set_mirrored(bool mirrored) override;
	u64 frame_number() const override;
	bool get_image(u8* buf, u64 size, u32& width, u32& height, u64& frame_number, u64& bytes_read) override;

private:
	void handle_lock_status(QCamera::LockStatus, QCamera::LockChangeReason);
	void handle_capture_modes(QCamera::CaptureModes capture_modes);
	void handle_camera_state(QCamera::State state);
	void handle_camera_status(QCamera::Status status);
	void handle_camera_error(QCamera::Error error);
	void update_camera_settings();

	std::unique_ptr<QCamera> m_camera;
	std::unique_ptr<qt_camera_video_surface> m_surface;
};
