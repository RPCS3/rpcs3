#include "stdafx.h"
#include "raw_mouse_handler.h"

#include "Emu/RSX/GSRender.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/Io/interception.h"
#include "Input/raw_mouse_config.h"
#include "Utilities/Timer.h"

#ifdef _WIN32
#include <hidusage.h>
#endif

#define RAW_MOUSE_DEBUG_CURSOR_ENABLED 0
#if RAW_MOUSE_DEBUG_CURSOR_ENABLED
#include "Emu/RSX/Overlays/overlay_cursor.h"
static inline void draw_overlay_cursor(u32 index, s32 x_pos, s32 y_pos, s32 x_max, s32 y_max)
{
	const s16 x = static_cast<s16>(x_pos / (x_max / static_cast<f32>(rsx::overlays::overlay::virtual_width)));
	const s16 y = static_cast<s16>(y_pos / (y_max / static_cast<f32>(rsx::overlays::overlay::virtual_height)));

	const color4f color = { 1.0f, 0.0f, 0.0f, 1.0f };

	rsx::overlays::set_cursor(rsx::overlays::cursor_offset::last + index, x, y, color, 2'000'000, false);
}
#else
[[maybe_unused]] static inline void draw_overlay_cursor(u32, s32, s32, s32, s32) {}
#endif

const std::unordered_map<int, raw_mouse::mouse_button> raw_mouse::btn_pairs =
{
	{ 0, {}},
#ifdef _WIN32
	{ RI_MOUSE_BUTTON_1_UP, mouse_button{ RI_MOUSE_BUTTON_1_DOWN, RI_MOUSE_BUTTON_1_UP, 0, false }},
	{ RI_MOUSE_BUTTON_2_UP, mouse_button{ RI_MOUSE_BUTTON_2_DOWN, RI_MOUSE_BUTTON_2_UP, 0, false }},
	{ RI_MOUSE_BUTTON_3_UP, mouse_button{ RI_MOUSE_BUTTON_3_DOWN, RI_MOUSE_BUTTON_3_UP, 0, false }},
	{ RI_MOUSE_BUTTON_4_UP, mouse_button{ RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_4_UP, 0, false }},
	{ RI_MOUSE_BUTTON_5_UP, mouse_button{ RI_MOUSE_BUTTON_5_DOWN, RI_MOUSE_BUTTON_5_UP, 0, false }},
#endif
};

LOG_CHANNEL(input_log, "Input");

raw_mice_config g_cfg_raw_mouse;
shared_mutex g_registered_handlers_mutex;
u32 g_registered_handlers = 0;

raw_mouse::raw_mouse(u32 index, const std::string& device_name, void* handle, raw_mouse_handler* handler)
	: m_index(index), m_device_name(device_name), m_handle(handle), m_handler(handler)
{
	reload_config();
}

raw_mouse::~raw_mouse()
{
}

void raw_mouse::reload_config()
{
	m_buttons.clear();

	if (m_index < ::size32(g_cfg_raw_mouse.players))
	{
		if (const auto& player = ::at32(g_cfg_raw_mouse.players, m_index))
		{
			input_log.notice("Raw mouse config for player %d=\n%s", m_index, player->to_string());

			m_mouse_acceleration = static_cast<float>(player->mouse_acceleration.get()) / 100.0f;

			m_buttons[CELL_MOUSE_BUTTON_1] = get_mouse_button(player->mouse_button_1);
			m_buttons[CELL_MOUSE_BUTTON_2] = get_mouse_button(player->mouse_button_2);
			m_buttons[CELL_MOUSE_BUTTON_3] = get_mouse_button(player->mouse_button_3);
			m_buttons[CELL_MOUSE_BUTTON_4] = get_mouse_button(player->mouse_button_4);
			m_buttons[CELL_MOUSE_BUTTON_5] = get_mouse_button(player->mouse_button_5);
			m_buttons[CELL_MOUSE_BUTTON_6] = get_mouse_button(player->mouse_button_6);
			m_buttons[CELL_MOUSE_BUTTON_7] = get_mouse_button(player->mouse_button_7);
			m_buttons[CELL_MOUSE_BUTTON_8] = get_mouse_button(player->mouse_button_8);
		}
	}
}

void raw_mouse::set_index(u32 index)
{
	m_index = index;
	m_reload_requested = true;
}

raw_mouse::mouse_button raw_mouse::get_mouse_button(const cfg::string& button)
{
	const std::string value = button.to_string();

	if (const auto it = raw_mouse_button_map.find(value); it != raw_mouse_button_map.cend())
	{
		return ::at32(btn_pairs, it->second);
	}

	if (value.starts_with(raw_mouse_config::key_prefix))
	{
		s64 scan_code{};
		if (try_to_int64(&scan_code, value.substr(raw_mouse_config::key_prefix.size()), s32{smin}, s32{smax}))
		{
			return mouse_button{ 0, 0, static_cast<s32>(scan_code), true };
		}
	}

	return {};
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

void raw_mouse::center_cursor()
{
	m_pos_x = m_window_width / 2;
	m_pos_y = m_window_height / 2;
}

#ifdef _WIN32
void raw_mouse::update_values(const RAWMOUSE& state)
{
	ensure(m_handler != nullptr);

	// Update window handle and size
	update_window_handle();

	if (std::exchange(m_reload_requested, false))
	{
		reload_config();
	}

	if (m_handler->is_for_gui())
	{
		for (const auto& [up, btn] : btn_pairs)
		{
			// Only update the value if either down or up flags are present
			if ((state.usButtonFlags & btn.down))
			{
				m_handler->mouse_press_callback(m_device_name, up, true);
			}
			else if ((state.usButtonFlags & btn.up))
			{
				m_handler->mouse_press_callback(m_device_name, up, false);
			}
		}
		return;
	}

	// Get mouse buttons
	for (const auto& [button, btn] : m_buttons)
	{
		if (btn.is_key) continue;

		// Only update the value if either down or up flags are present
		if ((state.usButtonFlags & btn.down))
		{
			m_handler->Button(m_index, button, true);
		}
		else if ((state.usButtonFlags & btn.up))
		{
			m_handler->Button(m_index, button, false);
		}
	}

	// Get mouse wheel
	if ((state.usButtonFlags & RI_MOUSE_WHEEL))
	{
		m_handler->Scroll(m_index, 0, std::clamp(static_cast<s16>(state.usButtonData) / WHEEL_DELTA, -128, 127));
	}
	else if ((state.usButtonFlags & RI_MOUSE_HWHEEL))
	{
		m_handler->Scroll(m_index, std::clamp(static_cast<s16>(state.usButtonData) / WHEEL_DELTA, -128, 127), 0);
	}

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

			// Adjust mouse position if the window size changed
			if (m_window_width_old != m_window_width || m_window_height_old != m_window_height)
			{
				m_pos_x = static_cast<int>(std::round((m_pos_x / static_cast<f32>(m_window_width_old)) * m_window_width));
				m_pos_y = static_cast<int>(std::round((m_pos_y / static_cast<f32>(m_window_height_old)) * m_window_height));

				m_window_width_old = m_window_width;
				m_window_height_old = m_window_height;
			}

			// Calculate new position
			m_pos_x = std::max(0, std::min(m_pos_x + delta_x, m_window_width - 1));
			m_pos_y = std::max(0, std::min(m_pos_y + delta_y, m_window_height - 1));

			// Move mouse relative to old position
			m_handler->Move(m_index, m_pos_x, m_pos_y, m_window_width, m_window_height, true, delta_x, delta_y);
			draw_overlay_cursor(m_index, m_pos_x, m_pos_y, m_window_width, m_window_height);
		}
	}
}

void raw_mouse::update_values(s32 scan_code, bool pressed)
{
	ensure(m_handler != nullptr);

	if (std::exchange(m_reload_requested, false))
	{
		reload_config();
	}

	if (m_handler->is_for_gui())
	{
		m_handler->key_press_callback(m_device_name, scan_code, pressed);
		return;
	}

	// Get mouse buttons
	for (const auto& [button, btn] : m_buttons)
	{
		if (!btn.is_key || btn.scan_code != scan_code) return;

		m_handler->Button(m_index, button, pressed);
	}
}
#endif

raw_mouse_handler::~raw_mouse_handler()
{
	// Join thread
	m_thread.reset();

#ifdef _WIN32
	unregister_raw_input_devices();
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

	m_mice.clear();
	m_mice.resize(max_connect);

	// Get max device index
	std::set<u32> connected_mice{};
	const u32 now_connect = get_now_connect(connected_mice);

	// Init mouse info
	m_info = {};
	m_info.max_connect = max_connect;
	m_info.now_connect = std::min(now_connect, max_connect);
	m_info.info = input::g_mice_intercepted ? CELL_MOUSE_INFO_INTERCEPTED : 0; // Ownership of mouse data: 0=Application, 1=System

	for (u32 i = 0; i < m_info.max_connect; i++)
	{
		m_info.status[i] = connected_mice.contains(i) ? CELL_MOUSE_STATUS_CONNECTED : CELL_MOUSE_STATUS_DISCONNECTED;
		m_info.mode[i] = CELL_MOUSE_INFO_TABLET_MOUSE_MODE;
		m_info.tablet_is_supported[i] = CELL_MOUSE_INFO_TABLET_NOT_SUPPORTED;
		m_info.vendor_id[0] = 0x1234;
		m_info.product_id[0] = 0x1234;
	}

	type = mouse_handler::raw;

	if (m_is_for_gui)
	{
		// No need for a thread. We call update_devices manually.
		return;
	}

	m_thread = std::make_unique<named_thread<std::function<void()>>>("Raw Mouse Thread", [this]()
	{
		input_log.notice("raw_mouse_handler: thread started");

		while (thread_ctrl::state() != thread_state::aborting)
		{
			update_devices();

			thread_ctrl::wait_for(1'000'000);
		}

		input_log.notice("raw_mouse_handler: thread stopped");
	});
}

void raw_mouse_handler::update_devices()
{
	u32 max_connect;
	{
		std::lock_guard lock(mutex);
		max_connect = m_info.max_connect;
	}

	{
		std::map<void*, raw_mouse> enumerated = enumerate_devices(max_connect);

		std::lock_guard lock(m_raw_mutex);

		if (m_is_for_gui)
		{
			// Use the new devices
			m_raw_mice = std::move(enumerated);
		}
		else
		{
			// Integrate the new devices
			std::map<void*, raw_mouse> updated_mice;

			for (u32 i = 0; i < std::min(max_connect, ::size32(g_cfg_raw_mouse.players)); i++)
			{
				const auto& player = ::at32(g_cfg_raw_mouse.players, i);
				const std::string device_name = player->device.to_string();

				// Check if the configured device for this player is connected
				if (const auto it = std::find_if(enumerated.begin(), enumerated.end(), [&device_name](const auto& entry){ return entry.second.device_name() == device_name; });
					it != enumerated.cend())
				{
					// Check if the device was already known
					const auto it_exists = m_raw_mice.find(it->first);
					const bool exists = it_exists != m_raw_mice.cend();

					// Copy by value to allow for the same device for multiple players
					raw_mouse& mouse = updated_mice[it->first];
					mouse = exists ? it_exists->second : it->second;
					mouse.set_index(i);

					if (!exists)
					{
						// Initialize and center mouse
						mouse.update_window_handle();
						mouse.center_cursor();

						input_log.notice("raw_mouse_handler: added new device for player %d: '%s'", i, device_name);
					}
				}
			}

			// Log disconnected devices
			for (const auto& [handle, mouse] : m_raw_mice)
			{
				if (!updated_mice.contains(handle))
				{
					input_log.notice("raw_mouse_handler: removed device for player %d: '%s'", mouse.index(), mouse.device_name());
				}
			}

			m_raw_mice = std::move(updated_mice);
		}
	}

	// Update mouse info
	std::set<u32> connected_mice{};
	const u32 now_connect = get_now_connect(connected_mice);
	{
		std::lock_guard lock(mutex);

		m_info.now_connect = std::min(now_connect, m_info.max_connect);

		for (u32 i = 0; i < m_info.max_connect; i++)
		{
			m_info.status[i] = connected_mice.contains(i) ? CELL_MOUSE_STATUS_CONNECTED : CELL_MOUSE_STATUS_DISCONNECTED;
		}
	}

#ifdef _WIN32
	register_raw_input_devices();
#endif
}

#ifdef _WIN32
void raw_mouse_handler::register_raw_input_devices()
{
	std::lock_guard lock(m_raw_mutex);

	if (m_registered_raw_input_devices || !m_info.max_connect || m_raw_mice.empty())
	{
		return;
	}

	// Get the window handle of the first mouse
	raw_mouse& mouse = m_raw_mice.begin()->second;

	std::vector<RAWINPUTDEVICE> raw_input_devices;
	raw_input_devices.push_back(RAWINPUTDEVICE {
		.usUsagePage = HID_USAGE_PAGE_GENERIC,
		.usUsage = HID_USAGE_GENERIC_MOUSE,
		.dwFlags = 0,
		.hwndTarget = mouse.window_handle()
	});
	raw_input_devices.push_back(RAWINPUTDEVICE {
		.usUsagePage = HID_USAGE_PAGE_GENERIC,
		.usUsage = HID_USAGE_GENERIC_KEYBOARD,
		.dwFlags = 0,
		.hwndTarget = mouse.window_handle()
	});

	{
		std::lock_guard lock(g_registered_handlers_mutex);

		if (!RegisterRawInputDevices(raw_input_devices.data(), ::size32(raw_input_devices), sizeof(RAWINPUTDEVICE)))
		{
			input_log.error("raw_mouse_handler: RegisterRawInputDevices failed: %s", fmt::win_error{GetLastError(), nullptr});
			return;
		}

		g_registered_handlers++;
	}

	m_registered_raw_input_devices = true;
}

void raw_mouse_handler::unregister_raw_input_devices() const
{
	if (!m_registered_raw_input_devices)
	{
		return;
	}

	std::lock_guard lock(g_registered_handlers_mutex);

	if (!g_registered_handlers || (--g_registered_handlers > 0))
	{
		input_log.notice("raw_mouse_handler: Skip unregistering devices. %d other handlers are still registered.", g_registered_handlers);
		return;
	}

	input_log.notice("raw_mouse_handler: Unregistering devices");

	std::vector<RAWINPUTDEVICE> raw_input_devices;
	raw_input_devices.push_back(RAWINPUTDEVICE {
		.usUsagePage = HID_USAGE_PAGE_GENERIC,
		.usUsage = HID_USAGE_GENERIC_MOUSE,
		.dwFlags = RIDEV_REMOVE,
		.hwndTarget = nullptr
	});
	raw_input_devices.push_back(RAWINPUTDEVICE {
		.usUsagePage = HID_USAGE_PAGE_GENERIC,
		.usUsage = HID_USAGE_GENERIC_KEYBOARD,
		.dwFlags = 0,
		.hwndTarget = nullptr
	});

	if (!RegisterRawInputDevices(raw_input_devices.data(), ::size32(raw_input_devices), sizeof(RAWINPUTDEVICE)))
	{
		input_log.error("raw_mouse_handler: RegisterRawInputDevices (unregister) failed: %s", fmt::win_error{GetLastError(), nullptr});
	}
}
#endif

u32 raw_mouse_handler::get_now_connect(std::set<u32>& connected_mice)
{
	u32 now_connect = 0;
	connected_mice.clear();
	{
		std::lock_guard lock(m_raw_mutex);

		for (const auto& [handle, mouse] : m_raw_mice)
		{
			now_connect = std::max(now_connect, mouse.index() + 1);
			connected_mice.insert(mouse.index());
		}
	}
	return now_connect;
}

std::map<void*, raw_mouse> raw_mouse_handler::enumerate_devices(u32 max_connect)
{
	input_log.trace("raw_mouse_handler: enumerating devices (max_connect=%d)", max_connect);

	std::map<void*, raw_mouse> raw_mice;

	if (max_connect == 0)
	{
		return raw_mice;
	}

	Timer timer{};

#ifdef _WIN32
	u32 num_devices{};
	u32 res = GetRawInputDeviceList(nullptr, &num_devices, sizeof(RAWINPUTDEVICELIST));
	if (res == umax)
	{
		input_log.error("raw_mouse_handler: GetRawInputDeviceList (count) failed: %s", fmt::win_error{GetLastError(), nullptr});
		return raw_mice;
	}

	if (num_devices == 0)
	{
		return raw_mice;
	}

	std::vector<RAWINPUTDEVICELIST> device_list(num_devices);

	res = GetRawInputDeviceList(device_list.data(), &num_devices, sizeof(RAWINPUTDEVICELIST));
	if (res == umax)
	{
		input_log.error("raw_mouse_handler: GetRawInputDeviceList (fetch) failed: %s", fmt::win_error{GetLastError(), nullptr});
		return raw_mice;
	}

	for (RAWINPUTDEVICELIST& device : device_list)
	{
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
		input_log.trace("raw_mouse_handler: found device: '%s'", device_name);

		raw_mice[device.hDevice] = raw_mouse(::size32(raw_mice), device_name, device.hDevice, this);
	}
#endif

	input_log.trace("raw_mouse_handler: found %d devices in %f ms", raw_mice.size(), timer.GetElapsedTimeInMilliSec());
	return raw_mice;
}

#ifdef _WIN32
void raw_mouse_handler::handle_native_event(const MSG& msg)
{
	if (m_info.max_connect == 0)
	{
		// Not initialized
		return;
	}

	if (msg.message != WM_INPUT && msg.message != WM_KEYDOWN && msg.message != WM_KEYUP)
	{
		return;
	}

	if (GET_RAWINPUT_CODE_WPARAM(msg.wParam) != RIM_INPUT)
	{
		return;
	}

	RAWINPUT raw_input{};
	UINT size = sizeof(RAWINPUT);

	const u32 res = GetRawInputData(reinterpret_cast<HRAWINPUT>(msg.lParam), RID_INPUT, &raw_input, &size, sizeof(RAWINPUTHEADER));
	if (res == umax)
	{
		return;
	}

	if ((raw_input.header.dwType == RIM_TYPEMOUSE || raw_input.header.dwType == RIM_TYPEKEYBOARD) &&
		g_cfg_raw_mouse.reload_requested.exchange(false))
	{
		std::lock_guard lock(m_raw_mutex);

		for (auto& [handle, mouse] : m_raw_mice)
		{
			mouse.request_reload();
		}
	}

	switch (raw_input.header.dwType)
	{
	case RIM_TYPEMOUSE:
	{
		std::lock_guard lock(m_raw_mutex);

		if (auto it = m_raw_mice.find(raw_input.header.hDevice); it != m_raw_mice.end())
		{
			it->second.update_values(raw_input.data.mouse);
		}
		break;
	}
	case RIM_TYPEKEYBOARD:
	{
		const RAWKEYBOARD& keyboard = raw_input.data.keyboard;

		// Ignore key overrun state and keys not mapped to any virtual key code
		if (keyboard.MakeCode == KEYBOARD_OVERRUN_MAKE_CODE || keyboard.VKey >= UCHAR_MAX)
		{
			break;
		}

		WORD scan_code;

		if (keyboard.MakeCode)
		{
			// Compose the full scan code value with its extended byte
			scan_code = MAKEWORD(keyboard.MakeCode & 0x7f, ((keyboard.Flags & RI_KEY_E0) ? 0xe0 : ((keyboard.Flags & RI_KEY_E1) ? 0xe1 : 0x00)));
		}
		else
		{
			// Scan code value may be empty for some buttons (for example multimedia buttons)
			// Try to get the scan code from the virtual key code
			scan_code = LOWORD(MapVirtualKey(keyboard.VKey, MAPVK_VK_TO_VSC_EX));
		}

		const LONG scan_code_extended = static_cast<LONG>(MAKELPARAM(0, (HIBYTE(scan_code) ? KF_EXTENDED : 0x00) | LOBYTE(scan_code)));
		const bool pressed = !(keyboard.Flags & RI_KEY_BREAK);

		std::lock_guard lock(m_raw_mutex);

		for (auto& [handle, mouse] : m_raw_mice)
		{
			mouse.update_values(scan_code_extended, pressed);
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
