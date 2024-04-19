#include "stdafx.h"
#include "GameTablet.h"
#include "MouseHandler.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/system_config.h"
#include "Input/pad_thread.h"

LOG_CHANNEL(gametablet_log);

usb_device_gametablet::usb_device_gametablet(const std::array<u8, 7>& location)
	: usb_device_emulated(location)
{
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE,
		UsbDeviceDescriptor {
			.bcdUSB             = 0x0200,
			.bDeviceClass       = 0x00,
			.bDeviceSubClass    = 0x00,
			.bDeviceProtocol    = 0x00,
			.bMaxPacketSize0    = 0x08,
			.idVendor           = 0x20d6,
			.idProduct          = 0xcb17,
			.bcdDevice          = 0x0108,
			.iManufacturer      = 0x01,
			.iProduct           = 0x02,
			.iSerialNumber      = 0x00,
			.bNumConfigurations = 0x01});
	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG,
		UsbDeviceConfiguration {
			.wTotalLength        = 0x0029,
			.bNumInterfaces      = 0x01,
			.bConfigurationValue = 0x01,
			.iConfiguration      = 0x00,
			.bmAttributes        = 0x80,
			.bMaxPower           = 0x32}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
		UsbDeviceInterface {
			.bInterfaceNumber   = 0x00,
			.bAlternateSetting  = 0x00,
			.bNumEndpoints      = 0x02,
			.bInterfaceClass    = 0x03,
			.bInterfaceSubClass = 0x00,
			.bInterfaceProtocol = 0x00,
			.iInterface         = 0x00}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_HID,
		UsbDeviceHID {
			.bcdHID            = 0x0110,
			.bCountryCode      = 0x00,
			.bNumDescriptors   = 0x01,
			.bDescriptorType   = 0x22,
			.wDescriptorLength = 0x0089}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
		UsbDeviceEndpoint {
			.bEndpointAddress = 0x83,
			.bmAttributes     = 0x03,
			.wMaxPacketSize   = 0x0040,
			.bInterval        = 0x0A}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
		UsbDeviceEndpoint {
			.bEndpointAddress = 0x04,
			.bmAttributes     = 0x03,
			.wMaxPacketSize   = 0x0040,
			.bInterval        = 0x0A}));

	add_string("THQ Inc");
	add_string("THQ uDraw Game Tablet for PS3");
}

usb_device_gametablet::~usb_device_gametablet()
{
}

void usb_device_gametablet::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake            = true;
	transfer->expected_count  = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	transfer->expected_time   = get_timestamp() + 100;

	// Control transfers are nearly instant
	switch (bmRequestType)
	{
	case 0U /*silences warning*/ | LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE: // 0x21
		switch (bRequest)
		{
		case 0x09: // SET_REPORT
			ensure(buf_size > 2);
			gametablet_log.trace("Leds: %s/%s/%s/%s",
				buf[2] & 1 ? "ON" : "OFF",
				buf[2] & 2 ? "ON" : "OFF",
				buf[2] & 4 ? "ON" : "OFF",
				buf[2] & 8 ? "ON" : "OFF");
			break;
		default:
			gametablet_log.error("Unhandled Request: 0x%02X/0x%02X", bmRequestType, bRequest);
			break;
		}
		break;
	default:
		usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
		break;
	}
}

extern bool is_input_allowed();

void usb_device_gametablet::interrupt_transfer(u32 buf_size, u8* buf, u32 /*endpoint*/, UsbTransfer* transfer)
{
	ensure(buf_size > 0x1a);

	transfer->fake            = true;
	transfer->expected_count  = 27;
	transfer->expected_result = HC_CC_NOERR;
	// Interrupt transfers are slow (6ms, TODO accurate measurement)
	transfer->expected_time = get_timestamp() + 6000;

	std::memset(buf, 0, buf_size);

	buf[0x02] = 0x0f; // dpad

	buf[0x03] = 0x80;
	buf[0x04] = 0x80;
	buf[0x05] = 0x80;
	buf[0x06] = 0x80;

	buf[0x0d] = 0x72; // pressure
	buf[0x0f] = 0x0f; // pos X
	buf[0x10] = 0x0f; // pos Y
	buf[0x11] = 0xff;
	buf[0x12] = 0xff;

	buf[0x13] = 0x01; // accel X
	buf[0x14] = 0x02;
	buf[0x15] = 0x00; // accel Y
	buf[0x16] = 0x02;
	buf[0x17] = 0xea; // accel Z
	buf[0x18] = 0x01;

	buf[0x1a] = 0x02;

	if (!is_input_allowed())
	{
		return;
	}

	if (g_cfg.io.mouse != mouse_handler::basic)
	{
		gametablet_log.warning("GameTablet requires mouse_handler configured to basic");
		return;
	}

	bool up = false, right = false, down = false, left = false;

	{
		std::lock_guard lock(pad::g_pad_mutex);
		const auto gamepad_handler = pad::get_current_handler();
		const auto& pads = gamepad_handler->GetPads();

		constexpr s32 pad_index = 1; // Player2
		const auto& pad = ::at32(pads, pad_index);
		if (pad->m_port_status & CELL_PAD_STATUS_CONNECTED)
		{
			for (Button& button : pad->m_buttons)
			{
				if (!button.m_pressed)
				{
					continue;
				}

				if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL1)
				{
					switch (button.m_outKeyCode)
					{
					case CELL_PAD_CTRL_SELECT:
						buf[1] |= (1 << 0);
						break;
					case CELL_PAD_CTRL_START:
						buf[1] |= (1 << 1);
						break;
					case CELL_PAD_CTRL_UP:
						up = true;
						break;
					case CELL_PAD_CTRL_RIGHT:
						right = true;
						break;
					case CELL_PAD_CTRL_DOWN:
						down = true;
						break;
					case CELL_PAD_CTRL_LEFT:
						left = true;
						break;
					default:
						break;
					}
				}
				else if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL2)
				{
					switch (button.m_outKeyCode)
					{
					case CELL_PAD_CTRL_SQUARE:
						buf[0] |= (1 << 0);
						break;
					case CELL_PAD_CTRL_CROSS:
						buf[0] |= (1 << 1);
						break;
					case CELL_PAD_CTRL_CIRCLE:
						buf[0] |= (1 << 2);
						break;
					case CELL_PAD_CTRL_TRIANGLE:
						buf[0] |= (1 << 3);
						break;
					case CELL_PAD_CTRL_PS:
						buf[1] |= (1 << 4);
						break;
					default:
						break;
					}
				}
			}
		}
	}

	if (!up && !right && !down && !left)
		buf[0x02] = 0x0f;
	else if (up && !left && !right)
		buf[0x02] = 0x00;
	else if (up && right)
		buf[0x02] = 0x01;
	else if (right && !up && !down)
		buf[0x02] = 0x02;
	else if (down && right)
		buf[0x02] = 0x03;
	else if (down && !left && !right)
		buf[0x02] = 0x04;
	else if (down && left)
		buf[0x02] = 0x05;
	else if (left && !up && !down)
		buf[0x02] = 0x06;
	else if (up && left)
		buf[0x02] = 0x07;

	auto& mouse_handler = g_fxo->get<MouseHandlerBase>();
	std::lock_guard mouse_lock(mouse_handler.mutex);

	mouse_handler.Init(1);

	constexpr u8 mouse_index = 0;
	if (mouse_index >= mouse_handler.GetMice().size())
	{
		return;
	}

	const Mouse& mouse_data = ::at32(mouse_handler.GetMice(), mouse_index);
	if (mouse_data.x_max <= 0 || mouse_data.y_max <= 0)
	{
		return;
	}

	static u8 noise_x = 0; // Toggle the LSB to simulate a noisy signal, Instant Artist dislikes a pen held perfectly still
	static u8 noise_y = 0;
	constexpr s32 tablet_max_x = 1920;
	constexpr s32 tablet_max_y = 1080;

	const s32 tablet_x_pos = (mouse_data.x_pos * tablet_max_x / mouse_data.x_max) ^ noise_x;
	const s32 tablet_y_pos = (mouse_data.y_pos * tablet_max_y / mouse_data.y_max) ^ noise_y;
	noise_x ^= 0x1;
	noise_y ^= 0x1;

	buf[0x0b] = 0x40; // pen
	buf[0x0d] = mouse_data.buttons & CELL_MOUSE_BUTTON_1 ? 0xbb : 0x72; // pressure
	buf[0x0f] = static_cast<u8>(tablet_x_pos / 0x100);
	buf[0x10] = static_cast<u8>(tablet_y_pos / 0x100);
	buf[0x11] = static_cast<u8>(tablet_x_pos % 0x100);
	buf[0x12] = static_cast<u8>(tablet_y_pos % 0x100);
}
