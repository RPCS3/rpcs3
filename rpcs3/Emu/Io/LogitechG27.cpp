// Logitech G27

// ffb ref
// https://opensource.logitech.com/wiki/force_feedback/Logitech_Force_Feedback_Protocol_V1.6.pdf
// https://github.com/mathijsvandenberg/g29emu/files/14395098/Logitech_Force_Feedback_Protocol_V1.6.pdf

// shifter input ref
// https://github.com/sonik-br/lgff_wheel_adapter/blob/d97f7823154818e1b3edff6d51498a122c302728/pico_lgff_wheel_adapter/reports.h#L265-L310

#include "stdafx.h"

#ifdef HAVE_SDL3

#include "LogitechG27.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/system_config.h"
#include "Input/pad_thread.h"
#include "Input/sdl_instance.h"

LOG_CHANNEL(logitech_g27_log, "LOGIG27");

// ref: https://github.com/libsdl-org/SDL/issues/7941, need to use SDL_HAPTIC_STEERING_AXIS for some windows drivers
static const SDL_HapticDirection STEERING_DIRECTION =
{
	.type = SDL_HAPTIC_STEERING_AXIS,
	.dir = {0, 0, 0}
};

usb_device_logitech_g27::usb_device_logitech_g27(u32 controller_index, const std::array<u8, 7>& location)
	: usb_device_emulated(location), m_controller_index(controller_index)
{
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{0x0200, 0, 0, 0, 16, 0x046d, 0xc29b, 0x1350, 1, 2, 0, 1});

	// parse the raw response like with passthrough device
	static constexpr u8 raw_config[] = {0x9, 0x2, 0x29, 0x0, 0x1, 0x1, 0x4, 0x80, 0x31, 0x9, 0x4, 0x0, 0x0, 0x2, 0x3, 0x0, 0x0, 0x0, 0x9, 0x21, 0x11, 0x1, 0x21, 0x1, 0x22, 0x85, 0x0, 0x7, 0x5, 0x81, 0x3, 0x10, 0x0, 0x2, 0x7, 0x5, 0x1, 0x3, 0x10, 0x0, 0x2};
	auto& conf = device.add_node(UsbDescriptorNode(raw_config[0], raw_config[1], &raw_config[2]));
	for (unsigned int index = raw_config[0]; index < sizeof(raw_config);)
	{
		conf.add_node(UsbDescriptorNode(raw_config[index], raw_config[index + 1], &raw_config[index + 2]));
		index += raw_config[index];
	}

	m_default_spring_effect.type = SDL_HAPTIC_SPRING;
	m_default_spring_effect.condition.direction = STEERING_DIRECTION;
	m_default_spring_effect.condition.length = SDL_HAPTIC_INFINITY;
	for (int i = 0; i < 1 /*3*/; i++)
	{
		m_default_spring_effect.condition.right_sat[i] = 0x7FFF;
		m_default_spring_effect.condition.left_sat[i] = 0x7FFF;
		m_default_spring_effect.condition.right_coeff[i] = 0x7FFF;
		m_default_spring_effect.condition.left_coeff[i] = 0x7FFF;
	}

	g_cfg_logitech_g27.load();

	m_enabled = g_cfg_logitech_g27.enabled.get() && sdl_instance::get_instance().initialize();

	if (!m_enabled)
		return;

	m_house_keeping_thread = std::make_unique<named_thread<std::function<void()>>>("Logitech G27", [this]()
	{
		while (thread_ctrl::state() != thread_state::aborting)
		{
			sdl_refresh();
			thread_ctrl::wait_for(5'000'000);
		}
	});
}

bool usb_device_logitech_g27::open_device()
{
	return m_enabled;
}

static void clear_sdl_joysticks(std::map<u64, std::vector<SDL_Joystick*>>& joystick_map)
{
	for (auto& [type, joysticks] : joystick_map)
	{
		for (SDL_Joystick* joystick : joysticks)
		{
			if (joystick)
				SDL_CloseJoystick(joystick);
		}
	}
	joystick_map.clear();
}

usb_device_logitech_g27::~usb_device_logitech_g27()
{
	// Wait for the house keeping thread to finish
	m_house_keeping_thread.reset();

	// Close sdl handles
	{
		const std::lock_guard lock(m_sdl_handles_mutex);
		if (m_haptic_handle)
		{
			SDL_CloseHaptic(m_haptic_handle);
			m_haptic_handle = nullptr;
		}
		clear_sdl_joysticks(m_joysticks);
	}
}

std::shared_ptr<usb_device> usb_device_logitech_g27::make_instance(u32 controller_index, const std::array<u8, 7>& location)
{
	return std::make_shared<usb_device_logitech_g27>(controller_index, location);
}

u16 usb_device_logitech_g27::get_num_emu_devices()
{
	return 1;
}

void usb_device_logitech_g27::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	logitech_g27_log.todo("control transfer bmRequestType %02x, bRequest %02x, wValue %04x, wIndex %04x, wLength %04x, %s", bmRequestType, bRequest, wValue, wIndex, wLength, fmt::buf_to_hexstring(buf, buf_size));

	usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
}

static bool sdl_joysticks_equal(std::map<u64, std::vector<SDL_Joystick*>>& left, std::map<u64, std::vector<SDL_Joystick*>>& right)
{
	if (left.size() != right.size())
	{
		return false;
	}
	for (const auto& [left_type, left_joysticks] : left)
	{
		const auto right_joysticks = right.find(left_type);
		if (right_joysticks == right.cend())
		{
			return false;
		}
		if (left_joysticks.size() != right_joysticks->second.size())
		{
			return false;
		}
		for (const SDL_Joystick* left_joystick : left_joysticks)
		{
			if (std::none_of(right_joysticks->second.begin(), right_joysticks->second.end(), [left_joystick](const SDL_Joystick* right_joystick)
				{
					return left_joystick == right_joystick;
				}))
			{
				return false;
			}
		}
	}
	return true;
}

static inline logitech_g27_sdl_mapping get_runtime_mapping()
{
	logitech_g27_sdl_mapping mapping {};

	const auto convert_mapping = [](const emulated_logitech_g27_mapping& cfg, sdl_mapping& mapping)
	{
		mapping.device_type_id = cfg.device_type_id.get();
		mapping.type = cfg.type.get();
		mapping.id = cfg.id.get();
		mapping.hat = cfg.hat.get();
		mapping.reverse = cfg.reverse.get();
		mapping.positive_axis = false;
	};

	const auto& cfg = g_cfg_logitech_g27;

	convert_mapping(cfg.steering, mapping.steering);
	convert_mapping(cfg.throttle, mapping.throttle);
	convert_mapping(cfg.brake, mapping.brake);
	convert_mapping(cfg.clutch, mapping.clutch);
	convert_mapping(cfg.shift_up, mapping.shift_up);
	convert_mapping(cfg.shift_down, mapping.shift_down);

	convert_mapping(cfg.up, mapping.up);
	convert_mapping(cfg.down, mapping.down);
	convert_mapping(cfg.left, mapping.left);
	convert_mapping(cfg.right, mapping.right);

	convert_mapping(cfg.triangle, mapping.triangle);
	convert_mapping(cfg.cross, mapping.cross);
	convert_mapping(cfg.square, mapping.square);
	convert_mapping(cfg.circle, mapping.circle);

	convert_mapping(cfg.l2, mapping.l2);
	convert_mapping(cfg.l3, mapping.l3);
	convert_mapping(cfg.r2, mapping.r2);
	convert_mapping(cfg.r3, mapping.r3);

	convert_mapping(cfg.plus, mapping.plus);
	convert_mapping(cfg.minus, mapping.minus);

	convert_mapping(cfg.dial_clockwise, mapping.dial_clockwise);
	convert_mapping(cfg.dial_anticlockwise, mapping.dial_anticlockwise);

	convert_mapping(cfg.select, mapping.select);
	convert_mapping(cfg.pause, mapping.pause);

	convert_mapping(cfg.shifter_1, mapping.shifter_1);
	convert_mapping(cfg.shifter_2, mapping.shifter_2);
	convert_mapping(cfg.shifter_3, mapping.shifter_3);
	convert_mapping(cfg.shifter_4, mapping.shifter_4);
	convert_mapping(cfg.shifter_5, mapping.shifter_5);
	convert_mapping(cfg.shifter_6, mapping.shifter_6);
	convert_mapping(cfg.shifter_r, mapping.shifter_r);
	convert_mapping(cfg.shifter_press, mapping.shifter_press);

	return mapping;
}

void usb_device_logitech_g27::sdl_refresh()
{
	std::unique_lock lock(g_cfg_logitech_g27.m_mutex);

	m_mapping = get_runtime_mapping();

	m_reverse_effects = g_cfg_logitech_g27.reverse_effects.get();

	const u64 ffb_device_type_id = g_cfg_logitech_g27.ffb_device_type_id.get();
	const u64 led_device_type_id = g_cfg_logitech_g27.led_device_type_id.get();

	lock.unlock();

	SDL_Joystick* new_led_joystick_handle = nullptr;
	SDL_Haptic* new_haptic_handle = nullptr;
	std::map<u64, std::vector<SDL_Joystick*>> new_joysticks;

	int joystick_count = 0;
	if (SDL_JoystickID* joystick_ids = SDL_GetJoysticks(&joystick_count))
	{
		for (int i = 0; i < joystick_count; i++)
		{
			SDL_Joystick* cur_joystick = SDL_OpenJoystick(joystick_ids[i]);
			if (!cur_joystick)
			{
				logitech_g27_log.error("Failed opening joystick %d, %s", joystick_ids[i], SDL_GetError());
				continue;
			}
			const u16 cur_vendor_id = SDL_GetJoystickVendor(cur_joystick);
			const u16 cur_product_id = SDL_GetJoystickProduct(cur_joystick);
			const emulated_g27_device_type_id joystick_type_id_struct =
			{
				.product_id = static_cast<u64>(cur_product_id),
				.vendor_id = static_cast<u64>(cur_vendor_id),
				.num_axes = static_cast<u64>(SDL_GetNumJoystickAxes(cur_joystick)),
				.num_hats = static_cast<u64>(SDL_GetNumJoystickHats(cur_joystick)),
				.num_buttons = static_cast<u64>(SDL_GetNumJoystickButtons(cur_joystick))
			};
			const u64 joystick_type_id = joystick_type_id_struct.as_u64();
			auto joysticks_of_type = new_joysticks.find(joystick_type_id);
			if (joysticks_of_type == new_joysticks.end())
			{
				new_joysticks[joystick_type_id] = { cur_joystick };
			}
			else
			{
				joysticks_of_type->second.push_back(cur_joystick);
			}

			if (joystick_type_id == ffb_device_type_id && new_haptic_handle == nullptr)
			{
				SDL_Haptic* cur_haptic = SDL_OpenHapticFromJoystick(cur_joystick);
				if (cur_haptic == nullptr)
				{
					logitech_g27_log.error("Failed opening haptic device from selected ffb device %04x:%04x, %s", cur_vendor_id, cur_product_id, SDL_GetError());
				}
				else
				{
					new_haptic_handle = cur_haptic;
				}
			}

			if (joystick_type_id == led_device_type_id && new_led_joystick_handle == nullptr)
			{
				new_led_joystick_handle = cur_joystick;
			}
		}
		SDL_free(joystick_ids);
	}
	else
	{
		logitech_g27_log.error("Failed fetching joystick list, %s", SDL_GetError());
	}

	const bool joysticks_changed = !sdl_joysticks_equal(m_joysticks, new_joysticks);
	const bool haptic_changed = m_haptic_handle != new_haptic_handle;
	const bool led_joystick_changed = m_led_joystick_handle != new_led_joystick_handle;

	// if we should touch the mutex
	if (joysticks_changed || haptic_changed || led_joystick_changed)
	{
		const std::lock_guard<std::mutex> lock(m_sdl_handles_mutex);
		if (haptic_changed)
		{
			if (m_haptic_handle)
			{
				SDL_CloseHaptic(m_haptic_handle);
				m_haptic_handle = nullptr;
			}
			// reset effects if the ffb device is changed
			for (logitech_g27_ffb_slot& slot : m_effect_slots)
			{
				slot.effect_id = -1;
			}
			m_default_spring_effect_id = -1;
			m_haptic_handle = new_haptic_handle;
		}
		if (led_joystick_changed)
		{
			SDL_SetJoystickLED(m_led_joystick_handle, 0, 0, 0);
			m_led_joystick_handle = new_led_joystick_handle;
		}
		if (joysticks_changed)
		{
			// finally clear out previous joystick handles
			clear_sdl_joysticks(m_joysticks);
			m_joysticks = new_joysticks;
		}
	}

	if (!joysticks_changed)
	{
		clear_sdl_joysticks(new_joysticks);
	}

	if (!haptic_changed)
	{
		if (new_haptic_handle)
			SDL_CloseHaptic(new_haptic_handle);
	}
}

static inline s16 logitech_g27_force_to_level(u8 force, bool reverse = false)
{
	/*
	 * Wheel documentation:
	 * level 0, maximum clock wise force
	 * level 127-128, no force
	 * level 255, maximum anticlockwise force
	 */

	if (force == 127 || force == 128)
	{
		return 0;
	}

	const s16 subtrahend = force > 128 ? 128 : 127;
	s16 converted = ((force - subtrahend) * 0x7FFF) / 127;

	if (reverse)
		converted *= -1;

	return converted;
}

static inline s16 logitech_g27_position_to_center(u8 left, u8 right)
{
	const u16 center_unsigned = (((right + left) * 0xFFFF) / 255) / 2;
	return center_unsigned - 0x8000;
}

static inline s16 logitech_g27_high_resolution_position_to_center(u16 left, u16 right)
{
	const u16 center_unsigned = (((right + left) * 0xFFFF) / (0xFFFF >> 5)) / 2;
	return center_unsigned - 0x8000;
}

static inline u16 logitech_g27_position_to_width(u8 left, u8 right)
{
	return ((right - left) * 0xFFFF) / 255;
}

static inline u16 logitech_g27_high_resolution_position_to_width(u16 left, u16 right)
{
	return ((right - left) * 0xFFFF) / (0xFFFF >> 5);
}

static inline s16 logitech_g27_coeff_to_coeff(u8 coeff, u8 invert)
{
	if (!invert)
	{
		return (coeff * 0x7FFF) / 7;
	}
	return (coeff * 0x7FFF * -1) / 7;
}

static inline s16 logitech_g27_high_resolution_coeff_to_coeff(u8 coeff, u8 invert)
{
	if (!invert)
	{
		return (coeff * 0x7FFF) / 15;
	}
	return (coeff * 0x7FFF * -1) / 15;
}

static inline s16 logitech_g27_friction_coeff_to_coeff(u8 coeff, u8 invert)
{
	if (!invert)
	{
		return (coeff * 0x7FFF) / 255;
	}
	return (coeff * 0x7FFF * -1) / 255;
}

static inline s16 logitech_g27_clip_to_saturation(u8 clip)
{
	return (clip * 0x7FFF) / 255;
}

static inline s16 logitech_g27_amplitude_to_magnitude(u8 amplitude)
{
	return ((amplitude * 0x7FFF) / 2) / 255;
}

static inline u16 logitech_g27_loops_to_ms(u16 loops, bool afap)
{
	if (afap)
	{
		return loops;
	}
	return loops * 2;
}

extern bool is_input_allowed();

static u8 sdl_hat_to_logitech_g27_hat(u8 sdl_hat)
{
	switch (sdl_hat)
	{
	case SDL_HAT_CENTERED:
		return 8;
	case SDL_HAT_UP:
		return 0;
	case SDL_HAT_RIGHTUP:
		return 1;
	case SDL_HAT_RIGHT:
		return 2;
	case SDL_HAT_RIGHTDOWN:
		return 3;
	case SDL_HAT_DOWN:
		return 4;
	case SDL_HAT_LEFTDOWN:
		return 5;
	case SDL_HAT_LEFT:
		return 6;
	case SDL_HAT_LEFTUP:
		return 7;
	default:
		break;
	}
	return 0;
}

static u8 hat_components_to_logitech_g27_hat(bool up, bool down, bool left, bool right)
{
	u8 sdl_hat = 0;
	if (up)
		sdl_hat |= SDL_HAT_UP;
	if (down)
		sdl_hat |= SDL_HAT_DOWN;
	if (left)
		sdl_hat |= SDL_HAT_LEFT;
	if (right)
		sdl_hat |= SDL_HAT_RIGHT;
	return sdl_hat_to_logitech_g27_hat(sdl_hat);
}

static bool fetch_sdl_as_button(SDL_Joystick* joystick, const sdl_mapping& mapping)
{
	switch (mapping.type)
	{
	case sdl_mapping_type::button:
	{
		const bool pressed = SDL_GetJoystickButton(joystick, static_cast<s32>(mapping.id));
		return mapping.reverse ? !pressed : pressed;
	}
	case sdl_mapping_type::hat:
	{
		const u8 hat_value = SDL_GetJoystickHat(joystick, static_cast<s32>(mapping.id));
		bool pressed = false;
		switch (mapping.hat)
		{
		case hat_component::up:
			pressed = (hat_value & SDL_HAT_UP) ? true : false;
			break;
		case hat_component::down:
			pressed = (hat_value & SDL_HAT_DOWN) ? true : false;
			break;
		case hat_component::left:
			pressed = (hat_value & SDL_HAT_LEFT) ? true : false;
			break;
		case hat_component::right:
			pressed = (hat_value & SDL_HAT_RIGHT) ? true : false;
			break;
		case hat_component::none:
			break;
		}
		return mapping.reverse ? !pressed : pressed;
	}
	case sdl_mapping_type::axis:
	{
		const s32 axis_value = SDL_GetJoystickAxis(joystick, static_cast<s32>(mapping.id));
		bool pressed = false;
		if (mapping.positive_axis)
		{
			pressed = axis_value > (0x7FFF / 2);
		}
		else
		{
			pressed = axis_value < (0x7FFF / (-2));
		}
		return mapping.reverse ? !pressed : pressed;
	}
	}
	return false;
}

static s16 fetch_sdl_as_axis(SDL_Joystick* joystick, const sdl_mapping& mapping)
{
	constexpr s16 MAX = 0x7FFF;
	constexpr s16 MIN = -0x8000;
	constexpr s16 MID = 0;

	switch (mapping.type)
	{
	case sdl_mapping_type::button:
	{
		bool pressed = SDL_GetJoystickButton(joystick, static_cast<s32>(mapping.id));
		if (mapping.reverse)
		{
			pressed = !pressed;
		}
		return pressed ? (mapping.positive_axis ? MAX : MIN) : MID;
	}
	case sdl_mapping_type::hat:
	{
		const u8 hat_value = SDL_GetJoystickHat(joystick, static_cast<s32>(mapping.id));
		bool pressed = false;
		switch (mapping.hat)
		{
		case hat_component::up:
			pressed = (hat_value & SDL_HAT_UP) ? true : false;
			break;
		case hat_component::down:
			pressed = (hat_value & SDL_HAT_DOWN) ? true : false;
			break;
		case hat_component::left:
			pressed = (hat_value & SDL_HAT_LEFT) ? true : false;
			break;
		case hat_component::right:
			pressed = (hat_value & SDL_HAT_RIGHT) ? true : false;
			break;
		case hat_component::none:
			break;
		}
		if (mapping.reverse)
		{
			pressed = !pressed;
		}
		return pressed ? (mapping.positive_axis ? MAX : MIN) : MID;
	}
	case sdl_mapping_type::axis:
	{
		s32 axis_value = SDL_GetJoystickAxis(joystick, static_cast<s32>(mapping.id));
		if (mapping.reverse)
			axis_value *= -1;
		if (axis_value == (MIN + 1))
			axis_value = MIN;
		return std::clamp<s32>(axis_value, MIN, MAX);
	}
	}
	return 0;
}

static s16 fetch_sdl_axis_avg(std::map<u64, std::vector<SDL_Joystick*>>& joysticks, const sdl_mapping& mapping)
{
	constexpr s16 MAX = 0x7FFF;
	constexpr s16 MIN = -0x8000;

	auto joysticks_of_type = joysticks.find(mapping.device_type_id);
	if (joysticks_of_type == joysticks.end())
	{
		return mapping.reverse ? MAX : MIN;
	}

	if (joysticks_of_type->second.empty())
	{
		return mapping.reverse ? MAX : MIN;
	}

	// TODO account for deadzone and only pick up active devices
	s32 sdl_joysticks_total_value = 0;
	for (SDL_Joystick* joystick : joysticks_of_type->second)
	{
		sdl_joysticks_total_value += fetch_sdl_as_axis(joystick, mapping);
	}

	return std::clamp<s16>(sdl_joysticks_total_value / static_cast<s32>(joysticks_of_type->second.size()), MIN, MAX);
}

static bool sdl_to_logitech_g27_button(std::map<u64, std::vector<SDL_Joystick*>>& joysticks, const sdl_mapping& mapping)
{
	auto joysticks_of_type = joysticks.find(mapping.device_type_id);
	if (joysticks_of_type == joysticks.end())
	{
		return mapping.reverse;
	}

	if (joysticks_of_type->second.empty())
	{
		return mapping.reverse;
	}

	bool pressed = false;
	for (SDL_Joystick* joystick : joysticks_of_type->second)
	{
		pressed |= fetch_sdl_as_button(joystick, mapping);
	}
	return pressed;
}

static u16 sdl_to_logitech_g27_steering(std::map<u64, std::vector<SDL_Joystick*>>& joysticks, const sdl_mapping& mapping)
{
	const s16 avg = fetch_sdl_axis_avg(joysticks, mapping);
	const u16 unsigned_avg = avg + 0x8000;
	return unsigned_avg * (0xFFFF >> 2) / 0xFFFF;
}

static u8 sdl_to_logitech_g27_pedal(std::map<u64, std::vector<SDL_Joystick*>>& joysticks, const sdl_mapping& mapping)
{
	const s16 avg = fetch_sdl_axis_avg(joysticks, mapping);
	const u16 unsigned_avg = avg + 0x8000;
	return unsigned_avg * 0xFF / 0xFFFF;
}

static inline void set_bit(u8* buf, int bit_num, bool set)
{
	const int byte_num = bit_num / 8;
	bit_num %= 8;
	const u8 mask = 1 << bit_num;
	if (set)
		buf[byte_num] = buf[byte_num] | mask;
	else
		buf[byte_num] = buf[byte_num] & (~mask);
}

void usb_device_logitech_g27::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer)
{
	transfer->fake = true;
	transfer->expected_result = HC_CC_NOERR;
	// G29 in G27 mode polls at 500 hz, let's try a delay of 1ms for now, for wheels that updates that fast
	transfer->expected_time = get_timestamp() + 1000;

	if (endpoint & (1 << 7))
	{
		if (buf_size < 11)
		{
			logitech_g27_log.error("Not populating input buffer with a buffer of the size of %u", buf_size);
			return;
		}

		ensure(buf_size >= 11);
		memset(buf, 0, buf_size);

		transfer->expected_count = 11;

		sdl_instance::get_instance().pump_events();

		// Fetch input states from SDL
		m_sdl_handles_mutex.lock();
		const u16 steering = sdl_to_logitech_g27_steering(m_joysticks, m_mapping.steering);
		const u8 throttle = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.throttle);
		const u8 brake = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.brake);
		const u8 clutch = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.clutch);
		const bool shift_up = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_up);
		const bool shift_down = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_down);

		const bool up = sdl_to_logitech_g27_button(m_joysticks, m_mapping.up);
		const bool down = sdl_to_logitech_g27_button(m_joysticks, m_mapping.down);
		const bool left = sdl_to_logitech_g27_button(m_joysticks, m_mapping.left);
		const bool right = sdl_to_logitech_g27_button(m_joysticks, m_mapping.right);

		const bool triangle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.triangle);
		const bool cross = sdl_to_logitech_g27_button(m_joysticks, m_mapping.cross);
		const bool square = sdl_to_logitech_g27_button(m_joysticks, m_mapping.square);
		const bool circle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.circle);

		const bool l2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l2);
		const bool l3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l3);
		const bool r2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r2);
		const bool r3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r3);

		const bool plus = sdl_to_logitech_g27_button(m_joysticks, m_mapping.plus);
		const bool minus = sdl_to_logitech_g27_button(m_joysticks, m_mapping.minus);

		const bool dial_clockwise = sdl_to_logitech_g27_button(m_joysticks, m_mapping.dial_clockwise);
		const bool dial_anticlockwise = sdl_to_logitech_g27_button(m_joysticks, m_mapping.dial_anticlockwise);

		const bool select = sdl_to_logitech_g27_button(m_joysticks, m_mapping.select);
		const bool pause = sdl_to_logitech_g27_button(m_joysticks, m_mapping.pause);

		const bool shifter_1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_1);
		const bool shifter_2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_2);
		const bool shifter_3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_3);
		const bool shifter_4 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_4);
		const bool shifter_5 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_5);
		const bool shifter_6 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_6);
		const bool shifter_r = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_r);
		const bool shifter_press = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_press);
		m_sdl_handles_mutex.unlock();

		// populate buffer
		buf[0] = hat_components_to_logitech_g27_hat(up, down, left, right);
		set_bit(buf, 8, shift_up);
		set_bit(buf, 9, shift_down);

		set_bit(buf, 7, triangle);
		set_bit(buf, 4, cross);
		set_bit(buf, 5, square);
		set_bit(buf, 6, circle);

		set_bit(buf, 11, l2);
		set_bit(buf, 15, l3);
		set_bit(buf, 10, r2);
		set_bit(buf, 14, r3);

		set_bit(buf, 22, dial_clockwise);
		set_bit(buf, 23, dial_anticlockwise);

		set_bit(buf, 24, plus);
		set_bit(buf, 25, minus);

		set_bit(buf, 12, select);
		set_bit(buf, 13, pause);

		set_bit(buf, 16, shifter_1);
		set_bit(buf, 17, shifter_2);
		set_bit(buf, 18, shifter_3);
		set_bit(buf, 19, shifter_4);
		set_bit(buf, 20, shifter_5);
		set_bit(buf, 21, shifter_6);
		set_bit(buf, 80, shifter_r);

		// calibrated, unsure
		set_bit(buf, 82, true);
		// shifter connected
		set_bit(buf, 83, true);
		/*
		 * shifter pressed/down bit
		 * mechanical references:
		 * - G29 shifter mechanical explanation https://youtu.be/d7qCn3o8K98?t=1124
		 * - same mechanism on the G27 https://youtu.be/rdjejtIfkVA?t=760
		 * - same mechanism on the G25 https://youtu.be/eCyt_4luwF0?t=130
		 * on healthy G29/G27/G25 shifters, shifter is mechnically kept pressed in reverse, the bit should be set
		 * the shifter_press mapping alone captures instead a shifter press without going into reverse, ie. neutral press, just in case there are games using it for input
		 */
		set_bit(buf, 86, shifter_press | shifter_r);

		buf[3] = (steering << 2) | buf[3];
		buf[4] = steering >> 6;
		buf[5] = throttle;
		buf[6] = brake;
		buf[7] = clutch;

		// rough analog values recorded in https://github.com/RPCS3/rpcs3/pull/17199#issuecomment-2883934412
		// buf[8] shifter x
		// buf[9] shifter y
		constexpr u8 shifter_coord_center = 0x80;
		constexpr u8 shifter_coord_top = 0xb7;
		constexpr u8 shifter_coord_bottom = 0x32;
		constexpr u8 shifter_coord_left = 0x30;
		constexpr u8 shifter_coord_right = 0xb3;
		constexpr u8 shifter_coord_right_reverse = 0xaa;
		if (shifter_1)
		{
			buf[8] = shifter_coord_left;
			buf[9] = shifter_coord_top;
		}
		else if (shifter_2)
		{
			buf[8] = shifter_coord_left;
			buf[9] = shifter_coord_bottom;
		}
		else if (shifter_3)
		{
			buf[8] = shifter_coord_center;
			buf[9] = shifter_coord_top;
		}
		else if (shifter_4)
		{
			buf[8] = shifter_coord_center;
			buf[9] = shifter_coord_bottom;
		}
		else if (shifter_5)
		{
			buf[8] = shifter_coord_right;
			buf[9] = shifter_coord_top;
		}
		else if (shifter_6)
		{
			buf[8] = shifter_coord_right;
			buf[9] = shifter_coord_bottom;
		}
		else if (shifter_r)
		{
			buf[8] = shifter_coord_right_reverse;
			buf[9] = shifter_coord_bottom;
		}
		else
		{
			buf[8] = shifter_coord_center;
			buf[9] = shifter_coord_center;
		}

		buf[10] = buf[10] | (m_wheel_range > 360 ? 0x90 : 0x10);

		// logitech_g27_log.error("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10]);

		return;
	}
	else
	{
		// Sending data to wheel
		if (buf_size < 7)
		{
			logitech_g27_log.error("Unhandled wheel command with size %u != 16, %s", buf_size, fmt::buf_to_hexstring(buf, buf_size));
			return;
		}

		transfer->expected_count = buf_size;

		// logitech_g27_log.error("%02x %02x %02x %02x %02x %02x %02x", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
		// printf("%02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);

		// TODO maybe force clipping from cfg

		// Process effects
		if (buf[0] == 0xf8)
		{
			const std::lock_guard lock(m_sdl_handles_mutex);
			switch (buf[1])
			{
			case 0x01:
			{
				// Change to DFP
				logitech_g27_log.error("Drive Force Pro mode switch command ignored");
				break;
			}
			case 0x02:
			{
				// Change wheel range to 200 degrees
				logitech_g27_log.error("Change wheel range to 200 degrees command not forwarded");
				m_wheel_range = 200;
				break;
			}
			case 0x03:
			{
				// Change wheel range to 900 degrees
				logitech_g27_log.error("Change wheel range to 900 degrees command not forwarded");
				m_wheel_range = 900;
				break;
			}
			case 0x09:
			{
				// Change device mode
				logitech_g27_log.error("Change device mode to %d %s detaching command ignored", buf[2], buf[3] ? "with" : "without");
				break;
			}
			case 0x0a:
			{
				// Revert indentity
				logitech_g27_log.error("Revert device identity after reset %s command ignored", buf[2] ? "enable" : "disable");
				break;
			}
			case 0x10:
			{
				// Switch to G25 with detach
				logitech_g27_log.error("Switch to G25 with detach command ignored");
				break;
			}
			case 0x11:
			{
				// Switch to G25 without detach
				logitech_g27_log.error("Switch to G25 without detach command ignored");
				break;
			}
			case 0x12:
			{
				// Incoming data is a 5 bit mask, for each individual bulb
				if (m_led_joystick_handle == nullptr)
				{
					break;
				}
				// Mux into total amount of bulbs on, since sdl only takes intensity
				u8 new_led_level = 0;
				for (int i = 0; i < 5; i++)
				{
					new_led_level += (buf[2] & (1 << i)) ? 1 : 0;
				}

				const u8 intensity = new_led_level * 255 / 5;
				SDL_SetJoystickLED(m_led_joystick_handle, intensity, intensity, intensity);
				break;
			}
			case 0x81:
			{
				// Wheel range change
				m_wheel_range = (buf[3] << 8) | buf[2];
				logitech_g27_log.error("Wheel range change to %u command not forwarded", m_wheel_range);
				break;
			}
			default:
			{
				logitech_g27_log.error("Unknown extended command %02x %02x %02x %02x %02x %02x %02x ignored", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
				break;
			}
			}
		}
		else
		{
			const std::lock_guard lock(m_sdl_handles_mutex);
			const u8 cmd = buf[0] & 0xf;
			const u8 slot_mask = buf[0] >> 4;
			switch (cmd)
			{
			case 0x00:
			case 0x01:
			case 0x0c:
			{
				// Download/Download play/Refresh
				for (int i = 0; i < 4; i++)
				{
					if (!(slot_mask & (1 << i)))
					{
						continue;
					}
					SDL_HapticEffect new_effect {};
					// hack: need to reduce Download play spams for some drivers
					bool update_hack = false;
					bool unknown_effect = false;
					switch (buf[1])
					{
					case 0x00:
					{
						// Constant force
						new_effect.type = SDL_HAPTIC_CONSTANT;
						new_effect.constant.direction = STEERING_DIRECTION;
						new_effect.constant.length = SDL_HAPTIC_INFINITY;
						new_effect.constant.level = logitech_g27_force_to_level(buf[2 + i], m_reverse_effects);
						break;
					}
					case 0x01:
					case 0x0b:
					{
						// Spring/High resolution spring
						new_effect.type = SDL_HAPTIC_SPRING;
						new_effect.condition.direction = STEERING_DIRECTION;
						new_effect.condition.length = SDL_HAPTIC_INFINITY;
						const u8 s1 = buf[5] & 1;
						const u8 s2 = (buf[5] >> 4) & 1;
						const u16 saturation = logitech_g27_clip_to_saturation(buf[6]);
						s16 center = 0;
						u16 deadband = 0;
						s16 left_coeff = 0;
						s16 right_coeff = 0;
						if (buf[1] == 0x01)
						{
							const u8 d1 = buf[2];
							const u8 d2 = buf[3];
							const u8 k1 = buf[4] & (0xf >> 1);
							const u8 k2 = (buf[4] >> 4) & (0xf >> 1);
							center = logitech_g27_position_to_center(d1, d2);
							deadband = logitech_g27_position_to_width(d1, d2);
							left_coeff = logitech_g27_coeff_to_coeff(k1, s1);
							right_coeff = logitech_g27_coeff_to_coeff(k2, s2);
						}
						else
						{
							const u16 d1 = (buf[2] << 3) | ((buf[5] >> 1) & (0xf >> 1));
							const u16 d2 = (buf[3] << 3) | (buf[5] >> 5);
							const u8 k1 = buf[4] & 0xf;
							const u8 k2 = buf[4] >> 4;
							center = logitech_g27_high_resolution_position_to_center(d1, d2);
							deadband = logitech_g27_high_resolution_position_to_width(d1, d2);
							left_coeff = logitech_g27_high_resolution_coeff_to_coeff(k1, s1);
							right_coeff = logitech_g27_high_resolution_coeff_to_coeff(k2, s2);
						}
						if (m_reverse_effects)
						{
							std::swap(left_coeff, right_coeff);
						}
						for (int j = 0; j < 1 /*3*/; j++)
						{
							new_effect.condition.right_sat[j] = saturation;
							new_effect.condition.left_sat[j] = saturation;
							new_effect.condition.right_coeff[j] = right_coeff;
							new_effect.condition.left_coeff[j] = left_coeff;
							new_effect.condition.deadband[j] = deadband;
							new_effect.condition.center[j] = center;
						}
						break;
					}
					case 0x02:
					case 0x0c:
					{
						// Damper/High resolution damper
						new_effect.type = SDL_HAPTIC_DAMPER;
						new_effect.condition.direction = STEERING_DIRECTION;
						new_effect.condition.length = SDL_HAPTIC_INFINITY;
						const u8 s1 = buf[3] & 1;
						const u8 s2 = buf[5] & 1;
						// TODO direction cfg
						u16 saturation = 0x7FFF;
						s16 left_coeff = 0;
						s16 right_coeff = 0;
						if (buf[1] == 0x02)
						{
							const u8 k1 = buf[2] & (0xf >> 1);
							const u8 k2 = buf[4] & (0xf >> 1);
							left_coeff = logitech_g27_coeff_to_coeff(k1, s1);
							right_coeff = logitech_g27_coeff_to_coeff(k2, s2);
						}
						else
						{
							const u8 k1 = buf[2] & 0xf;
							const u8 k2 = buf[4] & 0xf;
							left_coeff = logitech_g27_high_resolution_coeff_to_coeff(k1, s1);
							right_coeff = logitech_g27_high_resolution_coeff_to_coeff(k2, s2);
							saturation = logitech_g27_clip_to_saturation(buf[6]);
						}
						if (m_reverse_effects)
						{
							std::swap(left_coeff, right_coeff);
						}
						for (int j = 0; j < 1 /*3*/; j++)
						{
							new_effect.condition.right_sat[j] = saturation;
							new_effect.condition.left_sat[j] = saturation;
							new_effect.condition.right_coeff[j] = right_coeff;
							new_effect.condition.left_coeff[j] = left_coeff;
						}
						break;
					}
					case 0x0e:
					{
						// Friction
						new_effect.type = SDL_HAPTIC_FRICTION;
						new_effect.condition.direction = STEERING_DIRECTION;
						new_effect.condition.length = SDL_HAPTIC_INFINITY;
						const u8 k1 = buf[2];
						const u8 k2 = buf[3];
						const u8 s1 = buf[5] & 1;
						const u8 s2 = (buf[5] >> 4) & 1;
						s16 left_coeff = logitech_g27_friction_coeff_to_coeff(k1, s1);
						s16 right_coeff = logitech_g27_friction_coeff_to_coeff(k2, s2);
						const s16 saturation = logitech_g27_clip_to_saturation(buf[4]);
						if (m_reverse_effects)
						{
							std::swap(left_coeff, right_coeff);
						}
						for (int j = 0; j < 1 /*3*/; j++)
						{
							new_effect.condition.right_sat[j] = saturation;
							new_effect.condition.left_sat[j] = saturation;
							new_effect.condition.right_coeff[j] = right_coeff;
							new_effect.condition.left_coeff[j] = left_coeff;
						}
						break;
					}
					case 0x03:
					case 0x0d:
					{
						// Auto center spring/High resolution auto center spring
						new_effect.type = SDL_HAPTIC_SPRING;
						new_effect.condition.direction = STEERING_DIRECTION;
						new_effect.condition.length = SDL_HAPTIC_INFINITY;
						const u16 saturation = logitech_g27_clip_to_saturation(buf[4]);
						const u16 deadband = 2 * 0xFFFF / 255;
						s16 center = 0;
						s16 left_coeff = 0;
						s16 right_coeff = 0;
						if (buf[1] == 0x03)
						{
							const u8 k1 = buf[2] & (0xf >> 1);
							const u8 k2 = buf[3] & (0xf >> 1);
							left_coeff = logitech_g27_coeff_to_coeff(k1, 0);
							right_coeff = logitech_g27_coeff_to_coeff(k2, 0);
						}
						else
						{
							const u8 k1 = buf[2] & 0xf;
							const u8 k2 = buf[3] & 0xf;
							left_coeff = logitech_g27_high_resolution_coeff_to_coeff(k1, 0);
							right_coeff = logitech_g27_high_resolution_coeff_to_coeff(k2, 0);
						}
						if (m_reverse_effects)
						{
							std::swap(left_coeff, right_coeff);
						}
						for (int j = 0; j < 1 /*3*/; j++)
						{
							new_effect.condition.right_sat[j] = saturation;
							new_effect.condition.left_sat[j] = saturation;
							new_effect.condition.right_coeff[j] = right_coeff;
							new_effect.condition.left_coeff[j] = left_coeff;
							new_effect.condition.deadband[j] = deadband;
							new_effect.condition.center[j] = center;
						}
						break;
					}
					case 0x04:
					case 0x05:
					{
						// Sawtooth up/Sawtooth down
						// for reversing sawtooth, it should be mirroring the offset, then play the up/down counterpart
						if (buf[1] == 0x04)
							new_effect.type = !m_reverse_effects ? SDL_HAPTIC_SAWTOOTHUP : SDL_HAPTIC_SAWTOOTHDOWN;
						else
							new_effect.type = m_reverse_effects ? SDL_HAPTIC_SAWTOOTHUP : SDL_HAPTIC_SAWTOOTHDOWN;
						new_effect.type = buf[1] == 0x04 ? SDL_HAPTIC_SAWTOOTHUP : SDL_HAPTIC_SAWTOOTHDOWN;
						new_effect.periodic.direction = STEERING_DIRECTION;
						new_effect.periodic.length = SDL_HAPTIC_INFINITY;
						const u8 l1 = buf[2];
						const u8 l2 = buf[3];
						const u8 l0 = buf[4];
						const u8 t3 = buf[6] >> 4;
						const u8 inc = buf[6] & 0xf;

						const s16 amplitude = (l1 - l2);
						const s16 progress = buf[1] == 0x04 ? l0 - l2 : l1 - l0;

						if (amplitude <= 0 || inc == 0 || t3 == 0)
						{
							logitech_g27_log.error("cannot evaluate period and phase for saw tooth effect, l1 %u l2 %u inc %u t3 %u", l1, l2, inc, t3);
							new_effect.periodic.period = 0;
							new_effect.periodic.phase = 0;
						}
						else
						{
							new_effect.periodic.period = (amplitude * logitech_g27_loops_to_ms(t3, !m_fixed_loop)) / inc;
							if (progress < 0)
							{
								logitech_g27_log.error("cannot evaluate phase for saw tooth effect, l1 %u l2 %u l0 %u", l1, l2, l0);
								new_effect.periodic.phase = 0;
							}
							else
								new_effect.periodic.phase = 36000 * progress / amplitude;
						}

						// TODO implement fallback if we actually find games abusing the above fail cases for constant force on l0

						new_effect.periodic.offset = logitech_g27_force_to_level((l1 + l2) / 2, m_reverse_effects);
						new_effect.periodic.magnitude = logitech_g27_force_to_level(std::abs(amplitude) / 2);

						break;
					}
					case 0x06:
					{
						// Trapezoid, convert to SDL_HAPTIC_SQUARE or SDL_HAPTIC_TRIANGLE
						// TODO full accuracy will need some kind of rendering thread, cannot be represented with a single effect
						new_effect.periodic.direction = STEERING_DIRECTION;
						new_effect.periodic.length = SDL_HAPTIC_INFINITY;
						const u8 l1 = buf[2];
						const u8 l2 = buf[3];
						const u8 t1 = buf[4];
						const u8 t2 = buf[5];
						const u8 t3 = buf[6] >> 4;
						const u8 s = buf[6] & 0xf;
						const u16 total_flat_time = logitech_g27_loops_to_ms(t1 + t2, !m_fixed_loop);
						const u16 total_slope_time = (((l1 - l2) * logitech_g27_loops_to_ms(t3, !m_fixed_loop)) / s) * 2;
						if (total_flat_time > total_slope_time)
						{
							new_effect.type = SDL_HAPTIC_SQUARE;
						}
						else
						{
							new_effect.type = SDL_HAPTIC_TRIANGLE;
						}
						new_effect.periodic.period = total_slope_time + total_flat_time;
						// when mirroring waves, both offset and magnitude should be mirrored
						new_effect.periodic.offset = logitech_g27_force_to_level((l1 + l2) / 2, m_reverse_effects);
						new_effect.periodic.magnitude = logitech_g27_force_to_level(l1, m_reverse_effects) - new_effect.periodic.offset;
						break;
					}
					case 0x07:
					{
						// Rectangle, convert to SDL_HAPTIC_SQUARE
						// TODO full accuracy will need some kind of rendering thread, cannot be represented with a single effect
						new_effect.type = SDL_HAPTIC_SQUARE;
						new_effect.periodic.direction = STEERING_DIRECTION;
						new_effect.periodic.length = SDL_HAPTIC_INFINITY;
						const u8 l1 = buf[2];
						const u8 l2 = buf[3];
						const u8 t1 = buf[4];
						const u8 t2 = buf[5];
						const u8 p = buf[6];
						new_effect.periodic.period = logitech_g27_loops_to_ms(t1, !m_fixed_loop) + logitech_g27_loops_to_ms(t2, !m_fixed_loop);
						new_effect.periodic.offset = logitech_g27_force_to_level((l1 + l2) / 2, m_reverse_effects);
						new_effect.periodic.magnitude = logitech_g27_force_to_level(l1, m_reverse_effects) - new_effect.periodic.offset;
						if (new_effect.periodic.period != 0)
							new_effect.periodic.phase = 36000 * logitech_g27_loops_to_ms(p, !m_fixed_loop) / new_effect.periodic.period;
						else
						{
							logitech_g27_log.error("cannot evaluate phase for square effect");
							new_effect.periodic.phase = 0;
						}
						break;
					}
					case 0x08:
					case 0x09:
					{
						// Variable/Ramp, convert to SDL_HAPTIC_CONSTANT / SDL_HAPTIC_RAMP
						if (i % 2 != 0)
						{
							continue;
						}
						const u8 l1 = buf[2];
						const u8 l2 = buf[3];
						const u8 t1 = buf[4] >> 4;
						const u8 s1 = buf[4] & 0xf;
						const u8 t2 = buf[5] >> 4;
						const u8 s2 = buf[5] & 0xf;
						const u8 d1 = buf[6] & 1;
						const u8 d2 = (buf[6] >> 4) & 1;
						if (buf[1] == 0x08)
						{
							new_effect.type = SDL_HAPTIC_CONSTANT;
							const u8 t = i == 0 ? t1 : t2;
							const u8 s = i == 0 ? s1 : s2;
							const u8 d = i == 0 ? d1 : d2;
							const u8 l = i == 0 ? l1 : l2;
							new_effect.constant.length = SDL_HAPTIC_INFINITY;
							new_effect.constant.direction = STEERING_DIRECTION;
							if (s == 0 || t == 0)
							{
								// gran turismo 6 does this, gives a variable force with no step so it just behaves as constant force
								new_effect.constant.level = logitech_g27_force_to_level(l, m_reverse_effects);
								// hack: gran turismo 6 spams download and play
								update_hack = true;
							}
							else
							{
								new_effect.constant.level = d ? -0x7FFF : 0x7FFF;
								if (m_reverse_effects)
									new_effect.constant.level *= -1;

								// TODO full accuracy will need some kind of rendering thread, cannot be represented with a single effect
								const s16 begin_level = logitech_g27_force_to_level(l, m_reverse_effects);
								if ((new_effect.constant.level > 0 && begin_level > 0) || (new_effect.constant.level < 0 && begin_level < 0))
								{
									new_effect.constant.attack_level = begin_level * 0x7FFF / new_effect.constant.level;
								}
							}
						}
						else
						{
							new_effect.type = SDL_HAPTIC_RAMP;
							new_effect.ramp.direction = STEERING_DIRECTION;
							if (l2 > l1)
								logitech_g27_log.error("min force is larger than max force in ramp effect, l1 %u l2 %u", l1, l2);
							const s16 l1_converted = logitech_g27_force_to_level(l1, m_reverse_effects);
							const s16 l2_converted = logitech_g27_force_to_level(l2, m_reverse_effects);
							new_effect.ramp.start = d1 ? l1_converted : l2_converted;
							new_effect.ramp.end = d1 ? l2_converted : l1_converted;
							new_effect.ramp.length = 0;
							if (s2 == 0 || t2 == 0)
								logitech_g27_log.error("cannot evaluate slope for ramp effect, loops per step %u level per step %u", t2, s2);
							else
								new_effect.ramp.length = std::abs(l1 - l2) * logitech_g27_loops_to_ms(t2, !m_fixed_loop) / s2;
						}
						break;
					}
					case 0x0a:
					{
						// Square
						new_effect.type = SDL_HAPTIC_SQUARE;
						new_effect.periodic.direction = STEERING_DIRECTION;
						const u8 a = buf[2];
						const u8 tl = buf[3];
						const u8 th = buf[4];
						const u8 n = buf[5];
						const u16 t = (th << 8) | tl;
						new_effect.periodic.period = logitech_g27_loops_to_ms(t * 2, !m_fixed_loop);
						new_effect.periodic.magnitude = logitech_g27_amplitude_to_magnitude(a);
						if (n == 0)
							new_effect.periodic.length = new_effect.periodic.period * 256;
						else
							new_effect.periodic.length = new_effect.periodic.period * n;
						break;
					}
					default:
					{
						unknown_effect = true;
						break;
					}
					}

					if (unknown_effect)
					{
						logitech_g27_log.error("Command %02x %02x %02x %02x %02x %02x %02x with unknown effect ignored", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
						continue;
					}

					const bool play_effect = (cmd == 0x01 || (cmd == 0x0c && m_effect_slots[i].effect_id == -1));

					if (update_hack)
					{
						if (m_effect_slots[i].effect_id == -1)
							update_hack = false;
						if (m_effect_slots[i].last_effect.type != new_effect.type)
							update_hack = false;
					}

					if (cmd == 0x00 || play_effect)
					{
						if (m_effect_slots[i].effect_id != -1 && m_haptic_handle && !update_hack)
						{
							SDL_DestroyHapticEffect(m_haptic_handle, m_effect_slots[i].effect_id);
							m_effect_slots[i].effect_id = -1;
						}
						if (m_haptic_handle && m_effect_slots[i].effect_id == -1)
						{
							m_effect_slots[i].effect_id = SDL_CreateHapticEffect(m_haptic_handle, &new_effect);
						}
						if (update_hack)
						{
							if (!SDL_UpdateHapticEffect(m_haptic_handle, m_effect_slots[i].effect_id, &new_effect))
								logitech_g27_log.error("Failed refreshing slot %d sdl effect %d, %s", i, new_effect.type, SDL_GetError());
						}
						m_effect_slots[i].state = logitech_g27_ffb_state::downloaded;
						m_effect_slots[i].last_effect = new_effect;
						m_effect_slots[i].last_update = SDL_GetTicks();
						if (m_effect_slots[i].effect_id == -1 && m_haptic_handle)
						{
							logitech_g27_log.error("Failed uploading effect %02x %02x %02x %02x %02x %02x %02x to slot %i, %s", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], i, SDL_GetError());
						}
					}
					if (play_effect && m_haptic_handle)
					{
						if (m_effect_slots[i].effect_id != -1)
						{
							if (!SDL_RunHapticEffect(m_haptic_handle, m_effect_slots[i].effect_id, 1))
							{
								logitech_g27_log.error("Failed playing sdl effect %d on slot %d, %s\n", m_effect_slots[i].last_effect.type, i, SDL_GetError());
							}
						}
						else
						{
							logitech_g27_log.error("Tried to play effect slot %d with sdl effect %d, but upload failed previously", i, m_effect_slots[i].last_effect.type);
						}
						m_effect_slots[i].state = logitech_g27_ffb_state::playing;
					}
					if (cmd == 0xc && !play_effect && m_haptic_handle)
					{
						if (!SDL_UpdateHapticEffect(m_haptic_handle, m_effect_slots[i].effect_id, &new_effect))
						{
							logitech_g27_log.error("Failed refreshing slot %d sdl effect %d, %s", i, new_effect.type, SDL_GetError());
						}
					}
				}
				break;
			}
			case 0x02:
			case 0x03:
			{
				for (int i = 0; i < 4; i++)
				{
					// Play/Stop
					if (!(slot_mask & (1 << i)))
					{
						continue;
					}
					if (m_effect_slots[i].state == logitech_g27_ffb_state::playing || m_effect_slots[i].state == logitech_g27_ffb_state::downloaded)
					{
						m_effect_slots[i].state = cmd == 0x02 ? logitech_g27_ffb_state::playing : logitech_g27_ffb_state::downloaded;
						if (m_haptic_handle != nullptr)
						{
							if (m_effect_slots[i].effect_id == -1)
							{
								m_effect_slots[i].effect_id = SDL_CreateHapticEffect(m_haptic_handle, &m_effect_slots[i].last_effect);
							}
							if (m_effect_slots[i].effect_id != -1)
							{
								if (cmd == 0x02)
								{
									if (!SDL_RunHapticEffect(m_haptic_handle, m_effect_slots[i].effect_id, 1))
									{
										logitech_g27_log.error("Failed playing sdl effect %d on slot %d, %s\n", m_effect_slots[i].last_effect.type, i, SDL_GetError());
									}
								}
								else
								{
									if (!SDL_StopHapticEffect(m_haptic_handle, m_effect_slots[i].effect_id))
									{
										logitech_g27_log.error("Failed stopping sdl effect %d on slot %d, %s\n", m_effect_slots[i].last_effect.type, i, SDL_GetError());
									}
								}
							}
							else
							{
								if (cmd == 0x02)
								{
									logitech_g27_log.error("Tried to play effect slot %d with sdl effect %d, but upload failed previously", i, m_effect_slots[i].last_effect.type);
								}
								else
								{
									logitech_g27_log.error("Tried to stop effect slot %d with sdl effect %d, but upload failed previously", i, m_effect_slots[i].last_effect.type);
								}
							}
						}
					}
					else
					{
						if (cmd == 0x02)
						{
							logitech_g27_log.error("Tried to play effect slot %d but it was never uploaded\n", i);
						}
						else
						{
							logitech_g27_log.error("Tried to stop effect slot %d but it was never uploaded\n", i);
						}
					}
				}
				break;
			}
			case 0x0e:
			{
				// Set Default Spring
				const u8 k1 = buf[2] & (0xf >> 1);
				const u8 k2 = buf[3] & (0xf >> 1);
				const u16 saturation = logitech_g27_clip_to_saturation(buf[4]);
				s16 left_coeff = logitech_g27_coeff_to_coeff(k1, 0);
				s16 right_coeff = logitech_g27_coeff_to_coeff(k2, 0);
				const u16 deadband = 2 * 0xFFFF / 255;
				s16 center = 0;
				if (m_reverse_effects)
				{
					std::swap(left_coeff, right_coeff);
				}
				for (int i = 0; i < 1 /*3*/; i++)
				{
					// TODO direction cfg
					m_default_spring_effect.condition.right_sat[i] = saturation;
					m_default_spring_effect.condition.left_sat[i] = saturation;
					m_default_spring_effect.condition.right_coeff[i] = right_coeff;
					m_default_spring_effect.condition.left_coeff[i] = left_coeff;
					m_default_spring_effect.condition.deadband[i] = deadband;
					m_default_spring_effect.condition.center[i] = center;
				}

				if (m_haptic_handle == nullptr)
				{
					break;
				}

				if (m_default_spring_effect_id == -1)
				{
					m_default_spring_effect_id = SDL_CreateHapticEffect(m_haptic_handle, &m_default_spring_effect);
					if (m_default_spring_effect_id == -1)
					{
						logitech_g27_log.error("Failed creating default spring effect, %s", SDL_GetError());
					}
				}
				else
				{
					if (!SDL_UpdateHapticEffect(m_haptic_handle, m_default_spring_effect_id, &m_default_spring_effect))
					{
						logitech_g27_log.error("Failed updating default spring effect, %s", SDL_GetError());
					}
				}
				break;
			}
			case 0x04:
			case 0x05:
			{
				// Default spring on/Default spring off
				if (m_haptic_handle == nullptr)
				{
					break;
				}

				if (m_default_spring_effect_id == -1)
				{
					m_default_spring_effect_id = SDL_CreateHapticEffect(m_haptic_handle, &m_default_spring_effect);
					if (m_default_spring_effect_id == -1)
					{
						logitech_g27_log.error("Failed creating default spring effect, %s", SDL_GetError());
					}
				}
				if (m_default_spring_effect_id != -1)
				{
					if (cmd == 0x04)
					{
						if (!SDL_RunHapticEffect(m_haptic_handle, m_default_spring_effect_id, 1))
						{
							logitech_g27_log.error("Failed playing default spring effect, %s", SDL_GetError());
						}
					}
					else
					{
						if (!SDL_StopHapticEffect(m_haptic_handle, m_default_spring_effect_id))
						{
							logitech_g27_log.error("Failed stopping default spring effect, %s", SDL_GetError());
						}
					}
				}
				break;
			}
			case 0x08:
			{
				// Normal Mode / Extended
				logitech_g27_log.error("Normal mode restore command ignored");
				break;
			}
			case 0x09:
			{
				// Set LED
				// TODO this is practically disabled at the moment, due to effect slot matching prior
				// need to figure out what this command actually does first, since RPM light command has priority to SDL_SetJoystickLED
				if (m_led_joystick_handle == nullptr)
				{
					break;
				}

				u8 new_led_level = 0;
				for (int i = 0; i < 8; i++)
				{
					new_led_level += (buf[1] & (1 << i)) ? 1 : 0;
				}
				const u8 intensity = new_led_level * 255 / 8;
				SDL_SetJoystickLED(m_led_joystick_handle, intensity, intensity, intensity);
				break;
			}
			case 0x0a:
			{
				// Set watchdog
				logitech_g27_log.error("Watchdog command with duration of %u loops ignored", buf[1]);
				break;
			}
			case 0x0b:
			{
				// Raw mode
				logitech_g27_log.error("Raw mode command ignored");
				break;
			}
			case 0x0d:
			{
				// Fixed time loop toggle
				m_fixed_loop = buf[1] ? true : false;
				if (!m_fixed_loop)
				{
					logitech_g27_log.error("as fast as possible mode requested, effect durations might be inaccurate");
				}
				break;
			}
			case 0x0f:
			{
				// Set dead band
				logitech_g27_log.error("Set dead band command ignored");
				break;
			}
			default:
			{
				logitech_g27_log.error("Unknown command %02x %02x %02x %02x %02x %02x %02x ignored", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
				break;
			}
			}
		}
	}
}

#endif
