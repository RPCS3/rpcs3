// Rock Band 3 MIDI Pro Adapter Emulator (drums Mode)

#include "stdafx.h"
#include "RB3MidiDrums.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/system_config.h"

using namespace std::chrono_literals;

LOG_CHANNEL(rb3_midi_drums_log);

namespace
{

namespace controller
{

// Bit flags by byte index.

const usz FLAG = 0;
const usz INDEX = 1;

using FlagByIndex = std::array<u8, 2>;

constexpr FlagByIndex BUTTON_1 = {0x01, 0};
constexpr FlagByIndex BUTTON_2 = {0x02, 0};
constexpr FlagByIndex BUTTON_3 = {0x04, 0};
constexpr FlagByIndex BUTTON_4 = {0x08, 0};
constexpr FlagByIndex BUTTON_5 = {0x10, 0};
constexpr FlagByIndex BUTTON_6 = {0x20, 0};
constexpr FlagByIndex BUTTON_7 = {0x40, 0};
constexpr FlagByIndex BUTTON_8 = {0x80, 0};

constexpr FlagByIndex BUTTON_9  = {0x01, 1};
constexpr FlagByIndex BUTTON_10 = {0x02, 1};
constexpr FlagByIndex BUTTON_11 = {0x04, 1};
constexpr FlagByIndex BUTTON_12 = {0x08, 1};
constexpr FlagByIndex BUTTON_13 = {0x10, 1};

const u8 HIHAT_UP		= 0x00;
const u8 HIHAT_RIGHT	= 0x02;
const u8 HIHAT_DOWN		= 0x04;
const u8 HIHAT_LEFT		= 0x06;
const u8 HIHAT_CENTER	= 0x08;
const u8 AXIS_CENTER	= 0x7F;

std::array<u8, 27> default_state() {
	std::array<u8, 27> bytes;
	bytes[0] = 0x00;                     // buttons 1 to 8
	bytes[1] = 0x00;                     // buttons 9 to 13
	bytes[2] = controller::HIHAT_CENTER; // hihat position
	bytes[3] = controller::AXIS_CENTER;  // x axis
	bytes[4] = controller::AXIS_CENTER;  // y axis
	bytes[5] = controller::AXIS_CENTER;  // z axis
	bytes[6] = controller::AXIS_CENTER;  // w axis
	bytes[7] = 0x00;
	bytes[8] = 0x00;
	bytes[9] = 0x00;
	bytes[10] = 0x00;
	bytes[11] = 0x00; // yellow drum/cymbal velocity
	bytes[12] = 0x00; // red drum/cymbal velocity
	bytes[13] = 0x00; // green drum/cymbal velocity
	bytes[14] = 0x00; // blue drum/cymbal velocity
	bytes[15] = 0x00;
	bytes[16] = 0x00;
	bytes[17] = 0x00;
	bytes[18] = 0x00;
	bytes[19] = 0x02;
	bytes[20] = 0x00;
	bytes[21] = 0x02;
	bytes[22] = 0x00;
	bytes[23] = 0x02;
	bytes[24] = 0x00;
	bytes[25] = 0x02;
	bytes[26] = 0x00;
	return bytes;
}

} // namespace controller

namespace drum
{

// Hold each hit for a period of time. Rock band doesn't pick up a single tick.
std::chrono::milliseconds hit_duration()
{
	return std::chrono::milliseconds(g_cfg.io.midi_drums_pulse_ms);
}

// Scale velocity from midi to what rock band expects.
u8 scale_velocity(u8 value)
{
	return (0xFF - (2 * value));
}

const usz FLAG = controller::FLAG;
const usz INDEX = controller::INDEX;

using FlagByIndex = controller::FlagByIndex;

constexpr FlagByIndex GREEN			= controller::BUTTON_2;
constexpr FlagByIndex RED			= controller::BUTTON_3;
constexpr FlagByIndex YELLOW		= controller::BUTTON_4;
constexpr FlagByIndex BLUE			= controller::BUTTON_1;
constexpr FlagByIndex KICK_PEDAL	= controller::BUTTON_5;
constexpr FlagByIndex HIHAT_PEDAL	= controller::BUTTON_6;

constexpr FlagByIndex IS_DRUM		= controller::BUTTON_11;
constexpr FlagByIndex IS_CYMBAL	    = controller::BUTTON_12;

constexpr FlagByIndex BACK_BUTTON	= controller::BUTTON_3;
constexpr FlagByIndex START_BUTTON  = controller::BUTTON_10;
constexpr FlagByIndex SYSTEM_BUTTON = controller::BUTTON_13;
constexpr FlagByIndex SELECT_BUTTON = controller::BUTTON_9;

KitState start_state()
{
	KitState s{};
	s.expiry = std::chrono::steady_clock::now() + drum::hit_duration();
	s.start = true;
	return s;
}

KitState select_state()
{
	KitState s{};
	s.expiry = std::chrono::steady_clock::now() + drum::hit_duration();
	s.select = true;
	return s;
}

} // namespace drum

namespace midi
{

// Minimum midi velocity required to be considered a real hit.
const u8 MIN_VELOCITY = 10;

// These are from alesis nitro mesh max. ymmv.

const u8 KICK			= 0x24;
const u8 HIHAT_CONTROL	= 0x2c;
const u8 SNARE			= 0x26;
const u8 SNARE_RIM		= 0x28;
const u8 TOM_1			= 0x30;
const u8 TOM_2			= 0x2d;
const u8 TOM_3			= 0x2b;
const u8 HIHAT_UP		= 0x2e; // i.e. w/ pedal up
const u8 HIHAT_DOWN		= 0x2a; // i.e. w/ pedal down
const u8 HIHAT_ALMOST_DOWN = 0x17; // If pedal is not 100% down, this will be sent instead of above.
const u8 CRASH			= 0x31;
const u8 RIDE			= 0x33;

namespace combo
{

const std::chrono::milliseconds MAX_DURATION = 2000ms;

struct ComboDef
{
	std::string name;
	std::vector<u8> notes;
	KitState state;
};

// note: hihat_control 3x in a row without other notes is very unlikely, so it makes for a good combo start.

std::vector<u8> start()
{
	return {HIHAT_CONTROL, HIHAT_CONTROL, HIHAT_CONTROL, SNARE_RIM};
}

std::vector<u8> select()
{
	return {HIHAT_CONTROL, HIHAT_CONTROL, HIHAT_CONTROL, KICK};
}

std::vector<ComboDef> definitions()
{
	return {
		{"start", start(), drum::start_state()},
		{"select", select(), drum::select_state()}
	};
}

}

} // namespace midi

std::string buf_to_string(u8* buf, usz size)
{
	std::string s;
	for (usz i = 0; i < size; ++i)
	{
		u8* elem = (buf + i);
		s.append(fmt::format("%x ", *elem));
	}
	return s;
}

void set_flag(u8* buf, std::string_view name, const controller::FlagByIndex& fbi)
{
	auto i = fbi[drum::INDEX];
	auto flag = fbi[drum::FLAG];
	buf[i] |= flag;
	// rb3_midi_drums_log.success("wrote flag %x at index %d", flag, i);
}

void set_flag_if_any(u8* buf, std::string_view name, const controller::FlagByIndex& fbi, const std::vector<u8> velocities)
{
	bool any = false;
	for (const auto& velocity : velocities)
	{
		if (velocity >= midi::MIN_VELOCITY)
		{
			any = true;
		}
	}
	if (!any)
	{
		return;
	}
	set_flag(buf, name, fbi);
}

}

usb_device_rb3_midi_drums::usb_device_rb3_midi_drums(const std::array<u8, 7>& location, const std::string& device_name)
	: usb_device_emulated(location)
{
	rb3_midi_drums_log.success("using pulse ms: %d", drum::hit_duration().count());

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
		//rb3_midi_drums_log.success("[control_transfer] config 0x21 0x9 length 40");

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

	auto bytes = controller::default_state();
	if (buf_size < bytes.size())
	{
		rb3_midi_drums_log.warning("buffer size < %x, bailing out early (buf_size=0x%x)", bytes.size(), buf_size);
		return;
	}
	memcpy(buf, bytes.data(), bytes.size());

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
		kit_states.push_back(std::move(kit_state));
		if (auto combo_state = combo.take_state())
		{
			kit_states.push_back(std::move(combo_state.value()));
		}
	}

	// Clean expired states.
	auto now = std::chrono::steady_clock::now();
	kit_states.erase(std::remove_if(std::begin(kit_states), std::end(kit_states), [&now](const KitState& kit_state) {
		return now >= kit_state.expiry;
	}), std::end(kit_states));

	// Apply states to buf.
	bool cymbal = false;
	bool drum = false;
	size_t i = 0;
	for (; i < kit_states.size(); ++i)
	{
		const auto& kit_state = kit_states[i];

		// Drums & cymbals use the same inputs, and flip flags to indicate if its a drum or cymbal hit.
		// This causes problems when hitting a drum and cymbal at the exact same time: the game will
		// only register one or the other flag, and all hits will be counted as that type.
		//
		// My solution is to queue up all hits and apply additively to the current state as long as their
		// cymbal/drum flags don't collide. Once they do, buffer states until the current set expires,
		// then continue with the new states. Effectively this staggers drum/cymbal hits by drum::hit_duration()
		// which should be small enough to be imperceptible, but long enough that each will register properly
		// with the game.
		//
		// drum::hit_duration() should be in tens of milliseconds, which even at high BPM is going to be like
		// 1/64th notes. If you're jamming that hard... idk man join a real band.
		if (cymbal && kit_state.is_drum())
		{
			break;
		}
		if (drum && kit_state.is_cymbal())
		{
			break;
		}
		drum = kit_state.is_drum();
		cymbal = kit_state.is_cymbal();
		// Additively write non-mutually-exclusive states into buf.
		write_state(buf, kit_state);
	}

	// Extend expiry on buffered states since they are not active.
	for (; i < kit_states.size(); ++i)
	{
		kit_states[i].expiry = now + drum::hit_duration();
	}
}

KitState usb_device_rb3_midi_drums::parse_midi_message(u8* msg, usz size)
{
	//rb3_midi_drums_log.success("parse_midi_message: %s", buf_to_string(msg, size));

	if (size < 3)
	{
		rb3_midi_drums_log.warning("parse_midi_message: encountered message with size less than 3 bytes");
		return KitState{};
	}

	auto status = msg[0];
	auto id = msg[1];
	auto velocity = msg[2];

	if (status != 0x99)
	{
		// Ignore non-"note on" midi status messages.
		return KitState{};
	}

	if (velocity < midi::MIN_VELOCITY)
	{
		// Must check here so we don't overwrite good values.
		return KitState{};
	}

	KitState kit_state{};
	kit_state.expiry = std::chrono::steady_clock::now() + drum::hit_duration();
	switch (id)
	{
		case midi::KICK:			kit_state.kick_pedal		= velocity; break;
		case midi::HIHAT_CONTROL:	kit_state.hihat_control		= velocity; break;
		case midi::SNARE:			kit_state.snare				= velocity; break;
		case midi::SNARE_RIM:		kit_state.snare_rim			= velocity; break;
		case midi::TOM_1:			kit_state.tom1				= velocity; break;
		case midi::TOM_2:			kit_state.tom2				= velocity; break;
		case midi::TOM_3:			kit_state.tom3				= velocity; break;
		case midi::HIHAT_UP:		kit_state.hihat_up			= velocity; break;
		case midi::HIHAT_DOWN:		kit_state.hihat_down		= velocity; break;
		case midi::HIHAT_ALMOST_DOWN:kit_state.hihat_down		= velocity; break;
		case midi::CRASH:			kit_state.crash				= velocity; break;
		case midi::RIDE:			kit_state.ride				= velocity; break;
		default:
			// Ignored note.
			rb3_midi_drums_log.error("IGNORED NOTE: id = %x or %d", id, id);
			return KitState{};
	}

	combo.add(id);
	return kit_state;
}

void usb_device_rb3_midi_drums::write_state(u8* buf, const KitState& kit_state)
{
	// Interestingly, because cymbals use the same visual track as drums, a hit on that color can only be a drum OR a cymbal.
	// rockband handles this by taking a flag to indicate if the hit is a drum vs cymbal. idk what happens if you do both.
	set_flag_if_any(buf, "red", drum::RED, {kit_state.snare});
	set_flag_if_any(buf, "yellow", drum::YELLOW, {kit_state.tom1, kit_state.hihat_down});
	set_flag_if_any(buf, "blue", drum::BLUE, {kit_state.tom2, kit_state.crash, kit_state.hihat_up}); // Rock band uses blue cymbal for both hihat open and crash.
	set_flag_if_any(buf, "green", drum::GREEN, {kit_state.tom3, kit_state.ride});

	set_flag_if_any(buf, "is_drum", drum::IS_DRUM, {kit_state.snare, kit_state.tom1, kit_state.tom2, kit_state.tom3});
	set_flag_if_any(buf, "is_cymbal", drum::IS_CYMBAL, {kit_state.hihat_up, kit_state.hihat_down, kit_state.crash, kit_state.ride});

	set_flag_if_any(buf, "kick_pedal", drum::KICK_PEDAL, {kit_state.kick_pedal});
	set_flag_if_any(buf, "hihat_pedal", drum::HIHAT_PEDAL, {kit_state.hihat_control});

	buf[11] = drum::scale_velocity(std::max(kit_state.tom1, kit_state.hihat_down));
	buf[12] = drum::scale_velocity(kit_state.snare);
	buf[13] = drum::scale_velocity(std::max(kit_state.tom3, kit_state.ride));
	buf[14] = drum::scale_velocity(std::max({kit_state.tom2, kit_state.crash, kit_state.hihat_up}));

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

bool KitState::is_cymbal() const
{
	return std::max({hihat_up, hihat_down, crash, ride}) >= midi::MIN_VELOCITY;
}

bool KitState::is_drum() const
{
	return std::max({snare, tom1, tom2, tom3}) >= midi::MIN_VELOCITY;
}

void usb_device_rb3_midi_drums::ComboTracker::add(u8 note)
{
	if (midi_notes.size() > 0 && std::chrono::steady_clock::now() >= expiry)
	{
		// Combo expired.
		reset();
	}

	auto i = midi_notes.size();
	auto defs = midi::combo::definitions();
	bool is_in_combo = false;
	for (auto def : defs)
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
		expiry = std::chrono::steady_clock::now() + midi::combo::MAX_DURATION;
	}
}

void usb_device_rb3_midi_drums::ComboTracker::reset()
{
	midi_notes.clear();
}

std::optional<KitState> usb_device_rb3_midi_drums::ComboTracker::take_state()
{
	for (const auto& combo : midi::combo::definitions())
	{
		if (midi_notes == combo.notes)
		{
			rb3_midi_drums_log.success("hit combo: %s", combo.name);
			reset();
			return combo.state;
		}
	}
	return {};
}
