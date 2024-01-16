#pragma once

#include "Emu/Io/usb_device.h"

#include <rtmidi_c.h>
#include <chrono>
#include <vector>

struct KitState
{
	std::chrono::steady_clock::time_point expiry;

	u8 kick_pedal{};
	u8 hihat_control{};

	u8 snare{};
	u8 snare_rim{};
	u8 tom1{};
	u8 tom2{};
	u8 tom3{};

	u8 hihat_up{};
	u8 hihat_down{};
	u8 crash{};
	u8 ride{};

	// Buttons triggered by combos.
	bool start{};
	bool select{};

	bool is_cymbal() const;
	bool is_drum() const;
};

class usb_device_rb3_midi_drums : public usb_device_emulated
{
private:
	usz response_pos = 0;
	bool buttons_enabled = false;
	RtMidiInPtr midi_in{};
	std::vector<KitState> kit_states;

	class ComboTracker
	{
	public:
		void add(u8 note);
		void reset();
		std::optional<KitState> take_state();

	private:
		std::chrono::steady_clock::time_point expiry;
		std::vector<u8> midi_notes;
	};
	ComboTracker combo;

	KitState parse_midi_message(u8* msg, usz size);
	void write_state(u8* buf, const KitState&);

public:
	usb_device_rb3_midi_drums(const std::array<u8, 7>& location, const std::string& device_name);
	~usb_device_rb3_midi_drums();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;
};
