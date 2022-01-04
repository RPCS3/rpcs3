#pragma once

#include <QObject>
#include <QCamera>

class qt_camera_error_handler : public QObject
{
	Q_OBJECT

public:
	qt_camera_error_handler(std::shared_ptr<QCamera> camera, std::function<void(QCamera::Status)> status_callback);
	virtual ~qt_camera_error_handler();

private Q_SLOTS:
	void handle_availability(QMultimedia::AvailabilityStatus availability);
	void handle_lock_status(QCamera::LockStatus, QCamera::LockChangeReason);
	void handle_capture_modes(QCamera::CaptureModes capture_modes);
	void handle_camera_state(QCamera::State state);
	void handle_camera_status(QCamera::Status status);
	void handle_camera_error(QCamera::Error error);

private:
	std::shared_ptr<QCamera> m_camera;
	std::function<void(QCamera::Status)> m_status_callback = nullptr;
};
