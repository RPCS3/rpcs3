#include "stdafx.h"
#include "qt_camera_handler.h"
#include "permissions.h"
#include "Emu/system_config.h"
#include "Emu/System.h"
#include "Emu/Io/camera_config.h"

#include <QMediaDevices>

LOG_CHANNEL(camera_log, "Camera");

qt_camera_handler::qt_camera_handler() : camera_handler_base()
{
	// List available cameras
	for (const QCameraDevice& camera_device : QMediaDevices::videoInputs())
	{
		camera_log.success("Found camera: id=%s, description=%s", camera_device.id().toStdString(), camera_device.description());
	}

	if (!g_cfg_camera.load())
	{
		camera_log.notice("Could not load camera config. Using defaults.");
	}
}

qt_camera_handler::~qt_camera_handler()
{
	Emu.BlockingCallFromMainThread([&]()
	{
		close_camera();
		reset();
	});
}

void qt_camera_handler::reset()
{
	m_camera.reset();
	m_video_sink.reset();
	m_media_capture_session.reset();
}

void qt_camera_handler::set_camera(const QCameraDevice& camera_info)
{
	if (camera_info.isNull())
	{
		set_expected_state(camera_handler_state::closed);
		reset();
		return;
	}

	// Determine if the camera is front facing, in which case we will need to flip the image horizontally.
	const bool front_facing = camera_info.position() == QCameraDevice::Position::FrontFace;

	camera_log.success("Using camera: id=\"%s\", description=\"%s\", front_facing=%d", camera_info.id().toStdString(), camera_info.description(), front_facing);

	// Create camera and video surface
	m_media_capture_session = std::make_unique<QMediaCaptureSession>(nullptr);
	m_video_sink = std::make_unique<qt_camera_video_sink>(front_facing, nullptr);
	m_camera = std::make_unique<QCamera>(camera_info);

	connect(m_camera.get(), &QCamera::activeChanged, this, &qt_camera_handler::handle_camera_active);
	connect(m_camera.get(), &QCamera::errorOccurred, this, &qt_camera_handler::handle_camera_error);

	// Setup video sink
	m_media_capture_session->setCamera(m_camera.get());
	m_media_capture_session->setVideoSink(m_video_sink.get());

	// Update the settings
	update_camera_settings();
}

void qt_camera_handler::handle_camera_active(bool is_active)
{
	camera_log.notice("Camera active status changed to %d", is_active);

	// Check if the camera does what it's supposed to do.
	const camera_handler_state expected_state = get_expected_state();

	switch (expected_state)
	{
	case camera_handler_state::closed:
	case camera_handler_state::open:
	{
		if (is_active)
		{
			// This is not supposed to happen and indicates an unexpected QCamera issue
			camera_log.error("Camera started unexpectedly");
			set_state(camera_handler_state::running);
			return;
		}
		break;
	}
	case camera_handler_state::running:
	{
		if (!is_active)
		{
			// This is not supposed to happen and indicates an unexpected QCamera issue
			camera_log.error("Camera stopped unexpectedly");
			set_state(camera_handler_state::open);
			return;
		}
		break;
	}
	}

	set_state(expected_state);
}

void qt_camera_handler::handle_camera_error(QCamera::Error error, const QString& errorString)
{
	camera_log.error("Error event: \"%s\" (error=%d)", errorString, static_cast<int>(error));
	set_state(camera_handler_state::closed);
}

void qt_camera_handler::open_camera()
{
	camera_log.notice("Loading camera");

	if (const std::string camera_id = g_cfg.io.camera_id.to_string();
		m_camera_id != camera_id)
	{
		camera_log.notice("Switching camera from %s to %s", m_camera_id, camera_id);
		camera_log.notice("Stopping old camera...");
		if (m_camera)
		{
			set_expected_state(camera_handler_state::open);
			m_camera->stop();
		}
		m_camera_id = camera_id;
	}

	QCameraDevice selected_camera{};

	if (m_camera_id == g_cfg.io.camera_id.def)
	{
		selected_camera = QMediaDevices::defaultVideoInput();
	}
	else if (!m_camera_id.empty())
	{
		const QString camera_id = QString::fromStdString(m_camera_id);
		for (const QCameraDevice& camera_device : QMediaDevices::videoInputs())
		{
			if (camera_id == camera_device.id())
			{
				selected_camera = camera_device;
				break;
			}
		}
	}

	set_camera(selected_camera);

	if (!m_camera)
	{
		if (m_camera_id.empty()) camera_log.notice("Camera disabled");
		else camera_log.error("No camera found");
		set_state(camera_handler_state::closed);
		return;
	}

	if (m_camera->isActive())
	{
		camera_log.notice("Camera already active");
		return;
	}

	// List all supported formats for debugging
	for (const QCameraFormat& format : m_camera->cameraDevice().videoFormats())
	{
		camera_log.notice("Supported format: pixelformat=%s, resolution=%dx%d framerate=%f-%f", format.pixelFormat(), format.resolution().width(), format.resolution().height(), format.minFrameRate(), format.maxFrameRate());
	}

	// Update camera and view finder settings
	update_camera_settings();

	set_state(camera_handler_state::open);
}

void qt_camera_handler::close_camera()
{
	camera_log.notice("Unloading camera");

	if (!m_camera)
	{
		if (m_camera_id.empty()) camera_log.notice("Camera disabled");
		else camera_log.error("No camera found");
		set_state(camera_handler_state::closed);
		return;
	}

	// Unload/close camera
	set_expected_state(camera_handler_state::closed);
	m_camera->stop();
}

void qt_camera_handler::start_camera()
{
	camera_log.notice("Starting camera");

	if (!m_camera)
	{
		if (m_camera_id.empty()) camera_log.notice("Camera disabled");
		else camera_log.error("No camera found");
		set_state(camera_handler_state::closed);
		return;
	}

	if (m_camera->isActive())
	{
		camera_log.notice("Camera already started");
		return;
	}

	if (!gui::utils::check_camera_permission(this, [this](){ start_camera(); }, nullptr))
	{
		return;
	}

	// Start camera. We will start receiving frames now.
	set_expected_state(camera_handler_state::running);
	m_camera->start();
}

void qt_camera_handler::stop_camera()
{
	camera_log.notice("Stopping camera");

	if (!m_camera)
	{
		if (m_camera_id.empty()) camera_log.notice("Camera disabled");
		else camera_log.error("No camera found");
		set_state(camera_handler_state::closed);
		return;
	}

	if (!m_camera->isActive())
	{
		camera_log.notice("Camera already stopped");
		return;
	}

	// Stop camera. The camera will still be drawing power.
	set_expected_state(camera_handler_state::open);
	m_camera->stop();
}

void qt_camera_handler::set_format(s32 format, u32 bytesize)
{
	m_format = format;
	m_bytesize = bytesize;

	if (m_video_sink)
	{
		m_video_sink->set_format(m_format, m_bytesize);
	}
}

void qt_camera_handler::set_frame_rate(u32 frame_rate)
{
	m_frame_rate = frame_rate;
}

void qt_camera_handler::set_resolution(u32 width, u32 height)
{
	m_width = width;
	m_height = height;

	if (m_video_sink)
	{
		m_video_sink->set_resolution(m_width, m_height);
	}
}

void qt_camera_handler::set_mirrored(bool mirrored)
{
	m_mirrored = mirrored;

	if (m_video_sink)
	{
		m_video_sink->set_mirrored(m_mirrored);
	}
}

u64 qt_camera_handler::frame_number() const
{
	return m_video_sink ? m_video_sink->frame_number() : 0;
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
		set_state(camera_handler_state::closed);
		return camera_handler_state::closed;
	}

	if (m_camera_id.empty())
	{
		camera_log.notice("Camera disabled");
		set_state(camera_handler_state::closed);
		return camera_handler_state::closed;
	}

	if (!m_camera || !m_video_sink)
	{
		camera_log.fatal("Error: camera invalid");
		set_state(camera_handler_state::closed);
		return camera_handler_state::closed;
	}

	// Backup current state. State may change through events.
	const camera_handler_state current_state = get_state();

	if (current_state == camera_handler_state::running)
	{
		// Copy latest image into out buffer.
		m_video_sink->get_image(buf, size, width, height, frame_number, bytes_read);
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
	if (m_camera && m_camera->isAvailable())
	{
		// Get camera id. Use camera id of Qt default if the "Default" camera is selected.
		const std::string camera_id = (m_camera_id == g_cfg.io.camera_id.def) ? QMediaDevices::defaultVideoInput().id().toStdString() : m_camera_id;

		// Load selected settings from config file
		bool success = false;
		cfg_camera::camera_setting cfg_setting = g_cfg_camera.get_camera_setting(camera_id, success);

		if (success)
		{
			camera_log.notice("Found config entry for camera \"%s\" (m_camera_id='%s')", camera_id, m_camera_id);

			// List all available settings and choose the proper value if possible.
			const double epsilon = 0.001;
			success = false;
			for (const QCameraFormat& supported_setting : m_camera->cameraDevice().videoFormats())
			{
				if (supported_setting.resolution().width() == cfg_setting.width &&
					supported_setting.resolution().height() == cfg_setting.height &&
					supported_setting.minFrameRate() >= (cfg_setting.min_fps - epsilon) &&
					supported_setting.minFrameRate() <= (cfg_setting.min_fps + epsilon) &&
					supported_setting.maxFrameRate() >= (cfg_setting.max_fps - epsilon) &&
					supported_setting.maxFrameRate() <= (cfg_setting.max_fps + epsilon) &&
					supported_setting.pixelFormat() == static_cast<QVideoFrameFormat::PixelFormat>(cfg_setting.format))
				{
					// Apply settings.
					camera_log.notice("Setting view finder settings: frame_rate=%f, width=%d, height=%d, pixel_format=%s",
						supported_setting.maxFrameRate(), supported_setting.resolution().width(), supported_setting.resolution().height(), supported_setting.pixelFormat());
					m_camera->setCameraFormat(supported_setting);
					success = true;
					break;
				}
			}

			if (!success)
			{
				camera_log.warning("No matching camera setting available for the camera config: max_fps=%f, width=%d, height=%d, format=%d",
					cfg_setting.max_fps, cfg_setting.width, cfg_setting.height, cfg_setting.format);
			}
		}

		if (!success)
		{
			camera_log.notice("Using default view finder settings");
		}
	}

	// Update video surface if possible
	if (m_video_sink)
	{
		m_video_sink->set_resolution(m_width, m_height);
		m_video_sink->set_format(m_format, m_bytesize);
		m_video_sink->set_mirrored(m_mirrored);
	}
}
