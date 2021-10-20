#include "stdafx.h"
#include "qt_camera_handler.h"
#include "Emu/system_config.h"

#include <QMediaService>
#include <QCameraInfo>

LOG_CHANNEL(camera_log, "Camera");

qt_camera_handler::qt_camera_handler() : camera_handler_base()
{
	// List available cameras
	for (const QCameraInfo& cameraInfo : QCameraInfo::availableCameras())
	{
		camera_log.success("Found camera: name=%s, description=%s", cameraInfo.deviceName().toStdString(), cameraInfo.description().toStdString());
	}
}

qt_camera_handler::~qt_camera_handler()
{
	close_camera();
}

void qt_camera_handler::set_camera(const QCameraInfo& camera_info)
{
	if (camera_info.isNull())
	{
		m_surface.reset();
		m_camera.reset();
		m_error_handler.reset();
		return;
	}

	// Determine if the camera is front facing, in which case we will need to flip the image horizontally.
	const bool front_facing = camera_info.position() == QCamera::Position::FrontFace;

	camera_log.success("Using camera: name=\"%s\", description=\"%s\", front_facing=%d", camera_info.deviceName().toStdString(), camera_info.description().toStdString(), front_facing);

	// Create camera and video surface
	m_surface.reset(new qt_camera_video_surface(front_facing, nullptr));
	m_camera.reset(new QCamera(camera_info));
	m_error_handler.reset(new qt_camera_error_handler(m_camera,
		[this](QCamera::Status status)
		{
			switch (status)
			{
			case QCamera::UnavailableStatus:
				m_state = camera_handler_state::not_available;
				break;
			case QCamera::UnloadedStatus:
			case QCamera::UnloadingStatus:
				m_state = camera_handler_state::closed;
				break;
			case QCamera::StandbyStatus:
			case QCamera::StoppingStatus:
			case QCamera::LoadedStatus:
			case QCamera::LoadingStatus:
				m_state = camera_handler_state::open;
				break;
			case QCamera::StartingStatus:
			case QCamera::ActiveStatus:
				m_state = camera_handler_state::running;
				break;
			default:
				camera_log.error("Ignoring unknown status %d", static_cast<int>(status));
				break;
			}
		}));

	// Set view finder and update the settings
	m_camera->setViewfinder(m_surface.get());
	update_camera_settings();
}

void qt_camera_handler::open_camera()
{
	camera_log.notice("Loading camera");

	if (const std::string camera_id = g_cfg.io.camera_id.to_string();
		m_camera_id != camera_id)
	{
		camera_log.notice("Switching camera from %s to %s", camera_id, m_camera_id);
		camera_log.notice("Unloading old camera...");
		if (m_camera) m_camera->unload();
		m_camera_id = camera_id;
	}

	QCameraInfo selected_camera;

	if (m_camera_id == g_cfg.io.camera_id.def)
	{
		selected_camera = QCameraInfo::defaultCamera();
	}
	else if (!m_camera_id.empty())
	{
		const QString camera_id = QString::fromStdString(m_camera_id);
		for (const QCameraInfo& camera_info : QCameraInfo::availableCameras())
		{
			if (camera_id == camera_info.deviceName())
			{
				selected_camera = camera_info;
				break;
			}
		}
	}

	set_camera(selected_camera);

	if (!m_camera)
	{
		if (m_camera_id.empty()) camera_log.notice("Camera disabled");
		else camera_log.error("No camera found");
		m_state = camera_handler_state::not_available;
		return;
	}

	if (m_camera->state() != QCamera::State::UnloadedState)
	{
		camera_log.notice("Camera already loaded");
		return;
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
}

void qt_camera_handler::close_camera()
{
	camera_log.notice("Unloading camera");

	if (!m_camera)
	{
		if (m_camera_id.empty()) camera_log.notice("Camera disabled");
		else camera_log.error("No camera found");
		m_state = camera_handler_state::not_available;
		return;
	}

	if (m_camera->state() == QCamera::State::UnloadedState)
	{
		camera_log.notice("Camera already unloaded");
		return;
	}

	// Unload/close camera
	m_camera->unload();
}

void qt_camera_handler::start_camera()
{
	camera_log.notice("Starting camera");

	if (!m_camera)
	{
		if (m_camera_id.empty()) camera_log.notice("Camera disabled");
		else camera_log.error("No camera found");
		m_state = camera_handler_state::not_available;
		return;
	}

	if (m_camera->state() == QCamera::State::ActiveState)
	{
		camera_log.notice("Camera already started");
		return;
	}

	if (m_camera->state() == QCamera::State::UnloadedState)
	{
		camera_log.notice("Camera not open");
		open_camera();
	}

	// Start camera. We will start receiving frames now.
	m_camera->start();
}

void qt_camera_handler::stop_camera()
{
	camera_log.notice("Stopping camera");

	if (!m_camera)
	{
		if (m_camera_id.empty()) camera_log.notice("Camera disabled");
		else camera_log.error("No camera found");
		m_state = camera_handler_state::not_available;
		return;
	}

	if (m_camera->state() == QCamera::State::LoadedState)
	{
		camera_log.notice("Camera already stopped");
		return;
	}

	// Stop camera. The camera will still be drawing power.
	m_camera->stop();
}

void qt_camera_handler::set_format(s32 format, u32 bytesize)
{
	m_format = format;
	m_bytesize = bytesize;

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

camera_handler_base::camera_handler_state qt_camera_handler::get_image(u8* buf, u64 size, u32& width, u32& height, u64& frame_number, u64& bytes_read)
{
	width = 0;
	height = 0;
	frame_number = 0;
	bytes_read = 0;

	if (const std::string camera_id = g_cfg.io.camera_id.to_string();
		m_camera_id != camera_id)
	{
		camera_log.notice("Switching cameras");
		m_state = camera_handler_state::not_available;
		return camera_handler_state::not_available;
	}

	if (m_camera_id.empty())
	{
		camera_log.notice("Camera disabled");
		m_state = camera_handler_state::not_available;
		return camera_handler_state::not_available;
	}

	if (!m_camera || !m_surface)
	{
		camera_log.fatal("Error: camera invalid");
		m_state = camera_handler_state::not_available;
		return camera_handler_state::not_available;
	}

	// Backup current state. State may change through events.
	const camera_handler_state current_state = m_state;

	if (current_state == camera_handler_state::running)
	{
		// Copy latest image into out buffer.
		m_surface->get_image(buf, size, width, height, frame_number, bytes_read);
	}
	else
	{
		camera_log.error("Camera not running (m_state=%d)", static_cast<int>(current_state));
	}

	return current_state;
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
		m_surface->set_format(m_format, m_bytesize);
		m_surface->set_mirrored(m_mirrored);
	}
}
