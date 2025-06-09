#include "stdafx.h"
#include "pad_thread.h"
#include "product_info.h"
#include "ds3_pad_handler.h"
#include "ds4_pad_handler.h"
#include "dualsense_pad_handler.h"
#include "skateboard_pad_handler.h"
#include "ps_move_handler.h"
#ifdef _WIN32
#include "xinput_pad_handler.h"
#include "mm_joystick_handler.h"
#elif HAVE_LIBEVDEV
#include "evdev_joystick_handler.h"
#endif
#ifdef HAVE_SDL3
#include "sdl_pad_handler.h"
#endif
#ifndef ANDROID
#include "keyboard_pad_handler.h"
#endif
#include "Emu/Io/Null/NullPadHandler.h"
#include "Emu/Io/interception.h"
#include "Emu/Io/PadHandler.h"
#include "Emu/Io/pad_config.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/RSX/Overlays/HomeMenu/overlay_home_menu.h"
#include "Emu/RSX/Overlays/overlay_message.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/Cell/Modules/cellGem.h"
#include "Emu/Cell/timers.hpp"
#include "Utilities/Thread.h"
#include "util/atomic.hpp"

LOG_CHANNEL(sys_log, "SYS");

extern void pad_state_notify_state_change(usz index, u32 state);
extern bool is_input_allowed();
extern std::string g_input_config_override;

namespace pad
{
	atomic_t<pad_thread*> g_pad_thread = nullptr;
	shared_mutex g_pad_mutex;
	std::string g_title_id;
	atomic_t<bool> g_started{false};
	atomic_t<bool> g_reset{false};
	atomic_t<bool> g_enabled{true};
	atomic_t<bool> g_home_menu_requested{false};
}

namespace rsx
{
	void set_native_ui_flip();
}

struct pad_setting
{
	u32 port_status = 0;
	u32 device_capability = 0;
	u32 device_type = 0;
	u32 class_type = 0;
	u16 vendor_id = 0;
	u16 product_id = 0;
	bool is_ldd_pad = false;
};

pad_thread::pad_thread(void* curthread, void* curwindow, std::string_view title_id) : m_curthread(curthread), m_curwindow(curwindow)
{
	pad::g_title_id = title_id;
	pad::g_pad_thread = this;
	pad::g_started = false;
}

pad_thread::~pad_thread()
{
	pad::g_pad_thread = nullptr;
}

void pad_thread::Init()
{
	std::lock_guard lock(pad::g_pad_mutex);

	// Cache old settings if possible
	std::array<pad_setting, CELL_PAD_MAX_PORT_NUM> pad_settings;
	for (u32 i = 0; i < CELL_PAD_MAX_PORT_NUM; i++) // max 7 pads
	{
		if (m_pads[i])
		{
			pad_settings[i] =
			{
				m_pads[i]->m_port_status,
				m_pads[i]->m_device_capability,
				m_pads[i]->m_device_type,
				m_pads[i]->m_class_type,
				m_pads[i]->m_vendor_id,
				m_pads[i]->m_product_id,
				m_pads[i]->ldd
			};
		}
		else
		{
			pad_settings[i] =
			{
				CELL_PAD_STATUS_DISCONNECTED,
				CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_ACTUATOR,
				CELL_PAD_DEV_TYPE_STANDARD,
				CELL_PAD_PCLASS_TYPE_STANDARD,
				0,
				0,
				false
			};
		}
	}

	num_ldd_pad = 0;

	m_info.now_connect = 0;

	m_handlers.clear();

	g_cfg_input_configs.load();

	std::string active_config = g_cfg_input_configs.active_configs.get_value(pad::g_title_id);

	if (active_config.empty())
	{
		active_config = g_cfg_input_configs.active_configs.get_value(g_cfg_input_configs.global_key);
	}

	input_log.notice("Using input configuration: '%s' (override='%s')", active_config, g_input_config_override);

	// Load in order to get the pad handlers
	if (!g_cfg_input.load(pad::g_title_id, active_config))
	{
		input_log.notice("Loaded empty pad config");
	}

	// Adjust to the different pad handlers
	for (usz i = 0; i < g_cfg_input.player.size(); i++)
	{
		std::shared_ptr<PadHandlerBase> handler;
		pad_thread::InitPadConfig(g_cfg_input.player[i]->config, g_cfg_input.player[i]->handler, handler);
	}

	// Reload with proper defaults
	if (!g_cfg_input.load(pad::g_title_id, active_config))
	{
		input_log.notice("Reloaded empty pad config");
	}

	input_log.trace("Using pad config:\n%s", g_cfg_input);

#ifndef ANDROID
	std::shared_ptr<keyboard_pad_handler> keyptr;
#endif

	// Always have a Null Pad Handler
	std::shared_ptr<NullPadHandler> nullpad = std::make_shared<NullPadHandler>();
	m_handlers.emplace(pad_handler::null, nullpad);

	for (u32 i = 0; i < CELL_PAD_MAX_PORT_NUM; i++) // max 7 pads
	{
		cfg_player* cfg = g_cfg_input.player[i];
		std::shared_ptr<PadHandlerBase> cur_pad_handler;

		const pad_handler handler_type = pad_settings[i].is_ldd_pad ? pad_handler::null : cfg->handler.get();

		if (m_handlers.contains(handler_type))
		{
			cur_pad_handler = m_handlers[handler_type];
		}
		else
		{
			if (handler_type == pad_handler::keyboard)
			{
#ifndef ANDROID
				keyptr = std::make_shared<keyboard_pad_handler>();
				keyptr->moveToThread(static_cast<QThread*>(m_curthread));
				keyptr->SetTargetWindow(static_cast<QWindow*>(m_curwindow));
				cur_pad_handler = keyptr;
#else
				cur_pad_handler = nullpad;
#endif
			}
			else
			{
				cur_pad_handler = GetHandler(handler_type);
			}
			m_handlers.emplace(handler_type, cur_pad_handler);
		}
		cur_pad_handler->Init();

		std::shared_ptr<Pad> pad = std::make_shared<Pad>(handler_type, i, CELL_PAD_STATUS_DISCONNECTED, pad_settings[i].device_capability, pad_settings[i].device_type);
		m_pads[i] = pad;

		if (pad_settings[i].is_ldd_pad)
		{
			InitLddPad(i, &pad_settings[i].port_status);
		}
		else
		{
			if (!cur_pad_handler->bindPadToDevice(pad))
			{
				// Failed to bind the device to cur_pad_handler so binds to NullPadHandler
				input_log.error("Failed to bind device '%s' to handler %s. Falling back to NullPadHandler.", cfg->device.to_string(), handler_type);
				nullpad->bindPadToDevice(pad);
			}

			input_log.notice("Pad %d: device='%s', handler=%s, VID=0x%x, PID=0x%x, class_type=0x%x, class_profile=0x%x",
				i, cfg->device.to_string(), pad->m_pad_handler, pad->m_vendor_id, pad->m_product_id, pad->m_class_type, pad->m_class_profile);

			if (pad->m_pad_handler != pad_handler::null)
			{
				input_log.notice("Pad %d: config=\n%s", i, cfg->to_string());
			}

			// If the user changes the emulated controller, then simulate unplugging and plugging in a new controller
			if (m_pads_connected[i] && (pad_settings[i].class_type != pad->m_class_type || pad_settings[i].vendor_id != pad->m_vendor_id || pad_settings[i].product_id != pad->m_product_id))
			{
				pad->m_disconnection_timer = get_system_time() + 30'000ull;
			}
		}

		pad->is_fake_pad = ((g_cfg.io.move == move_handler::real || g_cfg.io.move == move_handler::fake) && i >= (static_cast<u32>(CELL_PAD_MAX_PORT_NUM) - static_cast<u32>(CELL_GEM_MAX_NUM)))
			|| (pad->m_class_type >= CELL_PAD_FAKE_TYPE_FIRST && pad->m_class_type < CELL_PAD_FAKE_TYPE_LAST);
		connect_usb_controller(i, input::get_product_by_vid_pid(pad->m_vendor_id, pad->m_product_id));
	}

	// Set copilots
	for (usz i = 0; i < m_pads.size(); i++)
	{
		auto& pad = m_pads[i];
		if (!pad)
			continue;

		pad->copilots.clear();

		if (pad->is_copilot())
			continue;

		for (usz j = 0; j < m_pads.size(); j++)
		{
			auto& other = m_pads[j];
			if (i == j || !other || other->copilot_player() != i)
				continue;

			pad->copilots.push_back(other);
		}
	}

	// Initialize active mouse and keyboard. Activate pad handler if one exists.
	input::set_mouse_and_keyboard(m_handlers.contains(pad_handler::keyboard) ? input::active_mouse_and_keyboard::pad : input::active_mouse_and_keyboard::emulated);
}

void pad_thread::SetRumble(u32 pad, u8 large_motor, bool small_motor)
{
	if (pad >= m_pads.size() || !m_pads[pad])
		return;

	const u64 now_us = get_system_time();

	m_pads[pad]->m_last_rumble_time_us = now_us;
	m_pads[pad]->m_vibrateMotors[0].m_value = large_motor;
	m_pads[pad]->m_vibrateMotors[1].m_value = small_motor ? 255 : 0;

	// Rumble copilots as well
	for (const auto& copilot : m_pads[pad]->copilots)
	{
		if (copilot && copilot->is_connected())
		{
			copilot->m_last_rumble_time_us = now_us;
			copilot->m_vibrateMotors[0].m_value = large_motor;
			copilot->m_vibrateMotors[1].m_value = small_motor ? 255 : 0;
		}
	}
}

void pad_thread::SetIntercepted(bool intercepted)
{
	if (intercepted)
	{
		m_info.system_info |= CELL_PAD_INFO_INTERCEPTED;
		m_info.ignore_input = true;
	}
	else
	{
		m_info.system_info &= ~CELL_PAD_INFO_INTERCEPTED;
	}
}

void pad_thread::apply_copilots()
{
	const auto normalize = [](s32 value)
	{
		return (value - 128) / 127.0f;
	};

	std::lock_guard lock(pad::g_pad_mutex);

	for (auto& pad : m_pads)
	{
		if (!pad || !pad->is_connected())
		{
			continue;
		}

		pad->m_buttons_external.resize(pad->m_buttons.size());

		for (usz i = 0; i < pad->m_buttons.size(); i++)
		{
			const Button& src = pad->m_buttons[i];
			Button& dst = pad->m_buttons_external[i];

			dst.m_offset = src.m_offset;
			dst.m_outKeyCode = src.m_outKeyCode;
			dst.m_value = src.m_value;
			dst.m_pressed = src.m_pressed;
		}

		for (usz i = 0; i < pad->m_sticks.size(); i++)
		{
			const AnalogStick& src = pad->m_sticks[i];
			AnalogStick& dst = pad->m_sticks_external[i];

			dst.m_offset = src.m_offset;
			dst.m_value = src.m_value;
		}

		if (pad->copilots.empty() || pad->is_copilot())
		{
			continue;
		}

		// Merge buttons
		for (const auto& copilot : pad->copilots)
		{
			if (!copilot || !copilot->is_connected())
			{
				continue;
			}

			for (Button& button : pad->m_buttons_external)
			{
				for (const Button& other : copilot->m_buttons)
				{
					if (button.m_offset == other.m_offset && button.m_outKeyCode == other.m_outKeyCode)
					{
						if (other.m_pressed)
						{
							button.m_pressed = true;

							if (button.m_value < other.m_value)
							{
								button.m_value = other.m_value;
							}
						}

						break;
					}
				}
			}
		}

		// Merge sticks
		for (AnalogStick& stick : pad->m_sticks_external)
		{
			f32 accumulated_value = normalize(stick.m_value);

			for (const auto& copilot : pad->copilots)
			{
				if (!copilot || !copilot->is_connected())
				{
					continue;
				}

				for (const AnalogStick& other : copilot->m_sticks)
				{
					if (stick.m_offset == other.m_offset)
					{
						accumulated_value += normalize(other.m_value);
						break;
					}
				}
			}

			stick.m_value = static_cast<u16>(std::round(std::clamp(accumulated_value * 127.0f + 128.0f, 0.0f, 255.0f)));
		}
	}
}

void pad_thread::update_pad_states()
{
	for (usz i = 0; i < m_pads.size(); i++)
	{
		const auto& pad = m_pads[i];

		// Simulate unplugging and plugging in a new controller
		if (pad && pad->m_disconnection_timer > 0)
		{
			const bool is_connected = pad->is_connected();
			const u64 now = get_system_time();

			if (is_connected && now < pad->m_disconnection_timer)
			{
				pad->m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
				pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
			}
			else if (!is_connected && now >= pad->m_disconnection_timer)
			{
				pad->m_port_status |= CELL_PAD_STATUS_CONNECTED + CELL_PAD_STATUS_ASSIGN_CHANGES;
				pad->m_disconnection_timer = 0;
			}
		}

		const bool connected = pad && !pad->is_fake_pad && pad->is_connected();

		if (m_pads_connected[i] == connected)
			continue;

		pad_state_notify_state_change(i, connected ? CELL_PAD_STATUS_CONNECTED : CELL_PAD_STATUS_DISCONNECTED);

		m_pads_connected[i] = connected;
	}
}

void pad_thread::operator()()
{
	Init();

	const bool is_vsh = Emu.IsVsh();

	pad::g_reset = false;
	pad::g_started = true;

	bool mode_changed = true;

	atomic_t<pad_handler_mode> pad_mode{g_cfg.io.pad_mode.get()};
	std::vector<std::unique_ptr<named_thread<std::function<void()>>>> threads;

	const auto stop_threads = [&threads]()
	{
		input_log.notice("Stopping pad threads...");

		for (auto& thread : threads)
		{
			// Join thread (ordered explicitly)
			thread.reset();
		}

		threads.clear();

		input_log.notice("Pad threads stopped");
	};

	const auto start_threads = [this, &threads, &pad_mode]()
	{
		if (pad_mode == pad_handler_mode::single_threaded)
		{
			return;
		}

		input_log.notice("Starting pad threads...");

#if defined(__APPLE__)
		// Let's keep hid handlers on the same thread
		std::vector<std::shared_ptr<PadHandlerBase>> hid_handlers;
		std::vector<std::shared_ptr<PadHandlerBase>> handlers;

		for (const auto& [type, handler] : m_handlers)
		{
			switch (type)
			{
			case pad_handler::null:
				break;
			case pad_handler::ds3:
			case pad_handler::ds4:
			case pad_handler::dualsense:
			case pad_handler::skateboard:
			case pad_handler::move:
				hid_handlers.push_back(handler);
				break;
			default:
				handlers.push_back(handler);
				break;
			}
		}

		if (!hid_handlers.empty())
		{
			threads.push_back(std::make_unique<named_thread<std::function<void()>>>("HID Thread", [handlers = std::move(hid_handlers)]()
			{
				while (thread_ctrl::state() != thread_state::aborting)
				{
					if (!pad::g_enabled || !is_input_allowed())
					{
						thread_ctrl::wait_for(30'000);
						continue;
					}

					for (auto& handler : handlers)
					{
						handler->process();
					}

					u64 pad_sleep = g_cfg.io.pad_sleep;

					if (Emu.IsPaused())
					{
						pad_sleep = std::max<u64>(pad_sleep, 30'000);
					}

					thread_ctrl::wait_for(pad_sleep);
				}
			}));
		}

		for (const auto& handler : handlers)
		{
#else
		for (const auto& [type, handler] : m_handlers)
		{
			if (type == pad_handler::null) continue;
#endif
			threads.push_back(std::make_unique<named_thread<std::function<void()>>>(fmt::format("%s Thread", handler->m_type), [handler]()
			{
				while (thread_ctrl::state() != thread_state::aborting)
				{
					if (!pad::g_enabled || !is_input_allowed())
					{
						thread_ctrl::wait_for(30'000);
						continue;
					}

					handler->process();

					u64 pad_sleep = g_cfg.io.pad_sleep;

					if (Emu.IsPaused())
					{
						pad_sleep = std::max<u64>(pad_sleep, 30'000);
					}

					thread_ctrl::wait_for(pad_sleep);
				}
			}));
		}

		input_log.notice("Pad threads started");
	};

	while (thread_ctrl::state() != thread_state::aborting)
	{
		if (!pad::g_enabled || !is_input_allowed())
		{
			m_resume_emulation_flag = false;
			m_mask_start_press_to_resume = 0;
			thread_ctrl::wait_for(30'000);
			continue;
		}

		// Update variables
		const bool needs_reset = pad::g_reset && pad::g_reset.exchange(false);
		const pad_handler_mode new_pad_mode = g_cfg.io.pad_mode.get();
		mode_changed |= new_pad_mode != pad_mode.exchange(new_pad_mode);

		// Reset pad handlers if necessary
		if (needs_reset || mode_changed)
		{
			mode_changed = false;

			stop_threads();

			if (needs_reset)
			{
				Init();
			}
			else
			{
				input_log.success("The pad mode was changed to %s", pad_mode.load());
			}

			start_threads();
		}

		u32 connected_devices = 0;

		if (pad_mode == pad_handler_mode::single_threaded)
		{
			for (auto& handler : m_handlers)
			{
				handler.second->process();
				connected_devices += handler.second->connected_devices;
			}
		}
		else
		{
			for (auto& handler : m_handlers)
			{
				connected_devices += handler.second->connected_devices;
			}
		}

		apply_copilots();

		if (Emu.IsRunning())
		{
			update_pad_states();
		}

		m_info.now_connect = connected_devices + num_ldd_pad;

		// The ignore_input section is only reached when a dialog was closed and the pads are still intercepted.
		// As long as any of the listed buttons is pressed, cellPadGetData will ignore all input (needed for Hotline Miami).
		// ignore_input was added because if we keep the pads intercepted, then some games will enter the menu due to unexpected system interception (tested with Ninja Gaiden Sigma).
		if (m_info.ignore_input && !(m_info.system_info & CELL_PAD_INFO_INTERCEPTED))
		{
			bool any_button_pressed = false;

			for (usz i = 0; i < m_pads.size() && !any_button_pressed; i++)
			{
				const auto& pad = m_pads[i];

				if (!pad->is_connected())
					continue;

				for (const auto& button : pad->m_buttons)
				{
					if (button.m_pressed && (
						button.m_outKeyCode == CELL_PAD_CTRL_CROSS ||
						button.m_outKeyCode == CELL_PAD_CTRL_CIRCLE ||
						button.m_outKeyCode == CELL_PAD_CTRL_TRIANGLE ||
						button.m_outKeyCode == CELL_PAD_CTRL_SQUARE ||
						button.m_outKeyCode == CELL_PAD_CTRL_START ||
						button.m_outKeyCode == CELL_PAD_CTRL_SELECT))
					{
						any_button_pressed = true;
						break;
					}
				}
			}

			if (!any_button_pressed)
			{
				m_info.ignore_input = false;
			}
		}

		// Handle home menu if requested
		if (!is_vsh && !m_home_menu_open && Emu.IsRunning())
		{
			bool ps_button_pressed = false;

			for (usz i = 0; i < m_pads.size() && !ps_button_pressed; i++)
			{
				if (i > 0 && g_cfg.io.lock_overlay_input_to_player_one)
					break;

				const auto& pad = m_pads[i];

				if (!pad->is_connected())
					continue;

				// Check if an LDD pad pressed the PS button (bit 0 of the first button)
				// NOTE: Rock Band 3 doesn't seem to care about the len. It's always 0.
				if (pad->ldd /*&& pad->ldd_data.len >= 1 */&& !!(pad->ldd_data.button[0] & CELL_PAD_CTRL_LDD_PS))
				{
					ps_button_pressed = true;
					break;
				}

				for (const auto& button : pad->m_buttons)
				{
					if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL1 && button.m_outKeyCode == CELL_PAD_CTRL_PS && button.m_pressed)
					{
						ps_button_pressed = true;
						break;
					}
				}
			}

			// Make sure we call this function only once per button press
			if ((ps_button_pressed && !m_ps_button_pressed) || pad::g_home_menu_requested.exchange(false))
			{
				open_home_menu();
			}

			m_ps_button_pressed = ps_button_pressed;
		}

		// Handle paused emulation (if triggered by home menu).
		if (m_home_menu_open && g_cfg.misc.pause_during_home_menu)
		{
			// Reset resume control if the home menu is open
			m_resume_emulation_flag = false;
			m_mask_start_press_to_resume = 0;
			m_track_start_press_begin_timestamp = 0;

			// Update UI
			rsx::set_native_ui_flip();
			thread_ctrl::wait_for(33'000);
			continue;
		}

		if (m_resume_emulation_flag)
		{
			m_resume_emulation_flag = false;

			Emu.BlockingCallFromMainThread([]()
			{
				Emu.Resume();
			});
		}

		u64 pad_sleep = g_cfg.io.pad_sleep;

		if (Emu.GetStatus(false) == system_state::paused)
		{
			pad_sleep = std::max<u64>(pad_sleep, 30'000);

			u64 timestamp = get_system_time();
			u32 pressed_mask = 0;

			for (usz i = 0; i < m_pads.size(); i++)
			{
				const auto& pad = m_pads[i];

				if (!pad->is_connected())
					continue;

				for (const auto& button : pad->m_buttons)
				{
					if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL1 && button.m_outKeyCode == CELL_PAD_CTRL_START && button.m_pressed)
					{
						pressed_mask |= 1u << i;
						break;
					}
				}
			}

			m_mask_start_press_to_resume &= pressed_mask;

			if (!pressed_mask || timestamp - m_track_start_press_begin_timestamp >= 700'000)
			{
				m_track_start_press_begin_timestamp = timestamp;

				if (std::exchange(m_mask_start_press_to_resume, u32{umax}))
				{
					m_mask_start_press_to_resume = 0;
					m_track_start_press_begin_timestamp = 0;

					sys_log.success("Resuming emulation using the START button in a few seconds...");

					auto msg_ref = std::make_shared<atomic_t<u32>>(1);
					rsx::overlays::queue_message(localized_string_id::EMULATION_RESUMING, 2'000'000, msg_ref);

					m_resume_emulation_flag = true;

					for (u32 i = 0; i < 40; i++)
					{
						if (Emu.GetStatus(false) != system_state::paused)
						{
							// Abort if emulation has been resumed by other means
							m_resume_emulation_flag = false;
							msg_ref->release(0);
							break;
						}

						thread_ctrl::wait_for(50'000);
						rsx::set_native_ui_flip();
					}
				}
			}
		}
		else
		{
			// Reset resume control if caught a state of unpaused emulation
			m_mask_start_press_to_resume = 0;
			m_track_start_press_begin_timestamp = 0;
		}

		thread_ctrl::wait_for(pad_sleep);
	}

	stop_threads();
}

void pad_thread::InitLddPad(u32 handle, const u32* port_status)
{
	if (handle >= m_pads.size())
	{
		return;
	}

	static const input::product_info product = input::get_product_info(input::product_type::playstation_3_controller);

	auto& pad = m_pads[handle];
	pad->ldd = true;
	pad->Init
	(
		port_status ? *port_status : CELL_PAD_STATUS_CONNECTED | CELL_PAD_STATUS_ASSIGN_CHANGES | CELL_PAD_STATUS_CUSTOM_CONTROLLER,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY,
		CELL_PAD_DEV_TYPE_LDD,
		CELL_PAD_PCLASS_TYPE_STANDARD,
		product.pclass_profile,
		product.vendor_id,
		product.product_id,
		50
	);

	input_log.notice("Pad %d: LDD, VID=0x%x, PID=0x%x, class_type=0x%x, class_profile=0x%x",
		handle, pad->m_vendor_id, pad->m_product_id, pad->m_class_type, pad->m_class_profile);

	num_ldd_pad++;
}

s32 pad_thread::AddLddPad()
{
	// Look for first null pad
	for (u32 i = 0; i < CELL_PAD_MAX_PORT_NUM; i++)
	{
		if (g_cfg_input.player[i]->handler == pad_handler::null && !m_pads[i]->ldd)
		{
			InitLddPad(i, nullptr);
			return i;
		}
	}

	return -1;
}

void pad_thread::UnregisterLddPad(u32 handle)
{
	auto& pad = ::at32(m_pads, handle);

	pad->ldd = false;
	pad->m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
	pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;

	num_ldd_pad--;
}

std::shared_ptr<PadHandlerBase> pad_thread::GetHandler(pad_handler type)
{
	switch (type)
	{
	case pad_handler::null:
		return std::make_shared<NullPadHandler>();
	case pad_handler::keyboard:
#ifdef ANDROID
		return std::make_shared<NullPadHandler>();
#else
		return std::make_shared<keyboard_pad_handler>();
#endif
	case pad_handler::ds3:
		return std::make_shared<ds3_pad_handler>();
	case pad_handler::ds4:
		return std::make_shared<ds4_pad_handler>();
	case pad_handler::dualsense:
		return std::make_shared<dualsense_pad_handler>();
	case pad_handler::skateboard:
		return std::make_shared<skateboard_pad_handler>();
	case pad_handler::move:
		return std::make_shared<ps_move_handler>();
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

void pad_thread::InitPadConfig(cfg_pad& cfg, pad_handler type, std::shared_ptr<PadHandlerBase>& handler)
{
	if (!handler)
	{
		handler = GetHandler(type);
	}

	ensure(!!handler);
	handler->init_config(&cfg);
}

extern bool send_open_home_menu_cmds();
extern void send_close_home_menu_cmds();
extern bool close_osk_from_ps_button();

void pad_thread::open_home_menu()
{
	// Check if the OSK is open and can be closed
	if (!close_osk_from_ps_button())
	{
		rsx::overlays::queue_message(get_localized_string(localized_string_id::CELL_OSK_DIALOG_BUSY));
		return;
	}

	if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
	{
		if (m_home_menu_open.exchange(true))
		{
			return;
		}

		if (!send_open_home_menu_cmds())
		{
			m_home_menu_open = false;
			return;
		}

		input_log.notice("opening home menu...");

		const error_code result = manager->create<rsx::overlays::home_menu_dialog>()->show([this](s32 status)
		{
			input_log.notice("closing home menu with status %d", status);

			m_home_menu_open = false;

			send_close_home_menu_cmds();
		});

		(result ? input_log.error : input_log.notice)("opened home menu with result %d", s32{result});
	}
}
