#ifdef HAVE_SDL3

#include "stdafx.h"
#include "sdl_camera_handler.h"
#include "sdl_camera_video_sink.h"
#include "sdl_instance.h"
#include "Emu/system_config.h"
#include "Emu/System.h"
#include "Emu/Io/camera_config.h"

LOG_CHANNEL(camera_log, "Camera");

#if !(SDL_VERSION_ATLEAST(3, 4, 0))
namespace SDL_CameraPermissionState
{
	constexpr int SDL_CAMERA_PERMISSION_STATE_DENIED = -1;
	constexpr int SDL_CAMERA_PERMISSION_STATE_PENDING = 0;
	constexpr int SDL_CAMERA_PERMISSION_STATE_APPROVED = 1;
}
#endif

template <>
void fmt_class_string<SDL_CameraSpec>::format(std::string& out, u64 arg)
{
	const SDL_CameraSpec& spec = get_object(arg);
 	out += fmt::format("format=0x%x, colorspace=0x%x, width=%d, height=%d, framerate_numerator=%d, framerate_denominator=%d, fps=%f",
		static_cast<u32>(spec.format), static_cast<u32>(spec.colorspace), spec.width, spec.height,
		spec.framerate_numerator, spec.framerate_denominator, spec.framerate_numerator / static_cast<f32>(spec.framerate_denominator));
}

std::vector<std::string> sdl_camera_handler::get_drivers()
{
	std::vector<std::string> drivers;

	if (const int num_drivers = SDL_GetNumCameraDrivers(); num_drivers > 0)
	{
		for (int i = 0; i < num_drivers; i++)
		{
			if (const char* driver = SDL_GetCameraDriver(i))
			{
				camera_log.notice("Found driver: %s", driver);
				drivers.push_back(driver);
				continue;
			}

			camera_log.error("Failed to get driver %d. SDL Error: %s", i, SDL_GetError());
		}
	}
	else
	{
		camera_log.error("No SDL camera drivers found");
	}

	return drivers;
}

std::map<SDL_CameraID, std::string> sdl_camera_handler::get_cameras()
{
	int camera_count = 0;
	if (SDL_CameraID* cameras = SDL_GetCameras(&camera_count))
	{
		std::map<SDL_CameraID, std::string> camera_map;

		for (int i = 0; i < camera_count && cameras[i]; i++)
		{
			if (const char* name = SDL_GetCameraName(cameras[i]))
			{
				camera_log.notice("Found camera: name=%s", name);
				camera_map[cameras[i]] = name;
				continue;
			}

			camera_log.error("Found camera (Failed to get name. SDL Error: %s", SDL_GetError());
		}

		SDL_free(cameras);

		if (camera_map.empty())
		{
			camera_log.notice("No SDL cameras found");
		}

		return camera_map;
	}

	camera_log.error("Could not get cameras! SDL Error: %s", SDL_GetError());
	return {};
}

sdl_camera_handler::sdl_camera_handler() : camera_handler_base()
{
	if (!g_cfg_camera.load())
	{
		camera_log.notice("Could not load camera config. Using defaults.");
	}

	if (!sdl_instance::get_instance().initialize())
	{
		camera_log.error("Could not initialize SDL");
		return;
	}

	// List available camera drivers
	sdl_camera_handler::get_drivers();

	// List available cameras
	sdl_camera_handler::get_cameras();
}

sdl_camera_handler::~sdl_camera_handler()
{
	Emu.BlockingCallFromMainThread([&]()
	{
		close_camera();
	});
}

void sdl_camera_handler::reset()
{
	m_video_sink.reset();

	if (m_camera)
	{
		SDL_CloseCamera(m_camera);
		m_camera = nullptr;
	}
}

void sdl_camera_handler::open_camera()
{
	camera_log.notice("Loading camera");

	if (const std::string camera_id = g_cfg.io.sdl_camera_id.to_string();
		m_camera_id != camera_id)
	{
		camera_log.notice("Switching camera from %s to %s", m_camera_id, camera_id);
		camera_log.notice("Stopping old camera...");
		if (m_camera)
		{
			set_expected_state(camera_handler_state::open);
			reset();
		}
		m_camera_id = camera_id;
	}

	// List available cameras
	int camera_count = 0;
	SDL_CameraID* cameras = SDL_GetCameras(&camera_count);

	if (!cameras)
	{
		camera_log.error("Could not get cameras! SDL Error: %s", SDL_GetError());
		set_state(camera_handler_state::closed);
		return;
	}

	if (camera_count <= 0)
	{
		camera_log.error("No cameras found");
		set_state(camera_handler_state::closed);
		SDL_free(cameras);
		return;
	}

	m_sdl_camera_id = 0;

	if (m_camera_id == g_cfg.io.sdl_camera_id.def)
	{
		m_sdl_camera_id = cameras[0];
	}
	else if (!m_camera_id.empty())
	{
		for (int i = 0; i < camera_count && cameras[i]; i++)
		{
			if (const char* name = SDL_GetCameraName(cameras[i]))
			{
				if (m_camera_id == name)
				{
					m_sdl_camera_id = cameras[i];
					break;
				}
			}
		}
	}

	SDL_free(cameras);

	if (!m_sdl_camera_id)
	{
		camera_log.error("Camera %s not found", m_camera_id);
		set_state(camera_handler_state::closed);
		return;
	}

	std::string camera_id;

	if (const char* name = SDL_GetCameraName(m_sdl_camera_id))
	{
		camera_log.notice("Using camera: name=%s", name);
		camera_id = name;
	}

	SDL_CameraSpec used_spec
	{
		.format = SDL_PixelFormat::SDL_PIXELFORMAT_RGBA32,
		.colorspace = SDL_Colorspace::SDL_COLORSPACE_RGB_DEFAULT,
		.width = static_cast<int>(m_width),
		.height = static_cast<int>(m_height),
		.framerate_numerator = 30,
		.framerate_denominator = 1
	};

	int num_formats = 0;
	if (SDL_CameraSpec** specs = SDL_GetCameraSupportedFormats(m_sdl_camera_id, &num_formats))
	{
		if (num_formats <= 0)
		{
			camera_log.error("No SDL camera specs found");
		}
		else
		{
			// Load selected settings from config file
			bool success = false;
			cfg_camera::camera_setting cfg_setting = g_cfg_camera.get_camera_setting(fmt::format("%s", camera_handler::sdl), camera_id, success);

			if (success)
			{
				camera_log.notice("Found config entry for camera \"%s\" (m_camera_id='%s')", camera_id, m_camera_id);

				// List all available settings and choose the proper value if possible.
				constexpr double epsilon = 0.001;
				success = false;

				for (int i = 0; i < num_formats; i++)
				{
					if (!specs[i]) continue;

					const SDL_CameraSpec& spec = *specs[i];
					const f64 fps = spec.framerate_numerator / static_cast<f64>(spec.framerate_denominator);

					if (spec.width == cfg_setting.width &&
						spec.height == cfg_setting.height &&
						fps >= (cfg_setting.min_fps - epsilon) &&
						fps <= (cfg_setting.min_fps + epsilon) &&
						fps >= (cfg_setting.max_fps - epsilon) &&
						fps <= (cfg_setting.max_fps + epsilon) &&
						spec.format == static_cast<SDL_PixelFormat>(cfg_setting.format) &&
						spec.colorspace == static_cast<SDL_Colorspace>(cfg_setting.colorspace))
					{
						// Apply settings.
						camera_log.notice("Setting camera spec: %s", spec);

						// TODO: SDL converts the image for us. We would have to do this manually if we want to use other formats.
						//used_spec = spec;
						used_spec.width = spec.width;
						used_spec.height = spec.height;
						used_spec.framerate_numerator = spec.framerate_numerator;
						used_spec.framerate_denominator = spec.framerate_denominator;
						success = true;
						break;
					}
				}

				if (!success)
				{
					camera_log.warning("No matching camera setting available for the camera config: max_fps=%f, width=%d, height=%d, format=%d, colorspace=%d",
						cfg_setting.max_fps, cfg_setting.width, cfg_setting.height, cfg_setting.format, cfg_setting.colorspace);
				}
			}

			if (!success)
			{
				camera_log.notice("Using default camera spec: %s", used_spec);
			}
		}
		SDL_free(specs);
	}
	else
	{
		camera_log.error("No SDL camera specs found. SDL Error: %s", SDL_GetError());
	}

	reset();

	camera_log.notice("Requesting camera spec: %s", used_spec);

	m_camera = SDL_OpenCamera(m_sdl_camera_id, &used_spec);

	if (!m_camera)
	{
		if (!m_camera_id.empty()) camera_log.notice("Camera disabled");
		else camera_log.error("No camera found");
		set_state(camera_handler_state::closed);
		return;
	}

	if (const char* driver = SDL_GetCurrentCameraDriver())
	{
		camera_log.notice("Using driver: %s", driver);
	}

	if (SDL_CameraSpec spec {}; SDL_GetCameraFormat(m_camera, &spec))
	{
		camera_log.notice("Using camera spec: %s", spec);
	}
	else
	{
		camera_log.error("Could not get camera spec. SDL Error: %s", SDL_GetError());
	}

	const SDL_CameraPosition position = SDL_GetCameraPosition(m_sdl_camera_id);
	const bool front_facing = position == SDL_CameraPosition::SDL_CAMERA_POSITION_FRONT_FACING;

	if (const SDL_PropertiesID property_id = SDL_GetCameraProperties(m_camera); property_id != 0)
	{
		if (!SDL_EnumerateProperties(property_id, [](void* /*userdata*/, SDL_PropertiesID /*props*/, const char* name)
			{
				if (name) camera_log.notice("SDL camera property available: %s", name);
			}, nullptr))
		{
			camera_log.warning("SDL_EnumerateProperties failed. SDL Error: %s", SDL_GetError());
		}
	}
	else
	{
		camera_log.warning("SDL_GetCameraProperties failed. SDL Error: %s", SDL_GetError());
	}

	m_video_sink = std::make_unique<sdl_camera_video_sink>(front_facing, m_camera);
	m_video_sink->set_resolution(m_width, m_height);
	m_video_sink->set_format(m_format, m_bytesize);
	m_video_sink->set_mirrored(m_mirrored);

	set_state(camera_handler_state::open);
}

void sdl_camera_handler::close_camera()
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
	reset();

	set_state(camera_handler_state::closed);
}

void sdl_camera_handler::start_camera()
{
	camera_log.notice("Starting camera");

	if (!m_camera)
	{
		if (m_camera_id.empty()) camera_log.notice("Camera disabled");
		else camera_log.error("No camera found");
		set_state(camera_handler_state::closed);
		return;
	}

	const auto camera_permission = SDL_GetCameraPermissionState(m_camera);
	switch (camera_permission)
	{
	case SDL_CameraPermissionState::SDL_CAMERA_PERMISSION_STATE_DENIED:
		camera_log.error("Camera permission denied");
		set_state(camera_handler_state::closed);
		reset();
		return;
	case SDL_CameraPermissionState::SDL_CAMERA_PERMISSION_STATE_PENDING:
		// TODO: try to get permission
		break;
	case SDL_CameraPermissionState::SDL_CAMERA_PERMISSION_STATE_APPROVED:
		break;
	default:
		fmt::throw_exception("Unknown SDL_CameraPermissionState %d", static_cast<s32>(camera_permission));
	}

	// Start camera. We will start receiving frames now.
	set_state(camera_handler_state::running);
}

void sdl_camera_handler::stop_camera()
{
	camera_log.notice("Stopping camera");

	if (!m_camera)
	{
		if (m_camera_id.empty()) camera_log.notice("Camera disabled");
		else camera_log.error("No camera found");
		set_state(camera_handler_state::closed);
		return;
	}

	// Stop camera. The camera will still be drawing power.
	set_expected_state(camera_handler_state::open);
}

void sdl_camera_handler::set_format(s32 format, u32 bytesize)
{
	m_format = format;
	m_bytesize = bytesize;

	if (m_video_sink)
	{
		m_video_sink->set_format(m_format, m_bytesize);
	}
}

void sdl_camera_handler::set_frame_rate(u32 frame_rate)
{
	m_frame_rate = frame_rate;
}

void sdl_camera_handler::set_resolution(u32 width, u32 height)
{
	m_width = width;
	m_height = height;

	if (m_video_sink)
	{
		m_video_sink->set_resolution(m_width, m_height);
	}
}

void sdl_camera_handler::set_mirrored(bool mirrored)
{
	m_mirrored = mirrored;

	if (m_video_sink)
	{
		m_video_sink->set_mirrored(m_mirrored);
	}
}

u64 sdl_camera_handler::frame_number() const
{
	return m_video_sink ? m_video_sink->frame_number() : 0;
}

camera_handler_base::camera_handler_state sdl_camera_handler::get_image(u8* buf, u64 size, u32& width, u32& height, u64& frame_number, u64& bytes_read)
{
	width = 0;
	height = 0;
	frame_number = 0;
	bytes_read = 0;

	if (const std::string camera_id = g_cfg.io.sdl_camera_id.to_string();
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
		m_video_sink->get_image(buf, size, width, height, frame_number, bytes_read);
	}
	else
	{
		camera_log.error("Camera not running (m_state=%d)", static_cast<int>(current_state));
	}

	return current_state;
}

#endif
