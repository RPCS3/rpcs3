#pragma once

#include "Emu/Io/usb_device.h"

#include <rtmidi_c.h>
#include <chrono>
#include <vector>

namespace rb3drums
{
struct KitState
{
	std::chrono::steady_clock::time_point expiry;

	u8 kick_pedal{};
	u8 hihat_pedal{};

	u8 snare{};
	u8 snare_rim{};
	u8 hi_tom{};
	u8 low_tom{};
	u8 floor_tom{};

	u8 hihat{};
	u8 ride{};
	u8 crash{};

	// Buttons triggered by combos.
	bool start{};
	bool select{};

	// Special flag that keeps kick pedal held until toggled off.
	// This is used in rb3 to access the category select dropdown in the song list.
	bool toggle_hold_kick{};

	bool is_cymbal() const;
	bool is_drum() const;
};

}; // namespace rb3drums

class usb_device_rb3_midi_drums : public usb_device_emulated
{
private:
	usz response_pos = 0;
	bool buttons_enabled = false;
	RtMidiInPtr midi_in{};
	std::vector<rb3drums::KitState> kit_states;
	bool hold_kick{};

	class ComboTracker
	{
	public:
		void add(u8 note);
		void reset();
		std::optional<rb3drums::KitState> take_state();

	private:
		std::chrono::steady_clock::time_point expiry;
		std::vector<u8> midi_notes;
	};
	ComboTracker combo;

	rb3drums::KitState parse_midi_message(u8* msg, usz size);
	void write_state(u8* buf, const rb3drums::KitState&);

public:
	usb_device_rb3_midi_drums(const std::array<u8, 7>& location, const std::string& device_name);
	~usb_device_rb3_midi_drums();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;
};
