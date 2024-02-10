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
#ifdef HAVE_SDL2
#include "sdl_pad_handler.h"
#endif
#include "Emu/Io/PadHandler.h"
#include "Emu/Io/pad_config.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Utilities/Thread.h"
#include "rpcs3qt/gui_settings.h"

#include <QApplication>

LOG_CHANNEL(gui_log, "GUI");

gui_pad_thread::gui_pad_thread()
{
	m_thread = std::make_unique<std::thread>(&gui_pad_thread::run, this);
}

gui_pad_thread::~gui_pad_thread()
{
	m_terminate = true;

	if (m_thread && m_thread->joinable())
	{
		m_thread->join();
		m_thread.reset();
	}
}

void gui_pad_thread::update_settings(const std::shared_ptr<gui_settings>& settings)
{
	ensure(!!settings);

	m_allow_global_input = settings->GetValue(gui::nav_global).toBool();
}

void gui_pad_thread::Init()
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
		std::shared_ptr<PadHandlerBase> cur_pad_handler;

		const pad_handler handler_type = cfg->handler.get();

		switch (handler_type)
		{
		case pad_handler::ds3:
			cur_pad_handler = std::make_shared<ds3_pad_handler>();
			break;
		case pad_handler::ds4:
			cur_pad_handler = std::make_shared<ds4_pad_handler>();
			break;
		case pad_handler::dualsense:
			cur_pad_handler = std::make_shared<dualsense_pad_handler>();
			break;
		case pad_handler::skateboard:
			cur_pad_handler = std::make_shared<skateboard_pad_handler>();
			break;
#ifdef _WIN32
		case pad_handler::xinput:
			cur_pad_handler = std::make_shared<xinput_pad_handler>();
			break;
		case pad_handler::mm:
			cur_pad_handler = std::make_shared<mm_joystick_handler>();
			break;
#endif
#ifdef HAVE_SDL2
		case pad_handler::sdl:
			cur_pad_handler = std::make_shared<sdl_pad_handler>();
			break;
#endif
#ifdef HAVE_LIBEVDEV
		case pad_handler::evdev:
			cur_pad_handler = std::make_shared<evdev_joystick_handler>();
			break;
#endif
		case pad_handler::keyboard:
		case pad_handler::null:
			// Makes no sense to use this if we are in the GUI anyway
			continue;
		}

		cur_pad_handler->Init();

		m_handler = cur_pad_handler;
		m_pad = std::make_shared<Pad>(handler_type, CELL_PAD_STATUS_DISCONNECTED, CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_ACTUATOR, CELL_PAD_DEV_TYPE_STANDARD);

		if (!cur_pad_handler->bindPadToDevice(m_pad, i))
		{
			gui_log.error("gui_pad_thread: Failed to bind device '%s' to handler %s.", cfg->device.to_string(), handler_type);
		}

		gui_log.notice("gui_pad_thread: Pad %d: device='%s', handler=%s, VID=0x%x, PID=0x%x, class_type=0x%x, class_profile=0x%x",
			i, cfg->device.to_string(), m_pad->m_pad_handler, m_pad->m_vendor_id, m_pad->m_product_id, m_pad->m_class_type, m_pad->m_class_profile);

		// We only use one pad
		break;
	}
}

std::shared_ptr<PadHandlerBase> gui_pad_thread::GetHandler(pad_handler type)
{
	switch (type)
	{
	case pad_handler::null:
	case pad_handler::keyboard:
		// Makes no sense to use this if we are in the GUI anyway
		return nullptr;
	case pad_handler::ds3:
		return std::make_unique<ds3_pad_handler>();
	case pad_handler::ds4:
		return std::make_unique<ds4_pad_handler>();
	case pad_handler::dualsense:
		return std::make_unique<dualsense_pad_handler>();
	case pad_handler::skateboard:
		return std::make_unique<skateboard_pad_handler>();
#ifdef _WIN32
	case pad_handler::xinput:
		return std::make_unique<xinput_pad_handler>();
	case pad_handler::mm:
		return std::make_unique<mm_joystick_handler>();
#endif
#ifdef HAVE_SDL2
	case pad_handler::sdl:
		return std::make_unique<sdl_pad_handler>();
#endif
#ifdef HAVE_LIBEVDEV
	case pad_handler::evdev:
		return std::make_unique<evdev_joystick_handler>();
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
	thread_base::set_name("Gui Pad Thread");

	gui_log.notice("gui_pad_thread: Pad thread started");

	Init();

	while (!m_terminate)
	{
		// Only process input if there is an active window
		if (m_handler && m_pad && (m_allow_global_input || QApplication::activeWindow()))
		{
			m_handler->process();

			if (m_terminate)
			{
				break;
			}

			process_input();
		}

		if (m_terminate)
		{
			break;
		}

		std::this_thread::sleep_for(10ms);
	}

	gui_log.notice("gui_pad_thread: Pad thread stopped");
}

void gui_pad_thread::process_input()
{
	if (!m_pad || !(m_pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
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

	for (const Button& button : m_pad->m_buttons)
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
	case mouse_button::none:
		return;
	case mouse_button::left:
		input.mi.dwFlags = pressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
		break;
	case mouse_button::right:
		input.mi.dwFlags = pressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
		break;
	case mouse_button::middle:
		input.mi.dwFlags = pressed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
		break;
	}

	if (SendInput(1, &input, sizeof(INPUT)) != 1)
	{
		gui_log.error("gui_pad_thread: SendInput() failed: %s", fmt::win_error{GetLastError(), nullptr});
	}
#endif
}

void gui_pad_thread::send_mouse_wheel_event(mouse_wheel wheel, float delta)
{
	gui_log.trace("gui_pad_thread::send_mouse_wheel_event: wheel=%d, delta=%f", static_cast<int>(wheel), delta);

#ifdef _WIN32
	INPUT input{};
	input.type = INPUT_MOUSE;
	input.mi.mouseData = delta;

	switch (wheel)
	{
	case mouse_wheel::none:
		return;
	case mouse_wheel::vertical:
		input.mi.dwFlags = MOUSEEVENTF_WHEEL;
		break;
	case mouse_wheel::horizontal:
		input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
		break;
	}

	if (SendInput(1, &input, sizeof(INPUT)) != 1)
	{
		gui_log.error("gui_pad_thread: SendInput() failed: %s", fmt::win_error{GetLastError(), nullptr});
	}
#endif
}

void gui_pad_thread::send_mouse_move_event(float delta_x, float delta_y)
{
	gui_log.trace("gui_pad_thread::send_mouse_move_event: delta_x=%f, delta_y=%f", delta_x, delta_y);

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
#endif
}
