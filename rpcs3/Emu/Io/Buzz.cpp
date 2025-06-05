// Buzz! buzzer emulator

#include "stdafx.h"
#include "Buzz.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/Io/buzz_config.h"
#include "Emu/system_config.h"
#include "Input/pad_thread.h"

LOG_CHANNEL(buzz_log, "BUZZ");

template <>
void fmt_class_string<buzz_btn>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](buzz_btn value)
	{
		switch (value)
		{
		case buzz_btn::red: return "Red";
		case buzz_btn::yellow: return "Yellow";
		case buzz_btn::green: return "Green";
		case buzz_btn::orange: return "Orange";
		case buzz_btn::blue: return "Blue";
		case buzz_btn::count: return "Count";
		}

		return unknown;
	});
}

usb_device_buzz::usb_device_buzz(u32 first_controller, u32 last_controller, const std::array<u8, 7>& location)
	: usb_device_emulated(location)
	, m_first_controller(first_controller)
	, m_last_controller(last_controller)
{
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE,
		UsbDeviceDescriptor{
			.bcdUSB             = 0x0200,
			.bDeviceClass       = 0x00,
			.bDeviceSubClass    = 0x00,
			.bDeviceProtocol    = 0x00,
			.bMaxPacketSize0    = 0x08,
			.idVendor           = 0x054c,
			.idProduct          = 0x0002,
			.bcdDevice          = 0x05a1,
			.iManufacturer      = 0x02,
			.iProduct           = 0x01,
			.iSerialNumber      = 0x00,
			.bNumConfigurations = 0x01});
	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG,
		UsbDeviceConfiguration{
			.wTotalLength        = 0x0022,
			.bNumInterfaces      = 0x01,
			.bConfigurationValue = 0x01,
			.iConfiguration      = 0x00,
			.bmAttributes        = 0x80,
			.bMaxPower           = 0x32}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
		UsbDeviceInterface{
			.bInterfaceNumber   = 0x00,
			.bAlternateSetting  = 0x00,
			.bNumEndpoints      = 0x01,
			.bInterfaceClass    = 0x03,
			.bInterfaceSubClass = 0x00,
			.bInterfaceProtocol = 0x00,
			.iInterface         = 0x00}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_HID,
		UsbDeviceHID{
			.bcdHID            = 0x0111,
			.bCountryCode      = 0x33,
			.bNumDescriptors   = 0x01,
			.bDescriptorType   = 0x22,
			.wDescriptorLength = 0x004e}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
		UsbDeviceEndpoint{
			.bEndpointAddress = 0x81,
			.bmAttributes     = 0x03,
			.wMaxPacketSize   = 0x0008,
			.bInterval        = 0x0A}));

	add_string("Logitech Buzz(tm) Controller V1");
	add_string("Logitech");
}

usb_device_buzz::~usb_device_buzz()
{
}

std::shared_ptr<usb_device> usb_device_buzz::make_instance(u32 controller_index, const std::array<u8, 7>& location)
{
	if (controller_index == 0)
	{
		return std::make_shared<usb_device_buzz>(0, 3, location);
	}

	// The current buzz emulation piggybacks on the pad input.
	// Since there can only be 7 pads connected on a PS3 the 8th player is currently not supported
	return std::make_shared<usb_device_buzz>(4, 6, location);
}

u16 usb_device_buzz::get_num_emu_devices()
{
	return static_cast<u16>(g_cfg.io.buzz.get());
}

void usb_device_buzz::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake            = true;
	transfer->expected_count  = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	// Control transfers are nearly instant
	transfer->expected_time   = get_timestamp() + 100;

	switch (bmRequestType)
	{
	case 0U /*silences warning*/ | LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE: // 0x21
		switch (bRequest)
		{
		case 0x09: // SET_REPORT
			ensure(buf_size > 4);
			buzz_log.trace("Leds: %s/%s/%s/%s",
				buf[1] == 0xff ? "ON" : "OFF",
				buf[2] == 0xff ? "ON" : "OFF",
				buf[3] == 0xff ? "ON" : "OFF",
				buf[4] == 0xff ? "ON" : "OFF"
			);
			break;
		default:
			buzz_log.error("Unhandled Request: 0x%02X/0x%02X", bmRequestType, bRequest);
			break;
		}
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
	const auto handler = pad::get_pad_thread();
	const auto& pads   = handler->GetPads();
	ensure(pads.size() > m_last_controller);
	ensure(g_cfg_buzz.players.size() > m_last_controller);

	for (u32 i = m_first_controller, index = 0; i <= m_last_controller; i++, index++)
	{
		const auto& pad = pads[i];

		if (!pad->is_connected() || pad->is_copilot())
		{
			continue;
		}

		const auto& cfg = g_cfg_buzz.players[i];
		cfg->handle_input(pad, true, [&buf, &index](buzz_btn btn, pad_button /*pad_btn*/, u16 /*value*/, bool pressed, bool& /*abort*/)
			{
				if (!pressed)
					return;

				switch (btn)
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
			});
	}
}
