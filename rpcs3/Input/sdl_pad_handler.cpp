#ifdef HAVE_SDL2

#include "stdafx.h"
#include "sdl_pad_handler.h"
#include "Emu/system_utils.hpp"
#include "Emu/system_config.h"

#include <mutex>

LOG_CHANNEL(sdl_log, "SDL");

struct sdl_instance
{
public:
	sdl_instance() = default;
	~sdl_instance()
	{
		// Only quit SDL once on exit. SDL uses a global state internally...
		if (m_initialized)
		{
			sdl_log.notice("Quitting SDL ...");
			SDL_Quit();
		}
	}

	bool initialize()
	{
		// Only init SDL once. SDL uses a global state internally...
		if (m_initialized)
		{
			return true;
		}

		sdl_log.notice("Initializing SDL ...");

		// Set non-dynamic hints before SDL_Init
		if (!SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1"))
		{
			sdl_log.error("Could not set SDL_HINT_JOYSTICK_THREAD: %s", SDL_GetError());
		}

		if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER) < 0)
		{
			sdl_log.error("Could not initialize! SDL Error: %s", SDL_GetError());
			return false;
		}

		SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
		SDL_LogSetOutputFunction([](void*, int category, SDL_LogPriority priority, const char* message)
		{
			std::string category_name;
			switch (category)
			{
			case SDL_LOG_CATEGORY_APPLICATION:
				category_name = "app";
				break;
			case SDL_LOG_CATEGORY_ERROR:
				category_name = "error";
				break;
			case SDL_LOG_CATEGORY_ASSERT:
				category_name = "assert";
				break;
			case SDL_LOG_CATEGORY_SYSTEM:
				category_name = "system";
				break;
			case SDL_LOG_CATEGORY_AUDIO:
				category_name = "audio";
				break;
			case SDL_LOG_CATEGORY_VIDEO:
				category_name = "video";
				break;
			case SDL_LOG_CATEGORY_RENDER:
				category_name = "render";
				break;
			case SDL_LOG_CATEGORY_INPUT:
				category_name = "input";
				break;
			case SDL_LOG_CATEGORY_TEST:
				category_name = "test";
				break;
			default:
				category_name = fmt::format("unknown(%d)", category);
				break;
			}

			switch (priority)
			{
			case SDL_LOG_PRIORITY_VERBOSE:
			case SDL_LOG_PRIORITY_DEBUG:
				sdl_log.trace("%s: %s", category_name, message);
				break;
			case SDL_LOG_PRIORITY_INFO:
				sdl_log.notice("%s: %s", category_name, message);
				break;
			case SDL_LOG_PRIORITY_WARN:
				sdl_log.warning("%s: %s", category_name, message);
				break;
			case SDL_LOG_PRIORITY_ERROR:
				sdl_log.error("%s: %s", category_name, message);
				break;
			case SDL_LOG_PRIORITY_CRITICAL:
				sdl_log.error("%s: %s", category_name, message);
				break;
			default:
				break;
			}
		}, nullptr);

		m_initialized = true;
		return true;
	}

private:
	bool m_initialized = false;
};

constexpr u32 rumble_duration_ms = 500; // Some high number to keep rumble updates at a minimum.
constexpr u32 rumble_refresh_ms = rumble_duration_ms - 100; // We need to keep updating the rumble. Choose a refresh timeout that is unlikely to run into missed rumble updates.

sdl_pad_handler::sdl_pad_handler() : PadHandlerBase(pad_handler::sdl)
{
	button_list =
	{
		{ SDLKeyCodes::None,     ""         },
		{ SDLKeyCodes::A,        "A"        },
		{ SDLKeyCodes::B,        "B"        },
		{ SDLKeyCodes::X,        "X"        },
		{ SDLKeyCodes::Y,        "Y"        },
		{ SDLKeyCodes::Left,     "Left"     },
		{ SDLKeyCodes::Right,    "Right"    },
		{ SDLKeyCodes::Up,       "Up"       },
		{ SDLKeyCodes::Down,     "Down"     },
		{ SDLKeyCodes::LB,       "LB"       },
		{ SDLKeyCodes::RB,       "RB"       },
		{ SDLKeyCodes::Back,     "Back"     },
		{ SDLKeyCodes::Start,    "Start"    },
		{ SDLKeyCodes::LS,       "LS"       },
		{ SDLKeyCodes::RS,       "RS"       },
		{ SDLKeyCodes::Guide,    "Guide"    },
		{ SDLKeyCodes::Misc1,    "Misc 1"   },
		{ SDLKeyCodes::Paddle1,  "Paddle 1" },
		{ SDLKeyCodes::Paddle2,  "Paddle 2" },
		{ SDLKeyCodes::Paddle3,  "Paddle 3" },
		{ SDLKeyCodes::Paddle4,  "Paddle 4" },
		{ SDLKeyCodes::Touchpad, "Touchpad" },
		{ SDLKeyCodes::LT,       "LT"       },
		{ SDLKeyCodes::RT,       "RT"       },
		{ SDLKeyCodes::LSXNeg,   "LS X-"    },
		{ SDLKeyCodes::LSXPos,   "LS X+"    },
		{ SDLKeyCodes::LSYPos,   "LS Y+"    },
		{ SDLKeyCodes::LSYNeg,   "LS Y-"    },
		{ SDLKeyCodes::RSXNeg,   "RS X-"    },
		{ SDLKeyCodes::RSXPos,   "RS X+"    },
		{ SDLKeyCodes::RSYPos,   "RS Y+"    },
		{ SDLKeyCodes::RSYNeg,   "RS Y-"    },
	};

	init_configs();

	// Define border values
	thumb_max = SDL_JOYSTICK_AXIS_MAX;
	trigger_min = 0;
	trigger_max = SDL_JOYSTICK_AXIS_MAX;

	// set capabilities
	b_has_config = true;
	b_has_deadzones = true;
	b_has_rumble = true;
	b_has_motion = true;
	b_has_led = true;
	b_has_rgb = true;
	b_has_battery = true;
	b_has_battery_led = true;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
}

sdl_pad_handler::~sdl_pad_handler()
{
	if (!m_is_init)
		return;

	for (auto& controller : m_controllers)
	{
		if (controller.second && controller.second->sdl.game_controller)
		{
			set_rumble(controller.second.get(), 0, 0);
			SDL_GameControllerClose(controller.second->sdl.game_controller);
			controller.second->sdl.game_controller = nullptr;
		}
	}
}

void sdl_pad_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
	cfg->ls_left.def  = ::at32(button_list, SDLKeyCodes::LSXNeg);
	cfg->ls_down.def  = ::at32(button_list, SDLKeyCodes::LSYNeg);
	cfg->ls_right.def = ::at32(button_list, SDLKeyCodes::LSXPos);
	cfg->ls_up.def    = ::at32(button_list, SDLKeyCodes::LSYPos);
	cfg->rs_left.def  = ::at32(button_list, SDLKeyCodes::RSXNeg);
	cfg->rs_down.def  = ::at32(button_list, SDLKeyCodes::RSYNeg);
	cfg->rs_right.def = ::at32(button_list, SDLKeyCodes::RSXPos);
	cfg->rs_up.def    = ::at32(button_list, SDLKeyCodes::RSYPos);
	cfg->start.def    = ::at32(button_list, SDLKeyCodes::Start);
	cfg->select.def   = ::at32(button_list, SDLKeyCodes::Back);
	cfg->ps.def       = ::at32(button_list, SDLKeyCodes::Guide);
	cfg->square.def   = ::at32(button_list, SDLKeyCodes::X);
	cfg->cross.def    = ::at32(button_list, SDLKeyCodes::A);
	cfg->circle.def   = ::at32(button_list, SDLKeyCodes::B);
	cfg->triangle.def = ::at32(button_list, SDLKeyCodes::Y);
	cfg->left.def     = ::at32(button_list, SDLKeyCodes::Left);
	cfg->down.def     = ::at32(button_list, SDLKeyCodes::Down);
	cfg->right.def    = ::at32(button_list, SDLKeyCodes::Right);
	cfg->up.def       = ::at32(button_list, SDLKeyCodes::Up);
	cfg->r1.def       = ::at32(button_list, SDLKeyCodes::RB);
	cfg->r2.def       = ::at32(button_list, SDLKeyCodes::RT);
	cfg->r3.def       = ::at32(button_list, SDLKeyCodes::RS);
	cfg->l1.def       = ::at32(button_list, SDLKeyCodes::LB);
	cfg->l2.def       = ::at32(button_list, SDLKeyCodes::LT);
	cfg->l3.def       = ::at32(button_list, SDLKeyCodes::LS);

	cfg->pressure_intensity_button.def = ::at32(button_list, SDLKeyCodes::None);

	// Set default misc variables
	cfg->lstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
	cfg->rstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
	cfg->lstickdeadzone.def    = 8000; // between 0 and SDL_JOYSTICK_AXIS_MAX
	cfg->rstickdeadzone.def    = 8000; // between 0 and SDL_JOYSTICK_AXIS_MAX
	cfg->ltriggerthreshold.def = 0; // between 0 and SDL_JOYSTICK_AXIS_MAX
	cfg->rtriggerthreshold.def = 0; // between 0 and SDL_JOYSTICK_AXIS_MAX
	cfg->lpadsquircling.def    = 8000;
	cfg->rpadsquircling.def    = 8000;

	// Set default color value
	cfg->colorR.def = 0;
	cfg->colorG.def = 0;
	cfg->colorB.def = 20;

	// Set default LED options
	cfg->led_battery_indicator.def            = false;
	cfg->led_battery_indicator_brightness.def = 10;
	cfg->led_low_battery_blink.def            = true;

	// apply defaults
	cfg->from_default();
}

bool sdl_pad_handler::Init()
{
	if (m_is_init)
		return true;

	static sdl_instance s_sdl_instance {};
	if (!s_sdl_instance.initialize())
		return false;

	if (g_cfg.io.load_sdl_mappings)
	{
		const std::string db_path = rpcs3::utils::get_input_config_root() + "gamecontrollerdb.txt";
		sdl_log.notice("Adding mappings from file '%s'", db_path);

		if (fs::is_file(db_path))
		{
			if (SDL_GameControllerAddMappingsFromFile(db_path.c_str()) < 0)
			{
				sdl_log.error("Could not add mappings from file '%s'! SDL Error: %s", db_path, SDL_GetError());
			}
		}
		else
		{
			sdl_log.warning("Could not add mappings from file '%s'! File does not exist!", db_path);
		}
	}

	SDL_version version{};
	SDL_GetVersion(&version);

	if (const char* revision = SDL_GetRevision(); revision && strlen(revision) > 0)
	{
		sdl_log.notice("Using version: %d.%d.%d (revision='%s')", version.major, version.minor, version.patch, revision);
	}
	else
	{
		sdl_log.notice("Using version: %d.%d.%d", version.major, version.minor, version.patch);
	}

	m_is_init = true;
	enumerate_devices();

	return true;
}

void sdl_pad_handler::process()
{
	if (!m_is_init)
		return;

	SDL_PumpEvents();

	PadHandlerBase::process();
}

SDLDevice::sdl_info sdl_pad_handler::get_sdl_info(int i)
{
	SDLDevice::sdl_info info{};
	info.game_controller = SDL_GameControllerOpen(i);

	if (!info.game_controller)
	{
		sdl_log.error("Could not open device %d! SDL Error: %s", i, SDL_GetError());
		return {};
	}

	if (const char* name = SDL_GameControllerName(info.game_controller))
	{
		info.name = name;
	}

	if (const char* path = SDL_GameControllerPath(info.game_controller))
	{
		info.path = path;
	}

	if (const char* serial = SDL_GameControllerGetSerial(info.game_controller))
	{
		info.serial = serial;
	}

	info.joystick = SDL_GameControllerGetJoystick(info.game_controller);
	info.type = SDL_GameControllerGetType(info.game_controller);
	info.vid = SDL_GameControllerGetVendor(info.game_controller);
	info.pid = SDL_GameControllerGetProduct(info.game_controller);
	info.product_version= SDL_GameControllerGetProductVersion(info.game_controller);
	info.firmware_version = SDL_GameControllerGetFirmwareVersion(info.game_controller);
	info.has_led = SDL_GameControllerHasLED(info.game_controller);
	info.has_rumble = SDL_GameControllerHasRumble(info.game_controller);
	info.has_rumble_triggers = SDL_GameControllerHasRumbleTriggers(info.game_controller);
	info.has_accel = SDL_GameControllerHasSensor(info.game_controller, SDL_SENSOR_ACCEL);
	info.has_gyro = SDL_GameControllerHasSensor(info.game_controller, SDL_SENSOR_GYRO);

	sdl_log.notice("Found game controller %d: type=%d, name='%s', path='%s', serial='%s', vid=0x%x, pid=0x%x, product_version=0x%x, firmware_version=0x%x, has_led=%d, has_rumble=%d, has_rumble_triggers=%d, has_accel=%d, has_gyro=%d",
		i, static_cast<int>(info.type), info.name, info.path, info.serial, info.vid, info.pid, info.product_version, info.firmware_version, info.has_led, info.has_rumble, info.has_rumble_triggers, info.has_accel, info.has_gyro);

	if (info.has_accel)
	{
		if (SDL_GameControllerSetSensorEnabled(info.game_controller, SDL_SENSOR_ACCEL, SDL_TRUE) != 0 ||
			!SDL_GameControllerIsSensorEnabled(info.game_controller, SDL_SENSOR_ACCEL))
		{
			sdl_log.error("Could not activate acceleration sensor of device %d! SDL Error: %s", i, SDL_GetError());
			info.has_accel = false;
		}
		else
		{
			info.data_rate_accel = SDL_GameControllerGetSensorDataRate(info.game_controller, SDL_SENSOR_ACCEL);
			sdl_log.notice("Acceleration sensor data rate of device %d = %.2f/s", i, info.data_rate_accel);
		}
	}

	if (info.has_gyro)
	{
		if (SDL_GameControllerSetSensorEnabled(info.game_controller, SDL_SENSOR_GYRO, SDL_TRUE) != 0 ||
			!SDL_GameControllerIsSensorEnabled(info.game_controller, SDL_SENSOR_GYRO))
		{
			sdl_log.error("Could not activate gyro sensor of device %d! SDL Error: %s", i, SDL_GetError());
			info.has_gyro = false;
		}
		else
		{
			info.data_rate_gyro = SDL_GameControllerGetSensorDataRate(info.game_controller, SDL_SENSOR_GYRO);
			sdl_log.notice("Gyro sensor data rate of device %d = %.2f/s", i, info.data_rate_accel);
		}
	}

	for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
	{
		const SDL_GameControllerButton button_id = static_cast<SDL_GameControllerButton>(i);
		if (SDL_GameControllerHasButton(info.game_controller, button_id))
		{
			info.button_ids.insert(button_id);
		}
	}

	for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; i++)
	{
		const SDL_GameControllerAxis axis_id = static_cast<SDL_GameControllerAxis>(i);
		if (SDL_GameControllerHasAxis(info.game_controller, axis_id))
		{
			info.axis_ids.insert(axis_id);
		}
	}

	return info;
}

std::vector<pad_list_entry> sdl_pad_handler::list_devices()
{
	std::vector<pad_list_entry> pads_list;

	if (!Init())
		return pads_list;

	for (const auto& controller : m_controllers)
	{
		pads_list.emplace_back(controller.first, false);
	}

	return pads_list;
}

void sdl_pad_handler::enumerate_devices()
{
	if (!m_is_init)
		return;

	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
		if (!SDL_IsGameController(i))
		{
			sdl_log.error("Joystick %d is not game controller interface compatible! SDL Error: %s", i, SDL_GetError());
			continue;
		}

		if (SDLDevice::sdl_info info = get_sdl_info(i); info.game_controller)
		{
			std::shared_ptr<SDLDevice> dev = std::make_shared<SDLDevice>();
			dev->sdl = std::move(info);

			// Count existing real devices with the same name
			u32 device_count = 1; // This device also counts
			for (const auto& controller : m_controllers)
			{
				if (controller.second && !controller.second->sdl.is_virtual_device && controller.second->sdl.name == dev->sdl.name)
				{
					device_count++;
				}
			}

			// Add real device
			const std::string device_name = fmt::format("%s %d", dev->sdl.name, device_count);
			m_controllers[device_name] = std::move(dev);
		}
	}
}

std::shared_ptr<SDLDevice> sdl_pad_handler::get_device_by_game_controller(SDL_GameController* game_controller) const
{
	if (!game_controller)
		return nullptr;

	const char* name = SDL_GameControllerName(game_controller);
	const char* path = SDL_GameControllerPath(game_controller);
	const char* serial = SDL_GameControllerGetSerial(game_controller);

	// Try to find a real device
	for (const auto& controller : m_controllers)
	{
		if (!controller.second || controller.second->sdl.is_virtual_device)
			continue;

		const auto is_same = [](const char* c, std::string_view s) -> bool
		{
			return c ? c == s : s.empty();
		};

		if (is_same(name, controller.second->sdl.name) &&
			is_same(path, controller.second->sdl.path) &&
			is_same(serial, controller.second->sdl.serial))
		{
			return controller.second;
		}
	}

	// Try to find a virtual device if we can't find a real device
	for (const auto& controller : m_controllers)
	{
		if (!controller.second || !controller.second->sdl.is_virtual_device)
			continue;

		if (name && controller.second->sdl.name.starts_with(name))
		{
			return controller.second;
		}
	}

	return nullptr;
}

std::shared_ptr<PadDevice> sdl_pad_handler::get_device(const std::string& device)
{
	if (!Init() || device.empty())
		return nullptr;

	if (auto it = m_controllers.find(device); it != m_controllers.end())
	{
		return it->second;
	}

	// Add a virtual controller until it is actually attached
	std::shared_ptr<SDLDevice> dev = std::make_unique<SDLDevice>();
	dev->sdl.name = device;
	dev->sdl.is_virtual_device = true;
	m_controllers.emplace(device, dev);
	sdl_log.warning("Adding empty device: %s", device);

	return dev;
}

PadHandlerBase::connection sdl_pad_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	if (SDLDevice* dev = static_cast<SDLDevice*>(device.get()))
	{
		if (dev->sdl.game_controller)
		{
			if (SDL_GameControllerGetAttached(dev->sdl.game_controller))
			{
				if (SDL_HasEvent(SDL_EventType::SDL_CONTROLLERBUTTONDOWN) ||
					SDL_HasEvent(SDL_EventType::SDL_CONTROLLERBUTTONUP) ||
					SDL_HasEvent(SDL_EventType::SDL_CONTROLLERAXISMOTION) ||
					SDL_HasEvent(SDL_EventType::SDL_CONTROLLERSENSORUPDATE) ||
					SDL_HasEvent(SDL_EventType::SDL_CONTROLLERTOUCHPADUP) ||
					SDL_HasEvent(SDL_EventType::SDL_CONTROLLERTOUCHPADDOWN) ||
					SDL_HasEvent(SDL_EventType::SDL_JOYBATTERYUPDATED))
				{
					return connection::connected;
				}

				return connection::no_data;
			}

			SDL_GameControllerClose(dev->sdl.game_controller);
			dev->sdl.game_controller = nullptr;
			dev->sdl.joystick = nullptr;
		}

		// Try to reconnect

		for (int i = 0; i < SDL_NumJoysticks(); i++)
		{
			if (!SDL_IsGameController(i))
			{
				continue;
			}

			// Get game controller
			SDL_GameController* game_controller = SDL_GameControllerOpen(i);
			if (!game_controller)
			{
				continue;
			}

			// Find out if we already know this controller
			std::shared_ptr<SDLDevice> sdl_device = get_device_by_game_controller(game_controller);
			if (!sdl_device)
			{
				// Close the game controller if we don't know it.
				SDL_GameControllerClose(game_controller);
				continue;
			}

			// Re-attach the controller if the device matches the current one
			if (sdl_device.get() == dev)
			{
				if (SDLDevice::sdl_info info = get_sdl_info(i); info.game_controller)
				{
					dev->sdl = std::move(info);
				}
				break;
			}
		}
	}

	return connection::disconnected;
}

void sdl_pad_handler::SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool /*player_led*/, bool battery_led, u32 battery_led_brightness)
{
	std::shared_ptr<PadDevice> device = get_device(padId);
	SDLDevice* dev = static_cast<SDLDevice*>(device.get());
	if (!dev)
		return;

	dev->player_id = player_id;
	dev->config = get_config(padId);

	ensure(dev->config);

	set_rumble(dev, large_motor, small_motor);

	if (battery_led)
	{
		const u32 combined_color = get_battery_color(dev->sdl.power_level, battery_led_brightness);
		dev->config->colorR.set(combined_color >> 8);
		dev->config->colorG.set(combined_color & 0xff);
		dev->config->colorB.set(0);
	}
	else if (r >= 0 && g >= 0 && b >= 0 && r <= 255 && g <= 255 && b <= 255)
	{
		dev->config->colorR.set(r);
		dev->config->colorG.set(g);
		dev->config->colorB.set(b);
	}

	if (dev->sdl.has_led && SDL_GameControllerSetLED(dev->sdl.game_controller, r, g, b) != 0)
	{
		sdl_log.error("Could not set LED of device %d! SDL Error: %s", player_id, SDL_GetError());
	}
}

u32 sdl_pad_handler::get_battery_color(SDL_JoystickPowerLevel power_level, u32 brightness) const
{
	u32 combined_color{};

	switch (power_level)
	{
	default:                         combined_color = 0xFF00; break;
	case SDL_JOYSTICK_POWER_UNKNOWN: combined_color = 0xFF00; break;
	case SDL_JOYSTICK_POWER_EMPTY:   combined_color = 0xFF33; break;
	case SDL_JOYSTICK_POWER_LOW:     combined_color = 0xFFCC; break;
	case SDL_JOYSTICK_POWER_MEDIUM:  combined_color = 0x66FF; break;
	case SDL_JOYSTICK_POWER_FULL:    combined_color = 0x00FF; break;
	case SDL_JOYSTICK_POWER_WIRED:   combined_color = 0x00FF; break;
	case SDL_JOYSTICK_POWER_MAX:     combined_color = 0x00FF; break;
	}

	const u32 red = (combined_color >> 8) * brightness / 100;
	const u32 green = (combined_color & 0xff) * brightness / 100;
	return ((red << 8) | green);
}

u32 sdl_pad_handler::get_battery_level(const std::string& padId)
{
	std::shared_ptr<PadDevice> device = get_device(padId);
	SDLDevice* dev = static_cast<SDLDevice*>(device.get());
	if (!dev)
		return 0;

	if (dev->sdl.joystick)
	{
		dev->sdl.power_level = SDL_JoystickCurrentPowerLevel(dev->sdl.joystick);

		switch (dev->sdl.power_level)
		{
		case SDL_JOYSTICK_POWER_UNKNOWN: return 0;
		case SDL_JOYSTICK_POWER_EMPTY:   return 5;
		case SDL_JOYSTICK_POWER_LOW:     return 20;
		case SDL_JOYSTICK_POWER_MEDIUM:  return 70;
		case SDL_JOYSTICK_POWER_FULL:    return 100;
		case SDL_JOYSTICK_POWER_WIRED:   return 100;
		case SDL_JOYSTICK_POWER_MAX:     return 100;
		default:                         return 0;
		}
	}

	 return 0;
}

void sdl_pad_handler::get_extended_info(const pad_ensemble& binding)
{
	const auto& pad = binding.pad;
	SDLDevice* dev = static_cast<SDLDevice*>(binding.device.get());
	if (!dev || !dev->sdl.game_controller || !pad)
		return;

	if (dev->sdl.joystick)
	{
		dev->sdl.power_level = SDL_JoystickCurrentPowerLevel(dev->sdl.joystick);
		pad->m_cable_state = dev->sdl.power_level == SDL_JOYSTICK_POWER_WIRED;

		switch (dev->sdl.power_level)
		{
		case SDL_JOYSTICK_POWER_UNKNOWN: pad->m_battery_level = 0; break;
		case SDL_JOYSTICK_POWER_EMPTY:   pad->m_battery_level = 5; break;
		case SDL_JOYSTICK_POWER_LOW:     pad->m_battery_level = 20; break;
		case SDL_JOYSTICK_POWER_MEDIUM:  pad->m_battery_level = 70; break;
		case SDL_JOYSTICK_POWER_FULL:    pad->m_battery_level = 100; break;
		case SDL_JOYSTICK_POWER_WIRED:   pad->m_battery_level = 100; break;
		case SDL_JOYSTICK_POWER_MAX:     pad->m_battery_level = 100; break;
		default:                         pad->m_battery_level = 0; break;
		}
	}
	else
	{
		pad->m_cable_state = 1;
		pad->m_battery_level = 100;
	}

	if (dev->sdl.has_accel)
	{
		if (SDL_GameControllerGetSensorData(dev->sdl.game_controller, SDL_SENSOR_ACCEL, dev->values_accel.data(), 3) != 0)
		{
			sdl_log.error("Could not get acceleration sensor data of device %d! SDL Error: %s", dev->player_id, SDL_GetError());
		}
		else
		{
			const float& accel_x = dev->values_accel[0]; // Angular speed around the x axis (pitch)
			const float& accel_y = dev->values_accel[1]; // Angular speed around the y axis (yaw)
			const float& accel_z = dev->values_accel[2]; // Angular speed around the z axis (roll

			// Convert to ds3. The ds3 resolution is 113/G.
			pad->m_sensors[0].m_value = Clamp0To1023((accel_x / SDL_STANDARD_GRAVITY) * -1 * 113 + 512);
			pad->m_sensors[1].m_value = Clamp0To1023((accel_y / SDL_STANDARD_GRAVITY) * -1 * 113 + 512);
			pad->m_sensors[2].m_value = Clamp0To1023((accel_z / SDL_STANDARD_GRAVITY) * -1 * 113 + 512);
		}
	}

	if (dev->sdl.has_gyro)
	{
		if (SDL_GameControllerGetSensorData(dev->sdl.game_controller, SDL_SENSOR_GYRO, dev->values_gyro.data(), 3) != 0)
		{
			sdl_log.error("Could not get gyro sensor data of device %d! SDL Error: %s", dev->player_id, SDL_GetError());
		}
		else
		{
			//const float& gyro_x = dev->values_gyro[0]; // Angular speed around the x axis (pitch)
			const float& gyro_y = dev->values_gyro[1]; // Angular speed around the y axis (yaw)
			//const float& gyro_z = dev->values_gyro[2]; // Angular speed around the z axis (roll)

			// Convert to ds3. The ds3 resolution is 123/90°/sec. The SDL gyro is measured in rad/sec.
			static constexpr f32 PI = 3.14159265f;
			const float degree = (gyro_y * 180.0f / PI);
			pad->m_sensors[3].m_value = Clamp0To1023(degree * (123.f / 90.f) + 512);
		}
	}
}

void sdl_pad_handler::get_motion_sensors(const std::string& pad_id, const motion_callback& callback, const motion_fail_callback& fail_callback, motion_preview_values preview_values, const std::array<AnalogSensor, 4>& sensors)
{
	if (!m_is_init)
		return;

	SDL_PumpEvents();

	PadHandlerBase::get_motion_sensors(pad_id, callback, fail_callback, preview_values, sensors);
}

PadHandlerBase::connection sdl_pad_handler::get_next_button_press(const std::string& padId, const pad_callback& callback, const pad_fail_callback& fail_callback, bool get_blacklist, const std::vector<std::string>& buttons)
{
	if (!m_is_init)
		return connection::disconnected;

	SDL_PumpEvents();

	return PadHandlerBase::get_next_button_press(padId, callback, fail_callback, get_blacklist, buttons);
}

void sdl_pad_handler::apply_pad_data(const pad_ensemble& binding)
{
	const auto& pad = binding.pad;
	SDLDevice* dev = static_cast<SDLDevice*>(binding.device.get());
	if (!dev || !pad)
		return;

	cfg_pad* cfg = dev->config;

	// The left motor is the low-frequency rumble motor. The right motor is the high-frequency rumble motor.
	// The two motors are not the same, and they create different vibration effects. Values range between 0 to 65535.
	const usz idx_l = cfg->switch_vibration_motors ? 1 : 0;
	const usz idx_s = cfg->switch_vibration_motors ? 0 : 1;

	if (dev->sdl.has_rumble || dev->sdl.has_rumble_triggers)
	{
		const u8 speed_large = cfg->enable_vibration_motor_large ? pad->m_vibrateMotors[idx_l].m_value : 0;
		const u8 speed_small = cfg->enable_vibration_motor_small ? pad->m_vibrateMotors[idx_s].m_value : 0;

		dev->has_new_rumble_data |= dev->large_motor != speed_large || dev->small_motor != speed_small;

		dev->large_motor = speed_large;
		dev->small_motor = speed_small;

		const steady_clock::time_point now = steady_clock::now();
		const s64 elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - dev->last_vibration).count();

		// XBox One Controller can't handle faster vibration updates than ~10ms. Elite is even worse. So I'll use 20ms to be on the safe side. No lag was noticable.
		if ((dev->has_new_rumble_data && elapsed_ms > 20) || (elapsed_ms > rumble_refresh_ms))
		{
			set_rumble(dev, speed_large, speed_small);

			dev->has_new_rumble_data = false;
			dev->last_vibration = steady_clock::now();
		}
	}

	if (dev->sdl.has_led)
	{
		// Use LEDs to indicate battery level
		if (cfg->led_battery_indicator)
		{
			// This makes sure that the LED color doesn't update every 1ms. DS4 only reports battery level in 10% increments
			if (dev->sdl.last_power_level != dev->sdl.power_level)
			{
				const u32 combined_color = get_battery_color(dev->sdl.power_level, cfg->led_battery_indicator_brightness);
				cfg->colorR.set(combined_color >> 8);
				cfg->colorG.set(combined_color & 0xff);
				cfg->colorB.set(0);

				dev->sdl.last_power_level = dev->sdl.power_level;
				dev->led_needs_update = true;
			}
		}

		// Blink LED when battery is low
		if (cfg->led_low_battery_blink)
		{
			const bool low_battery = pad->m_battery_level <= 20;

			if (dev->led_is_blinking && !low_battery)
			{
				dev->led_is_blinking = false;
				dev->led_needs_update = true;
			}
			else if (!dev->led_is_blinking && low_battery)
			{
				dev->led_is_blinking = true;
				dev->led_needs_update = true;
			}

			if (dev->led_is_blinking)
			{
				if (const steady_clock::time_point now = steady_clock::now(); (now - dev->led_timestamp) > 500ms)
				{
					dev->led_is_on = !dev->led_is_on;
					dev->led_timestamp = now;
					dev->led_needs_update = true;
				}
			}
		}
		else if (!dev->led_is_on)
		{
			dev->led_is_on = true;
			dev->led_needs_update = true;
		}

		if (dev->led_needs_update)
		{
			const u8 r = dev->led_is_on ? cfg->colorR : 0;
			const u8 g = dev->led_is_on ? cfg->colorG : 0;
			const u8 b = dev->led_is_on ? cfg->colorB : 0;

			if (SDL_GameControllerSetLED(dev->sdl.game_controller, r, g, b) != 0)
			{
				sdl_log.error("Could not set LED of device %d! SDL Error: %s", dev->player_id, SDL_GetError());
			}

			dev->led_needs_update = false;
		}
	}
}

void sdl_pad_handler::set_rumble(SDLDevice* dev, u8 speed_large, u8 speed_small)
{
	if (!dev || !dev->sdl.game_controller) return;

	if (dev->sdl.has_rumble)
	{
		if (SDL_GameControllerRumble(dev->sdl.game_controller, speed_large * 257, speed_small * 257, rumble_duration_ms) != 0)
		{
			sdl_log.error("Unable to play game controller rumble of player %d! SDL Error: %s", dev->player_id, SDL_GetError());
		}
	}

	// NOTE: Disabled for now. The triggers do not seem to be implemented correctly in SDL in the current version.
	//       The rumble is way too intensive and has a high frequency. It is very displeasing at the moment.
	//       In the future we could use additional trigger rumble to enhance the existing rumble.
	if (false && dev->sdl.has_rumble_triggers)
	{
		// Only the large motor shall control both triggers. It wouldn't make sense to differentiate here.
		if (SDL_GameControllerRumbleTriggers(dev->sdl.game_controller, speed_large * 257, speed_large * 257, rumble_duration_ms) != 0)
		{
			sdl_log.error("Unable to play game controller trigger rumble of player %d! SDL Error: %s", dev->player_id, SDL_GetError());
		}
	}
}

bool sdl_pad_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return keyCode == SDLKeyCodes::LT;
}

bool sdl_pad_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return keyCode == SDLKeyCodes::RT;
}

bool sdl_pad_handler::get_is_left_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	switch (keyCode)
	{
	case SDLKeyCodes::LSXNeg:
	case SDLKeyCodes::LSXPos:
	case SDLKeyCodes::LSYPos:
	case SDLKeyCodes::LSYNeg:
		return true;
	default:
		return false;
	}
}

bool sdl_pad_handler::get_is_right_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	switch (keyCode)
	{
	case SDLKeyCodes::RSXNeg:
	case SDLKeyCodes::RSXPos:
	case SDLKeyCodes::RSYPos:
	case SDLKeyCodes::RSYNeg:
		return true;
	default:
		return false;
	}
}

std::unordered_map<u64, u16> sdl_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> values;
	SDLDevice* dev = static_cast<SDLDevice*>(device.get());
	if (!dev || !dev->sdl.game_controller)
		return values;

	for (SDL_GameControllerButton button_id : dev->sdl.button_ids)
	{
		const u8 value = SDL_GameControllerGetButton(dev->sdl.game_controller, button_id);
		const SDLKeyCodes key_code = get_button_code(button_id);

		// TODO: SDL does not support DS3 button intensity in the current version
		values[key_code] = value ? 255 : 0;
	}

	for (SDL_GameControllerAxis axis_id : dev->sdl.axis_ids)
	{
		const s16 value = SDL_GameControllerGetAxis(dev->sdl.game_controller, axis_id);

		switch (axis_id)
		{
		case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT:
			values[SDLKeyCodes::LT] = std::max<u16>(0, value);
			break;
		case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
			values[SDLKeyCodes::RT] = std::max<u16>(0, value);
			break;
		case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX:
			values[SDLKeyCodes::LSXNeg] = value < 0 ? std::abs(value) - 1 : 0;
			values[SDLKeyCodes::LSXPos] = value > 0 ? value : 0;
			break;
		case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY:
			values[SDLKeyCodes::LSYNeg] = value > 0 ? value : 0;
			values[SDLKeyCodes::LSYPos] = value < 0 ? std::abs(value) - 1 : 0;
			break;
		case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX:
			values[SDLKeyCodes::RSXNeg] = value < 0 ? std::abs(value) - 1 : 0;
			values[SDLKeyCodes::RSXPos] = value > 0 ? value : 0;
			break;
		case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY:
			values[SDLKeyCodes::RSYNeg] = value > 0 ? value : 0;
			values[SDLKeyCodes::RSYPos] = value < 0 ? std::abs(value) - 1 : 0;
			break;
		default:
			break;
		}
	}

	return values;
}

pad_preview_values sdl_pad_handler::get_preview_values(const std::unordered_map<u64, u16>& data)
{
	return {
		::at32(data, LT),
		::at32(data, RT),
		::at32(data, LSXPos) - ::at32(data, LSXNeg),
		::at32(data, LSYPos) - ::at32(data, LSYNeg),
		::at32(data, RSXPos) - ::at32(data, RSXNeg),
		::at32(data, RSYPos) - ::at32(data, RSYNeg)
	};
}

std::string sdl_pad_handler::button_to_string(SDL_GameControllerButton button)
{
	if (const char* name = SDL_GameControllerGetStringForButton(button))
	{
		return name;
	}

	return {};
}

std::string sdl_pad_handler::axis_to_string(SDL_GameControllerAxis axis)
{
	if (const char* name = SDL_GameControllerGetStringForAxis(axis))
	{
		return name;
	}

	return {};
}

sdl_pad_handler::SDLKeyCodes sdl_pad_handler::get_button_code(SDL_GameControllerButton button)
{
	switch (button)
	{
	default:
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID: return SDLKeyCodes::None;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A: return SDLKeyCodes::A;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B: return SDLKeyCodes::B;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X: return SDLKeyCodes::X;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y: return SDLKeyCodes::Y;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT: return SDLKeyCodes::Left;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return SDLKeyCodes::Right;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP: return SDLKeyCodes::Up;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN: return SDLKeyCodes::Down;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return SDLKeyCodes::LB;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return SDLKeyCodes::RB;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_BACK: return SDLKeyCodes::Back;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START: return SDLKeyCodes::Start;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSTICK: return SDLKeyCodes::LS;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSTICK: return SDLKeyCodes::RS;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_GUIDE: return SDLKeyCodes::Guide;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MISC1: return SDLKeyCodes::Misc1;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_PADDLE1: return SDLKeyCodes::Paddle1;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_PADDLE2: return SDLKeyCodes::Paddle2;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_PADDLE3: return SDLKeyCodes::Paddle3;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_PADDLE4: return SDLKeyCodes::Paddle4;
	case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_TOUCHPAD: return SDLKeyCodes::Touchpad;
	}
}

#endif
