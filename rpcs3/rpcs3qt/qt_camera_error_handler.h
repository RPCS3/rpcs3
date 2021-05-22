#pragma once

#include <QObject>
#include <QCamera>

class qt_camera_error_handler : public QObject
{
	Q_OBJECT

public:
	qt_camera_error_handler(std::shared_ptr<QCamera> camera, std::function<void(bool)> status_callback);
	virtual ~qt_camera_error_handler();

private Q_SLOTS:
	void handle_camera_active(bool is_active);
	void handle_camera_error(QCamera::Error error, const QString& errorString);

private:
	std::shared_ptr<QCamera> m_camera;
	std::function<void(bool)> m_status_callback = nullptr;
};
