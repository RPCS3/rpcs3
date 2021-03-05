// Buzz! buzzer emulator

#include "stdafx.h"
#include "Buzz.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Input/pad_thread.h"

LOG_CHANNEL(buzz_log);

usb_device_buzz::usb_device_buzz(int first_controller, int last_controller)
{
	this->first_controller = first_controller;
	this->last_controller  = last_controller;

	device        = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{0x0200, 0x00, 0x00, 0x00, 0x08, 0x054c, 0x0002, 0x05a1, 0x03, 0x01, 0x00, 0x01});
	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG, UsbDeviceConfiguration{0x0022, 0x01, 0x01, 0x00, 0x80, 0x32}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE, UsbDeviceInterface{0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_HID, UsbDeviceHID{0x0111, 0x33, 0x01, 0x22, 0x004e}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x81, 0x03, 0x0008, 0x0A}));
}

usb_device_buzz::~usb_device_buzz()
{
}

void usb_device_buzz::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake = true;

	// Control transfers are nearly instant
	switch (bmRequestType)
	{
		case 0x01:
		case 0x21:
		case 0x80:
			buzz_log.error("Unhandled Query Len: 0x%02X", buf_size);
			buzz_log.error("Unhandled Query Type: 0x%02X", (buf_size > 0) ? buf[0] : -1);
			break;
		default:
			usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
			break;
	}
}

void usb_device_buzz::interrupt_transfer(u32 buf_size, u8* buf, u32 /*endpoint*/, UsbTransfer* transfer)
{
	transfer->fake            = true;
	transfer->expected_count  = 5;
	transfer->expected_result = HC_CC_NOERR;
	// Interrupt transfers are slow (6ms, TODO accurate measurement)
	transfer->expected_time = get_timestamp() + 6000;

	memset(buf, 0, buf_size);

	// https://gist.github.com/Lewiscowles1986/eef220dac6f0549e4702393a7b9351f6
	buf[0] = 0x7f;
	buf[1] = 0x7f;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0xf0;

	const auto handler = pad::get_current_handler();
	const auto& pads   = handler->GetPads();

	for (int index = 0; index <= (last_controller - first_controller); index++)
	{
		const auto pad = pads[first_controller + index];

		if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
			continue;

		for (Button& button : pad->m_buttons)
		{
			if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL2)
			{
				switch (button.m_outKeyCode)
				{
					case CELL_PAD_CTRL_R1:
						if (button.m_pressed)
							buf[2 + (0 + 5 * index) / 8] |= 1 << ((0 + 5 * index) % 8); // Red
						break;
					case CELL_PAD_CTRL_TRIANGLE:
						if (button.m_pressed)
							buf[2 + (4 + 5 * index) / 8] |= 1 << ((4 + 5 * index) % 8); // Blue
						break;
					case CELL_PAD_CTRL_SQUARE:
						if (button.m_pressed)
							buf[2 + (3 + 5 * index) / 8] |= 1 << ((3 + 5 * index) % 8); // Orange
						break;
					case CELL_PAD_CTRL_CIRCLE:
						if (button.m_pressed)
							buf[2 + (2 + 5 * index) / 8] |= 1 << ((2 + 5 * index) % 8); // Green
						break;
					case CELL_PAD_CTRL_CROSS:
						if (button.m_pressed)
							buf[2 + (1 + 5 * index) / 8] |= 1 << ((1 + 5 * index) % 8); // Yellow
						break;
					default:
						break;
				}
			}
		}
	}
}
