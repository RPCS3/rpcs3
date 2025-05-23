#pragma once

#include "Emu/Io/usb_device.h"
#include "Utilities/Thread.h"
#include "LogitechG27Config.h"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "SDL3/SDL.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#include <map>
#include <vector>

enum class logitech_g27_ffb_state
{
	inactive,
	downloaded,
	playing
};

struct logitech_g27_ffb_slot
{
	logitech_g27_ffb_state state = logitech_g27_ffb_state::inactive;
	u64 last_update = 0;
	SDL_HapticEffect last_effect {};

	// TODO switch to SDL_HapticEffectID when it becomes available in a future SDL release
	// Match the return of SDL_CreateHapticEffect for now
	int effect_id = -1;
};

struct sdl_mapping
{
	/*
	 * orginally 32bit, just vendor product match
	 * v1: (vendor_id << 16) | product_id
	 *
	 * now 64bit, matching more to handle Fanatec's shenanigans, should be good until Fanatec desides that it's funny to register > 1023 axes/hats/buttons, then have two hid devices with the exact same numbers with one single wheel base
	 * serial/version/firmware/guid is not used for now because those are still unreliable in SDL in the context of config
	 * not migrating to string yet, don't want to make joystick look up heavy
	 * v2: (num_buttons:10 << 52) | (num_hats:10 << 42) | (num_axes:10 << 32) | (vendor_id:16 << 16) | product_id:16
	 */
	u64 device_type_id = 0;
	sdl_mapping_type type = sdl_mapping_type::button;
	u64 id = 0;
	hat_component hat = hat_component::none;
	bool reverse = false;
	bool positive_axis = false;
};

struct logitech_g27_sdl_mapping
{
	sdl_mapping steering {};
	sdl_mapping throttle {};
	sdl_mapping brake {};
	sdl_mapping clutch {};
	sdl_mapping shift_up {};
	sdl_mapping shift_down {};

	sdl_mapping up {};
	sdl_mapping down {};
	sdl_mapping left {};
	sdl_mapping right {};

	sdl_mapping triangle {};
	sdl_mapping cross {};
	sdl_mapping square {};
	sdl_mapping circle {};

	// mappings based on g27 compat mode on g29
	sdl_mapping l2 {};
	sdl_mapping l3 {};
	sdl_mapping r2 {};
	sdl_mapping r3 {};

	sdl_mapping plus {};
	sdl_mapping minus {};

	sdl_mapping dial_clockwise {};
	sdl_mapping dial_anticlockwise {};

	sdl_mapping select {};
	sdl_mapping pause {};

	sdl_mapping shifter_1 {};
	sdl_mapping shifter_2 {};
	sdl_mapping shifter_3 {};
	sdl_mapping shifter_4 {};
	sdl_mapping shifter_5 {};
	sdl_mapping shifter_6 {};
	sdl_mapping shifter_r {};
	sdl_mapping shifter_press {};
};

class usb_device_logitech_g27 : public usb_device_emulated
{
public:
	usb_device_logitech_g27(u32 controller_index, const std::array<u8, 7>& location);
	~usb_device_logitech_g27();

	static std::shared_ptr<usb_device> make_instance(u32 controller_index, const std::array<u8, 7>& location);
	static u16 get_num_emu_devices();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;
	bool open_device() override;

private:
	void sdl_refresh();

	u32 m_controller_index = 0;

	logitech_g27_sdl_mapping m_mapping {};
	bool m_reverse_effects = false;

	std::mutex m_sdl_handles_mutex;
	SDL_Joystick* m_led_joystick_handle = nullptr;
	SDL_Haptic* m_haptic_handle = nullptr;
	std::map<u64, std::vector<SDL_Joystick*>> m_joysticks;
	bool m_fixed_loop = false;
	u16 m_wheel_range = 200;
	std::array<logitech_g27_ffb_slot, 4> m_effect_slots {};
	SDL_HapticEffect m_default_spring_effect {};

	// TODO switch to SDL_HapticEffectID when it becomes available in a future SDL release
	int m_default_spring_effect_id = -1;

	bool m_enabled = false;

	std::unique_ptr<named_thread<std::function<void()>>> m_house_keeping_thread;
};
