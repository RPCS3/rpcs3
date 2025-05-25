#include "stdafx.h"
#include "gui_pad_thread.h"
#include "ds3_pad_handler.h"
#include "ds4_pad_handler.h"
#include "dualsense_pad_handler.h"
#include "skateboard_pad_handler.h"
#ifdef _WIN32
#include "xinput_pad_handler.h"
#include "mm_joystick_handler.h"
#elif HAVE_LIBEVDEV
#include "evdev_joystick_handler.h"
#endif
#ifdef HAVE_SDL3
#include "sdl_pad_handler.h"
#endif
#include "Emu/Io/PadHandler.h"
#include "Emu/system_config.h"
#include "rpcs3qt/gui_settings.h"

#ifdef __linux__
#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#define CHECK_IOCTRL_RET(res) if (res == -1) { gui_log.error("gui_pad_thread: ioctl failed (errno=%d=%s)", res, strerror(errno)); }
#elif defined(__APPLE__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wnullability-completeness"
#pragma GCC diagnostic ignored "-Wdeprecated-anon-enum-enum-conversion"
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#pragma GCC diagnostic pop
#endif

#include <QApplication>

LOG_CHANNEL(gui_log, "GUI");

atomic_t<bool> gui_pad_thread::m_reset = false;

gui_pad_thread::gui_pad_thread()
{
	m_thread = std::make_unique<named_thread<std::function<void()>>>("Gui Pad Thread", [this](){ run(); });
}

gui_pad_thread::~gui_pad_thread()
{
	// Join thread
	m_thread.reset();

#ifdef __linux__
	if (m_uinput_fd != 1)
	{
		gui_log.notice("gui_pad_thread: closing /dev/uinput");
		CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_DEV_DESTROY));
		int res = close(m_uinput_fd);
		if (res == -1)
		{
			gui_log.error("gui_pad_thread: Failed to close /dev/uinput (errno=%d=%s)", res, strerror(errno));
		}
		m_uinput_fd = -1;
	}
#endif
}

void gui_pad_thread::update_settings(const std::shared_ptr<gui_settings>& settings)
{
	ensure(!!settings);

	m_allow_global_input = settings->GetValue(gui::nav_global).toBool();
}

bool gui_pad_thread::init()
{
	m_handler.reset();
	m_pad.reset();

	// Initialize last button states as pressed to avoid unwanted button presses when starting the thread.
	m_last_button_state.fill(true);
	m_timestamp = steady_clock::now();
	m_initial_timestamp = steady_clock::now();
	m_last_auto_repeat_button = pad_button::pad_button_max_enum;

	g_cfg_input_configs.load();

	std::string active_config = g_cfg_input_configs.active_configs.get_value("");

	if (active_config.empty())
	{
		active_config = g_cfg_input_configs.active_configs.get_value(g_cfg_input_configs.global_key);
	}

	gui_log.notice("gui_pad_thread: Using input configuration: '%s'", active_config);

	// Load in order to get the pad handlers
	if (!g_cfg_input.load("", active_config))
	{
		gui_log.notice("gui_pad_thread: Loaded empty pad config");
	}

	// Adjust to the different pad handlers
	for (usz i = 0; i < g_cfg_input.player.size(); i++)
	{
		std::shared_ptr<PadHandlerBase> handler;
		gui_pad_thread::InitPadConfig(g_cfg_input.player[i]->config, g_cfg_input.player[i]->handler, handler);
	}

	// Reload with proper defaults
	if (!g_cfg_input.load("", active_config))
	{
		gui_log.notice("gui_pad_thread: Reloaded empty pad config");
	}

	gui_log.trace("gui_pad_thread: Using pad config:\n%s", g_cfg_input);

	for (u32 i = 0; i < CELL_PAD_MAX_PORT_NUM; i++) // max 7 pads
	{
		cfg_player* cfg = g_cfg_input.player[i];

		const pad_handler handler_type = cfg->handler.get();
		std::shared_ptr<PadHandlerBase> cur_pad_handler = GetHandler(handler_type);

		if (!cur_pad_handler)
		{
			continue;
		}

		cur_pad_handler->Init();

		m_handler = cur_pad_handler;
		m_pad = std::make_shared<Pad>(handler_type, i, CELL_PAD_STATUS_DISCONNECTED, CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_ACTUATOR, CELL_PAD_DEV_TYPE_STANDARD);

		if (!cur_pad_handler->bindPadToDevice(m_pad))
		{
			gui_log.error("gui_pad_thread: Failed to bind device '%s' to handler %s.", cfg->device.to_string(), handler_type);
		}

		gui_log.notice("gui_pad_thread: Pad %d: device='%s', handler=%s, VID=0x%x, PID=0x%x, class_type=0x%x, class_profile=0x%x",
			i, cfg->device.to_string(), m_pad->m_pad_handler, m_pad->m_vendor_id, m_pad->m_product_id, m_pad->m_class_type, m_pad->m_class_profile);

		if (handler_type != pad_handler::null)
		{
			input_log.notice("gui_pad_thread %d: config=\n%s", i, cfg->to_string());
		}

		// We only use one pad
		break;
	}

	if (!m_handler || !m_pad)
	{
		gui_log.notice("gui_pad_thread: No devices configured.");
		return false;
	}

#ifdef __linux__
	gui_log.notice("gui_pad_thread: opening /dev/uinput");

	m_uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (m_uinput_fd == -1)
	{
		gui_log.error("gui_pad_thread: Failed to open /dev/uinput (errno=%d=%s)", m_uinput_fd, strerror(errno));
		return false;
	}

	struct uinput_setup usetup{};
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0x1234;
	usetup.id.product = 0x1234;
	std::strcpy(usetup.name, "RPCS3 GUI Input Device");

	// The ioctls below will enable the device that is about to be created to pass events.
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_EVBIT, EV_KEY));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_KEYBIT, KEY_ESC));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_KEYBIT, KEY_ENTER));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_KEYBIT, KEY_BACKSPACE));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_KEYBIT, KEY_TAB));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_KEYBIT, KEY_LEFT));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_KEYBIT, KEY_RIGHT));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_KEYBIT, KEY_UP));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_KEYBIT, KEY_DOWN));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_KEYBIT, BTN_LEFT));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_KEYBIT, BTN_RIGHT));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_KEYBIT, BTN_MIDDLE));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_EVBIT, EV_REL));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_RELBIT, REL_X));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_RELBIT, REL_Y));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_RELBIT, REL_WHEEL));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_SET_RELBIT, REL_HWHEEL));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_DEV_SETUP, &usetup));
	CHECK_IOCTRL_RET(ioctl(m_uinput_fd, UI_DEV_CREATE));
#endif

	return true;
}

std::shared_ptr<PadHandlerBase> gui_pad_thread::GetHandler(pad_handler type)
{
	switch (type)
	{
	case pad_handler::null:
	case pad_handler::keyboard:
	case pad_handler::move:
		// Makes no sense to use this if we are in the GUI anyway
		return nullptr;
	case pad_handler::ds3:
		return std::make_shared<ds3_pad_handler>();
	case pad_handler::ds4:
		return std::make_shared<ds4_pad_handler>();
	case pad_handler::dualsense:
		return std::make_shared<dualsense_pad_handler>();
	case pad_handler::skateboard:
		return std::make_shared<skateboard_pad_handler>();
#ifdef _WIN32
	case pad_handler::xinput:
		return std::make_shared<xinput_pad_handler>();
	case pad_handler::mm:
		return std::make_shared<mm_joystick_handler>();
#endif
#ifdef HAVE_SDL3
	case pad_handler::sdl:
		return std::make_shared<sdl_pad_handler>();
#endif
#ifdef HAVE_LIBEVDEV
	case pad_handler::evdev:
		return std::make_shared<evdev_joystick_handler>();
#endif
	}

	return nullptr;
}

void gui_pad_thread::InitPadConfig(cfg_pad& cfg, pad_handler type, std::shared_ptr<PadHandlerBase>& handler)
{
	if (!handler)
	{
		handler = GetHandler(type);

		if (handler)
		{
			handler->init_config(&cfg);
		}
	}
}

void gui_pad_thread::run()
{
	gui_log.notice("gui_pad_thread: Pad thread started");

	m_reset = true;

	while (thread_ctrl::state() != thread_state::aborting)
	{
		if (m_reset && m_reset.exchange(false))
		{
			if (!init())
			{
				gui_log.warning("gui_pad_thread: Pad thread stopped (init failed during reset)");
				return;
			}
		}

		// Only process input if there is an active window
		if (m_handler && m_pad && (m_allow_global_input || QApplication::activeWindow()))
		{
			m_handler->process();

			if (thread_ctrl::state() == thread_state::aborting)
			{
				break;
			}

			process_input();
		}

		thread_ctrl::wait_for(10000);
	}

	gui_log.notice("gui_pad_thread: Pad thread stopped");
}

void gui_pad_thread::process_input()
{
	if (!m_pad || !m_pad->is_connected())
	{
		return;
	}

	constexpr u64 ms_threshold = 500;

	const auto on_button_pressed = [this](pad_button button_id, bool pressed, u16 value)
	{
		if (button_id == m_mouse_boost_button)
		{
			m_boost_mouse = pressed;
			return;
		}

		u16 key = 0;
		mouse_button btn = mouse_button::none;
		mouse_wheel wheel = mouse_wheel::none;
		float wheel_delta = 0.0f;
		const float wheel_multiplier = pressed ? (m_boost_mouse ? 10.0f : 1.0f) : 0.0f;
		const float move_multiplier = pressed ? (m_boost_mouse ? 40.0f : 20.0f) : 0.0f;

		switch (button_id)
		{
#ifdef _WIN32
		case pad_button::dpad_up: key = VK_UP; break;
		case pad_button::dpad_down: key = VK_DOWN; break;
		case pad_button::dpad_left: key = VK_LEFT; break;
		case pad_button::dpad_right: key = VK_RIGHT; break;
		case pad_button::circle: key = VK_ESCAPE; break;
		case pad_button::cross: key = VK_RETURN; break;
		case pad_button::square: key = VK_BACK; break;
		case pad_button::triangle: key = VK_TAB; break;
#elif defined(__linux__)
		case pad_button::dpad_up: key = KEY_UP; break;
		case pad_button::dpad_down: key = KEY_DOWN; break;
		case pad_button::dpad_left: key = KEY_LEFT; break;
		case pad_button::dpad_right: key = KEY_RIGHT; break;
		case pad_button::circle: key = KEY_ESC; break;
		case pad_button::cross: key = KEY_ENTER; break;
		case pad_button::square: key = KEY_BACKSPACE; break;
		case pad_button::triangle: key = KEY_TAB; break;
#elif defined (__APPLE__)
		case pad_button::dpad_up: key = kVK_UpArrow; break;
		case pad_button::dpad_down: key = kVK_DownArrow; break;
		case pad_button::dpad_left: key = kVK_LeftArrow; break;
		case pad_button::dpad_right: key = kVK_RightArrow; break;
		case pad_button::circle: key = kVK_Escape; break;
		case pad_button::cross: key = kVK_Return; break;
		case pad_button::square: key = kVK_Delete; break;
		case pad_button::triangle: key = kVK_Tab; break;
#endif
		case pad_button::L1: btn = mouse_button::left; break;
		case pad_button::R1: btn = mouse_button::right; break;
		case pad_button::rs_up: wheel = mouse_wheel::vertical; wheel_delta = 10.0f * wheel_multiplier; break;
		case pad_button::rs_down: wheel = mouse_wheel::vertical; wheel_delta = -10.0f * wheel_multiplier; break;
		case pad_button::rs_left: wheel = mouse_wheel::horizontal; wheel_delta = -10.0f * wheel_multiplier; break;
		case pad_button::rs_right: wheel = mouse_wheel::horizontal; wheel_delta = 10.0f * wheel_multiplier; break;
		case pad_button::ls_up: m_mouse_delta_y -= (abs(value - 128) / 255.f) * move_multiplier; break;
		case pad_button::ls_down: m_mouse_delta_y += (abs(value - 128) / 255.f) * move_multiplier; break;
		case pad_button::ls_left: m_mouse_delta_x -= (abs(value - 128) / 255.f) * move_multiplier; break;
		case pad_button::ls_right: m_mouse_delta_x += (abs(value - 128) / 255.f) * move_multiplier; break;
		default: return;
		}

		if (key)
		{
			send_key_event(key, pressed);
		}
		else if (btn != mouse_button::none)
		{
			send_mouse_button_event(btn, pressed);
		}
		else if (wheel != mouse_wheel::none && pressed)
		{
			send_mouse_wheel_event(wheel, wheel_delta);
		}
	};

	const auto handle_button_press = [&](pad_button button_id, bool pressed, u16 value)
	{
		if (button_id >= pad_button::pad_button_max_enum)
		{
			return;
		}

		bool& last_state = m_last_button_state[static_cast<u32>(button_id)];

		if (pressed)
		{
			const bool is_auto_repeat_button = m_auto_repeat_buttons.contains(button_id);
			const bool is_mouse_move_button = m_mouse_move_buttons.contains(button_id);

			if (!last_state)
			{
				if (button_id != m_mouse_boost_button && !is_mouse_move_button)
				{
					// The button was not pressed before, so this is a new button press. Reset auto-repeat.
					m_timestamp = steady_clock::now();
					m_initial_timestamp = m_timestamp;
					m_last_auto_repeat_button = is_auto_repeat_button ? button_id : pad_button::pad_button_max_enum;
				}

				on_button_pressed(static_cast<pad_button>(button_id), true, value);
			}
			else if (is_auto_repeat_button)
			{
				if (m_last_auto_repeat_button == button_id
				    && m_input_timer.GetMsSince(m_initial_timestamp) > ms_threshold
				    && m_input_timer.GetMsSince(m_timestamp) > m_auto_repeat_buttons.at(button_id))
				{
					// The auto-repeat button was pressed for at least the given threshold in ms and will trigger at an interval.
					m_timestamp = steady_clock::now();
					on_button_pressed(static_cast<pad_button>(button_id), true, value);
				}
				else if (m_last_auto_repeat_button == pad_button::pad_button_max_enum)
				{
					// An auto-repeat button was already pressed before and will now start triggering again after the next threshold.
					m_last_auto_repeat_button = button_id;
				}
			}
			else if (is_mouse_move_button)
			{
				on_button_pressed(static_cast<pad_button>(button_id), pressed, value);
			}
		}
		else if (last_state)
		{
			if (m_last_auto_repeat_button == button_id)
			{
				// We stopped pressing an auto-repeat button, so re-enable auto-repeat for other buttons.
				m_last_auto_repeat_button = pad_button::pad_button_max_enum;
			}

			on_button_pressed(static_cast<pad_button>(button_id), false, value);
		}

		last_state = pressed;
	};

	for (const auto& button : m_pad->m_buttons)
	{
		pad_button button_id = pad_button::pad_button_max_enum;
		if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL1)
		{
			switch (button.m_outKeyCode)
			{
			case CELL_PAD_CTRL_LEFT:
				button_id = pad_button::dpad_left;
				break;
			case CELL_PAD_CTRL_RIGHT:
				button_id = pad_button::dpad_right;
				break;
			case CELL_PAD_CTRL_DOWN:
				button_id = pad_button::dpad_down;
				break;
			case CELL_PAD_CTRL_UP:
				button_id = pad_button::dpad_up;
				break;
			case CELL_PAD_CTRL_L3:
				button_id = pad_button::L3;
				break;
			case CELL_PAD_CTRL_R3:
				button_id = pad_button::R3;
				break;
			case CELL_PAD_CTRL_SELECT:
				button_id = pad_button::select;
				break;
			case CELL_PAD_CTRL_START:
				button_id = pad_button::start;
				break;
			default:
				break;
			}
		}
		else if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL2)
		{
			switch (button.m_outKeyCode)
			{
			case CELL_PAD_CTRL_TRIANGLE:
				button_id = pad_button::triangle;
				break;
			case CELL_PAD_CTRL_CIRCLE:
				button_id = g_cfg.sys.enter_button_assignment == enter_button_assign::circle ? pad_button::cross : pad_button::circle;
				break;
			case CELL_PAD_CTRL_SQUARE:
				button_id = pad_button::square;
				break;
			case CELL_PAD_CTRL_CROSS:
				button_id = g_cfg.sys.enter_button_assignment == enter_button_assign::circle ? pad_button::circle : pad_button::cross;
				break;
			case CELL_PAD_CTRL_L1:
				button_id = pad_button::L1;
				break;
			case CELL_PAD_CTRL_R1:
				button_id = pad_button::R1;
				break;
			case CELL_PAD_CTRL_L2:
				button_id = pad_button::L2;
				break;
			case CELL_PAD_CTRL_R2:
				button_id = pad_button::R2;
				break;
			case CELL_PAD_CTRL_PS:
				button_id = pad_button::ps;
				break;
			default:
				break;
			}
		}

		handle_button_press(button_id, button.m_pressed, button.m_value);
	}

	for (const AnalogStick& stick : m_pad->m_sticks)
	{
		pad_button button_id = pad_button::pad_button_max_enum;
		pad_button release_id = pad_button::pad_button_max_enum;

		switch (stick.m_offset)
		{
		case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X:
			button_id = (stick.m_value <= 128) ? pad_button::ls_left : pad_button::ls_right;
			release_id = (stick.m_value > 128) ? pad_button::ls_left : pad_button::ls_right;
			break;
		case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y:
			button_id = (stick.m_value <= 128) ? pad_button::ls_up : pad_button::ls_down;
			release_id = (stick.m_value > 128) ? pad_button::ls_up : pad_button::ls_down;
			break;
		case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X:
			button_id = (stick.m_value <= 128) ? pad_button::rs_left : pad_button::rs_right;
			release_id = (stick.m_value > 128) ? pad_button::rs_left : pad_button::rs_right;
			break;
		case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y:
			button_id = (stick.m_value <= 128) ? pad_button::rs_up : pad_button::rs_down;
			release_id = (stick.m_value > 128) ? pad_button::rs_up : pad_button::rs_down;
			break;
		default:
			break;
		}

		bool pressed;

		if (m_mouse_move_buttons.contains(button_id))
		{
			// Mouse move sticks are always pressed if they surpass a tiny deadzone.
			constexpr int deadzone = 5;
			pressed = std::abs(stick.m_value - 128) > deadzone;
		}
		else
		{
			// Let's say other sticks are only pressed if they are almost completely tilted. Otherwise navigation feels really wacky.
			pressed = stick.m_value < 30 || stick.m_value > 225;
		}

		// Release other direction on the same axis first
		handle_button_press(release_id, false, stick.m_value);

		// Handle currently pressed stick direction
		handle_button_press(button_id, pressed, stick.m_value);
	}

	// Send mouse move event at the end to prevent redundant calls

	// Round to lower integer
	const int delta_x = m_mouse_delta_x;
	const int delta_y = m_mouse_delta_y;

	if (delta_x || delta_y)
	{
		// Remove integer part
		m_mouse_delta_x -= delta_x;
		m_mouse_delta_y -= delta_y;

		// Send event
		send_mouse_move_event(delta_x, delta_y);
	}
}

#ifdef __linux__
void gui_pad_thread::emit_event(int type, int code, int val)
{
	struct input_event ie{};
	ie.type = type;
	ie.code = code;
	ie.value = val;

	int res = write(m_uinput_fd, &ie, sizeof(ie));
	if (res == -1)
	{
		gui_log.error("gui_pad_thread::emit_event: write failed (errno=%d=%s)", res, strerror(errno));
	}
}
#endif

void gui_pad_thread::send_key_event(u32 key, bool pressed)
{
	gui_log.trace("gui_pad_thread::send_key_event: key=%d, pressed=%d", key, pressed);

#ifdef _WIN32
	INPUT input{};
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = key;

	if (!pressed)
	{
		input.ki.dwFlags = KEYEVENTF_KEYUP;
	}

	if (SendInput(1, &input, sizeof(INPUT)) != 1)
	{
		gui_log.error("gui_pad_thread: SendInput() failed: %s", fmt::win_error{GetLastError(), nullptr});
	}
#elif defined(__linux__)
	emit_event(EV_KEY, key, pressed ? 1 : 0);
	emit_event(EV_SYN, SYN_REPORT, 0);
#elif defined(__APPLE__)
	CGEventRef ev = CGEventCreateKeyboardEvent(NULL, static_cast<CGKeyCode>(key), pressed);
	if (!ev)
	{
		gui_log.error("gui_pad_thread: CGEventCreateKeyboardEvent() failed");
		return;
	}

	CGEventPost(kCGHIDEventTap, ev);
	CFRelease(ev);
#endif
}

void gui_pad_thread::send_mouse_button_event(mouse_button btn, bool pressed)
{
	gui_log.trace("gui_pad_thread::send_mouse_button_event: btn=%d, pressed=%d", static_cast<int>(btn), pressed);

#ifdef _WIN32
	INPUT input{};
	input.type = INPUT_MOUSE;

	switch (btn)
	{
	case mouse_button::none: return;
	case mouse_button::left: input.mi.dwFlags = pressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP; break;
	case mouse_button::right: input.mi.dwFlags = pressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP; break;
	case mouse_button::middle: input.mi.dwFlags = pressed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP; break;
	}

	if (SendInput(1, &input, sizeof(INPUT)) != 1)
	{
		gui_log.error("gui_pad_thread: SendInput() failed: %s", fmt::win_error{GetLastError(), nullptr});
	}
#elif defined(__linux__)
	int key = 0;

	switch (btn)
	{
	case mouse_button::none: return;
	case mouse_button::left: key = BTN_LEFT; break;
	case mouse_button::right: key = BTN_RIGHT; break;
	case mouse_button::middle: key = BTN_MIDDLE; break;
	}

	emit_event(EV_KEY, key, pressed ? 1 : 0);
	emit_event(EV_SYN, SYN_REPORT, 0);
#elif defined(__APPLE__)
	CGEventType type{};
	CGMouseButton mouse_btn{};

	switch (btn)
	{
	case mouse_button::none: return;
	case mouse_button::left:
		type = pressed ? kCGEventLeftMouseDown : kCGEventLeftMouseUp;
		mouse_btn = kCGMouseButtonLeft;
		break;
	case mouse_button::right:
		type = pressed ? kCGEventRightMouseDown : kCGEventRightMouseUp;
		mouse_btn = kCGMouseButtonRight;
		break;
	case mouse_button::middle:
		type = pressed ? kCGEventOtherMouseDown : kCGEventOtherMouseUp;
		mouse_btn = kCGMouseButtonCenter;
		break;
	}

	CGEventRef ev = CGEventCreateMouseEvent(NULL, type, CGPointMake(m_mouse_abs_x, m_mouse_abs_y), mouse_btn);
	if (!ev)
	{
		gui_log.error("gui_pad_thread: CGEventCreateMouseEvent() failed");
		return;
	}

	CGEventPost(kCGHIDEventTap, ev);
	CFRelease(ev);
#endif
}

void gui_pad_thread::send_mouse_wheel_event(mouse_wheel wheel, int delta)
{
	gui_log.trace("gui_pad_thread::send_mouse_wheel_event: wheel=%d, delta=%d", static_cast<int>(wheel), delta);

	if (!delta)
	{
		return;
	}

#ifdef _WIN32
	INPUT input{};
	input.type = INPUT_MOUSE;
	input.mi.mouseData = delta;

	switch (wheel)
	{
	case mouse_wheel::none: return;
	case mouse_wheel::vertical: input.mi.dwFlags = MOUSEEVENTF_WHEEL; break;
	case mouse_wheel::horizontal: input.mi.dwFlags = MOUSEEVENTF_HWHEEL; break;
	}

	if (SendInput(1, &input, sizeof(INPUT)) != 1)
	{
		gui_log.error("gui_pad_thread: SendInput() failed: %s", fmt::win_error{GetLastError(), nullptr});
	}
#elif defined(__linux__)
	int axis = 0;

	switch (wheel)
	{
	case mouse_wheel::none: return;
	case mouse_wheel::vertical: axis = REL_WHEEL; break;
	case mouse_wheel::horizontal: axis = REL_HWHEEL; break;
	}

	emit_event(EV_REL, axis, delta);
	emit_event(EV_SYN, SYN_REPORT, 0);
#elif defined(__APPLE__)
	int v_delta = 0;
	int h_delta = 0;

	switch (wheel)
	{
	case mouse_wheel::none: return;
	case mouse_wheel::vertical: v_delta = delta; break;
	case mouse_wheel::horizontal: h_delta = delta; break;
	}

	constexpr u32 wheel_count = 2;
	CGEventRef ev = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitPixel, wheel_count, v_delta, h_delta);
	if (!ev)
	{
		gui_log.error("gui_pad_thread: CGEventCreateScrollWheelEvent() failed");
		return;
	}

	CGEventPost(kCGHIDEventTap, ev);
	CFRelease(ev);
#endif
}

void gui_pad_thread::send_mouse_move_event(int delta_x, int delta_y)
{
	gui_log.trace("gui_pad_thread::send_mouse_move_event: delta_x=%d, delta_y=%d", delta_x, delta_y);

	if (!delta_x && !delta_y)
	{
		return;
	}

#ifdef _WIN32
	INPUT input{};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	input.mi.dx = delta_x;
	input.mi.dy = delta_y;

	if (SendInput(1, &input, sizeof(INPUT)) != 1)
	{
		gui_log.error("gui_pad_thread: SendInput() failed: %s", fmt::win_error{GetLastError(), nullptr});
	}
#elif defined(__linux__)
	if (delta_x) emit_event(EV_REL, REL_X, delta_x);
	if (delta_y) emit_event(EV_REL, REL_Y, delta_y);
	emit_event(EV_SYN, SYN_REPORT, 0);
#elif defined(__APPLE__)
	CGDirectDisplayID display = CGMainDisplayID();
	const usz width = CGDisplayPixelsWide(display);
	const usz height = CGDisplayPixelsHigh(display);
	const float mouse_abs_x = std::clamp(m_mouse_abs_x + delta_x, 0.0f, width - 1.0f);
	const float mouse_abs_y = std::clamp(m_mouse_abs_y + delta_y, 0.0f, height - 1.0f);

	if (m_mouse_abs_x == mouse_abs_x && m_mouse_abs_y == mouse_abs_y)
	{
		return;
	}

	m_mouse_abs_x = mouse_abs_x;
	m_mouse_abs_y = mouse_abs_y;

	CGEventRef ev = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, CGPointMake(m_mouse_abs_x, m_mouse_abs_y), {});
	if (!ev)
	{
		gui_log.error("gui_pad_thread: CGEventCreateMouseEvent() failed");
		return;
	}

	CGEventPost(kCGHIDEventTap, ev);
	CFRelease(ev);
#endif
}
