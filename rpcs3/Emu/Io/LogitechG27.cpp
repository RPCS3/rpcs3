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

LOG_CHANNEL(logitech_g27_log, "logitech_g27");

#pragma pack(push, 1)
struct DFEX_data
{
	u8 square : 1;
	u8 cross : 1;
	u8 circle : 1;
	u8 triangle : 1;
	u8 l1 : 1; // Left_paddle
	u8 r1 : 1; // Right_paddle
	u8 l2 : 1;
	u8 r2 : 1;

	u8 select : 1; // Share
	u8 start : 1; // Options
	u8 l3 : 1;
	u8 r3 : 1;
	u8 ps : 1;
	u8 : 3;

	u8 dpad; // 00=N, 01=NE, 02=E, 03=SW, 04=S, 05=SW, 06=W, 07=NW, 08=IDLE
	u8 steering; // 00=Left, ff=Right
	u8 brake_throttle; // 00=ThrottlePressed, 7f=BothReleased/BothPressed, ff=BrakePressed
	u8 const1;
	u8 const2;
	u32 : 32;
	u32 : 32;
	u16 : 16;
	u8 brake; // 00=Released, ff=Pressed
	u8 throttle; // 00=Released, ff=Pressed
	u32 : 32;
	u32 : 32;
};

struct DFP_data
{
	u16 steering : 14; // 0000=Left, 1fff=Mid, 3fff=Right
	u16 cross: 1;
	u16 square : 1;

	u8 circle : 1;
	u8 triangle : 1;
	u8 r1 : 1; // Right_paddle
	u8 l1 : 1; // Left_paddle
	u8 r2 : 1;
	u8 l2 : 1;
	u8 select : 1; // Share
	u8 start : 1; // Options

	u8 r3 : 1;
	u8 l3 : 1;
	u8 r3_2 : 1;
	u8 l3_2 : 1;
	u8 dpad : 4; // 0=N, 1=NE, 2=E, 3=SW, 4=S, 5=SW, 6=W, 7=NW, 8=IDLE

	u8 brake_throttle; // 00=ThrottlePressed, 7f=BothReleased/BothPressed, ff=BrakePressed
	u8 throttle; // 00=Pressed, ff=Released
	u8 brake; // 00=Pressed, ff=Released

	u8 pedals_attached : 1;
	u8 powered : 1;
	u8 : 1;
	u8 self_check_done : 1;
	u8 set1 : 1; // always set
	u8 : 3;
};

struct DFGT_data
{
	u8 dpad : 4; // 0=N, 1=NE, 2=E, 3=SW, 4=S, 5=SW, 6=W, 7=NW, 8=IDLE
	u8 cross: 1;
	u8 square : 1;
	u8 circle : 1;
	u8 triangle : 1;

	u8 r1 : 1; // Right_paddle
	u8 l1 : 1; // Left_paddle
	u8 r2 : 1;
	u8 l2 : 1;
	u8 select : 1; // Share
	u8 start : 1; // Options
	u8 r3 : 1;
	u8 l3 : 1;

	u8 : 2;
	u8 dial_center : 1;
	u8 plus : 1;
	u8 dial_cw : 1;
	u8 dial_ccw : 1;
	u8 minus : 1;
	u8 : 1;

	u8 ps : 1;
	u8 pedals_attached : 1;
	u8 powered : 1;
	u8 self_check_done : 1;
	u8 set1 : 1;
	u8 : 1;
	u8 set2 : 1;
	u8 : 1;

	u16 steering : 14; // 0000=Left, 1fff=Mid, 3fff=Right
	u16 : 2;
	u8 throttle; // 00=Pressed, ff=Released
	u8 brake; // 00=Pressed, ff=Released
};

struct G25_data
{
	u8 dpad : 4; // 0=N, 1=NE, 2=E, 3=SW, 4=S, 5=SW, 6=W, 7=NW, 8=IDLE
	u8 cross: 1;
	u8 square : 1;
	u8 circle : 1;
	u8 triangle : 1;

	u8 r1 : 1; // Right_paddle
	u8 l1 : 1; // Left_paddle
	u8 r2 : 1; // + dial_center
	u8 l2 : 1;
	u8 select : 1; // Share
	u8 start : 1; // Options
	u8 r3 : 1; // + dial_cw + plus
	u8 l3 : 1; // + dial_ccw + minus

	u8 gear1 : 1;
	u8 gear2 : 1;
	u8 gear3 : 1;
	u8 gear4 : 1;
	u8 gear5 : 1;
	u8 gear6 : 1;
	u8 gearR : 1;
	u8 : 1;

	u16 pedals_detached : 1;
	u16 powered : 1;
	u16 steering : 14; // 0000=Left, 1fff=Mid, 3fff=Right

	u8 throttle; // 00=Pressed, ff=Released
	u8 brake; // 00=Pressed, ff=Released
	u8 clutch; // 00=Pressed, ff=Released

	u8 shifter_x; // 30=left(1,2), 7a=middle(3,4), b2=right(5,6)
	u8 shifter_y; // 32=bottom(2,4,6), b7=top(1,3,5)

	u8 shifter_attached : 1;
	u8 set1 : 1;
	u8 : 1;
	u8 shifter_pressed : 1;
	u8 : 4;
};

struct G27_data
{
	u8 dpad : 4; // 0=N, 1=NE, 2=E, 3=SW, 4=S, 5=SW, 6=W, 7=NW, 8=IDLE
	u8 cross: 1;
	u8 square : 1;
	u8 circle : 1;
	u8 triangle : 1;

	u8 r1 : 1; // Right_paddle
	u8 l1 : 1; // Left_paddle
	u8 r2 : 1;
	u8 l2 : 1;
	u8 select : 1; // Share
	u8 start : 1; // Options
	u8 r3 : 1; // + dial_center
	u8 l3 : 1;

	u8 gear1 : 1;
	u8 gear2 : 1;
	u8 gear3 : 1;
	u8 gear4 : 1;
	u8 gear5 : 1;
	u8 gear6 : 1;
	u8 dial_cw : 1;
	u8 dial_ccw : 1;

	u16 plus : 1;
	u16 minus : 1;
	u16 steering : 14; // 0000=Left, 1fff=Mid, 3fff=Right

	u8 throttle; // 00=Pressed, ff=Released
	u8 brake; // 00=Pressed, ff=Released
	u8 clutch; // 00=Pressed, ff=Released

	u8 shifter_x; // 30=left(1,2), 7a=middle(3,4), b2=right(5,6)
	u8 shifter_y; // 32=bottom(2,4,6), b7=top(1,3,5)

	u8 gearR : 1;
	u8 pedals_detached : 1;
	u8 powered : 1;
	u8 shifter_attached : 1;
	u8 set1 : 1;
	u8 : 1;
	u8 shifter_pressed : 1;
	u8 range : 1;
};
#pragma pack(pop)

static const std::map<logitech_personality,
	std::pair<UsbDeviceDescriptor, std::array<u8, 0x29>>> s_logitech_personality = {
{
	logitech_personality::driving_force_ex,
	{
		UsbDeviceDescriptor{0x0200, 0x00, 0x00, 0x00, 0x10, 0x046D, 0xC294, 0x1350, 0x01, 0x02, 0x00, 0x01},
		{0x09, 0x02, 0x29, 0x00, 0x01, 0x01, 0x04, 0x80, 0x31, 0x09, 0x04, 0x00, 0x00, 0x02, 0x03, 0x00,
			0x00, 0x00, 0x09, 0x21, 0x00, 0x01, 0x21, 0x01, 0x22, 0x9D, 0x00, 0x07, 0x05, 0x81, 0x03, 0x40,
			0x00, 0x0A, 0x07, 0x05, 0x01, 0x03, 0x10, 0x00, 0x0A}
	}
},
{
	logitech_personality::driving_force_pro,
	{
		UsbDeviceDescriptor{0x0200, 0x00, 0x00, 0x00, 0x10, 0x046D, 0xC298, 0x1350, 0x01, 0x02, 0x00, 0x01},
		{0x09, 0x02, 0x29, 0x00, 0x01, 0x01, 0x04, 0x80, 0x31, 0x09, 0x04, 0x00, 0x00, 0x02, 0x03, 0x00,
			0x00, 0x00, 0x09, 0x21, 0x00, 0x01, 0x21, 0x01, 0x22, 0x61, 0x00, 0x07, 0x05, 0x81, 0x03, 0x08,
			0x00, 0x0A, 0x07, 0x05, 0x01, 0x03, 0x08, 0x00, 0x0A}
	}
},
{
	logitech_personality::g25,
	{
		UsbDeviceDescriptor{0x0200, 0x00, 0x00, 0x00, 0x10, 0x046D, 0xC299, 0x1350, 0x01, 0x02, 0x00, 0x01},
		{0x09, 0x02, 0x29, 0x00, 0x01, 0x01, 0x04, 0x80, 0x31, 0x09, 0x04, 0x00, 0x00, 0x02, 0x03, 0x00,
			0x00, 0x00, 0x09, 0x21, 0x11, 0x01, 0x21, 0x01, 0x22, 0x6F, 0x00, 0x07, 0x05, 0x81, 0x03, 0x10,
			0x00, 0x02, 0x07, 0x05, 0x01, 0x03, 0x10, 0x00, 0x02}
	}
},
{
	logitech_personality::driving_force_gt,
	{
		UsbDeviceDescriptor{0x0200, 0x00, 0x00, 0x00, 0x10, 0x046D, 0xC29A, 0x1350, 0x00, 0x02, 0x00, 0x01},
		{0x09, 0x02, 0x29, 0x00, 0x01, 0x01, 0x00, 0x80, 0x31, 0x09, 0x04, 0x00, 0x00, 0x02, 0x03, 0x00,
			0x00, 0xFE, 0x09, 0x21, 0x11, 0x01, 0x21, 0x01, 0x22, 0x73, 0x00, 0x07, 0x05, 0x81, 0x03, 0x10,
			0x00, 0x02, 0x07, 0x05, 0x01, 0x03, 0x10, 0x00, 0x02}
	}
},
{
	logitech_personality::g27,
	{
		UsbDeviceDescriptor{0x0200, 0x00, 0x00, 0x00, 0x10, 0x046D, 0xC29B, 0x1350, 0x01, 0x02, 0x00, 0x01},
		{0x09, 0x02, 0x29, 0x00, 0x01, 0x01, 0x04, 0x80, 0x31, 0x09, 0x04, 0x00, 0x00, 0x02, 0x03, 0x00,
			0x00, 0x00, 0x09, 0x21, 0x11, 0x01, 0x21, 0x01, 0x22, 0x85, 0x00, 0x07, 0x05, 0x81, 0x03, 0x10,
			0x00, 0x02, 0x07, 0x05, 0x01, 0x03, 0x10, 0x00, 0x02}
	}
}
};

// ref: https://github.com/libsdl-org/SDL/issues/7941, need to use SDL_HAPTIC_STEERING_AXIS for some windows drivers
static constexpr SDL_HapticDirection STEERING_DIRECTION =
{
	.type = SDL_HAPTIC_STEERING_AXIS,
	.dir = {0, 0, 0}
};

void usb_device_logitech_g27::set_personality(logitech_personality personality, bool reconnect)
{
	m_personality = personality;
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, ::at32(s_logitech_personality, personality).first);

	// parse the raw response like with passthrough device
	const u8* raw_config = ::at32(s_logitech_personality, personality).second.data();
	auto& conf = device.add_node(UsbDescriptorNode(raw_config[0], raw_config[1], &raw_config[2]));
	for (unsigned int index = raw_config[0]; index < raw_config[2];)
	{
		conf.add_node(UsbDescriptorNode(raw_config[index], raw_config[index + 1], &raw_config[index + 2]));
		index += raw_config[index];
	}

	if (reconnect)
	{
		reconnect_usb(assigned_number);
	}
}

usb_device_logitech_g27::usb_device_logitech_g27(u32 controller_index, const std::array<u8, 7>& location)
	: usb_device_emulated(location), m_controller_index(controller_index)
{
	set_personality(logitech_personality::driving_force_ex);

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
			thread_ctrl::wait_for(1'000'000);

			std::unique_lock lock(g_cfg_logitech_g27.m_mutex);
			if (logitech_personality::invalid != m_next_personality && m_personality != m_next_personality)
			{
				set_personality(m_next_personality, true);
				m_next_personality = logitech_personality::invalid;
			}
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
	logitech_g27_log.notice("control transfer bmRequestType %02x, bRequest %02x, wValue %04x, wIndex %04x, wLength %04x, %s", bmRequestType, bRequest, wValue, wIndex, wLength, fmt::buf_to_hexstring(buf, buf_size));

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
	convert_mapping(cfg.dial_center, mapping.dial_center);

	convert_mapping(cfg.select, mapping.select);
	convert_mapping(cfg.start, mapping.start);
	convert_mapping(cfg.ps, mapping.ps);

	convert_mapping(cfg.shifter_1, mapping.shifter_1);
	convert_mapping(cfg.shifter_2, mapping.shifter_2);
	convert_mapping(cfg.shifter_3, mapping.shifter_3);
	convert_mapping(cfg.shifter_4, mapping.shifter_4);
	convert_mapping(cfg.shifter_5, mapping.shifter_5);
	convert_mapping(cfg.shifter_6, mapping.shifter_6);
	convert_mapping(cfg.shifter_r, mapping.shifter_r);

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

static std::pair<u8, u8> shifter_to_coord_xy(bool shifter_1, bool shifter_2, bool shifter_3, bool shifter_4,
		bool shifter_5, bool shifter_6, bool shifter_r)
{
	// rough analog values recorded in https://github.com/RPCS3/rpcs3/pull/17199#issuecomment-2883934412
	constexpr u8 coord_center = 0x80;
	constexpr u8 coord_top = 0xb7;
	constexpr u8 coord_bottom = 0x32;
	constexpr u8 coord_left = 0x30;
	constexpr u8 coord_right = 0xb3;
	constexpr u8 coord_right_reverse = 0xaa;
	if (shifter_1)
	{
		return {coord_left, coord_top};
	}
	else if (shifter_2)
	{
		return {coord_left, coord_bottom};
	}
	else if (shifter_3)
	{
		return {coord_center, coord_top};
	}
	else if (shifter_4)
	{
		return {coord_center, coord_bottom};
	}
	else if (shifter_5)
	{
		return {coord_right, coord_top};
	}
	else if (shifter_6)
	{
		return {coord_right, coord_bottom};
	}
	else if (shifter_r)
	{
		return {coord_right_reverse, coord_bottom};
	}
	else
	{
		return {coord_center, coord_center};
	}
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

void usb_device_logitech_g27::transfer_dfex(u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	DFEX_data data{};
	ensure(buf_size >= sizeof(data));
	transfer->expected_count = sizeof(data);

	const std::lock_guard lock(m_sdl_handles_mutex);
	data.square = sdl_to_logitech_g27_button(m_joysticks, m_mapping.square);
	data.cross = sdl_to_logitech_g27_button(m_joysticks, m_mapping.cross);
	data.circle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.circle);
	data.triangle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.triangle);
	data.l1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_down);
	data.r1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_up);
	data.l2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l2);
	data.r2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r2);
	data.select = sdl_to_logitech_g27_button(m_joysticks, m_mapping.select);
	data.start = sdl_to_logitech_g27_button(m_joysticks, m_mapping.start);
	data.l3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l3);
	data.r3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r3);
	data.dpad = hat_components_to_logitech_g27_hat(
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.up),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.down),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.left),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.right)
	);
	data.steering = sdl_to_logitech_g27_steering(m_joysticks, m_mapping.steering) >> 6;
	data.brake_throttle = 0x7f;
	data.const1 = 0x7f;
	data.const2 = 0x7f;
	data.brake = 0xff - sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.brake);
	data.throttle = 0xff - sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.throttle);
	std::memcpy(buf, &data, sizeof(data));
}

void usb_device_logitech_g27::transfer_dfp(u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	DFP_data data{};
	ensure(buf_size >= sizeof(data));
	transfer->expected_count = sizeof(data);

	const std::lock_guard lock(m_sdl_handles_mutex);
	data.steering = sdl_to_logitech_g27_steering(m_joysticks, m_mapping.steering);
	data.cross = sdl_to_logitech_g27_button(m_joysticks, m_mapping.cross);
	data.square = sdl_to_logitech_g27_button(m_joysticks, m_mapping.square);
	data.circle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.circle);
	data.triangle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.triangle);
	data.r1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_up);
	data.l1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_down);
	data.r2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r2);
	data.l2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l2);
	data.select = sdl_to_logitech_g27_button(m_joysticks, m_mapping.select);
	data.start = sdl_to_logitech_g27_button(m_joysticks, m_mapping.start);
	data.r3 = data.r3_2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r3);
	data.l3 = data.l3_2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l3);
	data.dpad = hat_components_to_logitech_g27_hat(
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.up),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.down),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.left),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.right)
	);
	data.brake_throttle = 0x7f;
	data.throttle = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.throttle);
	data.brake = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.brake);
	data.pedals_attached = 1;
	data.powered = 1;
	data.self_check_done = 1;
	data.set1 = 1;
	std::memcpy(buf, &data, sizeof(data));
}

void usb_device_logitech_g27::transfer_dfgt(u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	DFGT_data data{};
	ensure(buf_size >= sizeof(data));
	transfer->expected_count = sizeof(data);

	const std::lock_guard lock(m_sdl_handles_mutex);
	data.dpad = hat_components_to_logitech_g27_hat(
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.up),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.down),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.left),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.right)
	);
	data.cross = sdl_to_logitech_g27_button(m_joysticks, m_mapping.cross);
	data.square = sdl_to_logitech_g27_button(m_joysticks, m_mapping.square);
	data.circle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.circle);
	data.triangle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.triangle);
	data.r1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_up);
	data.l1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_down);
	data.r2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r2);
	data.l2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l2);
	data.select = sdl_to_logitech_g27_button(m_joysticks, m_mapping.select);
	data.start = sdl_to_logitech_g27_button(m_joysticks, m_mapping.start);
	data.r3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r3);
	data.l3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l3);
	data.dial_center = sdl_to_logitech_g27_button(m_joysticks, m_mapping.dial_center);
	data.plus = sdl_to_logitech_g27_button(m_joysticks, m_mapping.plus);
	data.dial_cw = sdl_to_logitech_g27_button(m_joysticks, m_mapping.dial_clockwise);
	data.dial_ccw = sdl_to_logitech_g27_button(m_joysticks, m_mapping.dial_anticlockwise);
	data.minus = sdl_to_logitech_g27_button(m_joysticks, m_mapping.minus);
	data.ps = sdl_to_logitech_g27_button(m_joysticks, m_mapping.ps);
	data.pedals_attached = 1;
	data.powered = 1;
	data.self_check_done = 1;
	data.set1 = 1;
	data.set2 = 1;
	data.steering = sdl_to_logitech_g27_steering(m_joysticks, m_mapping.steering);
	data.throttle = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.throttle);
	data.brake = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.brake);
	std::memcpy(buf, &data, sizeof(data));
}

void usb_device_logitech_g27::transfer_g25(u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	G25_data data{};
	ensure(buf_size >= sizeof(data));
	transfer->expected_count = sizeof(data);

	const std::lock_guard lock(m_sdl_handles_mutex);
	data.dpad = hat_components_to_logitech_g27_hat(
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.up),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.down),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.left),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.right)
	);
	data.cross = sdl_to_logitech_g27_button(m_joysticks, m_mapping.cross);
	data.square = sdl_to_logitech_g27_button(m_joysticks, m_mapping.square);
	data.circle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.circle);
	data.triangle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.triangle);
	data.r1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_up);
	data.l1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_down);
	data.r2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r2);
	data.l2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l2);
	data.select = sdl_to_logitech_g27_button(m_joysticks, m_mapping.select);
	data.start = sdl_to_logitech_g27_button(m_joysticks, m_mapping.start);
	data.r3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r3);
	data.l3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l3);
	data.gear1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_1);
	data.gear2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_2);
	data.gear3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_3);
	data.gear4 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_4);
	data.gear5 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_5);
	data.gear6 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_6);
	data.gearR = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_r);
	data.pedals_detached = 0;
	data.powered = 1;
	data.steering = sdl_to_logitech_g27_steering(m_joysticks, m_mapping.steering);
	data.throttle = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.throttle);
	data.brake = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.brake);
	data.clutch = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.clutch);
	auto [shifter_x, shifter_y] = shifter_to_coord_xy(data.gear1, data.gear2,
		data.gear3, data.gear4, data.gear5, data.gear6, data.gearR);
	data.shifter_x = shifter_x;
	data.shifter_y = shifter_y;
	data.shifter_attached = 1;
	data.set1 = 1;
	data.shifter_pressed = data.gearR;
	std::memcpy(buf, &data, sizeof(data));
}

void usb_device_logitech_g27::transfer_g27(u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	G27_data data{};
	ensure(buf_size >= sizeof(data));
	transfer->expected_count = sizeof(data);

	const std::lock_guard lock(m_sdl_handles_mutex);
	data.dpad = hat_components_to_logitech_g27_hat(
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.up),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.down),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.left),
		sdl_to_logitech_g27_button(m_joysticks, m_mapping.right)
	);
	data.cross = sdl_to_logitech_g27_button(m_joysticks, m_mapping.cross);
	data.square = sdl_to_logitech_g27_button(m_joysticks, m_mapping.square);
	data.circle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.circle);
	data.triangle = sdl_to_logitech_g27_button(m_joysticks, m_mapping.triangle);
	data.r1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_up);
	data.l1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shift_down);
	data.r2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r2);
	data.l2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l2);
	data.select = sdl_to_logitech_g27_button(m_joysticks, m_mapping.select);
	data.start = sdl_to_logitech_g27_button(m_joysticks, m_mapping.start);
	data.r3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.r3);
	data.l3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.l3);
	data.gear1 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_1);
	data.gear2 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_2);
	data.gear3 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_3);
	data.gear4 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_4);
	data.gear5 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_5);
	data.gear6 = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_6);
	const bool shifter_r = sdl_to_logitech_g27_button(m_joysticks, m_mapping.shifter_r);
	data.dial_cw = sdl_to_logitech_g27_button(m_joysticks, m_mapping.dial_clockwise);
	data.dial_ccw = sdl_to_logitech_g27_button(m_joysticks, m_mapping.dial_anticlockwise);
	data.plus = sdl_to_logitech_g27_button(m_joysticks, m_mapping.plus);
	data.minus = sdl_to_logitech_g27_button(m_joysticks, m_mapping.minus);
	data.steering = sdl_to_logitech_g27_steering(m_joysticks, m_mapping.steering);
	data.throttle = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.throttle);
	data.brake = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.brake);
	data.clutch = sdl_to_logitech_g27_pedal(m_joysticks, m_mapping.clutch);
	auto [shifter_x, shifter_y] = shifter_to_coord_xy(data.gear1, data.gear2,
		data.gear3, data.gear4, data.gear5, data.gear6, shifter_r);
	data.shifter_x = shifter_x;
	data.shifter_y = shifter_y;
	data.gearR = shifter_r;
	data.pedals_detached = 0;
	data.powered = 1;
	data.shifter_attached = 1;
	data.set1 = 1;
	data.shifter_pressed = shifter_r;
	data.range = (m_wheel_range > 360);
	std::memcpy(buf, &data, sizeof(data));
}

void usb_device_logitech_g27::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer)
{
	transfer->fake = true;
	transfer->expected_result = HC_CC_NOERR;
	// G29 in G27 mode polls at 500 hz, let's try a delay of 1ms for now, for wheels that updates that fast
	transfer->expected_time = get_timestamp() + 1000;

	if (endpoint & (1 << 7))
	{
		memset(buf, 0, buf_size);
		sdl_instance::get_instance().pump_events();

		switch (m_personality)
		{
		case logitech_personality::driving_force_ex:
			transfer_dfex(buf_size, buf, transfer);
			break;
		case logitech_personality::driving_force_pro:
			transfer_dfp(buf_size, buf, transfer);
			break;
		case logitech_personality::g25:
			transfer_g25(buf_size, buf, transfer);
			break;
		case logitech_personality::driving_force_gt:
			transfer_dfgt(buf_size, buf, transfer);
			break;
		case logitech_personality::g27:
			transfer_g27(buf_size, buf, transfer);
			break;
		case logitech_personality::invalid:
			fmt::throw_exception("unreachable");
		}

		// logitech_g27_log.error("dev=%d, ep in : %s",  static_cast<u8>(m_personality), fmt::buf_to_hexstring(buf, buf_size));

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

		// logitech_g27_log.error("ep out : %s", fmt::buf_to_hexstring(buf, buf_size));

		// TODO maybe force clipping from cfg

		// Process effects
		if (buf[0] == 0xf8)
		{
			const std::lock_guard lock(m_sdl_handles_mutex);
			switch (buf[1])
			{
			case 0x01:
			case 0x09:
			case 0x10:
			{
				// Change device mode
				u8 cmd = buf[1];
				u8 arg = buf[2];
				if (buf[8] == 0xf8) // we have 2 commands back to back
				{
					cmd = buf[9];
					arg = buf[10];
				}

				m_next_personality = logitech_personality::invalid;
				if (cmd == 0x09 && arg == 0x04)
					m_next_personality = logitech_personality::g27;
				else if (cmd == 0x09 && arg == 0x03)
					m_next_personality = logitech_personality::driving_force_gt;
				else if ((cmd == 0x09 && arg == 0x02) || cmd == 0x10)
					m_next_personality = logitech_personality::g25;
				else if ((cmd == 0x09 && arg == 0x01) || cmd == 0x01)
					m_next_personality = logitech_personality::driving_force_pro;
				else if (cmd == 0x09 && arg == 0x00)
					m_next_personality = logitech_personality::driving_force_ex;

				if (logitech_personality limit = static_cast<logitech_personality>(g_cfg_logitech_g27.compatibility_limit.get());
					limit < m_next_personality)
				{
					m_next_personality = limit;
				}

				logitech_g27_log.success("Change device mode : buf=[%s], cmd=0x%x, arg=0x%x, lim=%d -> pers=%d(%s)",
					fmt::buf_to_hexstring(buf, buf_size), cmd, arg, g_cfg_logitech_g27.compatibility_limit.get(),
					static_cast<u8>(m_next_personality),
					m_next_personality == logitech_personality::g27 ? "G27"
					: m_next_personality == logitech_personality::driving_force_gt ? "Driving Force GT"
					: m_next_personality == logitech_personality::g25 ? "G25"
					: m_next_personality == logitech_personality::driving_force_pro ? "Driving Force Pro"
					: m_next_personality == logitech_personality::driving_force_ex ? "Driving Force EX" : "Invalid");
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
			case 0x0a:
			{
				// Revert indentity
				logitech_g27_log.error("Revert device identity after reset %s command ignored", buf[2] ? "enable" : "disable");
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
						constexpr u16 deadband = 2 * 0xFFFF / 255;
						constexpr s16 center = 0;
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
								logitech_g27_log.error("Failed playing sdl effect %d on slot %d, %s", m_effect_slots[i].last_effect.type, i, SDL_GetError());
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
										logitech_g27_log.error("Failed playing sdl effect %d on slot %d, %s", m_effect_slots[i].last_effect.type, i, SDL_GetError());
									}
								}
								else
								{
									if (!SDL_StopHapticEffect(m_haptic_handle, m_effect_slots[i].effect_id))
									{
										logitech_g27_log.error("Failed stopping sdl effect %d on slot %d, %s", m_effect_slots[i].last_effect.type, i, SDL_GetError());
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
							logitech_g27_log.error("Tried to play effect slot %d but it was never uploaded", i);
						}
						else
						{
							logitech_g27_log.error("Tried to stop effect slot %d but it was never uploaded", i);
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
