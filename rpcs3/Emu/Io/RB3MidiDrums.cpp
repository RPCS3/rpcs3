// Rock Band 3 MIDI Pro Adapter Emulator (drums Mode)

#include "stdafx.h"
#include "RB3MidiDrums.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/Io/rb3drums_config.h"

using namespace std::chrono_literals;

LOG_CHANNEL(rb3_midi_drums_log);

namespace controller
{

// Bit flags by byte index.

constexpr usz FLAG = 0;
constexpr usz INDEX = 1;

using FlagByIndex = std::array<u8, 2>;

constexpr FlagByIndex BUTTON_1 = {0x01, 0};
constexpr FlagByIndex BUTTON_2 = {0x02, 0};
constexpr FlagByIndex BUTTON_3 = {0x04, 0};
constexpr FlagByIndex BUTTON_4 = {0x08, 0};
constexpr FlagByIndex BUTTON_5 = {0x10, 0};
constexpr FlagByIndex BUTTON_6 = {0x20, 0};
// constexpr FlagByIndex BUTTON_7 = {0x40, 0};
// constexpr FlagByIndex BUTTON_8 = {0x80, 0};

constexpr FlagByIndex BUTTON_9  = {0x01, 1};
constexpr FlagByIndex BUTTON_10 = {0x02, 1};
constexpr FlagByIndex BUTTON_11 = {0x04, 1};
constexpr FlagByIndex BUTTON_12 = {0x08, 1};
// constexpr FlagByIndex BUTTON_13 = {0x10, 1};

constexpr usz DPAD_INDEX = 2;
enum class DPad : u8
{
	Up     = 0x00,
	Right  = 0x02,
	Down   = 0x04,
	Left   = 0x06,
	Center = 0x08,
};

constexpr u8 AXIS_CENTER = 0x7F;

constexpr std::array<u8, 27> default_state = {
	0x00,                     // buttons 1 to 8
	0x00,                     // buttons 9 to 13
	static_cast<u8>(controller::DPad::Center),
	controller::AXIS_CENTER,  // x axis
	controller::AXIS_CENTER,  // y axis
	controller::AXIS_CENTER,  // z axis
	controller::AXIS_CENTER,  // w axis
	0x00,
	0x00,
	0x00,
	0x00,
	0x00, // yellow drum/cymbal velocity
	0x00, // red drum/cymbal velocity
	0x00, // green drum/cymbal velocity
	0x00, // blue drum/cymbal velocity
	0x00,
	0x00,
	0x00,
	0x00,
	0x02,
	0x00,
	0x02,
	0x00,
	0x02,
	0x00,
	0x02,
	0x00
};

} // namespace controller

namespace drum
{

// Hold each hit for a period of time. Rock band doesn't pick up a single tick.
std::chrono::milliseconds hit_duration()
{
	return std::chrono::milliseconds(g_cfg_rb3drums.pulse_ms);
}

// Scale velocity from midi to what rock band expects.
u8 scale_velocity(u8 value)
{
	return (0xFF - (2 * value));
}

constexpr usz FLAG = controller::FLAG;
constexpr usz INDEX = controller::INDEX;

using FlagByIndex = controller::FlagByIndex;

constexpr FlagByIndex GREEN         = controller::BUTTON_2;
constexpr FlagByIndex RED           = controller::BUTTON_3;
constexpr FlagByIndex YELLOW        = controller::BUTTON_4;
constexpr FlagByIndex BLUE          = controller::BUTTON_1;
constexpr FlagByIndex KICK_PEDAL    = controller::BUTTON_5;
constexpr FlagByIndex HIHAT_PEDAL   = controller::BUTTON_6;

constexpr FlagByIndex IS_DRUM       = controller::BUTTON_11;
constexpr FlagByIndex IS_CYMBAL	    = controller::BUTTON_12;

// constexpr FlagByIndex BACK_BUTTON   = controller::BUTTON_3;
constexpr FlagByIndex START_BUTTON  = controller::BUTTON_10;
// constexpr FlagByIndex SYSTEM_BUTTON = controller::BUTTON_13;
constexpr FlagByIndex SELECT_BUTTON = controller::BUTTON_9;

rb3drums::KitState start_state()
{
	rb3drums::KitState s{};
	s.expiry = std::chrono::steady_clock::now() + drum::hit_duration();
	s.start = true;
	return s;
}

rb3drums::KitState select_state()
{
	rb3drums::KitState s{};
	s.expiry = std::chrono::steady_clock::now() + drum::hit_duration();
	s.select = true;
	return s;
}

rb3drums::KitState toggle_hold_kick_state()
{
	rb3drums::KitState s{};
	s.expiry = std::chrono::steady_clock::now() + drum::hit_duration();
	s.toggle_hold_kick = true;
	return s;
}

rb3drums::KitState kick_state()
{
	rb3drums::KitState s{};
	s.expiry = std::chrono::steady_clock::now() + drum::hit_duration();
	s.kick_pedal = 127;
	return s;
}

} // namespace drum

namespace midi
{

u8 min_velocity()
{
	return g_cfg_rb3drums.minimum_velocity;
}

Note str_to_note(const std::string_view name)
{
	static const std::unordered_map<std::string_view, Note> mapping{
		{"Invalid", Note::Invalid},
		{"Kick", Note::Kick},
		{"HihatPedal", Note::HihatPedal},
		{"Snare", Note::Snare},
		{"SnareRim", Note::SnareRim},
		{"HiTom", Note::HiTom},
		{"LowTom", Note::LowTom},
		{"FloorTom", Note::FloorTom},
		{"HihatWithPedalUp", Note::HihatWithPedalUp},
		{"Hihat", Note::Hihat},
		{"Ride", Note::Ride},
		{"Crash", Note::Crash},
	};
	auto it = mapping.find(name);
	return it != std::end(mapping) ? it->second : Note::Invalid;
}

std::optional<std::pair<Id, Note>> parse_midi_override(const std::string_view config)
{
	auto split = fmt::split(config, {"="});
	if (split.size() != 2)
	{
		return {};
	}
	uint64_t id_int = 0;
	if (!try_to_uint64(&id_int, split[0], 0, 255))
	{
		rb3_midi_drums_log.warning("midi override: %s is not a valid midi id", split[0]);
		return {};
	}
	auto id = static_cast<Id>(id_int);
	auto note = str_to_note(split[1]);
	if (note == Note::Invalid)
	{
		rb3_midi_drums_log.warning("midi override: %s is not a valid note", split[1]);
		return {};
	}
	rb3_midi_drums_log.success("found valid midi override: %s", config);
	return {{id, note}};
}

std::unordered_map<Id, Note> create_id_to_note_mapping()
{
	std::unordered_map<Id, Note> mapping{
		{Id::MidiCC,            Note::Kick},
		{Id::Kick0,             Note::Kick},
		{Id::Kick1,             Note::Kick},
		{Id::Kick2,             Note::Kick},
		{Id::HihatPedal,        Note::HihatPedal},
		{Id::HihatPedalPartial, Note::HihatPedal},
		{Id::Snare0,            Note::Snare},
		{Id::Snare1,            Note::Snare},
		{Id::Snare2,            Note::Snare},
		{Id::Snare3,            Note::Snare},
		{Id::Snare4,            Note::Snare},
		{Id::SnareRim,          Note::SnareRim},
		{Id::HiTom0,            Note::HiTom},
		{Id::HiTom1,            Note::HiTom},
		{Id::LowTom0,           Note::LowTom},
		{Id::LowTom1,           Note::LowTom},
		{Id::FloorTom0,         Note::FloorTom},
		{Id::FloorTom1,         Note::FloorTom},
		{Id::Hihat0,            Note::Hihat},
		{Id::Hihat1,            Note::Hihat},
		{Id::Hihat2,            Note::Hihat},
		{Id::Hihat3,            Note::Hihat},
		{Id::HihatWithPedalUp,  Note::Hihat},
		{Id::Ride0,             Note::Ride},
		{Id::Ride1,             Note::Ride},
		{Id::Ride2,             Note::Ride},
		{Id::Ride3,             Note::Ride},
		{Id::Crash0,            Note::Crash},
		{Id::Crash1,            Note::Crash},
		{Id::Crash2,            Note::Crash},
		{Id::Crash3,            Note::Crash},
	};

	// Apply configured overrides.
	const std::vector<std::string> segments = fmt::split(g_cfg_rb3drums.midi_overrides.to_string(), {","});
	for (const std::string& segment : segments)
	{
		if (const auto midi_override = parse_midi_override(segment))
		{
			const auto id = midi_override->first;
			const auto note = midi_override->second;
			mapping[id] = note;
		}
	}
	return mapping;
}

namespace combo
{

std::vector<u8> parse_combo(const std::string_view name, const std::string_view csv)
{
	if (csv.empty())
	{
		return {};
	}
	std::vector<u8> notes;
	const auto& note_names = fmt::split(csv, {","});
	for (const auto& note_name : note_names)
	{
		const auto note = str_to_note(note_name);
		if (note != midi::Note::Invalid)
		{
			notes.push_back(static_cast<u8>(note));
		}
		else
		{
			rb3_midi_drums_log.warning("invalid note '%s' in configured combo '%s'", note_name, name);
		}
	}
	return notes;
}

std::chrono::milliseconds window()
{
	return std::chrono::milliseconds{g_cfg_rb3drums.combo_window_ms};
}

}

} // namespace midi

namespace
{

void set_flag(u8* buf, [[maybe_unused]] std::string_view name, const controller::FlagByIndex& fbi)
{
	auto i = fbi[drum::INDEX];
	auto flag = fbi[drum::FLAG];
	buf[i] |= flag;
	// rb3_midi_drums_log.success("wrote flag %x at index %d", flag, i);
}

void set_flag_if_any(u8* buf, std::string_view name, const controller::FlagByIndex& fbi, const std::vector<u8> velocities)
{
	if (std::none_of(velocities.begin(), velocities.end(), [](u8 velocity){ return velocity >= midi::min_velocity(); }))
	{
		return;
	}
	set_flag(buf, name, fbi);
}

}

usb_device_rb3_midi_drums::Definition::Definition(std::string name, const std::string_view csv, const std::function<rb3drums::KitState()> create_state)
	: name{std::move(name)}
	, notes{midi::combo::parse_combo(this->name, csv)}
	, create_state{create_state}
{}

usb_device_rb3_midi_drums::usb_device_rb3_midi_drums(const std::array<u8, 7>& location, const std::string& device_name)
	: usb_device_emulated(location)
{
	m_id_to_note_mapping = midi::create_id_to_note_mapping();
	combo.reload_definitions();

	UsbDeviceDescriptor descriptor{};
	descriptor.bcdDevice = 0x0200;
	descriptor.bDeviceClass = 0x00;
	descriptor.bDeviceSubClass = 0x00;
	descriptor.bDeviceProtocol = 0x00;
	descriptor.bMaxPacketSize0 = 64;
	descriptor.idVendor = 0x12BA; // Harmonix
	descriptor.idProduct = 0x0210; // Drums
	descriptor.bcdDevice = 0x01;
	descriptor.iManufacturer = 0x01;
	descriptor.iProduct = 0x02;
	descriptor.iSerialNumber = 0x00;
	descriptor.bNumConfigurations = 0x01;
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, descriptor);

	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG, UsbDeviceConfiguration{41, 1, 1, 0, 0x80, 32}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE, UsbDeviceInterface{0, 0, 2, 3, 0, 0, 0}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_HID, UsbDeviceHID{0x0111, 0x00, 0x01, 0x22, 137}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x81, 0x03, 0x0040, 10}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x02, 0x03, 0x0040, 10}));

	usb_device_emulated::add_string("Licensed by Sony Computer Entertainment America");
	usb_device_emulated::add_string("Harmonix RB3 MIDI Drums Interface for PlayStation®3");

	// connect to midi device
	midi_in = rtmidi_in_create_default();
	ensure(midi_in);

	if (!midi_in->ok)
	{
		rb3_midi_drums_log.error("Could not get MIDI in ptr: %s", midi_in->msg);
		return;
	}

	const RtMidiApi api = rtmidi_in_get_current_api(midi_in);

	if (!midi_in->ok)
	{
		rb3_midi_drums_log.error("Could not get MIDI api: %s", midi_in->msg);
		return;
	}

	if (const char* api_name = rtmidi_api_name(api))
	{
		rb3_midi_drums_log.notice("Using %s api", api_name);
	}
	else
	{
		rb3_midi_drums_log.warning("Could not get MIDI api name");
	}

	rtmidi_in_ignore_types(midi_in, false, true, true);

	const u32 port_count = rtmidi_get_port_count(midi_in);

	if (!midi_in->ok || port_count == umax)
	{
		rb3_midi_drums_log.error("Could not get MIDI port count: %s", midi_in->msg);
		return;
	}

	for (u32 port_number = 0; port_number < port_count; port_number++)
	{
		char buf[128]{};
		s32 size = sizeof(buf);
		if (rtmidi_get_port_name(midi_in, port_number, buf, &size) == -1 || !midi_in->ok)
		{
			rb3_midi_drums_log.error("Error getting port name for port %d: %s", port_number, midi_in->msg);
			return;
		}

		rb3_midi_drums_log.notice("Found device with name: %s", buf);

		if (device_name == buf)
		{
			rtmidi_open_port(midi_in, port_number, "RPCS3 MIDI Drums Input");

			if (!midi_in->ok)
			{
				rb3_midi_drums_log.error("Could not open port %d for device '%s': %s", port_number, device_name, midi_in->msg);
				return;
			}

			rb3_midi_drums_log.success("Connected to device: %s", device_name);
			return;
		}
	}

	rb3_midi_drums_log.error("Could not find device with name: %s", device_name);
}

usb_device_rb3_midi_drums::~usb_device_rb3_midi_drums()
{
	rtmidi_in_free(midi_in);
}

static const std::array<u8, 40> disabled_response = {
	0xe9, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0f, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x82,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x21, 0x26, 0x02, 0x06, 0x00, 0x00, 0x00, 0x00};

static const std::array<u8, 40> enabled_response = {
	0xe9, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x8a,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x21, 0x26, 0x02, 0x06, 0x00, 0x00, 0x00, 0x00};

void usb_device_rb3_midi_drums::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake = true;

	// configuration packets sent by rock band 3
	// we only really need to check 1 byte here to figure out if the game
	// wants to enable midi data or disable it
	if (bmRequestType == 0x21 && bRequest == 0x9 && wLength == 40)
	{
		if (buf_size < 3)
		{
			rb3_midi_drums_log.warning("buffer size < 3, bailing out early (buf_size=0x%x)", buf_size);
			return;
		}

		switch (buf[2])
		{
		case 0x89:
			rb3_midi_drums_log.notice("MIDI data enabled.");
			buttons_enabled = true;
			response_pos = 0;
			break;
		case 0x81:
			rb3_midi_drums_log.notice("MIDI data disabled.");
			buttons_enabled = false;
			response_pos = 0;
			break;
		default:
			rb3_midi_drums_log.warning("Unhandled SET_REPORT request: 0x%02X");
			break;
		}
	}

	// the game expects some sort of response to the configuration packet
	else if (bmRequestType == 0xa1 && bRequest == 0x1)
	{
		//rb3_midi_drums_log.success("[control_transfer] config 0xa1 0x1 length %d", wLength);

		transfer->expected_count = buf_size;
		if (buttons_enabled)
		{
			const usz remaining_bytes = enabled_response.size() - response_pos;
			const usz copied_bytes = std::min<usz>(remaining_bytes, buf_size);
			memcpy(buf, &enabled_response[response_pos], copied_bytes);
			response_pos += copied_bytes;
		}
		else
		{
			const usz remaining_bytes = disabled_response.size() - response_pos;
			const usz copied_bytes = std::min<usz>(remaining_bytes, buf_size);
			memcpy(buf, &disabled_response[response_pos], copied_bytes);
			response_pos += copied_bytes;
		}
	}
	//else if (bmRequestType == 0x00 && bRequest == 0x9)
	//{
	//  // idk what this is but if I handle it we don't get input.
	//	rb3_midi_drums_log.success("handled -- request %x, type %x, length %x", bmRequestType, bRequest, wLength);
	//}
	//else if (bmRequestType == 0x80 && bRequest == 0x6)
	//{
	//  // idk what this is but if I handle it we don't get input.
	//	rb3_midi_drums_log.success("handled -- request %x, type %x, length %x", bmRequestType, bRequest, wLength);
	//}
	else if (bmRequestType == 0x21 && bRequest == 0x9 && wLength == 8)
	{
		// the game uses this request to do things like set the LEDs
		// we don't have any LEDs, so do nothing
	}
	else
	{
		rb3_midi_drums_log.error("unhandled control_transfer: request %x, type %x, length %x", bmRequestType, bRequest, wLength);
		usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
	}
}

void usb_device_rb3_midi_drums::interrupt_transfer(u32 buf_size, u8* buf, u32 /*endpoint*/, UsbTransfer* transfer)
{
	transfer->fake = true;
	transfer->expected_count = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	// the real device takes 8ms to send a response, but there is
	// no reason we can't make it faster
	transfer->expected_time = get_timestamp() + 1'000;

	const auto& bytes = controller::default_state;
	if (buf_size < bytes.size())
	{
		rb3_midi_drums_log.warning("buffer size < %x, bailing out early (buf_size=0x%x)", bytes.size(), buf_size);
		return;
	}
	memcpy(buf, bytes.data(), bytes.size());

	if (g_cfg_rb3drums.reload_requested)
	{
		m_id_to_note_mapping = midi::create_id_to_note_mapping();
		combo.reload_definitions();
	}

	while (true)
	{
		u8 midi_msg[32];
		usz size = sizeof(midi_msg);

		// This returns a double as some sort of delta time, with -1.0
		// being used to signal an error.
		if (rtmidi_in_get_message(midi_in, midi_msg, &size) == -1.0)
		{
			rb3_midi_drums_log.error("Error getting MIDI message: %s", midi_in->msg);
			return;
		}

		if (size == 0)
		{
			break;
		}

		auto kit_state = parse_midi_message(midi_msg, size);
		if (auto combo_state = combo.take_state())
		{
			if (combo_state->toggle_hold_kick)
			{
				hold_kick = !hold_kick;
			}
			else
			{
				kit_states.push_back(std::move(combo_state.value()));
			}
		}
		else
		{
			bool is_cancel = kit_state.snare >= midi::min_velocity();
			bool is_accept = kit_state.floor_tom >= midi::min_velocity();
			if (hold_kick && (is_cancel || is_accept))
			{
				// Hold kick brings up the song category selector menu, which can be dismissed using accept/cancel buttons.
				hold_kick = false;
			}
			else
			{
				kit_states.push_back(std::move(kit_state));
			}
		}
	}

	// Clean expired states.
	auto now = std::chrono::steady_clock::now();
	kit_states.erase(std::remove_if(std::begin(kit_states), std::end(kit_states), [&now](const rb3drums::KitState& kit_state) {
		return now >= kit_state.expiry;
	}), std::end(kit_states));

	bool cymbal_hit = false;
	usz i = 0;
	for (; i < kit_states.size(); ++i)
	{
		const auto& kit_state = kit_states[i];

		// Rockband sometimes has trouble registering both hits when two cymbals are hit at once.
		// To solve for this, we stagger cymbal hits so that they occur one after another instead of at the same time.
		// Note that this is staggering by pulse_ms (30ms default) so a human is unlikely to notice it in practice.
		if (g_cfg_rb3drums.stagger_cymbals && cymbal_hit && kit_state.is_cymbal())
		{
			// Already have a cymbal applied, buffer other inputs.
			break;
		}
		cymbal_hit = kit_state.is_cymbal();
		write_state(buf, kit_state);
	}

	if (hold_kick)
	{
		write_state(buf, drum::kick_state());
	}

	// Extend expiry on buffered states since they are not active.
	for (; i < kit_states.size(); ++i)
	{
		kit_states[i].expiry = now + drum::hit_duration();
	}
}

rb3drums::KitState usb_device_rb3_midi_drums::parse_midi_message(u8* msg, usz size)
{
	if (size < 3)
	{
		rb3_midi_drums_log.warning("parse_midi_message: encountered message with size less than 3 bytes");
		return rb3drums::KitState{};
	}

	auto status = msg[0];
	auto id = msg[1];
	auto value = msg[2];

	if (status == 0x99)
	{
		return parse_midi_note(id, value);
	}
	if (status == g_cfg_rb3drums.midi_cc_status)
	{
		if (is_midi_cc(id, value))
		{
			return parse_midi_note(static_cast<u8>(midi::Id::MidiCC), 127);
		}
	}
	// Ignore non-"note on" midi status messages.
	return rb3drums::KitState{};
}

midi::Note usb_device_rb3_midi_drums::id_to_note(midi::Id id)
{
	const auto it = m_id_to_note_mapping.find(id);
	return it != m_id_to_note_mapping.cend() ? it->second : midi::Note::Invalid;
}

rb3drums::KitState usb_device_rb3_midi_drums::parse_midi_note(const u8 id, const u8 velocity)
{
	if (velocity < midi::min_velocity())
	{
		// Must check here so we don't overwrite good values when applying states.
		return rb3drums::KitState{};
	}

	rb3drums::KitState kit_state{};
	kit_state.expiry = std::chrono::steady_clock::now() + drum::hit_duration();
	const midi::Note note = id_to_note(static_cast<midi::Id>(id));
	switch (note)
	{
		case midi::Note::Kick:              kit_state.kick_pedal    = velocity; break;
		case midi::Note::HihatPedal:        kit_state.hihat_pedal   = velocity; break;
		case midi::Note::Snare:             kit_state.snare         = velocity; break;
		case midi::Note::SnareRim:          kit_state.snare_rim     = velocity; break;
		case midi::Note::HiTom:             kit_state.hi_tom        = velocity; break;
		case midi::Note::LowTom:            kit_state.low_tom       = velocity; break;
		case midi::Note::FloorTom:          kit_state.floor_tom     = velocity; break;
		case midi::Note::Hihat:             kit_state.hihat         = velocity; break;
		case midi::Note::Ride:              kit_state.ride          = velocity; break;
		case midi::Note::Crash:             kit_state.crash         = velocity; break;
		default:
			// Ignored note.
			rb3_midi_drums_log.error("IGNORED NOTE: id = %x or %d", id, id);
			return rb3drums::KitState{};
	}

	combo.add(static_cast<u8>(note));
	return kit_state;
}

bool usb_device_rb3_midi_drums::is_midi_cc(const u8 id, const u8 value)
{
	if (id != g_cfg_rb3drums.midi_cc_number)
	{
		return false;
	}

	const auto is_past_threshold = [](u8 value)
	{
		const u8 threshold = g_cfg_rb3drums.midi_cc_threshold;
		return g_cfg_rb3drums.midi_cc_invert_threshold
			? value < threshold
			: value > threshold;
	};

	if (midi_cc_triggered)
	{
		if (!is_past_threshold(value))
		{
			// Reset triggered state when we fall back past threshold.
			midi_cc_triggered = false;
		}
	}
	else
	{
		if (is_past_threshold(value))
		{
			midi_cc_triggered = true;
			return true;
		}
	}
	return false;
}

void usb_device_rb3_midi_drums::write_state(u8* buf, const rb3drums::KitState& kit_state)
{
	// See: https://github.com/TheNathannator/PlasticBand/blob/main/Docs/Instruments/4-Lane%20Drums/PS3%20and%20Wii.md#input-info

	// Interestingly, because cymbals use the same visual track as drums, a hit on that color can only be a drum OR a cymbal.
	// rockband handles this by taking a flag to indicate if the hit is a drum vs cymbal.
	set_flag_if_any(buf, "red", drum::RED, {kit_state.snare});
	set_flag_if_any(buf, "yellow", drum::YELLOW, {kit_state.hi_tom, kit_state.hihat});
	set_flag_if_any(buf, "blue", drum::BLUE, {kit_state.low_tom, kit_state.ride}); // Rock band charts blue cymbal for both hihat open and ride sometimes.
	set_flag_if_any(buf, "green", drum::GREEN, {kit_state.floor_tom, kit_state.crash});

	// Additionally, Yellow (hihat) and Blue (ride) cymbals add dpad up or down, respectively. This allows rockband to disambiguate between tom+cymbals hit at the same time.
	if (kit_state.hihat >= midi::min_velocity())
	{
		buf[controller::DPAD_INDEX] = static_cast<u8>(controller::DPad::Up);
	}
	if (kit_state.ride >= midi::min_velocity())
	{
		buf[controller::DPAD_INDEX] = static_cast<u8>(controller::DPad::Down);
	}

	set_flag_if_any(buf, "is_drum", drum::IS_DRUM, {kit_state.snare, kit_state.hi_tom, kit_state.low_tom, kit_state.floor_tom});
	set_flag_if_any(buf, "is_cymbal", drum::IS_CYMBAL, {kit_state.hihat, kit_state.ride, kit_state.crash});

	set_flag_if_any(buf, "kick_pedal", drum::KICK_PEDAL, {kit_state.kick_pedal});
	set_flag_if_any(buf, "hihat_pedal", drum::HIHAT_PEDAL, {kit_state.hihat_pedal});

	buf[11] = drum::scale_velocity(std::max(kit_state.hi_tom, kit_state.hihat));
	buf[12] = drum::scale_velocity(kit_state.snare);
	buf[13] = drum::scale_velocity(std::max(kit_state.floor_tom, kit_state.crash));
	buf[14] = drum::scale_velocity(std::max({kit_state.low_tom, kit_state.ride}));

	if (kit_state.start)
	{
		set_flag(buf, "start", drum::START_BUTTON);
	}
	if (kit_state.select)
	{
		set_flag(buf, "select", drum::SELECT_BUTTON);
	}

	// Unbound cause idk what to bind them to, but you don't really need them anyway.
	// set_flag_if_any(buf, drum::SYSTEM_BUTTON, );
	// set_flag_if_any(buf, drum::BACK_BUTTON, );
}

bool rb3drums::KitState::is_cymbal() const
{
	return std::max({hihat, ride, crash}) >= midi::min_velocity();
}

bool rb3drums::KitState::is_drum() const
{
	return std::max({snare, hi_tom, low_tom, floor_tom}) >= midi::min_velocity();
}

void usb_device_rb3_midi_drums::ComboTracker::reload_definitions()
{
	m_definitions = {
		{"start",     g_cfg_rb3drums.combo_start.to_string(),            []{ return drum::start_state(); }},
		{"select",    g_cfg_rb3drums.combo_select.to_string(),           []{ return drum::select_state(); }},
		{"hold kick", g_cfg_rb3drums.combo_toggle_hold_kick.to_string(), []{ return drum::toggle_hold_kick_state(); }}
	};
}

void usb_device_rb3_midi_drums::ComboTracker::add(u8 note)
{
	if (!midi_notes.empty() && std::chrono::steady_clock::now() >= expiry)
	{
		// Combo expired.
		reset();
	}

	const usz i = midi_notes.size();
	bool is_in_combo = false;
	for (const auto& def : m_definitions)
	{
		if (i < def.notes.size() && note == def.notes[i])
		{
			// Track notes as long as we match any combo.
			midi_notes.push_back(note);
			is_in_combo = true;
			break;
		}
	}

	if (!is_in_combo)
	{
		reset();
	}

	if (midi_notes.size() == 1)
	{
		// New combo.
		expiry = std::chrono::steady_clock::now() + midi::combo::window();
	}
}

void usb_device_rb3_midi_drums::ComboTracker::reset()
{
	midi_notes.clear();
}

std::optional<rb3drums::KitState> usb_device_rb3_midi_drums::ComboTracker::take_state()
{
	if (midi_notes.empty())
	{
		return {};
	}
	for (const auto& combo : m_definitions)
	{
		if (midi_notes == combo.notes)
		{
			rb3_midi_drums_log.success("hit combo: %s", combo.name);
			reset();
			return combo.create_state();
		}
	}
	return {};
}
