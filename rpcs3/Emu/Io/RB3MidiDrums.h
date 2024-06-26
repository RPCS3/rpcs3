#pragma once

#include "Emu/Io/usb_device.h"

#include <rtmidi_c.h>
#include <chrono>
#include <vector>
#include <optional>

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

} // namespace rb3drums

namespace midi
{

enum class Id : u8
{
	// Each 'Note' can be triggered by multiple different numbers.
	// Keeping them flattened in an enum for simplicity / switch statement usage.

	// These follow the rockband 3 midi pro adapter support.
	Snare0 = 38,
	Snare1 = 31,
	Snare2 = 34,
	Snare3 = 37,
	Snare4 = 39,
	HiTom0 = 48,
	HiTom1 = 50,
	LowTom0 = 45,
	LowTom1 = 47,
	FloorTom0 = 41,
	FloorTom1 = 43,
	Hihat0 = 22,
	Hihat1 = 26,
	Hihat2 = 42,
	Hihat3 = 54,
	Ride0 = 51,
	Ride1 = 53,
	Ride2 = 56,
	Ride3 = 59,
	Crash0 = 49,
	Crash1 = 52,
	Crash2 = 55,
	Crash3 = 57,
	Kick0 = 33,
	Kick1 = 35,
	Kick2 = 36,
	HihatPedal = 44,

	// These are from alesis nitro mesh max. ymmv.
	SnareRim = 40, // midi pro adapter counts this as snare.
	HihatWithPedalUp = 46, // The midi pro adapter considers this a normal hihat hit.
	HihatPedalPartial = 23, // If pedal is not 100% down, this will be sent instead of a normal hihat hit.

	// Internal value used for converting midi CC.
	// Values past 127 are not used in midi notes.
	MidiCC = 255,
};

// Intermediate mapping regardless of which midi ids triggered it.
enum class Note : u8
{
	Invalid,
	Kick,
	HihatPedal,
	Snare,
	SnareRim,
	HiTom,
	LowTom,
	FloorTom,
	HihatWithPedalUp,
	Hihat,
	Ride,
	Crash,
};

}

class usb_device_rb3_midi_drums : public usb_device_emulated
{
public:
	usb_device_rb3_midi_drums(const std::array<u8, 7>& location, const std::string& device_name);
	~usb_device_rb3_midi_drums();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;

private:
	usz response_pos{};
	bool buttons_enabled{};
	RtMidiInPtr midi_in{};
	std::vector<rb3drums::KitState> kit_states;
	bool hold_kick{};
	bool midi_cc_triggered{};

	struct Definition
	{
		std::string name;
		std::vector<u8> notes;
		std::function<rb3drums::KitState()> create_state;
	
		Definition(std::string name, const std::string_view csv, const std::function<rb3drums::KitState()> create_state);
	};

	class ComboTracker
	{
	public:
		void reload_definitions();
		void add(u8 note);
		void reset();
		std::optional<rb3drums::KitState> take_state();

	private:
		std::chrono::steady_clock::time_point expiry;
		std::vector<u8> midi_notes;
		std::vector<Definition> m_definitions;
	};
	ComboTracker combo;

	std::unordered_map<midi::Id, midi::Note> m_id_to_note_mapping;

	midi::Note id_to_note(midi::Id id);
	rb3drums::KitState parse_midi_message(u8* msg, usz size);
	rb3drums::KitState parse_midi_note(u8 id, u8 velocity);
	bool is_midi_cc(u8 id, u8 value);
	void write_state(u8* buf, const rb3drums::KitState&);
};
