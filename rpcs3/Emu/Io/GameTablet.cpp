#include "stdafx.h"
#include "GameTablet.h"
#include "MouseHandler.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/system_config.h"
#include "Input/pad_thread.h"

LOG_CHANNEL(gametablet_log);

#pragma pack(push, 1)
struct GameTablet_data
{
	uint8_t btn_square : 1;
	uint8_t btn_cross : 1;
	uint8_t btn_circle : 1;
	uint8_t btn_triangle : 1;
	uint8_t : 4;

	uint8_t btn_select : 1;
	uint8_t btn_start : 1;
	uint8_t : 2;
	uint8_t btn_ps: 1;
	uint8_t : 3;

	uint8_t dpad;
	uint8_t stick_lx; // 0x80
	uint8_t stick_ly; // 0x80
	uint8_t stick_rx; // 0x80
	uint8_t stick_ry; // 0x80

	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t pen;
	uint8_t : 8;
	uint8_t pressure;
	uint8_t : 8;
	uint8_t pos_x_hi;
	uint8_t pos_y_hi;
	uint8_t pos_x_lo;
	uint8_t pos_y_lo;

	uint16_t accel_x;
	uint16_t accel_y;
	uint16_t accel_z;
	uint16_t unk; // 0x0200
};
#pragma pack(pop)

enum
{
	Dpad_North,
	Dpad_NE,
	Dpad_East,
	Dpad_SE,
	Dpad_South,
	Dpad_SW,
	Dpad_West,
	Dpad_NW,
	Dpad_None = 0x0f
};

usb_device_gametablet::usb_device_gametablet(u32 controller_index, const std::array<u8, 7>& location)
	: usb_device_emulated(location)
	, m_controller_index(controller_index)
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
	ensure(buf_size >= sizeof(GameTablet_data));

	transfer->fake            = true;
	transfer->expected_count  = sizeof(GameTablet_data);
	transfer->expected_result = HC_CC_NOERR;
	// Interrupt transfers are slow (6ms, TODO accurate measurement)
	transfer->expected_time = get_timestamp() + 6000;

	GameTablet_data gt{};

	gt.dpad = Dpad_None;
	gt.stick_lx = gt.stick_ly = gt.stick_rx = gt.stick_ry = 0x80;
	gt.pressure = 0x72;
	gt.pos_x_hi = gt.pos_y_hi = 0x0f;
	gt.pos_x_lo = gt.pos_y_lo = 0xff;
	gt.accel_x = gt.accel_y = gt.accel_z = gt.unk = 0x0200;

	if (!is_input_allowed())
	{
		std::memcpy(buf, &gt, sizeof(GameTablet_data));
		return;
	}

	if (g_cfg.io.mouse == mouse_handler::null)
	{
		gametablet_log.warning("GameTablet requires a Mouse Handler enabled");
		std::memcpy(buf, &gt, sizeof(GameTablet_data));
		return;
	}

	bool up = false, right = false, down = false, left = false;

	{
		std::lock_guard lock(pad::g_pad_mutex);
		const auto gamepad_handler = pad::get_pad_thread();
		const auto& pads = gamepad_handler->GetPads();
		const auto& pad = ::at32(pads, m_controller_index);
		if (pad->is_connected() && !pad->is_copilot())
		{
			for (Button& button : pad->m_buttons_external)
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
						gt.btn_select |= 1;
						break;
					case CELL_PAD_CTRL_START:
						gt.btn_start |= 1;
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
						gt.btn_square |= 1;
						break;
					case CELL_PAD_CTRL_CROSS:
						gt.btn_cross |= 1;
						break;
					case CELL_PAD_CTRL_CIRCLE:
						gt.btn_circle |= 1;
						break;
					case CELL_PAD_CTRL_TRIANGLE:
						gt.btn_triangle |= 1;
						break;
					case CELL_PAD_CTRL_PS:
						gt.btn_ps |= 1;
						break;
					default:
						break;
					}
				}
			}
		}
	}

	if (!up && !right && !down && !left)
		gt.dpad = Dpad_None;
	else if (up && !left && !right)
		gt.dpad = Dpad_North;
	else if (up && right)
		gt.dpad = Dpad_NE;
	else if (right && !up && !down)
		gt.dpad = Dpad_East;
	else if (down && right)
		gt.dpad = Dpad_SE;
	else if (down && !left && !right)
		gt.dpad = Dpad_South;
	else if (down && left)
		gt.dpad = Dpad_SW;
	else if (left && !up && !down)
		gt.dpad = Dpad_West;
	else if (up && left)
		gt.dpad = Dpad_NW;

	auto& mouse_handler = g_fxo->get<MouseHandlerBase>();
	std::lock_guard mouse_lock(mouse_handler.mutex);

	mouse_handler.Init(1);

	constexpr u8 mouse_index = 0;
	if (mouse_index >= mouse_handler.GetMice().size())
	{
		std::memcpy(buf, &gt, sizeof(GameTablet_data));
		return;
	}

	const Mouse& mouse_data = ::at32(mouse_handler.GetMice(), mouse_index);
	if (mouse_data.x_max <= 0 || mouse_data.y_max <= 0)
	{
		std::memcpy(buf, &gt, sizeof(GameTablet_data));
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

	gt.pen = 0x40;
	gt.pressure = mouse_data.buttons & CELL_MOUSE_BUTTON_1 ? 0xbb : 0x72;
	gt.pos_x_hi = static_cast<u8>(tablet_x_pos / 0x100);
	gt.pos_y_hi = static_cast<u8>(tablet_y_pos / 0x100);
	gt.pos_x_lo = static_cast<u8>(tablet_x_pos % 0x100);
	gt.pos_y_lo = static_cast<u8>(tablet_y_pos % 0x100);

	std::memcpy(buf, &gt, sizeof(GameTablet_data));
}
