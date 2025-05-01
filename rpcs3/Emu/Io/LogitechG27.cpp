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

usb_device_logitech_g27::usb_device_logitech_g27(u32 controller_index, const std::array<u8, 7>& location)
	: usb_device_emulated(location), m_controller_index(controller_index)
{
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{0x0200, 0, 0, 0, 16, 0x046d, 0xc29b, 0x1350, 1, 2, 0, 1});

	// parse the raw response like with passthrough device
	static const uint8_t raw_config[] = {0x9, 0x2, 0x29, 0x0, 0x1, 0x1, 0x4, 0x80, 0x31, 0x9, 0x4, 0x0, 0x0, 0x2, 0x3, 0x0, 0x0, 0x0, 0x9, 0x21, 0x11, 0x1, 0x21, 0x1, 0x22, 0x85, 0x0, 0x7, 0x5, 0x81, 0x3, 0x10, 0x0, 0x2, 0x7, 0x5, 0x1, 0x3, 0x10, 0x0, 0x2};
	auto& conf = device.add_node(UsbDescriptorNode(raw_config[0], raw_config[1], &raw_config[2]));
	for (unsigned int index = raw_config[0]; index < sizeof(raw_config);)
	{
		conf.add_node(UsbDescriptorNode(raw_config[index], raw_config[index + 1], &raw_config[index + 2]));
		index += raw_config[index];
	}

	// Initialize effect slots
	for (int i = 0; i < 4; i++)
	{
		m_effect_slots[i].state = G27_FFB_INACTIVE;
		m_effect_slots[i].effect_id = -1;
	}

	SDL_HapticDirection direction = {
		.type = SDL_HAPTIC_POLAR,
		.dir = {27000, 0}};
	m_default_spring_effect.type = SDL_HAPTIC_SPRING;
	m_default_spring_effect.condition.direction = direction;
	m_default_spring_effect.condition.length = SDL_HAPTIC_INFINITY;
	// for (int i = 0;i < 3;i++)
	for (int i = 0; i < 1; i++)
	{
		m_default_spring_effect.condition.right_sat[i] = 0x7FFF;
		m_default_spring_effect.condition.left_sat[i] = 0x7FFF;
		m_default_spring_effect.condition.right_coeff[i] = 0x7FFF;
		m_default_spring_effect.condition.left_coeff[i] = 0x7FFF;
	}

	{
		const std::lock_guard<std::mutex> lock(m_thread_control_mutex);
		m_stop_thread = false;
	}

	g_cfg_logitech_g27.load();

	bool sdl_init_state = sdl_instance::get_instance().initialize();

	m_enabled = g_cfg_logitech_g27.enabled.get() && sdl_init_state;

	if (!m_enabled)
		return;

	m_house_keeping_thread = std::thread([this]()
		{
			while (true)
			{
				this->m_thread_control_mutex.lock();
				if (this->m_stop_thread)
				{
					break;
				}
				this->m_thread_control_mutex.unlock();
				this->sdl_refresh();
				std::this_thread::sleep_for(std::chrono::seconds(5));
			}
			this->m_thread_control_mutex.unlock();
		});
}

bool usb_device_logitech_g27::open_device()
{
	return m_enabled;
}

static void clear_sdl_joysticks(std::map<uint32_t, std::vector<SDL_Joystick*>>& joysticks)
{
	for (auto joystick_type : joysticks)
	{
		for (auto joystick : joystick_type.second)
		{
			if (joystick)
				SDL_CloseJoystick(joystick);
		}
	}
	joysticks.clear();
}

usb_device_logitech_g27::~usb_device_logitech_g27()
{
	// stop the house keeping thread
	{
		const std::lock_guard<std::mutex> lock(m_thread_control_mutex);
		m_stop_thread = true;
	}

	// Close sdl handles
	{
		const std::lock_guard<std::mutex> lock(m_sdl_handles_mutex);
		if (m_haptic_handle != nullptr)
		{
			SDL_CloseHaptic(m_haptic_handle);
		}
		clear_sdl_joysticks(m_joysticks);
	}

	// wait for the house keeping thread to finish
	if (m_enabled)
		m_house_keeping_thread.join();
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
	transfer->fake = true;
	transfer->expected_count = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	transfer->expected_time = get_timestamp() + 100;

	// Log these for now, might not need to implement anything
	usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
}

static bool sdl_joysticks_equal(std::map<uint32_t, std::vector<SDL_Joystick*>>& left, std::map<uint32_t, std::vector<SDL_Joystick*>>& right)
{
	if (left.size() != right.size())
	{
		return false;
	}
	for (auto left_joysticks_of_type : left)
	{
		auto right_joysticks_of_type = right.find(left_joysticks_of_type.first);
		if (right_joysticks_of_type == right.end())
		{
			return false;
		}
		if (left_joysticks_of_type.second.size() != right_joysticks_of_type->second.size())
		{
			return false;
		}
		for (auto left_joystick : left_joysticks_of_type.second)
		{
			bool found = false;
			for (auto right_joystick : right_joysticks_of_type->second)
			{
				if (left_joystick == right_joystick)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				return false;
			}
		}
	}
	return true;
}

static inline logitech_g27_sdl_mapping get_runtime_mapping()
{
	logitech_g27_sdl_mapping mapping;

#define CONVERT_MAPPING(name)                                                                  \
	{                                                                                          \
		mapping.name.device_type_id = g_cfg_logitech_g27.name.device_type_id.get();            \
		mapping.name.type = static_cast<sdl_mapping_type>(g_cfg_logitech_g27.name.type.get()); \
		mapping.name.id = static_cast<uint8_t>(g_cfg_logitech_g27.name.id.get());              \
		mapping.name.hat = static_cast<hat_component>(g_cfg_logitech_g27.name.hat.get());      \
		mapping.name.reverse = g_cfg_logitech_g27.name.reverse.get();                          \
		mapping.name.positive_axis = false;                                                    \
	}

	CONVERT_MAPPING(steering);
	CONVERT_MAPPING(throttle);
	CONVERT_MAPPING(brake);
	CONVERT_MAPPING(clutch);
	CONVERT_MAPPING(shift_up);
	CONVERT_MAPPING(shift_down);

	CONVERT_MAPPING(up);
	CONVERT_MAPPING(down);
	CONVERT_MAPPING(left);
	CONVERT_MAPPING(right);

	CONVERT_MAPPING(triangle);
	CONVERT_MAPPING(cross);
	CONVERT_MAPPING(square);
	CONVERT_MAPPING(circle);

	CONVERT_MAPPING(l2);
	CONVERT_MAPPING(l3);
	CONVERT_MAPPING(r2);
	CONVERT_MAPPING(r3);

	CONVERT_MAPPING(plus);
	CONVERT_MAPPING(minus);

	CONVERT_MAPPING(dial_clockwise);
	CONVERT_MAPPING(dial_anticlockwise);

	CONVERT_MAPPING(select);
	CONVERT_MAPPING(pause);

	CONVERT_MAPPING(shifter_1);
	CONVERT_MAPPING(shifter_2);
	CONVERT_MAPPING(shifter_3);
	CONVERT_MAPPING(shifter_4);
	CONVERT_MAPPING(shifter_5);
	CONVERT_MAPPING(shifter_6);
	CONVERT_MAPPING(shifter_r);

#undef CONVERT_MAPPING

	return mapping;
}

void usb_device_logitech_g27::sdl_refresh()
{
	g_cfg_logitech_g27.m_mutex.lock();
	m_mapping = get_runtime_mapping();

	m_reverse_effects = g_cfg_logitech_g27.reverse_effects.get();

	uint32_t ffb_vendor_id = g_cfg_logitech_g27.ffb_device_type_id.get() >> 16;
	uint32_t ffb_product_id = g_cfg_logitech_g27.ffb_device_type_id.get() & 0xFFFF;

	uint32_t led_vendor_id = g_cfg_logitech_g27.led_device_type_id.get() >> 16;
	uint32_t led_product_id = g_cfg_logitech_g27.led_device_type_id.get() & 0xFFFF;
	g_cfg_logitech_g27.m_mutex.unlock();

	SDL_Joystick* new_led_joystick_handle = nullptr;
	SDL_Haptic* new_haptic_handle = nullptr;
	std::map<uint32_t, std::vector<SDL_Joystick*>> new_joysticks;

	int joystick_count;
	SDL_JoystickID* joystick_ids = SDL_GetJoysticks(&joystick_count);
	if (joystick_ids != nullptr)
	{
		for (int i = 0; i < joystick_count; i++)
		{
			SDL_Joystick* cur_joystick = SDL_OpenJoystick(joystick_ids[i]);
			if (cur_joystick == nullptr)
			{
				logitech_g27_log.error("Failed opening joystick %d, %s", joystick_ids[i], SDL_GetError());
				continue;
			}
			uint16_t cur_vendor_id = SDL_GetJoystickVendor(cur_joystick);
			uint16_t cur_product_id = SDL_GetJoystickProduct(cur_joystick);
			uint32_t joystick_type_id = (cur_vendor_id << 16) | cur_product_id;
			auto joysticks_of_type = new_joysticks.find(joystick_type_id);
			if (joysticks_of_type == new_joysticks.end())
			{
				std::vector<SDL_Joystick*> joystick_group = {cur_joystick};
				new_joysticks[joystick_type_id] = joystick_group;
			}
			else
			{
				joysticks_of_type->second.push_back(cur_joystick);
			}

			if (cur_vendor_id == ffb_vendor_id && cur_product_id == ffb_product_id && new_haptic_handle == nullptr)
			{
				SDL_Haptic* cur_haptic = SDL_OpenHapticFromJoystick(cur_joystick);
				if (cur_haptic == nullptr)
				{
					logitech_g27_log.error("Failed opening haptic device from selected ffb device %04x:%04x", cur_vendor_id, cur_product_id);
				}
				else
				{
					new_haptic_handle = cur_haptic;
				}
			}

			if (cur_vendor_id == led_vendor_id && cur_product_id == led_product_id && new_led_joystick_handle == nullptr)
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

	bool joysticks_changed = !sdl_joysticks_equal(m_joysticks, new_joysticks);
	bool haptic_changed = m_haptic_handle != new_haptic_handle;
	bool led_joystick_changed = m_led_joystick_handle != new_led_joystick_handle;

	// if we should touch the mutex
	if (joysticks_changed || haptic_changed || led_joystick_changed)
	{
		const std::lock_guard<std::mutex> lock(m_sdl_handles_mutex);
		if (joysticks_changed)
		{
			clear_sdl_joysticks(m_joysticks);
			m_joysticks = new_joysticks;
		}
		// reset effects if the ffb device is changed
		if (haptic_changed)
		{
			if (m_haptic_handle)
				SDL_CloseHaptic(m_haptic_handle);
			for (int i = 0; i < 4; i++)
			{
				m_effect_slots[i].effect_id = -1;
			}
			m_default_spring_effect_id = -1;
			m_led_joystick_handle = new_led_joystick_handle;
			m_haptic_handle = new_haptic_handle;
		}
		if (led_joystick_changed)
		{
			m_led_joystick_handle = new_led_joystick_handle;
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

static inline int16_t logitech_g27_force_to_level(uint8_t force)
{
	if (force == 127 || force == 128)
	{
		return 0;
	}
	if (force > 128)
	{
		return ((force - 128) * 0x7FFF) / (255 - 128);
	}
	return ((127 - force) * 0x7FFF * -1) / (127 - 0);
}

static inline int16_t logitech_g27_position_to_center(uint8_t left, uint8_t right)
{
	uint16_t center_unsigned = (((right + left) * 0xFFFF) / 255) / 2;
	return center_unsigned - 0x8000;
}

static inline int16_t logitech_g27_high_resolution_position_to_center(uint16_t left, uint16_t right)
{
	uint16_t center_unsigned = (((right + left) * 0xFFFF) / (0xFFFF >> 5)) / 2;
	return center_unsigned - 0x8000;
}

static inline uint16_t logitech_g27_position_to_width(uint8_t left, uint8_t right)
{
	return ((right - left) * 0xFFFF) / 255;
}

static inline uint16_t logitech_g27_high_resolution_position_to_width(uint16_t left, uint16_t right)
{
	return ((right - left) * 0xFFFF) / (0xFFFF >> 5);
}

static inline int16_t logitech_g27_coeff_to_coeff(uint8_t coeff, uint8_t invert)
{
	if (!invert)
	{
		return (coeff * 0x7FFF) / 7;
	}
	return (coeff * 0x7FFF * -1) / 7;
}

static inline int16_t logitech_g27_high_resolution_coeff_to_coeff(uint8_t coeff, uint8_t invert)
{
	if (!invert)
	{
		return (coeff * 0x7FFF) / 15;
	}
	return (coeff * 0x7FFF * -1) / 15;
}

static inline int16_t logitech_g27_friction_coeff_to_coeff(uint8_t coeff, uint8_t invert)
{
	if (!invert)
	{
		return (coeff * 0x7FFF) / 255;
	}
	return (coeff * 0x7FFF * -1) / 255;
}

static inline int16_t logitech_g27_clip_to_saturation(uint8_t clip)
{
	return (clip * 0x7FFF) / 255;
}

static inline int16_t logitech_g27_amplitude_to_magnitude(uint8_t amplitude)
{
	return ((amplitude * 0x7FFF) / 2) / 255;
}

static inline uint16_t logitech_g27_loops_to_ms(uint16_t loops, bool afap)
{
	if (afap)
	{
		return loops;
	}
	return loops * 2;
}

static inline uint16_t axis_to_logitech_g27_steering(int16_t axis)
{
	uint16_t unsigned_axis = axis + 0x8000;
	return (unsigned_axis * (0xFFFF >> 2)) / 0xFFFF;
}

static inline uint8_t axis_to_logitech_g27_pedal(int16_t axis)
{
	uint16_t unsigned_axis = axis + 0x8000;
	return (unsigned_axis * (0xFF)) / 0xFFFF;
}

extern bool is_input_allowed();

static uint8_t sdl_hat_to_logitech_g27_hat(uint8_t sdl_hat)
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
	}
	return 0;
}

static uint8_t hat_components_to_logitech_g27_hat(bool up, bool down, bool left, bool right)
{
	uint8_t sdl_hat = 0;
	if (up)
		sdl_hat = sdl_hat | SDL_HAT_UP;
	if (down)
		sdl_hat = sdl_hat | SDL_HAT_DOWN;
	if (left)
		sdl_hat = sdl_hat | SDL_HAT_LEFT;
	if (right)
		sdl_hat = sdl_hat | SDL_HAT_RIGHT;
	return sdl_hat_to_logitech_g27_hat(sdl_hat);
}

static bool fetch_sdl_as_button(SDL_Joystick* joystick, const sdl_mapping& mapping)
{
	switch (mapping.type)
	{
	case MAPPING_BUTTON:
	{
		bool pressed = SDL_GetJoystickButton(joystick, mapping.id);
		return mapping.reverse ? !pressed : pressed;
	}
	case MAPPING_HAT:
	{
		uint8_t hat_value = SDL_GetJoystickHat(joystick, mapping.id);
		bool pressed = false;
		switch (mapping.hat)
		{
		case HAT_UP:
			pressed = (hat_value & SDL_HAT_UP) ? true : false;
			break;
		case HAT_DOWN:
			pressed = (hat_value & SDL_HAT_DOWN) ? true : false;
			break;
		case HAT_LEFT:
			pressed = (hat_value & SDL_HAT_LEFT) ? true : false;
			break;
		case HAT_RIGHT:
			pressed = (hat_value & SDL_HAT_RIGHT) ? true : false;
			break;
		case HAT_NONE:
			break;
		}
		return mapping.reverse ? !pressed : pressed;
	}
	case MAPPING_AXIS:
	{
		int32_t axis_value = SDL_GetJoystickAxis(joystick, mapping.id);
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

static int16_t fetch_sdl_as_axis(SDL_Joystick* joystick, const sdl_mapping& mapping)
{
	const static int16_t MAX = 0x7FFF;
	const static int16_t MIN = -0x8000;
	const static int16_t MID = 0;

	switch (mapping.type)
	{
	case MAPPING_BUTTON:
	{
		bool pressed = SDL_GetJoystickButton(joystick, mapping.id);
		if (mapping.reverse)
		{
			pressed = !pressed;
		}
		int16_t pressed_value = mapping.positive_axis ? MAX : MIN;
		return pressed ? pressed_value : MID;
	}
	case MAPPING_HAT:
	{
		uint8_t hat_value = SDL_GetJoystickHat(joystick, mapping.id);
		bool pressed = false;
		switch (mapping.hat)
		{
		case HAT_UP:
			pressed = (hat_value & SDL_HAT_UP) ? true : false;
			break;
		case HAT_DOWN:
			pressed = (hat_value & SDL_HAT_DOWN) ? true : false;
			break;
		case HAT_LEFT:
			pressed = (hat_value & SDL_HAT_LEFT) ? true : false;
			break;
		case HAT_RIGHT:
			pressed = (hat_value & SDL_HAT_RIGHT) ? true : false;
			break;
		case HAT_NONE:
			break;
		}
		if (mapping.reverse)
		{
			pressed = !pressed;
		}
		int16_t pressed_value = mapping.positive_axis ? MAX : MIN;
		return pressed ? pressed_value : MID;
	}
	case MAPPING_AXIS:
	{
		int32_t axis_value = SDL_GetJoystickAxis(joystick, mapping.id);
		if (mapping.reverse)
			axis_value = axis_value * (-1);
		if (axis_value > MAX)
			axis_value = MAX;
		if (axis_value < MIN)
			axis_value = MIN;
		if (axis_value == (MIN + 1))
			axis_value = MIN;
		return axis_value;
	}
	}
	return 0;
}

static int16_t fetch_sdl_axis_avg(std::map<uint32_t, std::vector<SDL_Joystick*>>& joysticks, const sdl_mapping& mapping)
{
	const static int16_t MAX = 0x7FFF;
	const static int16_t MIN = -0x8000;

	auto joysticks_of_type = joysticks.find(mapping.device_type_id);
	if (joysticks_of_type == joysticks.end())
	{
		return mapping.reverse ? MAX : MIN;
	}

	if (joysticks_of_type->second.size() == 0)
	{
		return mapping.reverse ? MAX : MIN;
	}

	// TODO account for deadzone and only pick up active devices
	int32_t sdl_joysticks_total_value = 0;
	for (auto joystick : joysticks_of_type->second)
	{
		sdl_joysticks_total_value += fetch_sdl_as_axis(joystick, mapping);
	}

	return sdl_joysticks_total_value / joysticks_of_type->second.size();
}

static bool sdl_to_logitech_g27_button(std::map<uint32_t, std::vector<SDL_Joystick*>>& joysticks, const sdl_mapping& mapping)
{
	auto joysticks_of_type = joysticks.find(mapping.device_type_id);
	if (joysticks_of_type == joysticks.end())
	{
		return mapping.reverse;
	}

	if (joysticks_of_type->second.size() == 0)
	{
		return mapping.reverse;
	}

	bool pressed = false;
	for (auto joystick : joysticks_of_type->second)
	{
		pressed = pressed || fetch_sdl_as_button(joystick, mapping);
	}
	return pressed;
}

static uint16_t sdl_to_logitech_g27_steering(std::map<uint32_t, std::vector<SDL_Joystick*>>& joysticks, const sdl_mapping& mapping)
{
	int16_t avg = fetch_sdl_axis_avg(joysticks, mapping);
	uint16_t unsigned_avg = avg + 0x8000;
	return unsigned_avg * (0xFFFF >> 2) / 0xFFFF;
}

static uint8_t sdl_to_logitech_g27_pedal(std::map<uint32_t, std::vector<SDL_Joystick*>>& joysticks, const sdl_mapping& mapping)
{
	int16_t avg = fetch_sdl_axis_avg(joysticks, mapping);
	uint16_t unsigned_avg = avg + 0x8000;
	return unsigned_avg * 0xFF / 0xFFFF;
}

static inline void set_bit(uint8_t* buf, int bit_num, bool set)
{
	int byte_num = bit_num / 8;
	bit_num = bit_num % 8;
	uint8_t mask = 1 << bit_num;
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
		uint16_t steering = sdl_to_logitech_g27_steering(m_joysticks, m_mapping.steering);
		uint8_t throttle = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.throttle);
		uint8_t brake = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.brake);
		uint8_t clutch = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.clutch);
		bool shift_up = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_up);
		bool shift_down = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_down);

		bool up = sdl_to_logitech_g27_button(m_joysticks, m_mapping.up);
		bool down = sdl_to_logitech_g27_button(m_joysticks, m_mapping.down);
		bool left = sdl_to_logitech_g27_button(m_joysticks, m_mapping.left);
		bool right = sdl_to_logitech_g27_button(m_joysticks, m_mapping.right);

		bool triangle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.triangle);
		bool cross = sdl_to_logitech_g27_button(m_joysticks, m_mapping.cross);
		bool square = sdl_to_logitech_g27_button(m_joysticks, m_mapping.square);
		bool circle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.circle);

		bool l2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l2);
		bool l3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l3);
		bool r2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r2);
		bool r3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r3);

		bool plus = sdl_to_logitech_g27_button(m_joysticks, m_mapping.plus);
		bool minus = sdl_to_logitech_g27_button(m_joysticks, m_mapping.minus);

		bool dial_clockwise = sdl_to_logitech_g27_button(m_joysticks, m_mapping.dial_clockwise);
		bool dial_anticlockwise = sdl_to_logitech_g27_button(m_joysticks, m_mapping.dial_anticlockwise);

		bool select = sdl_to_logitech_g27_button(m_joysticks, m_mapping.select);
		bool pause = sdl_to_logitech_g27_button(m_joysticks, m_mapping.pause);

		bool shifter_1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_1);
		bool shifter_2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_2);
		bool shifter_3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_3);
		bool shifter_4 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_4);
		bool shifter_5 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_5);
		bool shifter_6 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_6);
		bool shifter_r = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_r);
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
		// shifter stick down
		set_bit(buf, 86, shifter_1 || shifter_2 || shifter_3 || shifter_4 || shifter_5 || shifter_6 || shifter_r);

		buf[3] = (steering << 2) | buf[3];
		buf[4] = steering >> 6;
		buf[5] = throttle;
		buf[6] = brake;
		buf[7] = clutch;

		buf[8] = 0x80; // shifter x, don't own one to test gear/coord mapping
		buf[9] = 0x80; // shifter y
		buf[10] = buf[10] | (m_wheel_range > 360 ? 0x90 : 0x10);

		// logitech_g27_log.error("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10]);

		return;
	}
	else
	{
		// Sending data to wheel
		if (buf_size < 7)
		{
			char* hex_buf = reinterpret_cast<char*>(malloc(buf_size * 3 + 1));
			if (hex_buf == nullptr)
			{
				logitech_g27_log.error("Unhandled wheel command with size %u != 16", buf_size);
				return;
			}
			int offset = 0;
			for (uint32_t i = 0; i < buf_size; i++)
			{
				offset += sprintf(&hex_buf[offset], "%02x ", buf[i]);
			}
			logitech_g27_log.error("Unhandled wheel command with size %u != 16, %s", buf_size, hex_buf);
			free(hex_buf);
			return;
		}

		transfer->expected_count = buf_size;

		// logitech_g27_log.error("%02x %02x %02x %02x %02x %02x %02x", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
		// printf("%02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);

		SDL_HapticDirection direction = {
			.type = SDL_HAPTIC_POLAR,
			.dir = {27000, 0}};
		if (m_reverse_effects)
		{
			direction.dir[0] = 9000;
		}

		// TODO maybe force clipping from cfg

		// Process effects
		if (buf[0] == 0xf8)
		{
			const std::lock_guard<std::mutex> lock(m_sdl_handles_mutex);
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
				uint8_t new_led_level = 0;
				for (int i = 0; i < 5; i++)
				{
					new_led_level += (buf[2] & (1 << i)) ? 1 : 0;
				}

				uint8_t intensity = new_led_level * 255 / 5;
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
			const std::lock_guard<std::mutex> lock(m_sdl_handles_mutex);
			uint8_t cmd = buf[0] & 0xf;
			uint8_t slot_mask = buf[0] >> 4;
			switch (cmd)
			{
			case 0x00:
			case 0x01:
			case 0x0c:
			{
				// Download/Download play/Refresh
				for (int i = 0; i < 4; i++)
				{
					SDL_HapticEffect new_effect = {0};
					// hack: need to reduce Download play spams for some drivers
					bool update_hack = false;
					if (!(slot_mask & (1 << i)))
					{
						continue;
					}
					bool unknown_effect = false;
					switch (buf[1])
					{
					case 0x00:
					{
						// Constant force
						new_effect.type = SDL_HAPTIC_CONSTANT;
						new_effect.constant.direction = direction;
						new_effect.constant.length = SDL_HAPTIC_INFINITY;
						new_effect.constant.level = logitech_g27_force_to_level(buf[2 + i]);
						break;
					}
					case 0x01:
					case 0x0b:
					{
						// Spring/High resolution spring
						new_effect.type = SDL_HAPTIC_SPRING;
						new_effect.condition.direction = direction;
						new_effect.condition.length = SDL_HAPTIC_INFINITY;
						uint8_t s1 = buf[5] & 1;
						uint8_t s2 = (buf[5] >> 4) & 1;
						// TODO direction cfg
						uint16_t saturation = logitech_g27_clip_to_saturation(buf[6]);
						int16_t center = 0;
						uint16_t deadband = 0;
						int16_t left_coeff = 0;
						int16_t right_coeff = 0;
						if (buf[1] == 0x01)
						{
							uint8_t d1 = buf[2];
							uint8_t d2 = buf[3];
							uint8_t k1 = buf[4] & (0xf >> 1);
							uint8_t k2 = (buf[4] >> 4) & (0xf >> 1);
							center = logitech_g27_position_to_center(d1, d2);
							deadband = logitech_g27_position_to_width(d1, d2);
							left_coeff = logitech_g27_coeff_to_coeff(k1, s1);
							right_coeff = logitech_g27_coeff_to_coeff(k2, s2);
						}
						else
						{
							uint16_t d1 = (buf[2] << 3) | ((buf[5] >> 1) & (0xf >> 1));
							uint16_t d2 = (buf[3] << 3) | (buf[5] >> 5);
							uint8_t k1 = buf[4] & 0xf;
							uint8_t k2 = buf[4] >> 4;
							center = logitech_g27_high_resolution_position_to_center(d1, d2);
							deadband = logitech_g27_high_resolution_position_to_width(d1, d2);
							left_coeff = logitech_g27_high_resolution_coeff_to_coeff(k1, s1);
							right_coeff = logitech_g27_high_resolution_coeff_to_coeff(k2, s2);
						}
						if (m_reverse_effects)
						{
							int16_t coeff = right_coeff;
							right_coeff = left_coeff;
							left_coeff = coeff;
						}
						// for(int j = 0;j < 3;j++)
						for (int j = 0; j < 1; j++)
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
						new_effect.condition.direction = direction;
						new_effect.condition.length = SDL_HAPTIC_INFINITY;
						uint8_t s1 = buf[3] & 1;
						uint8_t s2 = buf[5] & 1;
						// TODO direction cfg
						uint16_t saturation = 0x7FFF;
						int16_t left_coeff = 0;
						int16_t right_coeff = 0;
						if (buf[1] == 0x02)
						{
							uint8_t k1 = buf[2] & (0xf >> 1);
							uint8_t k2 = buf[4] & (0xf >> 1);
							left_coeff = logitech_g27_coeff_to_coeff(k1, s1);
							right_coeff = logitech_g27_coeff_to_coeff(k2, s2);
						}
						else
						{
							uint8_t k1 = buf[2] & 0xf;
							uint8_t k2 = buf[4] & 0xf;
							left_coeff = logitech_g27_high_resolution_coeff_to_coeff(k1, s1);
							right_coeff = logitech_g27_high_resolution_coeff_to_coeff(k2, s2);
							saturation = logitech_g27_clip_to_saturation(buf[6]);
						}
						if (m_reverse_effects)
						{
							int16_t coeff = right_coeff;
							right_coeff = left_coeff;
							left_coeff = coeff;
						}
						// for(int j = 0;j < 3;j++)
						for (int j = 0; j < 1; j++)
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
						new_effect.condition.direction = direction;
						new_effect.condition.length = SDL_HAPTIC_INFINITY;
						uint8_t k1 = buf[2];
						uint8_t k2 = buf[3];
						uint8_t s1 = buf[5] & 1;
						uint8_t s2 = (buf[5] >> 4) & 1;
						// TODO direction cfg
						int16_t left_coeff = logitech_g27_friction_coeff_to_coeff(k1, s1);
						int16_t right_coeff = logitech_g27_friction_coeff_to_coeff(k2, s2);
						int16_t saturation = logitech_g27_clip_to_saturation(buf[4]);
						if (m_reverse_effects)
						{
							int16_t coeff = right_coeff;
							right_coeff = left_coeff;
							left_coeff = coeff;
						}
						// for(int j = 0;j < 3;j++)
						for (int j = 0; j < 1; j++)
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
						new_effect.condition.direction = direction;
						new_effect.condition.length = SDL_HAPTIC_INFINITY;
						// TODO direction cfg
						uint16_t saturation = logitech_g27_clip_to_saturation(buf[4]);
						uint16_t deadband = 2 * 0xFFFF / 255;
						int16_t center = 0;
						int16_t left_coeff = 0;
						int16_t right_coeff = 0;
						if (buf[1] == 0x03)
						{
							uint8_t k1 = buf[2] & (0xf >> 1);
							uint8_t k2 = buf[3] & (0xf >> 1);
							left_coeff = logitech_g27_coeff_to_coeff(k1, 0);
							right_coeff = logitech_g27_coeff_to_coeff(k2, 0);
						}
						else
						{
							uint8_t k1 = buf[2] & 0xf;
							uint8_t k2 = buf[3] & 0xf;
							left_coeff = logitech_g27_high_resolution_coeff_to_coeff(k1, 0);
							right_coeff = logitech_g27_high_resolution_coeff_to_coeff(k2, 0);
						}
						if (m_reverse_effects)
						{
							int16_t coeff = right_coeff;
							right_coeff = left_coeff;
							left_coeff = coeff;
						}
						// for(int j = 0;j < 3;j++)
						for (int j = 0; j < 1; j++)
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
						new_effect.type = buf[1] == 0x04 ? SDL_HAPTIC_SAWTOOTHUP : SDL_HAPTIC_SAWTOOTHDOWN;
						new_effect.periodic.direction = direction;
						new_effect.periodic.length = SDL_HAPTIC_INFINITY;
						uint8_t l1 = buf[2];
						uint8_t l2 = buf[3];
						uint8_t l0 = buf[4];
						uint8_t t3 = buf[6] >> 4;
						uint8_t inc = buf[6] & 0xf;
						if (inc != 0)
							new_effect.periodic.period = ((l1 - l2) * logitech_g27_loops_to_ms(t3, !m_fixed_loop)) / inc;
						else
						{
							logitech_g27_log.error("cannot evaluate slope for saw tooth effect, loops per step %u level per step %u", t3, inc);
							new_effect.periodic.period = 1000;
						}
						new_effect.periodic.offset = logitech_g27_force_to_level((l1 + l2) / 2);
						new_effect.periodic.magnitude = logitech_g27_force_to_level(l1) - new_effect.periodic.offset;
						new_effect.periodic.phase = buf[1] == 0x04 ? 36000 * (l1 - l0) / (l1 - l2) : 36000 * (l0 - l2) / (l1 - l2);
						break;
					}
					case 0x06:
					{
						// Trapezoid, convert to SDL_HAPTIC_SQUARE or SDL_HAPTIC_TRIANGLE
						new_effect.periodic.direction = direction;
						new_effect.periodic.length = SDL_HAPTIC_INFINITY;
						uint8_t l1 = buf[2];
						uint8_t l2 = buf[3];
						uint8_t t1 = buf[4];
						uint8_t t2 = buf[5];
						uint8_t t3 = buf[6] >> 4;
						uint8_t s = buf[6] & 0xf;
						uint16_t total_flat_time = logitech_g27_loops_to_ms(t1 + t2, !m_fixed_loop);
						uint16_t total_slope_time = (((l1 - l2) * logitech_g27_loops_to_ms(t3, !m_fixed_loop)) / s) * 2;
						if (total_flat_time > total_slope_time)
						{
							new_effect.type = SDL_HAPTIC_SQUARE;
						}
						else
						{
							new_effect.type = SDL_HAPTIC_TRIANGLE;
						}
						new_effect.periodic.period = total_slope_time + total_flat_time;
						new_effect.periodic.offset = logitech_g27_force_to_level((l1 + l2) / 2);
						new_effect.periodic.magnitude = logitech_g27_force_to_level(l1) - new_effect.periodic.offset;
						break;
					}
					case 0x07:
					{
						// Rectangle, convert to SDL_HAPTIC_SQUARE
						new_effect.type = SDL_HAPTIC_SQUARE;
						new_effect.periodic.direction = direction;
						new_effect.periodic.length = SDL_HAPTIC_INFINITY;
						uint8_t l1 = buf[2];
						uint8_t l2 = buf[3];
						uint8_t t1 = buf[4];
						uint8_t t2 = buf[5];
						uint8_t p = buf[6];
						new_effect.periodic.period = logitech_g27_loops_to_ms(t1, !m_fixed_loop) + logitech_g27_loops_to_ms(t2, !m_fixed_loop);
						new_effect.periodic.offset = logitech_g27_force_to_level((l1 + l2) / 2);
						new_effect.periodic.magnitude = logitech_g27_force_to_level(l1) - new_effect.periodic.offset;
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
						// Variable/Ramp, convert to SDL_HAPTIC_CONSTANT
						if (i % 2 != 0)
						{
							continue;
						}
						new_effect.type = SDL_HAPTIC_CONSTANT;
						new_effect.constant.direction = direction;
						uint8_t l1 = buf[2];
						uint8_t l2 = buf[3];
						uint8_t t1 = buf[4] >> 4;
						uint8_t s1 = buf[4] & 0xf;
						uint8_t t2 = buf[5] >> 4;
						uint8_t s2 = buf[5] & 0xf;
						uint8_t d1 = buf[6] & 1;
						uint8_t d2 = (buf[6] >> 4) & 1;
						if (buf[1] == 0x08)
						{
							uint8_t t = i == 0 ? t1 : t2;
							uint8_t s = i == 0 ? s1 : s2;
							uint8_t d = i == 0 ? d1 : d2;
							uint8_t l = i == 0 ? l1 : l2;
							new_effect.constant.length = SDL_HAPTIC_INFINITY;
							if (s == 0 || t == 0)
							{
								// gran turismo 6 does this, gives a variable force with no step so it just behaves as constant force
								new_effect.constant.level = logitech_g27_force_to_level(l);
								// hack: gran turismo 6 spams download and play
								update_hack = true;
							}
							else
							{
								new_effect.constant.attack_level = logitech_g27_force_to_level(l);
								if (d)
								{
									new_effect.constant.level = 0;
									new_effect.constant.attack_length = l * logitech_g27_loops_to_ms(t, !m_fixed_loop) / s;
								}
								else
								{
									new_effect.constant.level = 0x7FFF;
									new_effect.constant.attack_length = (255 - l) * logitech_g27_loops_to_ms(t, !m_fixed_loop) / s;
								}
							}
						}
						else
						{
							if (s2 == 0 || t2 == 0)
							{
								logitech_g27_log.error("cannot evaluate slope for ramp effect, loops per step %u level per step %u", t2, s2);
							}
							else
							{
								new_effect.constant.length = (l1 - l2) * logitech_g27_loops_to_ms(t2, !m_fixed_loop) / s2;
								new_effect.constant.attack_length = new_effect.constant.length;
								new_effect.constant.attack_level = d1 ? logitech_g27_force_to_level(l1) : logitech_g27_force_to_level(l2);
							}
							new_effect.constant.level = d1 ? logitech_g27_force_to_level(l2) : logitech_g27_force_to_level(l1);
						}
						break;
					}
					case 0x0a:
					{
						// Square
						new_effect.type = SDL_HAPTIC_SQUARE;
						new_effect.periodic.direction = direction;
						uint8_t a = buf[2];
						uint8_t tl = buf[3];
						uint8_t th = buf[4];
						uint8_t n = buf[5];
						uint16_t t = (th << 8) | tl;
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
					}
					}

					if (unknown_effect)
					{
						logitech_g27_log.error("Command %02x %02x %02x %02x %02x %02x %02x with unknown effect ignored", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
						continue;
					}

					bool play_effect = (cmd == 0x01 || (cmd == 0x0c && m_effect_slots[i].effect_id == -1));

					if (update_hack)
					{
						if (m_effect_slots[i].effect_id == -1)
							update_hack = false;
						if (m_effect_slots[i].last_effect.type != new_effect.type)
							update_hack = false;
					}

					if (cmd == 0x00 || play_effect)
					{
						if (m_effect_slots[i].effect_id != -1 && m_haptic_handle != nullptr && !update_hack)
						{
							SDL_DestroyHapticEffect(m_haptic_handle, m_effect_slots[i].effect_id);
							m_effect_slots[i].effect_id = -1;
						}
						if (m_haptic_handle != nullptr && m_effect_slots[i].effect_id == -1)
						{
							m_effect_slots[i].effect_id = SDL_CreateHapticEffect(m_haptic_handle, &new_effect);
						}
						if (update_hack)
						{
							if (!SDL_UpdateHapticEffect(m_haptic_handle, m_effect_slots[i].effect_id, &new_effect))
								logitech_g27_log.error("Failed refreshing slot %d sdl effect %d, %s", i, new_effect.type, SDL_GetError());
						}
						m_effect_slots[i].state = G27_FFB_DOWNLOADED;
						m_effect_slots[i].last_effect = new_effect;
						m_effect_slots[i].last_update = SDL_GetTicks();
						if (m_effect_slots[i].effect_id == -1 && m_haptic_handle != nullptr)
						{
							logitech_g27_log.error("Failed uploading effect %02x %02x %02x %02x %02x %02x %02x to slot %i, %s", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], i, SDL_GetError());
						}
					}
					if (play_effect && m_haptic_handle != nullptr)
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
						m_effect_slots[i].state = G27_FFB_PLAYING;
					}
					if (cmd == 0xc && !play_effect && m_haptic_handle != nullptr)
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
					if (m_effect_slots[i].state == G27_FFB_PLAYING || m_effect_slots[i].state == G27_FFB_DOWNLOADED)
					{
						m_effect_slots[i].state = cmd == 0x02 ? G27_FFB_PLAYING : G27_FFB_DOWNLOADED;
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
				uint8_t k1 = buf[2] & (0xf >> 1);
				uint8_t k2 = buf[3] & (0xf >> 1);
				uint16_t saturation = logitech_g27_clip_to_saturation(buf[4]);
				int16_t left_coeff = logitech_g27_coeff_to_coeff(k1, 0);
				int16_t right_coeff = logitech_g27_coeff_to_coeff(k2, 0);
				uint16_t deadband = 2 * 0xFFFF / 255;
				int16_t center = 0;
				if (m_reverse_effects)
				{
					int16_t coeff = right_coeff;
					right_coeff = left_coeff;
					left_coeff = coeff;
				}
				// for (int i = 0;i < 3;i++){
				for (int i = 0; i < 1; i++)
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
				if (m_led_joystick_handle == nullptr)
				{
					break;
				}

				uint8_t new_led_level = 0;
				for (int i = 0; i < 8; i++)
				{
					new_led_level += (buf[1] & (1 << i)) ? 1 : 0;
				}
				uint8_t intensity = new_led_level * 255 / 7;
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
			}
			}
		}
	}
}

#endif
