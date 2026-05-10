#pragma once

#include "Emu/Io/usb_device.h"

#include <rtmidi_c.h>

class usb_device_rb3_midi_keyboard : public usb_device_emulated
{
private:
	usz response_pos = 0;
	bool buttons_enabled = false;
	RtMidiInPtr midi_in{};

	// button states
	// TODO: emulate velocity
	struct
	{
		u8 count = 0;

		bool cross = false;
		bool circle = false;
		bool square = false;
		bool triangle = false;

		bool start = false;
		bool select = false;
		bool overdrive = false;

		bool dpad_up = false;
		bool dpad_down = false;
		bool dpad_left = false;
		bool dpad_right = false;

		std::array<bool, 25> keys{};
		std::array<u8, 25> velocities{};

		s16 pitch_wheel = 0;
	} button_state;

	void parse_midi_message(u8* msg, usz size);
	void write_state(u8* buf);

public:
	usb_device_rb3_midi_keyboard(const std::array<u8, 7>& location, const std::string& device_name);
	~usb_device_rb3_midi_keyboard();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;
};
