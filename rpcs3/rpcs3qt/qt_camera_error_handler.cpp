#include "stdafx.h"
#include "qt_camera_error_handler.h"

LOG_CHANNEL(camera_log, "Camera");

template <>
void fmt_class_string<QCamera::Status>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](QCamera::Status value)
	{
		switch (value)
		{
		case QCamera::Status::UnavailableStatus: return "Unavailable";
		case QCamera::Status::UnloadedStatus: return "Unloaded";
		case QCamera::Status::LoadingStatus: return "Loading";
		case QCamera::Status::UnloadingStatus: return "Unloading";
		case QCamera::Status::LoadedStatus: return "Loaded";
		case QCamera::Status::StandbyStatus: return "Standby";
		case QCamera::Status::StartingStatus: return "Starting";
		case QCamera::Status::StoppingStatus: return "Stopping";
		case QCamera::Status::ActiveStatus: return "Active";
		}

		return unknown;
	});
}

qt_camera_error_handler::qt_camera_error_handler(std::shared_ptr<QCamera> camera, std::function<void(QCamera::Status)> status_callback)
	: m_camera(std::move(camera))
	, m_status_callback(std::move(status_callback))
{
	if (m_camera)
	{
		connect(m_camera.get(), QOverload<QMultimedia::AvailabilityStatus>::of(&QCamera::availabilityChanged), this, &qt_camera_error_handler::handle_availability);
		connect(m_camera.get(), &QCamera::stateChanged, this, &qt_camera_error_handler::handle_camera_state);
		connect(m_camera.get(), &QCamera::statusChanged, this, &qt_camera_error_handler::handle_camera_status);
		connect(m_camera.get(), &QCamera::errorOccurred, this, &qt_camera_error_handler::handle_camera_error);
		connect(m_camera.get(), &QCamera::captureModeChanged, this, &qt_camera_error_handler::handle_capture_modes);
		connect(m_camera.get(), QOverload<QCamera::LockStatus, QCamera::LockChangeReason>::of(&QCamera::lockStatusChanged), this, &qt_camera_error_handler::handle_lock_status);
	}
}

qt_camera_error_handler::~qt_camera_error_handler()
{
}

void qt_camera_error_handler::handle_availability(QMultimedia::AvailabilityStatus availability)
{
	camera_log.notice("Camera availability changed to %d", static_cast<int>(availability));
}

void qt_camera_error_handler::handle_camera_state(QCamera::State state)
{
	camera_log.notice("Camera state changed to %d", static_cast<int>(state));
}

void qt_camera_error_handler::handle_camera_status(QCamera::Status status)
{
	camera_log.notice("Camera status changed to %s", status);

	if (m_status_callback)
	{
		m_status_callback(status);
	}
}

void qt_camera_error_handler::handle_lock_status(QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
	camera_log.notice("Camera lock status changed to %d (reason=%d)", static_cast<int>(status), static_cast<int>(reason));
}

void qt_camera_error_handler::handle_capture_modes(QCamera::CaptureModes capture_modes)
{
	camera_log.notice("Camera capture modes changed to %d", static_cast<int>(capture_modes));
}

void qt_camera_error_handler::handle_camera_error(QCamera::Error error)
{
	camera_log.error("Error event: \"%s\" (error=%d)", m_camera ? m_camera->errorString() : "", static_cast<int>(error));
}
