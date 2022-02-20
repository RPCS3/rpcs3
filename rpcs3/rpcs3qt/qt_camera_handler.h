#pragma once

#include "Emu/Io/camera_handler_base.h"
#include "qt_camera_video_surface.h"
#include "qt_camera_error_handler.h"

#include <QCamera>
#include <QCameraImageCapture>
#include <QAbstractVideoSurface>

class qt_camera_handler final : public camera_handler_base
{
public:
	qt_camera_handler();
	virtual ~qt_camera_handler();

	void set_camera(const QCameraInfo& camera_info);

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
	void update_camera_settings();

	std::string m_camera_id;
	std::shared_ptr<QCamera> m_camera;
	std::unique_ptr<qt_camera_video_surface> m_surface;
	std::unique_ptr<qt_camera_error_handler> m_error_handler;
};
