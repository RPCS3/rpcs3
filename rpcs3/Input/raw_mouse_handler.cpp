#include "stdafx.h"
#include "raw_mouse_handler.h"

#include "Emu/RSX/GSRender.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/Io/interception.h"
#include "Input/raw_mouse_config.h"

#ifdef _WIN32
#include <hidusage.h>
#endif

#define RAW_MOUSE_DEBUG_CURSOR_ENABLED 0
#if RAW_MOUSE_DEBUG_CURSOR_ENABLED
#include "Emu/RSX/Overlays/overlay_cursor.h"
static inline void draw_overlay_cursor(u32 index, s32 x_pos, s32 y_pos, s32 x_max, s32 y_max)
{
	const u16 x = static_cast<u16>(x_pos / (x_max / static_cast<f32>(rsx::overlays::overlay::virtual_width)));
	const u16 y = static_cast<u16>(y_pos / (y_max / static_cast<f32>(rsx::overlays::overlay::virtual_height)));

	const color4f color = { 1.0f, 0.0f, 0.0f, 1.0f };

	rsx::overlays::set_cursor(rsx::overlays::cursor_offset::last + index, x, y, color, 2'000'000, false);
}
#else
static inline void draw_overlay_cursor(u32, s32, s32, s32, s32) {}
#endif

LOG_CHANNEL(input_log, "Input");

raw_mice_config g_cfg_raw_mouse;

raw_mouse::raw_mouse(u32 index, const std::string& device_name, void* handle, raw_mouse_handler* handler)
	: m_index(index), m_device_name(device_name), m_handle(handle), m_handler(handler)
{
	if (m_index < ::size32(g_cfg_raw_mouse.players))
	{
		if (const auto& player = ::at32(g_cfg_raw_mouse.players, m_index))
		{
			m_mouse_acceleration = static_cast<float>(player->mouse_acceleration.get()) / 100.0f;
		}
	}
}

raw_mouse::~raw_mouse()
{
}

void raw_mouse::update_window_handle()
{
#ifdef _WIN32
	if (GSRender* render = static_cast<GSRender*>(g_fxo->try_get<rsx::thread>()))
	{
		if (GSFrameBase* frame = render->get_frame())
		{
			if (display_handle_t window_handle = frame->handle(); window_handle && window_handle == GetActiveWindow())
			{
				m_window_handle = window_handle;
				m_window_width = frame->client_width();
				m_window_height = frame->client_height();
				return;
			}
		}
	}
#endif

	m_window_handle = {};
	m_window_width = 0;
	m_window_height = 0;
}

#ifdef _WIN32
void raw_mouse::update_values(const RAWMOUSE& state)
{
	ensure(m_handler != nullptr);

	// Update window handle and size
	update_window_handle();

	// Check if the window handle is active
	if (!m_window_handle)
	{
		return;
	}

	const auto get_button_pressed = [this](u8 button, int button_flags, int down, int up)
	{
		// Only update the value if either down or up flags are present
		if ((button_flags & down))
		{
			m_handler->Button(m_index, button, true);
		}
		else if ((button_flags & up))
		{
			m_handler->Button(m_index, button, false);
		}
	};

	// Get mouse buttons
	get_button_pressed(CELL_MOUSE_BUTTON_1, state.usButtonFlags, RI_MOUSE_BUTTON_1_DOWN, RI_MOUSE_BUTTON_1_UP);
	get_button_pressed(CELL_MOUSE_BUTTON_2, state.usButtonFlags, RI_MOUSE_BUTTON_2_DOWN, RI_MOUSE_BUTTON_2_UP);
	get_button_pressed(CELL_MOUSE_BUTTON_3, state.usButtonFlags, RI_MOUSE_BUTTON_3_DOWN, RI_MOUSE_BUTTON_3_UP);
	get_button_pressed(CELL_MOUSE_BUTTON_4, state.usButtonFlags, RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_4_UP);
	get_button_pressed(CELL_MOUSE_BUTTON_5, state.usButtonFlags, RI_MOUSE_BUTTON_5_DOWN, RI_MOUSE_BUTTON_5_UP);

	// Get vertical mouse wheel
	if ((state.usButtonFlags & RI_MOUSE_WHEEL))
	{
		m_handler->Scroll(m_index, static_cast<s16>(state.usButtonData));
	}

	// Get horizontal mouse wheel. Ignored until needed.
	//if ((state.usButtonFlags & RI_MOUSE_HWHEEL))
	//{
	//	m_handler->Scroll(m_index, static_cast<s16>(state.usButtonData));
	//}

	// Get mouse movement
	if ((state.usFlags & MOUSE_MOVE_ABSOLUTE))
	{
		// Get absolute mouse movement

		if (m_window_handle && m_window_width && m_window_height)
		{
			// Convert virtual coordinates to screen coordinates
			const bool is_virtual_desktop = (state.usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP;
			const int width = GetSystemMetrics(is_virtual_desktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
			const int height = GetSystemMetrics(is_virtual_desktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);

			if (width && height)
			{
				POINT pt {
					.x = long(state.lLastX / 65535.0f * width),
					.y = long(state.lLastY / 65535.0f * height)
				};

				if (ScreenToClient(m_window_handle, &pt))
				{
					// Move mouse with absolute position
					m_handler->Move(m_index, pt.x, pt.y, m_window_width, m_window_height);
					draw_overlay_cursor(m_index, pt.x, pt.y, m_window_width, m_window_height);
				}
			}
		}
	}
	else if (state.lLastX || state.lLastY)
	{
		// Get relative mouse movement (units most likely in raw dpi)

		if (m_window_handle && m_window_width && m_window_height)
		{
			// Apply mouse acceleration
			const int delta_x = static_cast<int>(state.lLastX * m_mouse_acceleration);
			const int delta_y = static_cast<int>(state.lLastY * m_mouse_acceleration);

			// Calculate new position
			m_pos_x = std::max(0, std::min(m_pos_x + delta_x, m_window_width - 1));
			m_pos_y = std::max(0, std::min(m_pos_y + delta_y, m_window_height - 1));

			// Move mouse relative to old position
			m_handler->Move(m_index, m_pos_x, m_pos_y, m_window_width, m_window_height, true, delta_x, delta_y);
			draw_overlay_cursor(m_index, m_pos_x, m_pos_y, m_window_width, m_window_height);
		}
	}
}
#endif

raw_mouse_handler::~raw_mouse_handler()
{
	if (m_raw_mice.empty())
	{
		return;
	}

#ifdef _WIN32
	std::vector<RAWINPUTDEVICE> raw_input_devices;
	raw_input_devices.push_back(RAWINPUTDEVICE {
		.usUsagePage = HID_USAGE_PAGE_GENERIC,
		.usUsage = HID_USAGE_GENERIC_MOUSE,
		.dwFlags = RIDEV_REMOVE,
		.hwndTarget = nullptr
	});
	if (!RegisterRawInputDevices(raw_input_devices.data(), ::size32(raw_input_devices), sizeof(RAWINPUTDEVICE)))
	{
		input_log.error("raw_mouse_handler: RegisterRawInputDevices (destructor) failed: %s", fmt::win_error{GetLastError(), nullptr});
	}
#endif
}

void raw_mouse_handler::Init(const u32 max_connect)
{
	if (m_info.max_connect > 0)
	{
		// Already initialized
		return;
	}

	if (!g_cfg_raw_mouse.load())
	{
		input_log.notice("raw_mouse_handler: Could not load raw mouse config. Using defaults.");
	}

	enumerate_devices(max_connect);

	m_mice.clear();

	for (u32 i = 0; i < std::min(::size32(m_raw_mice), max_connect); i++)
	{
		m_mice.emplace_back(Mouse());
	}

	m_info = {};
	m_info.max_connect = max_connect;
	m_info.now_connect = std::min(::size32(m_mice), max_connect);
	m_info.info = input::g_mice_intercepted ? CELL_MOUSE_INFO_INTERCEPTED : 0; // Ownership of mouse data: 0=Application, 1=System

	for (u32 i = 0; i < m_info.now_connect; i++)
	{
		m_info.status[i] = CELL_MOUSE_STATUS_CONNECTED;
		m_info.mode[i] = CELL_MOUSE_INFO_TABLET_MOUSE_MODE;
		m_info.tablet_is_supported[i] = CELL_MOUSE_INFO_TABLET_NOT_SUPPORTED;
		m_info.vendor_id[0] = 0x1234;
		m_info.product_id[0] = 0x1234;
	}

#ifdef _WIN32
	if (max_connect && !m_raw_mice.empty())
	{
		// Get the window handle of the first mouse
		raw_mouse& mouse = m_raw_mice.begin()->second;
		mouse.update_window_handle();

		std::vector<RAWINPUTDEVICE> raw_input_devices;
		raw_input_devices.push_back(RAWINPUTDEVICE {
			.usUsagePage = HID_USAGE_PAGE_GENERIC,
			.usUsage = HID_USAGE_GENERIC_MOUSE,
			.dwFlags = 0,
			.hwndTarget = mouse.window_handle()
		});
		if (!RegisterRawInputDevices(raw_input_devices.data(), ::size32(raw_input_devices), sizeof(RAWINPUTDEVICE)))
		{
			input_log.error("raw_mouse_handler: RegisterRawInputDevices failed: %s", fmt::win_error{GetLastError(), nullptr});
		}
	}
#endif

	type = mouse_handler::raw;
}

void raw_mouse_handler::enumerate_devices(u32 max_connect)
{
	input_log.notice("raw_mouse_handler: enumerating devices (max_connect=%d)", max_connect);

	m_raw_mice.clear();

	if (max_connect == 0)
	{
		return;
	}

#ifdef _WIN32
	u32 num_devices{};
	u32 res = GetRawInputDeviceList(nullptr, &num_devices, sizeof(RAWINPUTDEVICELIST));
	if (res == umax)
	{
		input_log.error("raw_mouse_handler: GetRawInputDeviceList (count) failed: %s", fmt::win_error{GetLastError(), nullptr});
		return;
	}

	if (num_devices == 0)
	{
		return;
	}

	std::vector<RAWINPUTDEVICELIST> device_list(num_devices);

	res = GetRawInputDeviceList(device_list.data(), &num_devices, sizeof(RAWINPUTDEVICELIST));
	if (res == umax)
	{
		input_log.error("raw_mouse_handler: GetRawInputDeviceList (fetch) failed: %s", fmt::win_error{GetLastError(), nullptr});
		return;
	}

	for (RAWINPUTDEVICELIST& device : device_list)
	{
		if (m_raw_mice.size() >= max_connect)
		{
			return;
		}

		if (device.dwType != RIM_TYPEMOUSE)
		{
			continue;
		}

		u32 size = 0;
		res = GetRawInputDeviceInfoW(device.hDevice, RIDI_DEVICENAME, nullptr, &size);
		if (res == umax)
		{
			input_log.error("raw_mouse_handler: GetRawInputDeviceInfoA (RIDI_DEVICENAME count) failed: %s", fmt::win_error{GetLastError(), nullptr});
			continue;
		}

		if (size == 0)
		{
			continue;
		}

		std::vector<wchar_t> buf(size);
		res = GetRawInputDeviceInfoW(device.hDevice, RIDI_DEVICENAME, buf.data(), &size);
		if (res == umax)
		{
			input_log.error("raw_mouse_handler: GetRawInputDeviceInfoA (RIDI_DEVICENAME fetch) failed: %s", fmt::win_error{GetLastError(), nullptr});
			continue;
		}

		const std::string device_name = wchar_to_utf8(buf.data());

		input_log.notice("raw_mouse_handler: adding device %d: '%s'", m_raw_mice.size(), device_name);

		m_raw_mice[device.hDevice] = raw_mouse(::size32(m_raw_mice), device_name, device.hDevice, this);
	}
#endif

	input_log.notice("raw_mouse_handler: found %d devices", m_raw_mice.size());
}

#ifdef _WIN32
void raw_mouse_handler::handle_native_event(const MSG& msg)
{
	if (msg.message != WM_INPUT)
	{
		return;
	}

	if (GET_RAWINPUT_CODE_WPARAM(msg.wParam) != RIM_INPUT)
	{
		return;
	}

	RAWINPUT raw_input{};
	UINT size = sizeof(RAWINPUT);

	u32 res = GetRawInputData(reinterpret_cast<HRAWINPUT>(msg.lParam), RID_INPUT, &raw_input, &size, sizeof(RAWINPUTHEADER));
	if (res == umax)
	{
		return;
	}

	switch (raw_input.header.dwType)
	{
	case RIM_TYPEMOUSE:
	{
		if (auto it = m_raw_mice.find(raw_input.header.hDevice); it != m_raw_mice.end())
		{
			it->second.update_values(raw_input.data.mouse);
		}
		break;
	}
	default:
	{
		break;
	}
	}
}
#endif
