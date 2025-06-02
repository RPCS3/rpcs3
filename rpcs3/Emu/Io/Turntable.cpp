// DJ Hero Turntable controller emulator

#include "stdafx.h"
#include "Turntable.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/Io/turntable_config.h"
#include "Input/pad_thread.h"
#include "Emu/system_config.h"

LOG_CHANNEL(turntable_log, "TURN");

template <>
void fmt_class_string<turntable_btn>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](turntable_btn value)
	{
		switch (value)
		{
		case turntable_btn::blue: return "Blue";
		case turntable_btn::green: return "Green";
		case turntable_btn::red: return "Red";
		case turntable_btn::dpad_up: return "D-Pad Up";
		case turntable_btn::dpad_down: return "D-Pad Down";
		case turntable_btn::dpad_left: return "D-Pad Left";
		case turntable_btn::dpad_right: return "D-Pad Right";
		case turntable_btn::start: return "Start";
		case turntable_btn::select: return "Select";
		case turntable_btn::square: return "Square";
		case turntable_btn::circle: return "Circle";
		case turntable_btn::cross: return "Cross";
		case turntable_btn::triangle: return "Triangle";
		case turntable_btn::right_turntable: return "Right Turntable";
		case turntable_btn::crossfader: return "Crossfader";
		case turntable_btn::effects_dial: return "Effects Dial";
		case turntable_btn::count: return "Count";
		}

		return unknown;
	});
}

usb_device_turntable::usb_device_turntable(u32 controller_index, const std::array<u8, 7>& location)
	: usb_device_emulated(location), m_controller_index(controller_index)
{
	device        = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{0x0100, 0x00, 0x00, 0x00, 0x40, 0x12BA, 0x0140, 0x0005, 0x01, 0x02, 0x00, 0x01});
	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG, UsbDeviceConfiguration{0x0029, 0x01, 0x01, 0x00, 0x80, 0x19}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE, UsbDeviceInterface{0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_HID, UsbDeviceHID{0x0110, 0x00, 0x01, 0x22, 0x0089}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x81, 0x03, 0x0040, 0x0a}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x02, 0x03, 0x0040, 0x0a}));
}

usb_device_turntable::~usb_device_turntable()
{
}

std::shared_ptr<usb_device> usb_device_turntable::make_instance(u32 controller_index, const std::array<u8, 7>& location)
{
	return std::make_shared<usb_device_turntable>(controller_index, location);
}

u16 usb_device_turntable::get_num_emu_devices()
{
	return static_cast<u16>(g_cfg.io.turntable.get());
}

void usb_device_turntable::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake = true;

	// Control transfers are nearly instant
	switch (bmRequestType)
	{
		case 0x21:
			switch (bRequest)
			{
			case 0x09:
				// Do nothing here - not sure what it should do.
				break;
			default:
				turntable_log.error("Unhandled Query: buf_size=0x%02X, Type=0x%02X, bRequest=0x%02X, bmRequestType=0x%02X", buf_size, (buf_size > 0) ? buf[0] : -1, bRequest, bmRequestType);
				break;
			}
			break;
		default:
			usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
			break;
	}
}

void usb_device_turntable::interrupt_transfer(u32 buf_size, u8* buf, u32 /*endpoint*/, UsbTransfer* transfer)
{
	ensure(buf_size >= 27);

	transfer->fake            = true;
	transfer->expected_count  = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	// Turntable runs at 100hz --> 10ms
	// But make the emulated table go at 1ms for better input behavior
	transfer->expected_time = get_timestamp() + 1'000;

	memset(buf, 0, buf_size);

	buf[0] = 0x00; // Face Buttons
	// FACE BUTTON HEXMASK:
	// 0x01 = Square
	// 0x02 = Cross
	// 0x04 = Circle
	// 0x08 = Triangle / Euphoria

	buf[1] = 0x00; // Start/Select Buttons
	// START/SELECT HEXMASK:
	// 0x01 = Select
	// 0x02 = Start
	// 0x10 = PS Button

	buf[2] = 0x0F; // D-Pad
	// DPAD VALUES:
	// 0x00 = Up
	// 0x01 = Up-Right
	// 0x02 = Right
	// 0x03 = Right-Down
	// 0x04 = Down
	// 0x05 = Down-Left
	// 0x06 = Left
	// 0x07 = Up-Left
	// 0x0F = None

	buf[3] = 0x80; // Unknown, always 0x80
	buf[4] = 0x80; // Unknown, always 0x80

	buf[5] = 0x80; // Left Turntable
	buf[6] = 0x80; // Right Turntable

	// The following bytes are NOTed (set to 0xFF) when active.
	// If multiple buttons are pressed for one byte, the byte is NOTed twice (reset to 0x00).
	buf[7]  = 0x00; // Square Button / D-Pad Right
	buf[8]  = 0x00; // D-Pad Left
	buf[9]  = 0x00; // Cross Button / D-Pad Up
	buf[10] = 0x00; // D-Pad Down
	buf[11] = 0x00; // Triangle / Euphoria Button
	buf[12] = 0x00; // Circle Button

	buf[19] = 0x00; // Effects Dial, lower 8 bits
	buf[20] = 0x02; // Effects Dial, upper 2 bits

	buf[21] = 0x00; // Crossfader, lower 8 bits
	buf[22] = 0x02; // Crossfader, upper 2 bits

	buf[23] = 0x00; // Platter Buttons
	// PLATTER BUTTON VALUES:
	// 0x01 = Right Platter, Green
	// 0x02 = Right Platter, Red
	// 0x04 = Right Platter, Blue
	// 0x10 = Left Platter, Green
	// 0x20 = Left Platter, Red
	// 0x40 = Left Platter, Blue

	buf[24] = 0x02; // Unknown, always 0x02
	buf[26] = 0x02; // Unknown, always 0x02

	// All other bufs are always 0x00

	std::lock_guard lock(pad::g_pad_mutex);
	const auto handler = pad::get_pad_thread();
	const auto& pads   = handler->GetPads();
	const auto& pad    = ::at32(pads, m_controller_index);

	if (!pad->is_connected() || pad->is_copilot())
		return;

	const auto& cfg = ::at32(g_cfg_turntable.players, m_controller_index);
	cfg->handle_input(pad, true, [&buf](turntable_btn btn, pad_button /*pad_btn*/, u16 value, bool pressed, bool& /*abort*/)
		{
			if (!pressed)
				return;

			switch (btn)
			{
			case turntable_btn::blue:
				buf[0] |= 0x01;   // Square Button
				buf[7] = ~buf[7]; // Square Button
				buf[23] |= 0x04;  // Right Platter Blue
				break;
			case turntable_btn::green:
				buf[0] |= 0x02;   // Cross Button
				buf[9] = ~buf[9]; // Cross Button
				buf[23] |= 0x01;  // Right Platter Green
				break;
			case turntable_btn::red:
				buf[0] |= 0x04;     // Circle Button
				buf[12] = ~buf[12]; // Circle Button
				buf[23] |= 0x02;    // Right Platter Red
				break;
			case turntable_btn::triangle:
				buf[0] |= 0x08;     // Triangle Button / Euphoria
				buf[11] = ~buf[11]; // Triangle Button / Euphoria
				break;
			case turntable_btn::cross:
				buf[0] |= 0x02;   // Cross Button Only
				buf[9] = ~buf[9]; // Cross Button Only
				break;
			case turntable_btn::circle:
				buf[0] |= 0x04;     // Circle Button Only
				buf[12] = ~buf[12]; // Circle Button Only
				break;
			case turntable_btn::square:
				buf[0] |= 0x01;   // Square Button Only
				buf[7] = ~buf[7]; // Square Button Only
				break;
			case turntable_btn::dpad_down:
				if (buf[2] == 0x02) // Right D-Pad
				{
					buf[2] = 0x03; // Right-Down D-Pad
				}
				else if (buf[2] == 0x06) // Left D-Pad
				{
					buf[2] = 0x05; // Left-Down D-Pad
				}
				else
				{
					buf[2] = 0x04; // Down D-Pad
				}
				buf[10] = ~buf[10]; // Down D-Pad;
				break;
			case turntable_btn::dpad_up:
				if (buf[2] == 0x02) // Right D-Pad
				{
					buf[2] = 0x01; // Right-Up D-Pad
				}
				else if (buf[2] == 0x06) // Left D-Pad
				{
					buf[2] = 0x07; // Left-Up D-Pad
				}
				else
				{
					buf[2] = 0x00; // Up D-Pad
				}
				buf[9] = ~buf[9]; // Up D-Pad;
				break;
			case turntable_btn::dpad_left:
				if (buf[2] == 0x00) // Up D-Pad
				{
					buf[2] = 0x07; // Left-Up D-Pad
				}
				else if (buf[2] == 0x04) // Down D-Pad
				{
					buf[2] = 0x05; // Left-Down D-Pad
				}
				else
				{
					buf[2] = 0x06; // Left D-Pad
				}
				buf[8] = ~buf[8]; // Left D-Pad;
				break;
			case turntable_btn::dpad_right:
				if (buf[2] == 0x00) // Up D-Pad
				{
					buf[2] = 0x01; // Right-Up D-Pad
				}
				else if (buf[2] == 0x04) // Down D-Pad
				{
					buf[2] = 0x03; // Right-Down D-Pad
				}
				else
				{
					buf[2] = 0x02; // Right D-Pad
				}
				buf[7] = ~buf[7]; // Right D-Pad
				break;
			case turntable_btn::start:
				buf[1] |= 0x02; // Start
				break;
			case turntable_btn::select:
				buf[1] |= 0x01; // Select
				break;
			case turntable_btn::right_turntable:
				// DJ Hero does not register input if the turntable is 0, so force it to 1.
				buf[6] = std::max(1, 255 - value); // Right Turntable
				// DJ Hero requires turntables to be centered at 128.
				// If this axis ends up centered at 127, force it to 128.
				if (buf[6] == 127)
				{
					buf[6] = 128;
				}
				break;
			case turntable_btn::crossfader:
				buf[21] = ((255 - value) & 0x3F) << 2; // Crossfader, lower 6 bits
				buf[22] = ((255 - value) & 0xC0) >> 6; // Crossfader, upper 2 bits
				break;
			case turntable_btn::effects_dial:
				buf[19] = (value & 0x3F) << 2; // Effects Dial, lower 6 bits
				buf[20] = (value & 0xC0) >> 6; // Effects Dial, upper 2 bits
				break;
			case turntable_btn::count:
				break;
			}
		});
}
