// Rock Band 3 MIDI Pro Adapter Emulator (Keyboard Mode)

#include "stdafx.h"
#include "RB3MidiKeyboard.h"
#include "Emu/Cell/lv2/sys_usbd.h"

LOG_CHANNEL(rb3_midi_keyboard_log);

usb_device_rb3_midi_keyboard::usb_device_rb3_midi_keyboard(const std::array<u8, 7>& location, const std::string& device_name)
	: usb_device_emulated(location)
{
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{0x0200, 0x00, 0x00, 0x00, 64, 0x12ba, 0x2338, 0x01, 0x01, 0x02, 0x00, 0x01});
	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG, UsbDeviceConfiguration{41, 1, 1, 0, 0x80, 32}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE, UsbDeviceInterface{0, 0, 2, 3, 0, 0, 0}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_HID, UsbDeviceHID{0x0111, 0x00, 0x01, 0x22, 137}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x81, 0x03, 0x0040, 10}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x02, 0x03, 0x0040, 10}));

	usb_device_emulated::add_string("Licensed by Sony Computer Entertainment America");
	usb_device_emulated::add_string("Harmonix RB3 MIDI Keyboard Interface for PlayStation®3");

	// connect to midi device
	midi_in = rtmidi_in_create_default();
	ensure(midi_in);

	if (!midi_in->ok)
	{
		rb3_midi_keyboard_log.error("Could not get MIDI in ptr: %s", midi_in->msg);
		return;
	}

	const RtMidiApi api = rtmidi_in_get_current_api(midi_in);

	if (!midi_in->ok)
	{
		rb3_midi_keyboard_log.error("Could not get MIDI api: %s", midi_in->msg);
		return;
	}

	if (const char* api_name = rtmidi_api_name(api))
	{
		rb3_midi_keyboard_log.notice("Using %s api", api_name);
	}
	else
	{
		rb3_midi_keyboard_log.warning("Could not get MIDI api name");
	}

	const u32 port_count = rtmidi_get_port_count(midi_in);

	if (!midi_in->ok || port_count == umax)
	{
		rb3_midi_keyboard_log.error("Could not get MIDI port count: %s", midi_in->msg);
		return;
	}

	for (u32 port_number = 0; port_number < port_count; port_number++)
	{
		char buf[128]{};
		s32 size = sizeof(buf);
		if (rtmidi_get_port_name(midi_in, port_number, buf, &size) == -1 || !midi_in->ok)
		{
			rb3_midi_keyboard_log.error("Error getting port name for port %d: %s", port_number, midi_in->msg);
			return;
		}

		rb3_midi_keyboard_log.notice("Found device with name: %s", buf);

		if (device_name == buf)
		{
			rtmidi_open_port(midi_in, port_number, "RPCS3 MIDI Keyboard Input");

			if (!midi_in->ok)
			{
				rb3_midi_keyboard_log.error("Could not open port %d for device '%s': %s", port_number, device_name, midi_in->msg);
				return;
			}

			rb3_midi_keyboard_log.success("Connected to device: %s", device_name);
			return;
		}
	}

	rb3_midi_keyboard_log.error("Could not find device with name: %s", device_name);
}

usb_device_rb3_midi_keyboard::~usb_device_rb3_midi_keyboard()
{
	rtmidi_in_free(midi_in);
}

static const std::array<u8, 40> disabled_response = {
	0xe9, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0d, 0x01,
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

void usb_device_rb3_midi_keyboard::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake = true;

	// configuration packets sent by rock band 3
	// we only really need to check 1 byte here to figure out if the game
	// wants to enable midi data or disable it
	if (bmRequestType == 0x21 && bRequest == 0x9 && wLength == 40)
	{
		if (buf_size < 3)
		{
			rb3_midi_keyboard_log.warning("buffer size < 3, bailing out early (buf_size=0x%x)", buf_size);
			return;
		}

		switch (buf[2])
		{
		case 0x89:
			rb3_midi_keyboard_log.notice("MIDI data enabled.");
			buttons_enabled = true;
			response_pos = 0;
			break;
		case 0x81:
			rb3_midi_keyboard_log.notice("MIDI data disabled.");
			buttons_enabled = false;
			response_pos = 0;
			break;
		default:
			rb3_midi_keyboard_log.warning("Unhandled SET_REPORT request: 0x%02X");
			break;
		}
	}
	// the game expects some sort of response to the configuration packet
	else if (bmRequestType == 0xa1 && bRequest == 0x1)
	{
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
	else if (bmRequestType == 0x21 && bRequest == 0x9 && wLength == 8)
	{
		// the game uses this request to do things like set the LEDs
		// we don't have any LEDs, so do nothing
	}
	else
	{
		usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
	}
}

void usb_device_rb3_midi_keyboard::interrupt_transfer(u32 buf_size, u8* buf, u32 /*endpoint*/, UsbTransfer* transfer)
{
	transfer->fake = true;
	transfer->expected_count = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	// the real device takes 8ms to send a response, but there is
	// no reason we can't make it faster
	transfer->expected_time = get_timestamp() + 1'000;

	// default input state
	static const std::array<u8, 27> bytes = {
		0x00, 0x00, 0x08, 0x80, 0x80, 0x80, 0x80, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00,
		0x02, 0x00, 0x02};

	if (buf_size < bytes.size())
	{
		rb3_midi_keyboard_log.warning("buffer size < %x, bailing out early (buf_size=0x%x)", bytes.size(), buf_size);
		return;
	}

	memcpy(buf, bytes.data(), bytes.size());

	while (true)
	{
		u8 midi_msg[32];
		usz size = sizeof(midi_msg);

		// this returns a double as some sort of delta time, with -1.0
		// being used to signal an error
		if (rtmidi_in_get_message(midi_in, midi_msg, &size) == -1.0)
		{
			rb3_midi_keyboard_log.error("Error getting MIDI message: %s", midi_in->msg);
			return;
		}

		if (size == 0)
		{
			break;
		}

		parse_midi_message(midi_msg, size);
	}

	write_state(buf);
}

void usb_device_rb3_midi_keyboard::parse_midi_message(u8* msg, usz size)
{
	// this is not emulated correctly but the game doesn't seem to care
	button_state.count++;

	// handle note on/off messages
	if (size == 3 && (msg[0] == 0x80 || msg[0] == 0x90))
	{
		const bool note_on = (0x10 & msg[0]) == 0x10 && msg[2] != 0;

		// handle navigation buttons
		switch (msg[1])
		{
		case 44: // G#2
			button_state.cross = note_on;
			break;
		case 42: // F#2
			button_state.circle = note_on;
			break;
		case 39: // D#2
			button_state.square = note_on;
			break;
		case 37: // C#2
			button_state.triangle = note_on;
			break;
		case 46: // A#2
			button_state.start = note_on;
			break;
		case 36: // C2
			button_state.select = note_on;
			break;
		case 45: // A2
			button_state.overdrive = note_on;
			break;
		case 41: // F2
			button_state.dpad_up = note_on;
			break;
		case 43: // G2
			button_state.dpad_down = note_on;
			break;
		case 38: // D2
			button_state.dpad_left = note_on;
			break;
		case 40: // E2
			button_state.dpad_right = note_on;
			break;
		default:
			break;
		}

		// handle keyboard keys
		if (msg[1] >= 48 && msg[1] <= (48 + button_state.keys.size()))
		{
			const u32 key = msg[1] - 48;
			button_state.keys[key] = note_on;
			button_state.velocities[key] = msg[2];
		}
	}

	// control channel for overdrive
	else if (size == 3 && msg[0] == 0xB0)
	{
		switch (msg[1])
		{
		case 0x1:
		case 0x40:
			button_state.overdrive = msg[2] > 40;
			break;
		default:
			break;
		}
	}

	// pitch wheel
	else if (size == 3 && msg[0] == 0xE0)
	{
		const u16 msb = msg[2];
		const u16 lsb = msg[1];
		button_state.pitch_wheel = (msb << 7) | lsb;
	}
}

void usb_device_rb3_midi_keyboard::write_state(u8* buf)
{
	// buttons
	buf[0] |= 0b0000'0010 * button_state.cross;
	buf[0] |= 0b0000'0100 * button_state.circle;
	buf[0] |= 0b0000'0001 * button_state.square;
	buf[0] |= 0b0000'1000 * button_state.triangle;
	buf[1] |= 0b0000'0010 * button_state.start;
	buf[1] |= 0b0000'0001 * button_state.select;

	// dpad
	if (button_state.dpad_up)
	{
		buf[2] = 0;
	}
	else if (button_state.dpad_down)
	{
		buf[2] = 4;
	}
	else if (button_state.dpad_left)
	{
		buf[2] = 6;
	}
	else if (button_state.dpad_right)
	{
		buf[2] = 2;
	}

	// build key bitfield and write velocities
	u32 key_mask = 0;
	u8 vel_idx = 0;

	for (usz i = 0; i < button_state.keys.size(); i++)
	{
		key_mask <<= 1;
		key_mask |= 0x1 * button_state.keys[i];

		// the keyboard can only report 5 velocities from left to right
		if (button_state.keys[i] && vel_idx < 5)
		{
			buf[8 + vel_idx++] = button_state.velocities[i];
		}
	}

	// write keys
	buf[5] = (key_mask >> 17) & 0xff;
	buf[6] = (key_mask >> 9) & 0xff;
	buf[7] = (key_mask >> 1) & 0xff;
	buf[8] |= 0b1000'0000 * (key_mask & 0x1);

	// overdrive
	buf[13] |= 0b1000'0000 * button_state.overdrive;

	// pitch wheel
	const u8 wheel_pos = std::abs((button_state.pitch_wheel >> 6) - 0x80);
	if (wheel_pos >= 5)
	{
		buf[15] = std::min<u8>(std::max<u8>(0x5, wheel_pos), 0x75);
	}
}
