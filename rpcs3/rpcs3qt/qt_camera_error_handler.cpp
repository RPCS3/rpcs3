#include "stdafx.h"
#include "qt_camera_error_handler.h"

LOG_CHANNEL(camera_log, "Camera");

qt_camera_error_handler::qt_camera_error_handler(std::shared_ptr<QCamera> camera, std::function<void(bool)> status_callback)
	: m_camera(std::move(camera))
	, m_status_callback(std::move(status_callback))
{
	if (m_camera)
	{
		connect(m_camera.get(), &QCamera::activeChanged, this, &qt_camera_error_handler::handle_camera_active);
		connect(m_camera.get(), &QCamera::errorOccurred, this, &qt_camera_error_handler::handle_camera_error);
	}
}

qt_camera_error_handler::~qt_camera_error_handler()
{
}

void qt_camera_error_handler::handle_camera_active(bool is_active)
{
	camera_log.notice("Camera active status changed to %d", is_active);

	if (m_status_callback)
	{
		m_status_callback(is_active);
	}
}

void qt_camera_error_handler::handle_camera_error(QCamera::Error error, const QString& errorString)
{
	camera_log.error("Error event: \"%s\" (error=%d)", errorString, static_cast<int>(error));
}
