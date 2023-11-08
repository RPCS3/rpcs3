// Rock Band 3 MIDI Pro Adapter Emulator (Guitar Mode)

#include "stdafx.h"
#include "RB3MidiGuitar.h"
#include "Emu/Cell/lv2/sys_usbd.h"

LOG_CHANNEL(rb3_midi_guitar_log);

usb_device_rb3_midi_guitar::usb_device_rb3_midi_guitar(const std::array<u8, 7>& location, const std::string& device_name, bool twentytwo_fret)
	: usb_device_emulated(location)
{
	// For the 22-fret guitar (Fender Squier), the only thing that's different
	// is the device ID reported by the MIDI Pro Adapter.
	//
	// Everything else is *exactly* the same as the 17-fret guitar (Fender Mustang).
	if (twentytwo_fret)
	{
		device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{0x0200, 0x00, 0x00, 0x00, 64, 0x12ba, 0x2538, 0x01, 0x01, 0x02, 0x00, 0x01});
	}
	else
	{
		device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{0x0200, 0x00, 0x00, 0x00, 64, 0x12ba, 0x2438, 0x01, 0x01, 0x02, 0x00, 0x01});
	}

	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG, UsbDeviceConfiguration{41, 1, 1, 0, 0x80, 32}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE, UsbDeviceInterface{0, 0, 2, 3, 0, 0, 0}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_HID, UsbDeviceHID{0x0111, 0x00, 0x01, 0x22, 137}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x81, 0x03, 0x0040, 10}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x02, 0x03, 0x0040, 10}));

	usb_device_emulated::add_string("Licensed by Sony Computer Entertainment America");
	usb_device_emulated::add_string("Harmonix RB3 MIDI Guitar Interface for PlayStation®3");

	// connect to midi device
	midi_in = rtmidi_in_create_default();
	ensure(midi_in);

	if (!midi_in->ok)
	{
		rb3_midi_guitar_log.error("Could not get MIDI in ptr: %s", midi_in->msg);
		return;
	}

	const RtMidiApi api = rtmidi_in_get_current_api(midi_in);

	if (!midi_in->ok)
	{
		rb3_midi_guitar_log.error("Could not get MIDI api: %s", midi_in->msg);
		return;
	}

	if (const char* api_name = rtmidi_api_name(api))
	{
		rb3_midi_guitar_log.notice("Using %s api", api_name);
	}
	else
	{
		rb3_midi_guitar_log.warning("Could not get MIDI api name");
	}

	rtmidi_in_ignore_types(midi_in, false, true, true);

	const u32 port_count = rtmidi_get_port_count(midi_in);

	if (!midi_in->ok || port_count == umax)
	{
		rb3_midi_guitar_log.error("Could not get MIDI port count: %s", midi_in->msg);
		return;
	}

	for (u32 port_number = 0; port_number < port_count; port_number++)
	{
		char buf[128]{};
		s32 size = sizeof(buf);
		if (rtmidi_get_port_name(midi_in, port_number, buf, &size) == -1 || !midi_in->ok)
		{
			rb3_midi_guitar_log.error("Error getting port name for port %d: %s", port_number, midi_in->msg);
			return;
		}

		rb3_midi_guitar_log.notice("Found device with name: %s", buf);

		if (device_name == buf)
		{
			rtmidi_open_port(midi_in, port_number, "RPCS3 MIDI Guitar Input");

			if (!midi_in->ok)
			{
				rb3_midi_guitar_log.error("Could not open port %d for device '%s': %s", port_number, device_name, midi_in->msg);
				return;
			}

			rb3_midi_guitar_log.success("Connected to device: %s", device_name);
			return;
		}
	}

	rb3_midi_guitar_log.error("Could not find device with name: %s", device_name);
}

usb_device_rb3_midi_guitar::~usb_device_rb3_midi_guitar()
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

void usb_device_rb3_midi_guitar::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake = true;

	// configuration packets sent by rock band 3
	// we only really need to check 1 byte here to figure out if the game
	// wants to enable midi data or disable it
	if (bmRequestType == 0x21 && bRequest == 0x9 && wLength == 40)
	{
		if (buf_size < 3)
		{
			rb3_midi_guitar_log.warning("buffer size < 3, bailing out early (buf_size=0x%x)", buf_size);
			return;
		}

		switch (buf[2])
		{
		case 0x89:
			rb3_midi_guitar_log.notice("MIDI data enabled.");
			buttons_enabled = true;
			response_pos = 0;
			break;
		case 0x81:
			rb3_midi_guitar_log.notice("MIDI data disabled.");
			buttons_enabled = false;
			response_pos = 0;
			break;
		default:
			rb3_midi_guitar_log.warning("Unhandled SET_REPORT request: 0x%02X");
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

void usb_device_rb3_midi_guitar::interrupt_transfer(u32 buf_size, u8* buf, u32 /*endpoint*/, UsbTransfer* transfer)
{
	transfer->fake = true;
	transfer->expected_count = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	// the real device takes 8ms to send a response, but there is
	// no reason we can't make it faster
	transfer->expected_time = get_timestamp() + 1'000;


	// default input state
	const std::array<u8, 27> bytes = {
		0x00, 0x00, 0x08, 0x80, 0x80, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
		0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00};

	if (buf_size < bytes.size())
	{
		rb3_midi_guitar_log.warning("buffer size < %x, bailing out early (buf_size=0x%x)", bytes.size(), buf_size);
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
			rb3_midi_guitar_log.error("Error getting MIDI message: %s", midi_in->msg);
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

void usb_device_rb3_midi_guitar::parse_midi_message(u8* msg, usz size)
{
	// this is not emulated correctly but the game doesn't seem to care
	button_state.count++;

	// read frets
	if (size == 8 && msg[0] == 0xF0 && msg[4] == 0x01)
	{
		switch (msg[5])
		{
		case 1:
			button_state.frets[0] = msg[6] - 0x40;
			break;
		case 2:
			button_state.frets[1] = msg[6] - 0x3B;
			break;
		case 3:
			button_state.frets[2] = msg[6] - 0x37;
			break;
		case 4:
			button_state.frets[3] = msg[6] - 0x32;
			break;
		case 5:
			button_state.frets[4] = msg[6] - 0x2D;
			break;
		case 6:
			button_state.frets[5] = msg[6] - 0x28;
			break;
		default:
			rb3_midi_guitar_log.warning("Invalid string for fret event: %d", msg[5]);
			break;
		}
	}

	// read strings
	if (size == 8 && msg[0] == 0xF0 && msg[4] == 0x05)
	{
		button_state.string_velocities[msg[5] - 1] = msg[6];
	}

	// read buttons
	if (size == 10 && msg[0] == 0xF0 && msg[4] == 0x08)
	{
		button_state.dpad = msg[7] & 0x0f;

		button_state.square = (msg[5] & 0b0000'0001) == 0b0000'0001;
		button_state.cross = (msg[5] & 0b0000'0010) == 0b0000'0010;
		button_state.circle = (msg[5] & 0b0000'0100) == 0b0000'0100;
		button_state.triangle = (msg[5] & 0b0000'1000) == 0b0000'1000;

		button_state.select = (msg[6] & 0b0000'0001) == 0b0000'0001;
		button_state.start = (msg[6] & 0b0000'0010) == 0b0000'0010;
		button_state.tilt_sensor = (msg[7] & 0b0100'0000) == 0b0100'0000;
	}

	// sustain pedal
	if (size == 3 && msg[0] == 0xB0 && msg[1] == 0x40)
	{
		button_state.sustain_pedal = msg[2] >= 40;
	}
}

void usb_device_rb3_midi_guitar::write_state(u8* buf)
{
	// encode frets
	buf[8] |= (button_state.frets[0] & 0b11111) << 2;
	buf[8] |= (button_state.frets[1] & 0b11000) >> 3;
	buf[7] |= (button_state.frets[1] & 0b00111) << 5;
	buf[7] |= (button_state.frets[2] & 0b11111) >> 0;
	buf[6] |= (button_state.frets[3] & 0b11111) << 2;
	buf[6] |= (button_state.frets[4] & 0b11000) >> 3;
	buf[5] |= (button_state.frets[4] & 0b00111) << 5;
	buf[5] |= (button_state.frets[5] & 0b11111) >> 0;

	// encode strings
	buf[14] = button_state.string_velocities[0];
	buf[13] = button_state.string_velocities[1];
	buf[12] = button_state.string_velocities[2];
	buf[11] = button_state.string_velocities[3];
	buf[10] = button_state.string_velocities[4];
	buf[9] = button_state.string_velocities[5];

	// encode frets for playing 5 fret on the pro guitar
	// this actually isn't done by the real MPA, but Rock Band 3 allows this
	// so there's no harm in supporting it.
	for (u8 i : button_state.frets)
	{
		switch (i)
		{
			case 1:
			case 6:
			case 13:
				buf[9] |= 0b1000'0000;
				break;
			case 2:
			case 7:
			case 14:
				buf[10] |= 0b1000'0000;
				break;
			case 3:
			case 8:
			case 15:
				buf[11] |= 0b1000'0000;
				break;
			case 4:
			case 9:
			case 16:
				buf[12] |= 0b1000'0000;
				break;
			case 5:
			case 10:
			case 17:
				buf[13] |= 0b1000'0000;
				break;
			default:
				break;
		}

		// enable the solo bit for frets >= 13
		if (i >= 13)
		{
				buf[8] |= 0b1000'0000;
		}
	}

	// encode tilt sensor/sustain_pedal
	if (button_state.tilt_sensor || button_state.sustain_pedal)
	{
		buf[15] = 0x7f;
		buf[16] = 0x7f;
		buf[17] = 0x7f;
	}

	buf[1] |= 0b0000'0001 * button_state.select;
	buf[1] |= 0b0000'0010 * button_state.start;

	buf[0] |= 0b0000'0010 * button_state.cross;
	buf[0] |= 0b0000'0100 * button_state.circle;
	buf[0] |= 0b0000'1000 * button_state.triangle;
	buf[0] |= 0b0000'0001 * button_state.square;

	buf[2] = button_state.dpad;
}
