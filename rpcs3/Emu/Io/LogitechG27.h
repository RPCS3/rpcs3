#pragma once

#include "Emu/Io/usb_device.h"
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
#include <thread>

enum logitech_g27_ffb_state
{
	G27_FFB_INACTIVE,
	G27_FFB_DOWNLOADED,
	G27_FFB_PLAYING
};

struct logitech_g27_ffb_slot
{
	logitech_g27_ffb_state state;
	uint64_t last_update;
	SDL_HapticEffect last_effect;
	int effect_id;
};

struct sdl_mapping
{
	uint32_t device_type_id; // (vendor_id << 16) | product_id
	sdl_mapping_type type;
	uint64_t id;
	hat_component hat;
	bool reverse;
	bool positive_axis;
};

struct logitech_g27_sdl_mapping
{
	sdl_mapping steering;
	sdl_mapping throttle;
	sdl_mapping brake;
	sdl_mapping clutch;
	sdl_mapping shift_up;
	sdl_mapping shift_down;

	sdl_mapping up;
	sdl_mapping down;
	sdl_mapping left;
	sdl_mapping right;

	sdl_mapping triangle;
	sdl_mapping cross;
	sdl_mapping square;
	sdl_mapping circle;

	// mappings based on g27 compat mode on g29
	sdl_mapping l2;
	sdl_mapping l3;
	sdl_mapping r2;
	sdl_mapping r3;

	sdl_mapping plus;
	sdl_mapping minus;

	sdl_mapping dial_clockwise;
	sdl_mapping dial_anticlockwise;

	sdl_mapping select;
	sdl_mapping pause;

	sdl_mapping shifter_1;
	sdl_mapping shifter_2;
	sdl_mapping shifter_3;
	sdl_mapping shifter_4;
	sdl_mapping shifter_5;
	sdl_mapping shifter_6;
	sdl_mapping shifter_r;
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
	u32 m_controller_index;

	logitech_g27_sdl_mapping m_mapping;
	bool m_reverse_effects;

	std::mutex m_sdl_handles_mutex;
	SDL_Joystick* m_led_joystick_handle = nullptr;
	SDL_Haptic* m_haptic_handle = nullptr;
	std::map<uint32_t, std::vector<SDL_Joystick*>> m_joysticks;
	bool m_fixed_loop = false;
	uint16_t m_wheel_range = 200;
	logitech_g27_ffb_slot m_effect_slots[4];
	SDL_HapticEffect m_default_spring_effect = {0};
	int m_default_spring_effect_id = -1;

	bool m_enabled = false;

	std::thread m_house_keeping_thread;
	std::mutex m_thread_control_mutex;
	bool m_stop_thread;

	void sdl_refresh();
};
