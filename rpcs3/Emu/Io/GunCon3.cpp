#include "stdafx.h"
#include "GunCon3.h"
#include "MouseHandler.h"
#include <cmath>
#include <climits>
#include <algorithm>
#include <mutex>
#include "Emu/IdManager.h"
#include "Emu/Io/guncon3_config.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/system_config.h"
#include "Input/wiimote_handler.h"
#include "Input/pad_thread.h"
#include "Emu/RSX/Overlays/overlay_cursor.h"

LOG_CHANNEL(guncon3_log);

template <>
void fmt_class_string<guncon3_btn>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](guncon3_btn value)
	{
		switch (value)
		{
		case guncon3_btn::trigger: return "Trigger";
		case guncon3_btn::a1: return "A1";
		case guncon3_btn::a2: return "A2";
		case guncon3_btn::a3: return "A3";
		case guncon3_btn::b1: return "B1";
		case guncon3_btn::b2: return "B2";
		case guncon3_btn::b3: return "B3";
		case guncon3_btn::c1: return "C1";
		case guncon3_btn::c2: return "C2";
		case guncon3_btn::as_x: return "A-stick X-Axis";
		case guncon3_btn::as_y: return "A-stick Y-Axis";
		case guncon3_btn::bs_x: return "B-stick X-Axis";
		case guncon3_btn::bs_y: return "B-stick Y-Axis";
		case guncon3_btn::count: return "Count";
		}

		return unknown;
	});
}

static const u8 KEY_TABLE[] = {
	0x91, 0xFD, 0x4C, 0x8B, 0x20, 0xC1, 0x7C, 0x09, 0x58, 0x14, 0xF6, 0x00, 0x52, 0x55, 0xBF, 0x41,
	0x75, 0xC0, 0x13, 0x30, 0xB5, 0xD0, 0x69, 0x85, 0x89, 0xBB, 0xD6, 0x88, 0xBC, 0x73, 0x18, 0x8D,
	0x58, 0xAB, 0x3D, 0x98, 0x5C, 0xF2, 0x48, 0xE9, 0xAC, 0x9F, 0x7A, 0x0C, 0x7C, 0x25, 0xD8, 0xFF,
	0xDC, 0x7D, 0x08, 0xDB, 0xBC, 0x18, 0x8C, 0x1D, 0xD6, 0x3C, 0x35, 0xE1, 0x2C, 0x14, 0x8E, 0x64,
	0x83, 0x39, 0xB0, 0xE4, 0x4E, 0xF7, 0x51, 0x7B, 0xA8, 0x13, 0xAC, 0xE9, 0x43, 0xC0, 0x08, 0x25,
	0x0E, 0x15, 0xC4, 0x20, 0x93, 0x13, 0xF5, 0xC3, 0x48, 0xCC, 0x47, 0x1C, 0xC5, 0x20, 0xDE, 0x60,
	0x55, 0xEE, 0xA0, 0x40, 0xB4, 0xE7, 0x74, 0x95, 0xB0, 0x46, 0xEC, 0xF0, 0xA5, 0xB8, 0x23, 0xC8,
	0x04, 0x06, 0xFC, 0x28, 0xCB, 0xF8, 0x17, 0x2C, 0x25, 0x1C, 0xCB, 0x18, 0xE3, 0x6C, 0x80, 0x85,
	0xDD, 0x7E, 0x09, 0xD9, 0xBC, 0x19, 0x8F, 0x1D, 0xD4, 0x3D, 0x37, 0xE1, 0x2F, 0x15, 0x8D, 0x64,
	0x06, 0x04, 0xFD, 0x29, 0xCF, 0xFA, 0x14, 0x2E, 0x25, 0x1F, 0xC9, 0x18, 0xE3, 0x6D, 0x81, 0x84,
	0x80, 0x3B, 0xB1, 0xE5, 0x4D, 0xF7, 0x51, 0x78, 0xA9, 0x13, 0xAD, 0xE9, 0x80, 0xC1, 0x0B, 0x25,
	0x93, 0xFC, 0x4D, 0x89, 0x23, 0xC2, 0x7C, 0x0B, 0x59, 0x15, 0xF6, 0x01, 0x50, 0x55, 0xBF, 0x81,
	0x75, 0xC3, 0x10, 0x31, 0xB5, 0xD3, 0x69, 0x84, 0x89, 0xBA, 0xD6, 0x89, 0xBD, 0x70, 0x19, 0x8E,
	0x58, 0xA8, 0x3D, 0x9B, 0x5D, 0xF0, 0x49, 0xE8, 0xAD, 0x9D, 0x7A, 0x0D, 0x7E, 0x24, 0xDA, 0xFC,
	0x0D, 0x14, 0xC5, 0x23, 0x91, 0x11, 0xF5, 0xC0, 0x4B, 0xCD, 0x44, 0x1C, 0xC5, 0x21, 0xDF, 0x61,
	0x54, 0xED, 0xA2, 0x81, 0xB7, 0xE5, 0x74, 0x94, 0xB0, 0x47, 0xEE, 0xF1, 0xA5, 0xBB, 0x21, 0xC8
};

#pragma pack(push, 1)
struct GunCon3_data
{
	uint8_t : 1;
	uint8_t btn_a2 : 1;
	uint8_t btn_a1 : 1;
	uint8_t btn_c2 : 1;
	uint8_t : 4;

	uint8_t : 1;
	uint8_t btn_b2 : 1;
	uint8_t btn_b1 : 1;
	uint8_t : 2;
	uint8_t btn_trigger : 1;
	uint8_t : 1;
	uint8_t btn_c1 : 1;

	uint8_t : 6;
	uint8_t btn_b3 : 1;
	uint8_t btn_a3 : 1;

	int16_t gun_x;
	int16_t gun_y;
	int16_t gun_z;
	uint8_t stick_bx;
	uint8_t stick_by;
	uint8_t stick_ax;
	uint8_t stick_ay;

	uint8_t checksum;
	uint8_t keyindex;
};
#pragma pack(pop)

static void guncon3_encode(const GunCon3_data* gc, u8* data, const u8* key)
{
	std::memcpy(data, gc, sizeof(GunCon3_data));

	u8 key_offset = ((((key[1] ^ key[2]) - key[3] - key[4]) ^ key[5]) + key[6] - key[7]) ^ data[14];
	u8 key_index = 0;

	for (int i = 0; i < 13; i++)
	{
		u8 byte = data[i];
		for (int j = 0; j < 3; j++)
		{
			u8 bkey = KEY_TABLE[key_offset];
			u8 keyr = key[++key_index];
			if (key_index == 7)
				key_index = 0;

			if ((bkey & 3) == 0)
				byte = (byte + bkey) + keyr;
			else if ((bkey & 3) == 1)
				byte = (byte - bkey) - keyr;
			else
				byte = (byte ^ bkey) ^ keyr;

			key_offset++;
		}
		data[i] = byte;
	}

	data[13] = ((((((key[7] + data[0] - data[1] - data[2]) ^ data[3])
			 + data[4] + data[5]) ^ data[6]) ^ data[7])
			 + data[8] + data[9] - data[10] - data[11]) ^ data[12];
}

usb_device_guncon3::usb_device_guncon3(u32 controller_index, const std::array<u8, 7>& location)
	: usb_device_emulated(location)
	, m_controller_index(controller_index)
{
	{
		std::lock_guard lock(s_instances_mutex);
		s_instances.push_back(this);
		// Sort instances by controller index (P1 < P2 < P3...)
		// This ensures that the first available GunCon (e.g. at P3) takes the first Wiimote,
		// and the second (e.g. at P4) takes the second Wiimote.
		std::sort(s_instances.begin(), s_instances.end(), [](auto* a, auto* b) {
			return a->m_controller_index < b->m_controller_index;
		});
	}

	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE,
		UsbDeviceDescriptor {
			.bcdUSB             = 0x0110,
			.bDeviceClass       = 0x00,
			.bDeviceSubClass    = 0x00,
			.bDeviceProtocol    = 0x00,
			.bMaxPacketSize0    = 0x08,
			.idVendor           = 0x0b9a,
			.idProduct          = 0x0800,
			.bcdDevice          = 0x8000,
			.iManufacturer      = 0x00,
			.iProduct           = 0x00,
			.iSerialNumber      = 0x00,
			.bNumConfigurations = 0x01});
	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG,
		UsbDeviceConfiguration {
			.wTotalLength        = 0x0020,
			.bNumInterfaces      = 0x01,
			.bConfigurationValue = 0x01,
			.iConfiguration      = 0x00,
			.bmAttributes        = 0x00,
			.bMaxPower           = 0x32}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
		UsbDeviceInterface {
			.bInterfaceNumber   = 0x00,
			.bAlternateSetting  = 0x00,
			.bNumEndpoints      = 0x02,
			.bInterfaceClass    = 0xff,
			.bInterfaceSubClass = 0x00,
			.bInterfaceProtocol = 0x00,
			.iInterface         = 0x00}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
		UsbDeviceEndpoint {
			.bEndpointAddress = 0x02,
			.bmAttributes     = 0x03,
			.wMaxPacketSize   = 0x0008,
			.bInterval        = 0x10}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
		UsbDeviceEndpoint {
			.bEndpointAddress = 0x82,
			.bmAttributes     = 0x03,
			.wMaxPacketSize   = 0x000f,
			.bInterval        = 0x04}));
}

usb_device_guncon3::~usb_device_guncon3()
{
	std::lock_guard lock(s_instances_mutex);
	if (auto it = std::find(s_instances.begin(), s_instances.end(), this); it != s_instances.end())
	{
		s_instances.erase(it);
	}
}

void usb_device_guncon3::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake            = true;
	transfer->expected_count  = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	transfer->expected_time   = get_timestamp() + 100;

	usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
}

extern bool is_input_allowed();

bool usb_device_guncon3::handle_wiimote(GunCon3_data& gc)
{
	auto* wm = wiimote_handler::get_instance();
	auto states = wm->get_states();

	// Determine which Wiimote to use based on our ordinal position among all GunCons
	int my_wiimote_index = -1;
	{
		std::lock_guard lock(s_instances_mutex);
		auto found = std::find(s_instances.begin(), s_instances.end(), this);
		if (found != s_instances.end())
		{
			my_wiimote_index = std::distance(s_instances.begin(), found);
		}
	}

	if (my_wiimote_index < 0 || static_cast<size_t>(my_wiimote_index) >= states.size())
		return false;

	const auto& ws = states[my_wiimote_index];
	if (!ws.connected)
		return false;

	const auto map = wm->get_mapping();
	const auto is_pressed = [&](wiimote_button btn) { return (ws.buttons & static_cast<u16>(btn)) != 0; };

	if (is_pressed(map.trigger)) gc.btn_trigger = 1;

	// Wiimote to GunCon3 Button Mapping
	if (is_pressed(map.a1)) gc.btn_a1 = 1;
	if (is_pressed(map.a2)) gc.btn_a2 = 1;
	if (is_pressed(map.a3)) gc.btn_a3 = 1;
	if (is_pressed(map.b1)) gc.btn_b1 = 1;
	if (is_pressed(map.b2)) gc.btn_b2 = 1;
	if (is_pressed(map.b3)) gc.btn_b3 = 1;
	if (is_pressed(map.c1)) gc.btn_c1 = 1;
	if (is_pressed(map.c2)) gc.btn_c2 = 1;

	// Secondary / Hardcoded Alts
	if (is_pressed(map.b1_alt)) gc.btn_b1 = 1;
	if (is_pressed(map.b2_alt)) gc.btn_b2 = 1;

	if (ws.ir[0].x < 1023)
	{
		// Map Wiimote IR (0..1023) to GunCon3 range (-32768..32767)
		const s32 raw_x = ws.ir[0].x;
		const s32 raw_y = ws.ir[0].y;

		const s32 x_res = SHRT_MAX - (raw_x * USHRT_MAX / 1023);
		const s32 y_res = SHRT_MAX - (raw_y * USHRT_MAX / 767);

		gc.gun_x = static_cast<int16_t>(std::clamp(x_res, SHRT_MIN, SHRT_MAX));
		gc.gun_y = static_cast<int16_t>(std::clamp(y_res, SHRT_MIN, SHRT_MAX));

		if (g_cfg.io.show_move_cursor)
		{
			const s16 ax = static_cast<s16>((gc.gun_x + SHRT_MAX + 1) * rsx::overlays::overlay::virtual_width / USHRT_MAX);
			const s16 ay = static_cast<s16>((SHRT_MAX - gc.gun_y) * rsx::overlays::overlay::virtual_height / USHRT_MAX);
			rsx::overlays::set_cursor(rsx::overlays::cursor_offset::cell_gem + my_wiimote_index, ax, ay, { 1.0f, 1.0f, 1.0f, 1.0f }, 100'000, false);
		}

		if (ws.ir[1].x < 1023)
		{
			const s32 dx = static_cast<s32>(ws.ir[0].x) - ws.ir[1].x;
			const s32 dy = static_cast<s32>(ws.ir[0].y) - ws.ir[1].y;
			gc.gun_z = static_cast<int16_t>(std::sqrt(dx * dx + dy * dy));
		}
	}

	return true;
}

void usb_device_guncon3::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer)
{
	transfer->fake = true;
	transfer->expected_count = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	transfer->expected_time = get_timestamp() + 4000;

	if ((endpoint & 0x80) == LIBUSB_ENDPOINT_OUT)
	{
		std::memcpy(m_key.data(), buf, std::min<usz>(buf_size, m_key.size()));
		return;
	}
	// else ENDPOINT_IN

	ensure(buf_size >= sizeof(GunCon3_data));

	GunCon3_data gc{};
	gc.stick_ax = gc.stick_ay = gc.stick_bx = gc.stick_by = 0x7f;

	if (!is_input_allowed())
	{
		guncon3_encode(&gc, buf, m_key.data());
		return;
	}

	if (handle_wiimote(gc))
	{
		guncon3_encode(&gc, buf, m_key.data());
		return;
	}

	if (g_cfg.io.mouse == mouse_handler::null)
	{
		guncon3_log.warning("GunCon3 requires a Mouse Handler enabled");
		guncon3_encode(&gc, buf, m_key.data());
		return;
	}

	if (m_controller_index >= g_cfg_guncon3.players.size())
	{
		guncon3_log.warning("GunCon3 controllers are only supported for Player1 to Player%d", g_cfg_guncon3.players.size());
		guncon3_encode(&gc, buf, m_key.data());
		return;
	}

	const auto input_callback = [&gc](guncon3_btn btn, pad_button /*pad_button*/, u16 value, bool pressed, bool& /*abort*/)
	{
		if (!pressed)
			return;

		switch (btn)
		{
		case guncon3_btn::trigger: gc.btn_trigger |= 1; break;
		case guncon3_btn::a1: gc.btn_a1 |= 1; break;
		case guncon3_btn::a2: gc.btn_a2 |= 1; break;
		case guncon3_btn::a3: gc.btn_a3 |= 1; break;
		case guncon3_btn::b1: gc.btn_b1 |= 1; break;
		case guncon3_btn::b2: gc.btn_b2 |= 1; break;
		case guncon3_btn::b3: gc.btn_b3 |= 1; break;
		case guncon3_btn::c1: gc.btn_c1 |= 1; break;
		case guncon3_btn::c2: gc.btn_c2 |= 1; break;
		case guncon3_btn::as_x: gc.stick_ax = static_cast<uint8_t>(value); break;
		case guncon3_btn::as_y: gc.stick_ay = static_cast<uint8_t>(value); break;
		case guncon3_btn::bs_x: gc.stick_bx = static_cast<uint8_t>(value); break;
		case guncon3_btn::bs_y: gc.stick_by = static_cast<uint8_t>(value); break;
		case guncon3_btn::count: break;
		}
	};

	const auto& cfg = ::at32(g_cfg_guncon3.players, m_controller_index);

	{
		std::lock_guard lock(pad::g_pad_mutex);
		const auto gamepad_handler = pad::get_pad_thread();
		const auto& pads = gamepad_handler->GetPads();
		const auto& pad = ::at32(pads, m_controller_index);
		if (pad->is_connected() && !pad->is_copilot())
		{
			cfg->handle_input(pad, true, input_callback);
		}
	}

	{
		auto& mouse_handler = g_fxo->get<MouseHandlerBase>();
		std::lock_guard mouse_lock(mouse_handler.mutex);

		mouse_handler.Init(4);

		const u32 mouse_index = g_cfg.io.mouse == mouse_handler::basic ? 0 : m_controller_index;
		if (mouse_index >= mouse_handler.GetMice().size())
		{
			guncon3_encode(&gc, buf, m_key.data());
			return;
		}

		const Mouse& mouse_data = ::at32(mouse_handler.GetMice(), mouse_index);
		cfg->handle_input(mouse_data, input_callback);

		if (mouse_data.x_max <= 0 || mouse_data.y_max <= 0)
		{
			guncon3_encode(&gc, buf, m_key.data());
			return;
		}

		// Expand 0..+wh to -32767..+32767
		gc.gun_x = (mouse_data.x_pos * USHRT_MAX / mouse_data.x_max) - SHRT_MAX;
		gc.gun_y = (mouse_data.y_pos * -USHRT_MAX / mouse_data.y_max) + SHRT_MAX;
	}

	guncon3_encode(&gc, buf, m_key.data());
}
