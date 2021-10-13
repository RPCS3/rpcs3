#include "stdafx.h"
#include "qt_camera_handler.h"
#include "Emu/System.h"

#include <QMediaService>
#include <QCameraInfo>

LOG_CHANNEL(camera_log, "Camera");

qt_camera_handler::qt_camera_handler() : camera_handler_base()
{
}

qt_camera_handler::~qt_camera_handler()
{
	close_camera();
}

void qt_camera_handler::set_camera(const QCameraInfo& cameraInfo)
{
	if (cameraInfo.isNull())
	{
		camera_log.error("No camera present");
		return;
	}

	// Determine if the camera is front facing, in which case we will need to flip the image horizontally.
	const bool front_facing = cameraInfo.position() == QCamera::Position::FrontFace;

	camera_log.success("Using camera: name=\"%s\", description=\"%s\", front_facing=%d", cameraInfo.deviceName().toStdString(), cameraInfo.description().toStdString(), front_facing);

	// Create camera and video surface
	m_surface.reset(new qt_camera_video_surface(front_facing, nullptr));
	m_camera.reset(new QCamera(cameraInfo));

	// Create connects (may not work due to threading)
	connect(m_camera.get(), &QCamera::stateChanged, this, [this](QCamera::State state){ handle_camera_state(state); });
	connect(m_camera.get(), &QCamera::statusChanged, this, [this](QCamera::Status status){ handle_camera_status(status); });
	connect(m_camera.get(), &QCamera::errorOccurred, this, [this](QCamera::Error error){ handle_camera_error(error); });
	connect(m_camera.get(), &QCamera::captureModeChanged, this, [this](QCamera::CaptureModes modes){ handle_capture_modes(modes); });
	connect(m_camera.get(), QOverload<QCamera::LockStatus, QCamera::LockChangeReason>::of(&QCamera::lockStatusChanged), this, [this](QCamera::LockStatus status, QCamera::LockChangeReason reason){ handle_lock_status(status, reason); });

	// Set view finder and update the settings
	m_camera->setViewfinder(m_surface.get());
	update_camera_settings();

	// Log some states
	handle_camera_state(m_camera->state());
	handle_lock_status(m_camera->lockStatus(), QCamera::UserRequest);
}

void qt_camera_handler::open_camera()
{
	// List available cameras
	for (const QCameraInfo& cameraInfo : QCameraInfo::availableCameras())
	{
		camera_log.success("Found camera: name=%s, description=%s", cameraInfo.deviceName().toStdString(), cameraInfo.description().toStdString());
	}

	// Let's use the default camera for now
	set_camera(QCameraInfo::defaultCamera());

	camera_log.notice("Loading camera");

	if (!m_camera)
	{
		camera_log.error("No camera found");
		m_state = camera_handler_state::closed;
		return;
	}

	if (m_camera->state() != QCamera::State::UnloadedState)
	{
		camera_log.notice("Camera already loaded");
	}

	// Load/open camera
	m_camera->load();

	// List all supported formats for debugging
	for (const QCamera::FrameRateRange& frame_rate : m_camera->supportedViewfinderFrameRateRanges())
	{
		camera_log.notice("Supported frame rate range: %f-%f", frame_rate.minimumFrameRate, frame_rate.maximumFrameRate);
	}
	for (const QVideoFrame::PixelFormat& pixel_format : m_camera->supportedViewfinderPixelFormats())
	{
		camera_log.notice("Supported pixel format: %d", static_cast<int>(pixel_format));
	}
	for (const QSize& resolution : m_camera->supportedViewfinderResolutions())
	{
		camera_log.notice("Supported resolution: %dx%d", resolution.width(), resolution.height());
	}

	// Update camera and view finder settings
	update_camera_settings();

	m_state = camera_handler_state::open;
}

void qt_camera_handler::close_camera()
{
	camera_log.notice("Unloading camera");

	if (!m_camera)
	{
		camera_log.error("No camera found");
		m_state = camera_handler_state::closed;
		return;
	}

	if (m_camera->state() == QCamera::State::UnloadedState)
	{
		camera_log.notice("Camera already unloaded");
	}

	// Unload/close camera
	m_camera->unload();
	m_state = camera_handler_state::closed;
}

void qt_camera_handler::start_camera()
{
	camera_log.notice("Starting camera");

	if (!m_camera)
	{
		camera_log.error("No camera found");
		m_state = camera_handler_state::closed;
		return;
	}

	if (m_camera->state() == QCamera::State::ActiveState)
	{
		camera_log.notice("Camera already started");
	}
	else if (m_camera->state() == QCamera::State::UnloadedState)
	{
		camera_log.notice("Camera not open");
		open_camera();
	}

	// Start camera. We will start receiving frames now.
	m_camera->start();
	m_state = camera_handler_state::running;
}

void qt_camera_handler::stop_camera()
{
	camera_log.notice("Stopping camera");

	if (!m_camera)
	{
		camera_log.error("No camera found");
		m_state = camera_handler_state::closed;
		return;
	}

	if (m_camera->state() == QCamera::State::LoadedState)
	{
		camera_log.notice("Camera already stopped");
	}

	// Stop camera. The camera will still be drawing power.
	m_camera->stop();
	m_state = camera_handler_state::open;
}

void qt_camera_handler::set_format(s32 format, u32 bytes_per_pixel)
{
	m_format = format;
	m_bytes_per_pixel = bytes_per_pixel;

	update_camera_settings();
}

void qt_camera_handler::set_frame_rate(u32 frame_rate)
{
	m_frame_rate = frame_rate;

	update_camera_settings();
}

void qt_camera_handler::set_resolution(u32 width, u32 height)
{
	m_width = width;
	m_height = height;

	update_camera_settings();
}

void qt_camera_handler::set_mirrored(bool mirrored)
{
	m_mirrored = mirrored;

	update_camera_settings();
}

u64 qt_camera_handler::frame_number() const
{
	return m_surface ? m_surface->frame_number() : 0;
}

bool qt_camera_handler::get_image(u8* buf, u64 size, u32& width, u32& height, u64& frame_number, u64& bytes_read)
{
	width = 0;
	height = 0;
	frame_number = 0;
	bytes_read = 0;

	// Check for errors
	if (!m_camera || !m_surface || m_state != camera_handler_state::running)
	{
		camera_log.error("Error: camera invalid");
		m_state = camera_handler_state::closed;
		return false;
	}

	if (QCamera::Error error = m_camera->error(); error != QCamera::NoError)
	{
		camera_log.error("Error: \"%s\" (error=%d)", m_camera ? m_camera->errorString().toStdString() : "", static_cast<int>(error));
		m_state = camera_handler_state::closed;
		return false;
	}

	switch (QCamera::Status status = m_camera->status())
	{
	case QCamera::UnavailableStatus:
	case QCamera::UnloadedStatus:
	case QCamera::UnloadingStatus:
		camera_log.error("Camera not open. State=%d", static_cast<int>(status));
		m_state = camera_handler_state::closed;
		return false;
	case QCamera::LoadedStatus:
	case QCamera::StandbyStatus:
	case QCamera::StoppingStatus:
		camera_log.error("Camera not active. State=%d", static_cast<int>(status));
		m_state = camera_handler_state::open;
		return false;
	case QCamera::LoadingStatus:
	case QCamera::StartingStatus:
	case QCamera::ActiveStatus:
	default:
		break;
	}

	// Copy latest image into out buffer.
	m_surface->get_image(buf, size, width, height, frame_number, bytes_read);

	return true;
}

void qt_camera_handler::update_camera_settings()
{
	// Update camera if possible. We can only do this if it is already loaded.
	if (m_camera && m_camera->state() != QCamera::State::UnloadedState)
	{
		// List all available settings in a cascading fashion and choose the proper value if possible.
		// After each step, the next one will only list the settings that are compatible with the prior ones.
		QCameraViewfinderSettings settings;

		// Set resolution if possible.
		for (const QSize& resolution : m_camera->supportedViewfinderResolutions(settings))
		{
			if (m_width == resolution.width() && m_height == resolution.height())
			{
				settings.setResolution(resolution.width(), resolution.height());
				break;
			}
		}

		// Set frame rate if possible.
		for (const QCamera::FrameRateRange& frame_rate : m_camera->supportedViewfinderFrameRateRanges(settings))
		{
			// Some cameras do not have an exact match, so let's approximate.
			if (static_cast<qreal>(m_frame_rate) >= (frame_rate.maximumFrameRate - 0.5) && static_cast<qreal>(m_frame_rate) <= (frame_rate.maximumFrameRate + 0.5))
			{
				// Lock the frame rate by setting the min and max to the same value.
				settings.setMinimumFrameRate(m_frame_rate);
				settings.setMaximumFrameRate(m_frame_rate);
				break;
			}
		}

		// Set pixel format if possible. (Unused for now, because formats differ between Qt and cell)
		//for (const QVideoFrame::PixelFormat& pixel_format : m_camera->supportedViewfinderPixelFormats(settings))
		//{
		//	if (pixel_format matches m_format)
		//	{
		//		settings.setPixelFormat(pixel_format);
		//		break;
		//	}
		//}

		camera_log.notice("Setting view finder settings: frame_rate=%f, width=%d, height=%d, pixel_format=%d",
			settings.maximumFrameRate(), settings.resolution().width(), settings.resolution().height(), static_cast<int>(settings.pixelFormat()));

		// Apply settings.
		m_camera->setViewfinderSettings(settings);
	}

	// Update video surface if possible
	if (m_surface)
	{
		m_surface->set_resolution(m_width, m_height);
		m_surface->set_format(m_format, m_bytes_per_pixel);
		m_surface->set_mirrored(m_mirrored);
	}
}

void qt_camera_handler::handle_camera_state(QCamera::State state)
{
	camera_log.notice("Camera state changed to %d", static_cast<int>(state));
}

void qt_camera_handler::handle_camera_status(QCamera::Status status)
{
	camera_log.notice("Camera status changed to %d", static_cast<int>(status));
}

void qt_camera_handler::handle_lock_status(QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
	camera_log.notice("Camera lock status changed to %d (reason=%d)", static_cast<int>(status), static_cast<int>(reason));
}

void qt_camera_handler::handle_capture_modes(QCamera::CaptureModes capture_modes)
{
	camera_log.notice("Camera capture modes changed to %d", static_cast<int>(capture_modes));
}

void qt_camera_handler::handle_camera_error(QCamera::Error error)
{
	camera_log.error("Error: \"%s\" (error=%d)", m_camera ? m_camera->errorString().toStdString() : "", static_cast<int>(error));
}
