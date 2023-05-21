// Buzz! buzzer emulator

#include "stdafx.h"
#include "Buzz.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/Io/buzz_config.h"
#include "Input/pad_thread.h"

LOG_CHANNEL(buzz_log, "BUZZ");

usb_device_buzz::usb_device_buzz(u32 first_controller, u32 last_controller, const std::array<u8, 7>& location)
	: usb_device_emulated(location)
	, m_first_controller(first_controller)
	, m_last_controller(last_controller)
{
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
		buzz_log.error("Unhandled Query: buf_size=0x%02X, Type=0x%02X, bmRequestType=0x%02X", buf_size, (buf_size > 0) ? buf[0] : -1, bmRequestType);
		break;
	default:
		usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
		break;
	}
}

extern bool is_input_allowed();

void usb_device_buzz::interrupt_transfer(u32 buf_size, u8* buf, u32 /*endpoint*/, UsbTransfer* transfer)
{
	const u8 max_index = 2 + (4 + 5 * m_last_controller) / 8;
	ensure(buf_size > max_index);

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

	if (!is_input_allowed())
	{
		return;
	}

	std::lock_guard lock(pad::g_pad_mutex);
	const auto handler = pad::get_current_handler();
	const auto& pads   = handler->GetPads();
	ensure(pads.size() > m_last_controller);
	ensure(g_cfg_buzz.players.size() > m_last_controller);

	for (u32 i = m_first_controller, index = 0; i <= m_last_controller; i++, index++)
	{
		const auto& pad = pads[i];
		const cfg_buzzer* cfg = g_cfg_buzz.players[i];

		if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
		{
			continue;
		}

		for (const Button& button : pad->m_buttons)
		{
			if (!button.m_pressed)
			{
				continue;
			}

			if (const auto btn = cfg->find_button(button.m_offset, button.m_outKeyCode))
			{
				switch (btn.value())
				{
				case buzz_btn::red:
					buf[2 + (0 + 5 * index) / 8] |= 1 << ((0 + 5 * index) % 8); // Red
					break;
				case buzz_btn::yellow:
					buf[2 + (1 + 5 * index) / 8] |= 1 << ((1 + 5 * index) % 8); // Yellow
					break;
				case buzz_btn::green:
					buf[2 + (2 + 5 * index) / 8] |= 1 << ((2 + 5 * index) % 8); // Green
					break;
				case buzz_btn::orange:
					buf[2 + (3 + 5 * index) / 8] |= 1 << ((3 + 5 * index) % 8); // Orange
					break;
				case buzz_btn::blue:
					buf[2 + (4 + 5 * index) / 8] |= 1 << ((4 + 5 * index) % 8); // Blue
					break;
				case buzz_btn::count:
					break;
				}
			}
		}
	}
}
