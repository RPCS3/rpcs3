#include "stdafx.h"
#include "TopShotFearmaster.h"
#include "MouseHandler.h"
#include "Emu/IdManager.h"
#include "Emu/Io/topshotfearmaster_config.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/system_config.h"
#include "Input/pad_thread.h"

LOG_CHANNEL(topshotfearmaster_log);

#define TSF_CALIB_LOG false
// 0 < Calib_Top < Calib_Bottom < 0x2ff
// 0 < Calib_Right < Calib_Left < 0x3ff
constexpr u16 TSF_CALIB_TOP    = 20;
constexpr u16 TSF_CALIB_BOTTOM = 840;
constexpr u16 TSF_CALIB_LEFT   = 900;
constexpr u16 TSF_CALIB_RIGHT  = 120;
constexpr u16 TSF_CALIB_DIST   = 95;

template <>
void fmt_class_string<topshotfearmaster_btn>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](topshotfearmaster_btn value)
	{
		switch (value)
		{
		case topshotfearmaster_btn::trigger: return "Trigger";
		case topshotfearmaster_btn::heartrate: return "Heartrate";
		case topshotfearmaster_btn::square: return "Square";
		case topshotfearmaster_btn::cross: return "Cross";
		case topshotfearmaster_btn::circle: return "Circle";
		case topshotfearmaster_btn::triangle: return "Triangle";
		case topshotfearmaster_btn::select: return "Select";
		case topshotfearmaster_btn::start: return "Start";
		case topshotfearmaster_btn::l3: return "L3";
		case topshotfearmaster_btn::ps: return "PS";
		case topshotfearmaster_btn::dpad_up: return "D-Pad Up";
		case topshotfearmaster_btn::dpad_down: return "D-Pad Down";
		case topshotfearmaster_btn::dpad_left: return "D-Pad Left";
		case topshotfearmaster_btn::dpad_right: return "D-Pad Right";
		case topshotfearmaster_btn::ls_x: return "Left Stick X-Axis";
		case topshotfearmaster_btn::ls_y: return "Left Stick Y-Axis";
		case topshotfearmaster_btn::count: return "Count";
		}

		return unknown;
	});
}

#pragma pack(push, 1)
struct TopShotFearmaster_data
{
	uint8_t btn_square : 1;
	uint8_t btn_cross : 1;
	uint8_t btn_circle : 1;
	uint8_t btn_triangle : 1;
	uint8_t : 1;
	uint8_t btn_trigger: 1;
	uint8_t : 2;

	uint8_t btn_select : 1;
	uint8_t btn_start : 1;
	uint8_t btn_l3 : 1;
	uint8_t btn_heartrate : 1;
	uint8_t btn_ps: 1;
	uint8_t : 3;

	uint8_t dpad;
	uint8_t stick_lx;
	uint8_t stick_ly;
	union
	{
		struct
		{
			uint8_t stick_rx;
			uint8_t stick_ry;
		};
		uint16_t heartrate;
	};

	uint8_t led_lx_hi : 8;
	uint8_t led_ly_hi : 6;
	uint8_t led_lx_lo : 2;
	uint8_t detect_l : 4;
	uint8_t led_ly_lo : 4;

	uint8_t led_rx_hi : 8;
	uint8_t led_ry_hi : 6;
	uint8_t led_rx_lo : 2;
	uint8_t detect_r : 4;
	uint8_t led_ry_lo : 4;

	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;

	uint8_t trigger;
	uint8_t : 8;
	uint8_t : 8;
	uint16_t unk[4];
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

usb_device_topshotfearmaster::usb_device_topshotfearmaster(u32 controller_index, const std::array<u8, 7>& location)
	: usb_device_emulated(location)
	, m_controller_index(controller_index)
	, m_mode(0)
{
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE,
		UsbDeviceDescriptor {
			.bcdUSB             = 0x0100,
			.bDeviceClass       = 0x00,
			.bDeviceSubClass    = 0x00,
			.bDeviceProtocol    = 0x00,
			.bMaxPacketSize0    = 0x20,
			.idVendor           = 0x12ba,
			.idProduct          = 0x04a1,
			.bcdDevice          = 0x0108,
			.iManufacturer      = 0x01,
			.iProduct           = 0x02,
			.iSerialNumber      = 0x03,
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
			.bEndpointAddress = 0x81,
			.bmAttributes     = 0x03,
			.wMaxPacketSize   = 0x0040,
			.bInterval        = 0x0a}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
		UsbDeviceEndpoint {
			.bEndpointAddress = 0x02,
			.bmAttributes     = 0x03,
			.wMaxPacketSize   = 0x0040,
			.bInterval        = 0x0a}));

	add_string("Dangerous Hunts for Playstation (R) 3");
	add_string("Dangerous Hunts for Playstation (R) 3");
}

usb_device_topshotfearmaster::~usb_device_topshotfearmaster()
{
}

void usb_device_topshotfearmaster::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake            = true;
	transfer->expected_count  = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	transfer->expected_time   = get_timestamp() + 100;

	switch (bmRequestType)
	{
	case 0U /*silences warning*/ | LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE: // 0x21
		switch (bRequest)
		{
		case 0x09: // SET_REPORT
			ensure(buf_size >= 8);
			switch (buf[0])
			{
			case 0x01:
				topshotfearmaster_log.trace("Leds: %s/%s/%s/%s",
					buf[2] & 1 ? "ON" : "OFF",
					buf[2] & 2 ? "ON" : "OFF",
					buf[2] & 4 ? "ON" : "OFF",
					buf[2] & 8 ? "ON" : "OFF");
				break;
			case 0x82:
				m_mode = buf[2];
				break;
			default:
				topshotfearmaster_log.error("Unhandled SET_REPORT packet : %x", buf[0]);
				break;
			}
			break;
		default:
			topshotfearmaster_log.error("Unhandled Request: 0x%02X/0x%02X", bmRequestType, bRequest);
			break;
		}
		break;
	default:
		usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
		break;
	}
}

extern bool is_input_allowed();

static int get_heartrate_sensor_value(u8 heartrate)
{
	static const uint16_t sensor_data[] = {
		0x7af, 0x771, 0x737, 0x6ff, 0x6cb, 0x69a, 0x66c, 0x640, 0x616, 0x5ef, // 30-39
		0x5ca, 0x5a6, 0x584, 0x563, 0x544, 0x527, 0x50a, 0x4ef, 0x4d5, 0x4bc, // 40-49
		0x4a4, 0x48d, 0x477, 0x462, 0x44d, 0x439, 0x426, 0x413, 0x401, 0x3f0, // 50-59
		0x3e0, 0x3cf, 0x3c0, 0x3b1, 0x3a2, 0x394, 0x386, 0x379, 0x36c, 0x35f, // 60-69
		0x353, 0x347, 0x33b, 0x330, 0x325, 0x31b, 0x310, 0x306, 0x2fc, 0x2f3, // 70-79
		0x2e9, 0x2e0, 0x2d7, 0x2ce, 0x2c6, 0x2bd, 0x2b5, 0x2ad, 0x2a6, 0x29e, // 80-89
		0x297, 0x290, 0x289, 0x282, 0x27b, 0x274, 0x26e, 0x267, 0x261, 0x25b, // 90-99
		0x255, 0x24f, 0x249, 0x243, 0x23e, 0x239, 0x233, 0x22e, 0x229, 0x224, // 100-109
		0x21f, 0x21a, 0x215, 0x210, 0x20c, 0x207, 0x203, 0x1fe, 0x1fa, 0x1f6, // 110-119
		0x1f2, 0x1ed, 0x1e9, 0x1e5, 0x1e2, 0x1de, 0x1da, 0x1d6, 0x1d3, 0x1cf, // 120-129
		0x1cc, 0x1c8, 0x1c5, 0x1c1, 0x1be, 0x1bb, 0x1b7, 0x1b4, 0x1b1, 0x1ae  // 130-139
	};

	if (heartrate < 30 || heartrate > 139)
	{
		return 0;
	}

	return sensor_data[heartrate - 30];
}

static void set_sensor_pos(struct TopShotFearmaster_data* ts, s32 led_lx, s32 led_ly, s32 led_rx, s32 led_ry, s32 detect_l, s32 detect_r)
{
	ts->led_lx_hi = led_lx >> 2;
	ts->led_lx_lo = led_lx & 0x3;
	ts->led_ly_hi = led_ly >> 4;
	ts->led_ly_lo = led_ly & 0xf;

	ts->led_rx_hi = led_rx >> 2;
	ts->led_rx_lo = led_rx & 0x3;
	ts->led_ry_hi = led_ry >> 4;
	ts->led_ry_lo = led_ry & 0xf;

	ts->detect_l = detect_l;
	ts->detect_r = detect_r;
}

static void prepare_data(const TopShotFearmaster_data* ts, u8* data)
{
	std::memcpy(data, ts, sizeof(TopShotFearmaster_data));
	topshotfearmaster_log.trace("interrupt_transfer: %s", fmt::buf_to_hexstring(data, sizeof(TopShotFearmaster_data)));
}

void usb_device_topshotfearmaster::interrupt_transfer(u32 buf_size, u8* buf, u32 /*endpoint*/, UsbTransfer* transfer)
{
	ensure(buf_size >= sizeof(TopShotFearmaster_data));

	transfer->fake = true;
	transfer->expected_count = sizeof(TopShotFearmaster_data);
	transfer->expected_result = HC_CC_NOERR;
	transfer->expected_time = get_timestamp() + 4000;

	struct TopShotFearmaster_data ts{};
	ts.dpad = Dpad_None;
	ts.stick_lx = ts.stick_ly = ts.stick_rx = ts.stick_ry = 0x7f;
	if (m_mode)
	{
		set_sensor_pos(&ts, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0xf, 0xf);
	}
	ts.unk[0] = ts.unk[1] = ts.unk[2] = ts.unk[3] = 0x0200;

	if (!is_input_allowed())
	{
		prepare_data(&ts, buf);
		return;
	}

	if (g_cfg.io.mouse == mouse_handler::null)
	{
		topshotfearmaster_log.warning("Top Shot Fearmaster requires a Mouse Handler enabled");
		prepare_data(&ts, buf);
		return;
	}

	if (m_controller_index >= g_cfg_topshotfearmaster.players.size())
	{
		topshotfearmaster_log.warning("Top Shot Fearmaster controllers are only supported for Player1 to Player%d", g_cfg_topshotfearmaster.players.size());
		prepare_data(&ts, buf);
		return;
	}

	bool up = false, right = false, down = false, left = false;
	const auto input_callback = [&ts, &up, &down, &left, &right](topshotfearmaster_btn btn, u16 value, bool pressed)
	{
		if (!pressed)
			return;

		switch (btn)
		{
		case topshotfearmaster_btn::trigger: ts.btn_trigger |= 1; break;
		case topshotfearmaster_btn::heartrate: ts.btn_heartrate |= 1; break;
		case topshotfearmaster_btn::square: ts.btn_square |= 1; break;
		case topshotfearmaster_btn::cross: ts.btn_cross |= 1; break;
		case topshotfearmaster_btn::circle: ts.btn_circle |= 1; break;
		case topshotfearmaster_btn::triangle: ts.btn_triangle |= 1; break;
		case topshotfearmaster_btn::select: ts.btn_select |= 1; break;
		case topshotfearmaster_btn::start: ts.btn_start |= 1; break;
		case topshotfearmaster_btn::l3: ts.btn_l3 |= 1; break;
		case topshotfearmaster_btn::ps: ts.btn_ps |= 1; break;
		case topshotfearmaster_btn::dpad_up: up = true; break;
		case topshotfearmaster_btn::dpad_down: down = true; break;
		case topshotfearmaster_btn::dpad_left: left = true; break;
		case topshotfearmaster_btn::dpad_right: right = true; break;
		case topshotfearmaster_btn::ls_x: ts.stick_lx = static_cast<uint8_t>(value); break;
		case topshotfearmaster_btn::ls_y: ts.stick_ly = static_cast<uint8_t>(value); break;
		case topshotfearmaster_btn::count: break;
		}
	};

	const auto& cfg = ::at32(g_cfg_topshotfearmaster.players, m_controller_index);

	{
		std::lock_guard lock(pad::g_pad_mutex);
		const auto gamepad_handler = pad::get_current_handler();
		const auto& pads = gamepad_handler->GetPads();
		const auto& pad = ::at32(pads, m_controller_index);
		if (pad->m_port_status & CELL_PAD_STATUS_CONNECTED)
		{
			cfg->handle_input(pad, true, input_callback);
		}
	}

	if (!up && !right && !down && !left)
		ts.dpad = Dpad_None;
	else if (up && !left && !right)
		ts.dpad = Dpad_North;
	else if (up && right)
		ts.dpad = Dpad_NE;
	else if (right && !up && !down)
		ts.dpad = Dpad_East;
	else if (down && right)
		ts.dpad = Dpad_SE;
	else if (down && !left && !right)
		ts.dpad = Dpad_South;
	else if (down && left)
		ts.dpad = Dpad_SW;
	else if (left && !up && !down)
		ts.dpad = Dpad_West;
	else if (up && left)
		ts.dpad = Dpad_NW;

	if (m_mode)
	{
		auto& mouse_handler = g_fxo->get<MouseHandlerBase>();
		std::lock_guard mouse_lock(mouse_handler.mutex);

		mouse_handler.Init(4);

		const u32 mouse_index = g_cfg.io.mouse == mouse_handler::basic ? 0 : m_controller_index;
		if (mouse_index >= mouse_handler.GetMice().size())
		{
			prepare_data(&ts, buf);
			return;
		}

		const Mouse& mouse_data = ::at32(mouse_handler.GetMice(), mouse_index);
		cfg->handle_input(mouse_data, input_callback);
		ts.trigger = ts.btn_trigger ? 0xff : 0x00;
		ts.heartrate = ts.btn_heartrate ? get_heartrate_sensor_value(60) : 0;

		if (mouse_data.x_max <= 0 || mouse_data.y_max <= 0)
		{
			prepare_data(&ts, buf);
			return;
		}

		s32 led_lx = 0x3ff - (TSF_CALIB_RIGHT + (mouse_data.x_pos * (TSF_CALIB_LEFT - TSF_CALIB_RIGHT) / mouse_data.x_max) + TSF_CALIB_DIST);
		s32 led_rx = 0x3ff - (TSF_CALIB_RIGHT + (mouse_data.x_pos * (TSF_CALIB_LEFT - TSF_CALIB_RIGHT) / mouse_data.x_max) - TSF_CALIB_DIST);

		s32 led_ly = TSF_CALIB_TOP + (mouse_data.y_pos * (TSF_CALIB_BOTTOM - TSF_CALIB_TOP) / mouse_data.y_max);
		s32 led_ry = TSF_CALIB_TOP + (mouse_data.y_pos * (TSF_CALIB_BOTTOM - TSF_CALIB_TOP) / mouse_data.y_max);

		u8 detect_l = 0x2, detect_r = 0x2; // 0x2 = led detected / 0xf = undetected

		if (led_lx < 0 || led_lx > 0x3ff || led_ly < 0 || led_ly > 0x3ff)
		{
			led_lx = 0x3ff;
			led_ly = 0x3ff;
			detect_l = 0xf;
		}

		if (led_rx < 0 || led_rx > 0x3ff || led_ry < 0 || led_ry > 0x3ff)
		{
			led_rx = 0x3ff;
			led_ry = 0x3ff;
			detect_r = 0xf;
		}

		set_sensor_pos(&ts, led_lx, led_ly, led_rx, led_ry, detect_l, detect_r);

		#if TSF_CALIB_LOG
			topshotfearmaster_log.error("L: %d x %d, R: %d x %d", led_lx + TSF_CALIB_DIST, led_ly, led_rx - TSF_CALIB_DIST, led_ry);
		#endif
	}

	prepare_data(&ts, buf);
}
