// DJ Hero Turntable controller emulator

#include "stdafx.h"
#include "Turntable.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Input/pad_thread.h"

LOG_CHANNEL(turntable_log);

usb_device_turntable::usb_device_turntable()
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
					turntable_log.error("Unhandled Query Type: 0x%02X", buf[0]);
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
	transfer->fake            = true;
	transfer->expected_count  = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	// Interrupt transfers are slow
	// Turntable runs at 100hz --> 10ms
	transfer->expected_time = get_timestamp() + 10000;

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

	const auto handler = pad::get_current_handler();
	const auto& pad    = handler->GetPads()[6];

	if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
		return;

	for (const Button& button : pad->m_buttons)
	{
		if (button.m_pressed)
		{
			if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL2)
			{
				switch (button.m_outKeyCode)
				{
					case CELL_PAD_CTRL_SQUARE:
						buf[0] |= 0x01;   // Square Button
						buf[7] = ~buf[7]; // Square Button
						buf[23] |= 0x04;  // Right Platter Blue
						break;
					case CELL_PAD_CTRL_CROSS:
						buf[0] |= 0x02;   // Cross Button
						buf[9] = ~buf[9]; // Cross Button
						buf[23] |= 0x01;  // Right Platter Green
						break;
					case CELL_PAD_CTRL_CIRCLE:
						buf[0] |= 0x04;     // Circle Button
						buf[12] = ~buf[12]; // Circle Button
						buf[23] |= 0x02;    // Right Platter Red
						break;
					case CELL_PAD_CTRL_TRIANGLE:
						buf[0] |= 0x08;     // Triangle Button / Euphoria
						buf[11] = ~buf[11]; // Triangle Button / Euphoria
						break;
					case CELL_PAD_CTRL_R1:
						buf[0] |= 0x02;   // Cross Button Only
						buf[9] = ~buf[9]; // Cross Button Only
						break;
					case CELL_PAD_CTRL_L1:
						buf[0] |= 0x04;     // Circle Button Only
						buf[12] = ~buf[12]; // Circle Button Only
						break;
					case CELL_PAD_CTRL_R2:
						buf[0] |= 0x01;   // Square Button Only
						buf[7] = ~buf[7]; // Square Button Only
						break;
					default:
						break;
				}
			}
			else if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL1)
			{
				switch (button.m_outKeyCode)
				{
					case CELL_PAD_CTRL_DOWN:
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
					case CELL_PAD_CTRL_UP:
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
					case CELL_PAD_CTRL_LEFT:
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
					case CELL_PAD_CTRL_RIGHT:
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
					case CELL_PAD_CTRL_START:
						buf[1] |= 0x02; // Start
						break;
					case CELL_PAD_CTRL_SELECT:
						buf[1] |= 0x01; // Select
						break;
					default:
						break;
				}
			}
		}
	}
	for (const AnalogStick& stick : pad->m_sticks)
	{
		switch (stick.m_offset)
		{
			case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y:
				buf[6] = ~(stick.m_value) + 0x01; // Right Turntable
				// Some pad handlers like MMJoystick are centered at 0x81 instead of 0x80
				// which leaves the turntable stuck at 0x7F instead of 0x80, causing auto-scrolling menus
				// so force 0x7F to 0x80.
				if (buf[6] == 0x7F)
					buf[6] = 0x80;
				break;
			case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y:
				buf[21] = (stick.m_value & 0x3F) << 2; // Crossfader, lower 6 bits
				buf[22] = (stick.m_value & 0xC0) >> 6; // Crossfader, upper 2 bits
				break;
			case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X:
				buf[19] = (stick.m_value & 0x3F) << 2; // Effects Dial, lower 6 bits
				buf[20] = (stick.m_value & 0xC0) >> 6; // Effects Dial, upper 2 bits
				break;
			default:
				break;
		}
	}
}
