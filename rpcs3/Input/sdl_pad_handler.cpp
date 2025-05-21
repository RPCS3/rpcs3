#ifdef HAVE_SDL3

#include "stdafx.h"
#include "sdl_pad_handler.h"
#include "sdl_instance.h"
#include "Emu/system_utils.hpp"
#include "Emu/system_config.h"
#include "Emu/System.h"

#include <mutex>

LOG_CHANNEL(sdl_log, "SDL");

template <>
void fmt_class_string<SDL_GUID>::format(std::string& out, u64 arg)
{
	const SDL_GUID& guid = get_object(arg);
	char str[sizeof(SDL_GUID) * 2 + 1] {};
	SDL_GUIDToString(guid, str, sizeof(str));
	fmt::append(out, "%s", str);
}

sdl_pad_handler::sdl_pad_handler() : PadHandlerBase(pad_handler::sdl)
{
	button_list =
	{
		{ SDLKeyCodes::None,     ""         },
		{ SDLKeyCodes::South,    "South"    },
		{ SDLKeyCodes::East,     "East"     },
		{ SDLKeyCodes::West,     "West"     },
		{ SDLKeyCodes::North,    "North"    },
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
		{ SDLKeyCodes::Misc2,    "Misc 2"   },
		{ SDLKeyCodes::Misc3,    "Misc 3"   },
		{ SDLKeyCodes::Misc4,    "Misc 4"   },
		{ SDLKeyCodes::Misc5,    "Misc 5"   },
		{ SDLKeyCodes::Misc6,    "Misc 6"   },
		{ SDLKeyCodes::RPaddle1, "R Paddle 1" },
		{ SDLKeyCodes::LPaddle1, "L Paddle 1" },
		{ SDLKeyCodes::RPaddle2, "R Paddle 2" },
		{ SDLKeyCodes::LPaddle2, "L Paddle 2" },
		{ SDLKeyCodes::Touchpad, "Touchpad" },
		{ SDLKeyCodes::Touch_L,  "Touch Left" },
		{ SDLKeyCodes::Touch_R,  "Touch Right" },
		{ SDLKeyCodes::Touch_U,  "Touch Up" },
		{ SDLKeyCodes::Touch_D,  "Touch Down" },
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
		{ SDLKeyCodes::PressureCross,    "South" }, // Same name as non-pressure button
		{ SDLKeyCodes::PressureCircle,   "East" },  // Same name as non-pressure button
		{ SDLKeyCodes::PressureSquare,   "West" },  // Same name as non-pressure button
		{ SDLKeyCodes::PressureTriangle, "North" }, // Same name as non-pressure button
		{ SDLKeyCodes::PressureL1,       "LB" },    // Same name as non-pressure button
		{ SDLKeyCodes::PressureR1,       "RB" },    // Same name as non-pressure button
		{ SDLKeyCodes::PressureUp,       "Up" },    // Same name as non-pressure button
		{ SDLKeyCodes::PressureDown,     "Down" },  // Same name as non-pressure button
		{ SDLKeyCodes::PressureLeft,     "Left" },  // Same name as non-pressure button
		{ SDLKeyCodes::PressureRight,    "Right" }, // Same name as non-pressure button
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
	b_has_player_led = true;
	b_has_rgb = true;
	b_has_battery = true;
	b_has_battery_led = true;
	b_has_orientation = true;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
}

sdl_pad_handler::~sdl_pad_handler()
{
	if (!m_is_init)
		return;

	for (auto& controller : m_controllers)
	{
		if (controller.second && controller.second->sdl.gamepad)
		{
			set_rumble(controller.second.get(), 0, 0);
			SDL_CloseGamepad(controller.second->sdl.gamepad);
			controller.second->sdl.gamepad = nullptr;
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
	cfg->square.def   = ::at32(button_list, SDLKeyCodes::West);
	cfg->cross.def    = ::at32(button_list, SDLKeyCodes::South);
	cfg->circle.def   = ::at32(button_list, SDLKeyCodes::East);
	cfg->triangle.def = ::at32(button_list, SDLKeyCodes::North);
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
	cfg->analog_limiter_button.def = ::at32(button_list, SDLKeyCodes::None);
	cfg->orientation_reset_button.def = ::at32(button_list, SDLKeyCodes::None);

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

	if (!sdl_instance::get_instance().initialize())
		return false;

	if (g_cfg.io.load_sdl_mappings)
	{
		const std::string db_path = rpcs3::utils::get_input_config_root() + "gamecontrollerdb.txt";
		sdl_log.notice("Adding mappings from file '%s'", db_path);

		if (fs::is_file(db_path))
		{
			if (SDL_AddGamepadMappingsFromFile(db_path.c_str()) < 0)
			{
				sdl_log.error("Could not add mappings from file '%s'! SDL Error: %s", db_path, SDL_GetError());
			}
		}
		else
		{
			sdl_log.warning("Could not add mappings from file '%s'! File does not exist!", db_path);
		}
	}

	if (const char* revision = SDL_GetRevision(); revision && strlen(revision) > 0)
	{
		sdl_log.notice("Using version: %d.%d.%d (revision='%s')", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION, revision);
	}
	else
	{
		sdl_log.notice("Using version: %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
	}

	m_is_init = true;
	enumerate_devices();

	return true;
}

void sdl_pad_handler::process()
{
	if (!m_is_init)
		return;

	sdl_instance::get_instance().pump_events();

	PadHandlerBase::process();
}

SDLDevice::sdl_info sdl_pad_handler::get_sdl_info(SDL_JoystickID id)
{
	SDLDevice::sdl_info info{};
	info.gamepad = SDL_OpenGamepad(id);

	if (!info.gamepad)
	{
		sdl_log.error("Could not open device %d! SDL Error: %s", id, SDL_GetError());
		return {};
	}

	if (const char* name = SDL_GetGamepadName(info.gamepad))
	{
		info.name = name;
	}

	if (const char* path = SDL_GetGamepadPath(info.gamepad))
	{
		info.path = path;
	}

	if (const char* serial = SDL_GetGamepadSerial(info.gamepad))
	{
		info.serial = serial;
	}

	const SDL_PropertiesID property_id = SDL_GetGamepadProperties(info.gamepad);
	if (!property_id)
	{
		sdl_log.error("Could not get properties of device %d! SDL Error: %s", id, SDL_GetError());
	}

	info.type = SDL_GetGamepadType(info.gamepad);
	info.real_type = SDL_GetRealGamepadType(info.gamepad);
	info.guid = SDL_GetGamepadGUIDForID(id);
	info.vid = SDL_GetGamepadVendor(info.gamepad);
	info.pid = SDL_GetGamepadProduct(info.gamepad);
	info.product_version = SDL_GetGamepadProductVersion(info.gamepad);
	info.firmware_version = SDL_GetGamepadFirmwareVersion(info.gamepad);
	info.has_led = SDL_HasProperty(property_id, SDL_PROP_GAMEPAD_CAP_RGB_LED_BOOLEAN);
	info.has_mono_led = SDL_HasProperty(property_id, SDL_PROP_GAMEPAD_CAP_MONO_LED_BOOLEAN);
	info.has_player_led = SDL_HasProperty(property_id, SDL_PROP_GAMEPAD_CAP_PLAYER_LED_BOOLEAN);
	info.has_rumble = SDL_HasProperty(property_id, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN);
	info.has_rumble_triggers = SDL_HasProperty(property_id, SDL_PROP_GAMEPAD_CAP_TRIGGER_RUMBLE_BOOLEAN);
	info.has_accel = SDL_GamepadHasSensor(info.gamepad, SDL_SENSOR_ACCEL);
	info.has_gyro = SDL_GamepadHasSensor(info.gamepad, SDL_SENSOR_GYRO);

	if (const int num_touchpads = SDL_GetNumGamepadTouchpads(info.gamepad); num_touchpads > 0)
	{
		info.touchpads.resize(num_touchpads);

		for (int i = 0; i < num_touchpads; i++)
		{
			SDLDevice::touchpad& touchpad = ::at32(info.touchpads, i);
			touchpad.index = i;

			if (const int num_fingers = SDL_GetNumGamepadTouchpadFingers(info.gamepad, touchpad.index); num_fingers > 0)
			{
				touchpad.fingers.resize(num_fingers);

				for (int f = 0; f < num_fingers; f++)
				{
					::at32(touchpad.fingers, f).index = f;
				}
			}
		}
	}

	sdl_log.notice("Found game pad %d: type=%d, real_type=%d, name='%s', guid='%s', path='%s', serial='%s', vid=0x%x, pid=0x%x, product_version=0x%x, firmware_version=0x%x, has_led=%d, has_player_led=%d, has_mono_led=%d, has_rumble=%d, has_rumble_triggers=%d, has_accel=%d, has_gyro=%d",
		id, static_cast<int>(info.type), static_cast<int>(info.real_type), info.name, info.guid, info.path, info.serial, info.vid, info.pid, info.product_version, info.firmware_version, info.has_led, info.has_player_led, info.has_mono_led, info.has_rumble, info.has_rumble_triggers, info.has_accel, info.has_gyro);

	if (info.has_accel)
	{
		if (!SDL_SetGamepadSensorEnabled(info.gamepad, SDL_SENSOR_ACCEL, true) ||
			!SDL_GamepadSensorEnabled(info.gamepad, SDL_SENSOR_ACCEL))
		{
			sdl_log.error("Could not activate acceleration sensor of device %d! SDL Error: %s", id, SDL_GetError());
			info.has_accel = false;
		}
		else
		{
			info.data_rate_accel = SDL_GetGamepadSensorDataRate(info.gamepad, SDL_SENSOR_ACCEL);
			sdl_log.notice("Acceleration sensor data rate of device %d = %.2f/s", id, info.data_rate_accel);
		}
	}

	if (info.has_gyro)
	{
		if (!SDL_SetGamepadSensorEnabled(info.gamepad, SDL_SENSOR_GYRO, true) ||
			!SDL_GamepadSensorEnabled(info.gamepad, SDL_SENSOR_GYRO))
		{
			sdl_log.error("Could not activate gyro sensor of device %d! SDL Error: %s", id, SDL_GetError());
			info.has_gyro = false;
		}
		else
		{
			info.data_rate_gyro = SDL_GetGamepadSensorDataRate(info.gamepad, SDL_SENSOR_GYRO);
			sdl_log.notice("Gyro sensor data rate of device %d = %.2f/s", id, info.data_rate_accel);
		}
	}

	for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
	{
		const SDL_GamepadButton button_id = static_cast<SDL_GamepadButton>(i);
		if (SDL_GamepadHasButton(info.gamepad, button_id))
		{
			info.button_ids.insert(button_id);
		}
	}

	for (int i = 0; i < SDL_GAMEPAD_AXIS_COUNT; i++)
	{
		const SDL_GamepadAxis axis_id = static_cast<SDL_GamepadAxis>(i);
		if (SDL_GamepadHasAxis(info.gamepad, axis_id))
		{
			info.axis_ids.insert(axis_id);
		}
	}

	// The DS3 may have extra pressure sensitive buttons as axis
	if (info.real_type == SDL_GamepadType::SDL_GAMEPAD_TYPE_PS3)
	{
		if (SDL_Joystick* joystick = SDL_GetGamepadJoystick(info.gamepad))
		{
			const int num_axes = SDL_GetNumJoystickAxes(joystick);
			const int num_buttons = SDL_GetNumJoystickButtons(joystick);

			info.is_ds3_with_pressure_buttons = num_axes == 16 && num_buttons == 11;

			sdl_log.notice("DS3 device %d has %d axis and %d buttons (has_pressure_buttons=%d)", id, num_axes, num_buttons, info.is_ds3_with_pressure_buttons);

			if (info.is_ds3_with_pressure_buttons)
			{
				// Add pressure buttons
				for (int i = SDL_GAMEPAD_AXIS_COUNT; i < num_axes; i++)
				{
					const SDL_GamepadAxis axis_id = static_cast<SDL_GamepadAxis>(i);
					//if (SDL_GamepadHasAxis(info.gamepad, axis_id)) // Always returns false for axis >= SDL_GAMEPAD_AXIS_COUNT
					{
						info.axis_ids.insert(axis_id);
					}
				}
			}
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

	int count = 0;
	SDL_JoystickID* gamepads = SDL_GetGamepads(&count);
	for (int i = 0; i < count && gamepads; i++)
	{
		if (SDLDevice::sdl_info info = get_sdl_info(gamepads[i]); info.gamepad)
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
	SDL_free(gamepads);
}

std::shared_ptr<SDLDevice> sdl_pad_handler::get_device_by_gamepad(SDL_Gamepad* gamepad) const
{
	if (!gamepad)
		return nullptr;

	const char* name = SDL_GetGamepadName(gamepad);
	const char* path = SDL_GetGamepadPath(gamepad);
	const char* serial = SDL_GetGamepadSerial(gamepad);

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
		if (dev->sdl.gamepad)
		{
			if (SDL_GamepadConnected(dev->sdl.gamepad))
			{
				if (SDL_HasEvents(SDL_EventType::SDL_EVENT_GAMEPAD_AXIS_MOTION, SDL_EventType::SDL_EVENT_GAMEPAD_BUTTON_UP) ||
					SDL_HasEvents(SDL_EventType::SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN, SDL_EventType::SDL_EVENT_GAMEPAD_SENSOR_UPDATE) ||
					SDL_HasEvent(SDL_EventType::SDL_EVENT_JOYSTICK_BATTERY_UPDATED))
				{
					return connection::connected;
				}

				return connection::no_data;
			}

			SDL_CloseGamepad(dev->sdl.gamepad);
			dev->sdl.gamepad = nullptr;
		}

		// Try to reconnect

		int count = 0;
		SDL_JoystickID* gamepads = SDL_GetGamepads(&count);
		for (int i = 0; i < count; i++)
		{
			// Get game pad
			SDL_Gamepad* gamepad = SDL_OpenGamepad(gamepads[i]);
			if (!gamepad)
			{
				continue;
			}

			// Find out if we already know this controller
			std::shared_ptr<SDLDevice> sdl_device = get_device_by_gamepad(gamepad);
			if (!sdl_device)
			{
				// Close the game pad if we don't know it.
				SDL_CloseGamepad(gamepad);
				continue;
			}

			// Re-attach the controller if the device matches the current one
			if (sdl_device.get() == dev)
			{
				if (SDLDevice::sdl_info info = get_sdl_info(gamepads[i]); info.gamepad)
				{
					dev->sdl = std::move(info);
				}
				break;
			}
		}
		SDL_free(gamepads);
	}

	return connection::disconnected;
}

void sdl_pad_handler::SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool player_led, bool battery_led, u32 battery_led_brightness)
{
	std::shared_ptr<PadDevice> device = get_device(padId);
	SDLDevice* dev = static_cast<SDLDevice*>(device.get());
	if (!dev)
		return;

	dev->player_id = player_id;
	dev->config = get_config(padId);

	ensure(dev->config);

	set_rumble(dev, large_motor, small_motor);

	dev->update_player_leds = true;
	dev->config->player_led_enabled.set(player_led);

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

	if ((dev->sdl.has_led || dev->sdl.has_mono_led) && !SDL_SetGamepadLED(dev->sdl.gamepad, r, g, b))
	{
		sdl_log.error("Could not set LED of device %d! SDL Error: %s", player_id, SDL_GetError());
	}

	if (dev->sdl.has_player_led && !SDL_SetGamepadPlayerIndex(dev->sdl.gamepad, player_led ? player_id : -1))
	{
		sdl_log.error("Could not set player LED of device %d! SDL Error: %s", player_id, SDL_GetError());
	}
}

u32 sdl_pad_handler::get_battery_color(int power_level, u32 brightness) const
{
	u32 combined_color{};

	if (power_level < 20)      combined_color = 0xFF00;
	else if (power_level < 40) combined_color = 0xFF33;
	else if (power_level < 60) combined_color = 0xFFCC;
	else if (power_level < 80) combined_color = 0x66FF;
	else                       combined_color = 0x00FF;

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

	if (dev->sdl.gamepad)
	{
		const SDL_PowerState power_state = SDL_GetGamepadPowerInfo(dev->sdl.gamepad, &dev->sdl.power_level);
		switch (power_state)
		{
		case SDL_PowerState::SDL_POWERSTATE_ERROR:
		case SDL_PowerState::SDL_POWERSTATE_UNKNOWN:
			return 0;
		default:
			return std::clamp(dev->sdl.power_level, 0, 100);
		}
	}

	 return 0;
}

void sdl_pad_handler::get_extended_info(const pad_ensemble& binding)
{
	const auto& pad = binding.pad;
	SDLDevice* dev = static_cast<SDLDevice*>(binding.device.get());
	if (!dev || !dev->sdl.gamepad || !pad)
		return;

	if (dev->sdl.gamepad)
	{
		const SDL_PowerState power_state = SDL_GetGamepadPowerInfo(dev->sdl.gamepad, &dev->sdl.power_level);
		switch (power_state)
		{
		case SDL_PowerState::SDL_POWERSTATE_ON_BATTERY:
			pad->m_battery_level = std::clamp(dev->sdl.power_level, 0, 100);
			pad->m_cable_state = 0;
			break;
		case SDL_PowerState::SDL_POWERSTATE_NO_BATTERY:
		case SDL_PowerState::SDL_POWERSTATE_CHARGING:
		case SDL_PowerState::SDL_POWERSTATE_CHARGED:
			pad->m_battery_level = std::clamp(dev->sdl.power_level, 0, 100);
			pad->m_cable_state = 1;
			break;
		default:
			pad->m_battery_level = 0;
			pad->m_cable_state = 0;
			break;
		}
	}
	else
	{
		pad->m_cable_state = 1;
		pad->m_battery_level = 100;
	}

	if (dev->sdl.has_accel)
	{
		if (!SDL_GetGamepadSensorData(dev->sdl.gamepad, SDL_SENSOR_ACCEL, dev->values_accel.data(), 3))
		{
			sdl_log.error("Could not get acceleration sensor data of device %d! SDL Error: %s", dev->player_id, SDL_GetError());
		}
		else
		{
			const f32 accel_x = dev->values_accel[0]; // Angular speed around the x axis (pitch)
			const f32 accel_y = dev->values_accel[1]; // Angular speed around the y axis (yaw)
			const f32 accel_z = dev->values_accel[2]; // Angular speed around the z axis (roll)

			// Convert to ds3. The ds3 resolution is 113/G.
			pad->m_sensors[0].m_value = Clamp0To1023((accel_x / SDL_STANDARD_GRAVITY) * -1 * MOTION_ONE_G + 512);
			pad->m_sensors[1].m_value = Clamp0To1023((accel_y / SDL_STANDARD_GRAVITY) * -1 * MOTION_ONE_G + 512);
			pad->m_sensors[2].m_value = Clamp0To1023((accel_z / SDL_STANDARD_GRAVITY) * -1 * MOTION_ONE_G + 512);
		}
	}

	if (dev->sdl.has_gyro)
	{
		if (!SDL_GetGamepadSensorData(dev->sdl.gamepad, SDL_SENSOR_GYRO, dev->values_gyro.data(), 3))
		{
			sdl_log.error("Could not get gyro sensor data of device %d! SDL Error: %s", dev->player_id, SDL_GetError());
		}
		else
		{
			//const f32 gyro_x = dev->values_gyro[0]; // Angular speed around the x axis (pitch)
			const f32 gyro_y = dev->values_gyro[1]; // Angular speed around the y axis (yaw)
			//const f32 gyro_z = dev->values_gyro[2]; // Angular speed around the z axis (roll)

			// Convert to ds3. The ds3 resolution is 123/90°/sec. The SDL gyro is measured in rad/sec.
			const f32 degree = rad_to_degree(gyro_y);
			pad->m_sensors[3].m_value = Clamp0To1023(degree * (123.f / 90.f) + 512);
		}
	}

	if (dev->sdl.has_accel || dev->sdl.has_gyro)
	{
		set_raw_orientation(*pad);
	}
}

void sdl_pad_handler::get_motion_sensors(const std::string& pad_id, const motion_callback& callback, const motion_fail_callback& fail_callback, motion_preview_values preview_values, const std::array<AnalogSensor, 4>& sensors)
{
	if (!m_is_init)
		return;

	sdl_instance::get_instance().pump_events();

	PadHandlerBase::get_motion_sensors(pad_id, callback, fail_callback, preview_values, sensors);
}

PadHandlerBase::connection sdl_pad_handler::get_next_button_press(const std::string& padId, const pad_callback& callback, const pad_fail_callback& fail_callback, gui_call_type call_type, const std::vector<std::string>& buttons)
{
	if (!m_is_init)
		return connection::disconnected;

	sdl_instance::get_instance().pump_events();

	return PadHandlerBase::get_next_button_press(padId, callback, fail_callback, call_type, buttons);
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
	if (dev->sdl.has_rumble || dev->sdl.has_rumble_triggers)
	{
		const u8 speed_large = cfg->get_large_motor_speed(pad->m_vibrateMotors);
		const u8 speed_small = cfg->get_small_motor_speed(pad->m_vibrateMotors);

		dev->new_output_data |= dev->large_motor != speed_large || dev->small_motor != speed_small;

		dev->large_motor = speed_large;
		dev->small_motor = speed_small;

		const auto now = steady_clock::now();
		const auto elapsed = now - dev->last_output;

		// XBox One Controller can't handle faster vibration updates than ~10ms. Elite is even worse. So I'll use 20ms to be on the safe side. No lag was noticable.
		if ((dev->new_output_data && elapsed > 20ms) || elapsed > min_output_interval)
		{
			set_rumble(dev, speed_large, speed_small);

			dev->new_output_data = false;
			dev->last_output = now;
		}
	}

	if (dev->sdl.has_led || dev->sdl.has_mono_led)
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

			if (!SDL_SetGamepadLED(dev->sdl.gamepad, r, g, b))
			{
				sdl_log.error("Could not set LED of device %d! SDL Error: %s", dev->player_id, SDL_GetError());
			}

			dev->led_needs_update = false;
		}
	}

	if (dev->sdl.has_player_led && (dev->update_player_leds || dev->enable_player_leds != cfg->player_led_enabled.get()))
	{
		dev->enable_player_leds = cfg->player_led_enabled.get();
		dev->update_player_leds = false;

		if (!SDL_SetGamepadPlayerIndex(dev->sdl.gamepad, dev->enable_player_leds ? dev->player_id : -1))
		{
			sdl_log.error("Could not set player LED of device %d! SDL Error: %s", dev->player_id, SDL_GetError());
		}
	}
}

void sdl_pad_handler::set_rumble(SDLDevice* dev, u8 speed_large, u8 speed_small)
{
	if (!dev || !dev->sdl.gamepad) return;

	constexpr u32 rumble_duration_ms = static_cast<u32>((min_output_interval + 100ms).count()); // Some number higher than the min_output_interval.

	if (dev->sdl.has_rumble)
	{
		if (!SDL_RumbleGamepad(dev->sdl.gamepad, speed_large * 257, speed_small * 257, rumble_duration_ms))
		{
			sdl_log.error("Unable to play game pad rumble of player %d! SDL Error: %s", dev->player_id, SDL_GetError());
		}
	}

	// NOTE: Disabled for now. The triggers do not seem to be implemented correctly in SDL in the current version.
	//       The rumble is way too intensive and has a high frequency. It is very displeasing at the moment.
	//       In the future we could use additional trigger rumble to enhance the existing rumble.
	if (false && dev->sdl.has_rumble_triggers)
	{
		// Only the large motor shall control both triggers. It wouldn't make sense to differentiate here.
		if (!SDL_RumbleGamepadTriggers(dev->sdl.gamepad, speed_large * 257, speed_large * 257, rumble_duration_ms))
		{
			sdl_log.error("Unable to play game pad trigger rumble of player %d! SDL Error: %s", dev->player_id, SDL_GetError());
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

bool sdl_pad_handler::get_is_touch_pad_motion(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	switch (keyCode)
	{
	case SDLKeyCodes::Touch_L:
	case SDLKeyCodes::Touch_R:
	case SDLKeyCodes::Touch_U:
	case SDLKeyCodes::Touch_D:
		return true;
	default:
		return false;
	}
}

std::unordered_map<u64, u16> sdl_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> values;
	SDLDevice* dev = static_cast<SDLDevice*>(device.get());
	if (!dev || !dev->sdl.gamepad)
		return values;

	std::set<SDLKeyCodes> pressed_pressure_buttons;

	for (SDL_GamepadButton button_id : dev->sdl.button_ids)
	{
		const bool value = SDL_GetGamepadButton(dev->sdl.gamepad, button_id);
		const SDLKeyCodes key_code = get_button_code(button_id);

		// NOTE: SDL does not simply support DS3 button intensity in the current version
		//       So we have to skip the normal buttons if a DS3 with pressure buttons was detected
		if (dev->sdl.is_ds3_with_pressure_buttons)
		{
			switch (key_code)
			{
			case SDLKeyCodes::North:
			case SDLKeyCodes::South:
			case SDLKeyCodes::West:
			case SDLKeyCodes::East:
			case SDLKeyCodes::Left:
			case SDLKeyCodes::Right:
			case SDLKeyCodes::Up:
			case SDLKeyCodes::Down:
			case SDLKeyCodes::LB:
			case SDLKeyCodes::RB:
			{
				static const std::map<SDLKeyCodes, SDLKeyCodes> button_to_pressure =
				{
					{ SDLKeyCodes::South, SDLKeyCodes::PressureCross },
					{ SDLKeyCodes::East, SDLKeyCodes::PressureCircle },
					{ SDLKeyCodes::West, SDLKeyCodes::PressureSquare },
					{ SDLKeyCodes::North, SDLKeyCodes::PressureTriangle },
					{ SDLKeyCodes::LB, SDLKeyCodes::PressureL1 },
					{ SDLKeyCodes::RB, SDLKeyCodes::PressureR1 },
					{ SDLKeyCodes::Up, SDLKeyCodes::PressureUp },
					{ SDLKeyCodes::Down, SDLKeyCodes::PressureDown },
					{ SDLKeyCodes::Left, SDLKeyCodes::PressureLeft },
					{ SDLKeyCodes::Right, SDLKeyCodes::PressureRight }
				};

				if (value)
				{
					pressed_pressure_buttons.insert(::at32(button_to_pressure, key_code));
				}
				continue;
			}
			default:
				break;
			}
		}

		values[key_code] = value ? 255 : 0;
	}

	for (SDL_GamepadAxis axis_id : dev->sdl.axis_ids)
	{
		s16 value = SDL_GetGamepadAxis(dev->sdl.gamepad, axis_id);

		switch (axis_id)
		{
		case SDL_GamepadAxis::SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
			values[SDLKeyCodes::LT] = std::max<u16>(0, value);
			break;
		case SDL_GamepadAxis::SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
			values[SDLKeyCodes::RT] = std::max<u16>(0, value);
			break;
		case SDL_GamepadAxis::SDL_GAMEPAD_AXIS_LEFTX:
			values[SDLKeyCodes::LSXNeg] = value < 0 ? std::abs(value) - 1 : 0;
			values[SDLKeyCodes::LSXPos] = value > 0 ? value : 0;
			break;
		case SDL_GamepadAxis::SDL_GAMEPAD_AXIS_LEFTY:
			values[SDLKeyCodes::LSYNeg] = value > 0 ? value : 0;
			values[SDLKeyCodes::LSYPos] = value < 0 ? std::abs(value) - 1 : 0;
			break;
		case SDL_GamepadAxis::SDL_GAMEPAD_AXIS_RIGHTX:
			values[SDLKeyCodes::RSXNeg] = value < 0 ? std::abs(value) - 1 : 0;
			values[SDLKeyCodes::RSXPos] = value > 0 ? value : 0;
			break;
		case SDL_GamepadAxis::SDL_GAMEPAD_AXIS_RIGHTY:
			values[SDLKeyCodes::RSYNeg] = value > 0 ? value : 0;
			values[SDLKeyCodes::RSYPos] = value < 0 ? std::abs(value) - 1 : 0;
			break;
		default:
		{
			if (dev->sdl.is_ds3_with_pressure_buttons)
			{
				// Get pressure button value from axis
				if (const int key_code = SDLKeyCodes::PressureBegin + 1 + axis_id - SDL_GAMEPAD_AXIS_COUNT;
					key_code > SDLKeyCodes::PressureBegin && key_code < SDLKeyCodes::PressureEnd)
				{
					// We need to get the joystick value directly for axis >= SDL_GAMEPAD_AXIS_COUNT
					if (SDL_Joystick* joystick = SDL_GetGamepadJoystick(dev->sdl.gamepad))
					{
						value = SDL_GetJoystickAxis(joystick, axis_id);
					}

					value = static_cast<s16>(ScaledInput(value, SDL_JOYSTICK_AXIS_MIN, SDL_JOYSTICK_AXIS_MAX, 0.0f, 255.0f));

					if (value <= 0 && pressed_pressure_buttons.contains(static_cast<SDLKeyCodes>(key_code)))
					{
						value = 1;
					}

					values[key_code] = Clamp0To255(value);
				}
			}
			break;
		}
		}
	}

	for (const SDLDevice::touchpad& touchpad : dev->sdl.touchpads)
	{
		for (const SDLDevice::touch_point& finger : touchpad.fingers)
		{
			bool down = false; // true means the finger is touching the pad
			f32 x = 0.0f; // 0 = left, 1 = right
			f32 y = 0.0f; // 0 = top, 1 = bottom
			f32 pressure = 0.0f; // In the current SDL version the pressure is always 1 if the state is 1

			if (!SDL_GetGamepadTouchpadFinger(dev->sdl.gamepad, touchpad.index, finger.index, &down, &x, &y, &pressure))
			{
				sdl_log.error("Could not get touchpad %d finger %d data of device %d! SDL Error: %s", touchpad.index, finger.index, dev->player_id, SDL_GetError());
			}
			else
			{
				sdl_log.trace("touchpad=%d, finger=%d, state=%d, x=%f, y=%f, pressure=%f", touchpad.index, finger.index, down, x, y, pressure);

				if (!down)
				{
					continue;
				}

				const f32 x_scaled = ScaledInput(x, 0.0f, 1.0f, 0.0f, 255.0f);
				const f32 y_scaled = ScaledInput(y, 0.0f, 1.0f, 0.0f, 255.0f);

				values[SDLKeyCodes::Touch_L] = Clamp0To255((127.5f - x_scaled) * 2.0f);
				values[SDLKeyCodes::Touch_R] = Clamp0To255((x_scaled - 127.5f) * 2.0f);

				values[SDLKeyCodes::Touch_U] = Clamp0To255((127.5f - y_scaled) * 2.0f);
				values[SDLKeyCodes::Touch_D] = Clamp0To255((y_scaled - 127.5f) * 2.0f);
			}
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

std::string sdl_pad_handler::button_to_string(SDL_GamepadButton button)
{
	if (const char* name = SDL_GetGamepadStringForButton(button))
	{
		return name;
	}

	return {};
}

std::string sdl_pad_handler::axis_to_string(SDL_GamepadAxis axis)
{
	if (const char* name = SDL_GetGamepadStringForAxis(axis))
	{
		return name;
	}

	return {};
}

sdl_pad_handler::SDLKeyCodes sdl_pad_handler::get_button_code(SDL_GamepadButton button)
{
	switch (button)
	{
	default:
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_INVALID: return SDLKeyCodes::None;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_SOUTH: return SDLKeyCodes::South;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_EAST: return SDLKeyCodes::East;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_WEST: return SDLKeyCodes::West;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_NORTH: return SDLKeyCodes::North;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_DPAD_LEFT: return SDLKeyCodes::Left;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return SDLKeyCodes::Right;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_DPAD_UP: return SDLKeyCodes::Up;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_DPAD_DOWN: return SDLKeyCodes::Down;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return SDLKeyCodes::LB;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return SDLKeyCodes::RB;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_BACK: return SDLKeyCodes::Back;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_START: return SDLKeyCodes::Start;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_LEFT_STICK: return SDLKeyCodes::LS;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_RIGHT_STICK: return SDLKeyCodes::RS;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_GUIDE: return SDLKeyCodes::Guide;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_MISC1: return SDLKeyCodes::Misc1;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_MISC2: return SDLKeyCodes::Misc2;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_MISC3: return SDLKeyCodes::Misc3;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_MISC4: return SDLKeyCodes::Misc4;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_MISC5: return SDLKeyCodes::Misc5;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_MISC6: return SDLKeyCodes::Misc6;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1: return SDLKeyCodes::RPaddle1;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_LEFT_PADDLE1: return SDLKeyCodes::LPaddle1;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2: return SDLKeyCodes::RPaddle2;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_LEFT_PADDLE2: return SDLKeyCodes::LPaddle2;
	case SDL_GamepadButton::SDL_GAMEPAD_BUTTON_TOUCHPAD: return SDLKeyCodes::Touchpad;
	}
}

#endif
