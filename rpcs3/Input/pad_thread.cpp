#include "stdafx.h"
#include "pad_thread.h"
#include "product_info.h"
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
#include "keyboard_pad_handler.h"
#include "Emu/Io/Null/NullPadHandler.h"
#include "Emu/Io/PadHandler.h"
#include "Emu/Io/pad_config.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/RSX/Overlays/HomeMenu/overlay_home_menu.h"
#include "Emu/RSX/Overlays/overlay_message.h"
#include "Utilities/Thread.h"
#include "util/atomic.hpp"

LOG_CHANNEL(sys_log, "SYS");

extern bool is_input_allowed();
extern std::string g_input_config_override;

namespace pad
{
	atomic_t<pad_thread*> g_current = nullptr;
	shared_mutex g_pad_mutex;
	std::string g_title_id;
	atomic_t<bool> g_started{false};
	atomic_t<bool> g_reset{false};
	atomic_t<bool> g_enabled{true};
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
	bool is_ldd_pad = false;
};

pad_thread::pad_thread(void* curthread, void* curwindow, std::string_view title_id) : m_curthread(curthread), m_curwindow(curwindow)
{
	pad::g_title_id = title_id;
	pad::g_current = this;
	pad::g_started = false;
}

pad_thread::~pad_thread()
{
	pad::g_current = nullptr;
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
				false
			};
		}
	}

	num_ldd_pad = 0;

	m_info.now_connect = 0;

	handlers.clear();

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

	std::shared_ptr<keyboard_pad_handler> keyptr;

	// Always have a Null Pad Handler
	std::shared_ptr<NullPadHandler> nullpad = std::make_shared<NullPadHandler>();
	handlers.emplace(pad_handler::null, nullpad);

	for (u32 i = 0; i < CELL_PAD_MAX_PORT_NUM; i++) // max 7 pads
	{
		cfg_player* cfg = g_cfg_input.player[i];
		std::shared_ptr<PadHandlerBase> cur_pad_handler;

		const pad_handler handler_type = pad_settings[i].is_ldd_pad ? pad_handler::null : cfg->handler.get();

		if (handlers.contains(handler_type))
		{
			cur_pad_handler = handlers[handler_type];
		}
		else
		{
			switch (handler_type)
			{
			case pad_handler::keyboard:
				keyptr = std::make_shared<keyboard_pad_handler>();
				keyptr->moveToThread(static_cast<QThread*>(m_curthread));
				keyptr->SetTargetWindow(static_cast<QWindow*>(m_curwindow));
				cur_pad_handler = keyptr;
				break;
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
			case pad_handler::null:
				break;
			}
			handlers.emplace(handler_type, cur_pad_handler);
		}
		cur_pad_handler->Init();

		m_pads[i] = std::make_shared<Pad>(handler_type, CELL_PAD_STATUS_DISCONNECTED, pad_settings[i].device_capability, pad_settings[i].device_type);

		if (pad_settings[i].is_ldd_pad)
		{
			InitLddPad(i, &pad_settings[i].port_status);
		}
		else
		{
			if (!cur_pad_handler->bindPadToDevice(m_pads[i], i))
			{
				// Failed to bind the device to cur_pad_handler so binds to NullPadHandler
				input_log.error("Failed to bind device '%s' to handler %s. Falling back to NullPadHandler.", cfg->device.to_string(), handler_type);
				nullpad->bindPadToDevice(m_pads[i], i);
			}

			input_log.notice("Pad %d: device='%s', handler=%s, VID=0x%x, PID=0x%x, class_type=0x%x, class_profile=0x%x",
				i, cfg->device.to_string(), m_pads[i]->m_pad_handler, m_pads[i]->m_vendor_id, m_pads[i]->m_product_id, m_pads[i]->m_class_type, m_pads[i]->m_class_profile);
		}
	}
}

void pad_thread::SetRumble(const u32 pad, u8 large_motor, bool small_motor)
{
	if (pad >= m_pads.size())
		return;

	if (m_pads[pad]->m_vibrateMotors.size() >= 2)
	{
		m_pads[pad]->m_vibrateMotors[0].m_value = large_motor;
		m_pads[pad]->m_vibrateMotors[1].m_value = small_motor ? 255 : 0;
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
			if (thread)
			{
				auto& enumeration_thread = *thread;
				enumeration_thread = thread_state::aborting;
				enumeration_thread();
			}
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

		for (const auto& handler : handlers)
		{
			if (handler.first == pad_handler::null)
			{
				continue;
			}

			threads.push_back(std::make_unique<named_thread<std::function<void()>>>(fmt::format("%s Thread", handler.second->m_type), [&handler = handler.second, &pad_mode]()
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
			for (auto& handler : handlers)
			{
				handler.second->process();
				connected_devices += handler.second->connected_devices;
			}
		}
		else
		{
			for (auto& handler : handlers)
			{
				connected_devices += handler.second->connected_devices;
			}
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

				if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
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

				if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
					continue;

				// TODO: this keeps opening the home menu. Find out how to do it properly.
				// Check if an LDD pad pressed the PS button (bit 0 of the first button)
				if (false && pad->ldd && !!(pad->ldd_data.button[0] & CELL_PAD_CTRL_LDD_PS))
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
			if (ps_button_pressed && !m_ps_button_pressed)
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

				if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
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

	m_pads[handle]->ldd = true;
	m_pads[handle]->Init
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
		handle, m_pads[handle]->m_vendor_id, m_pads[handle]->m_product_id, m_pads[handle]->m_class_type, m_pads[handle]->m_class_profile);

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
	ensure(handle < m_pads.size());

	m_pads[handle]->ldd = false;

	num_ldd_pad--;
}

std::shared_ptr<PadHandlerBase> pad_thread::GetHandler(pad_handler type)
{
	switch (type)
	{
	case pad_handler::null:
		return std::make_unique<NullPadHandler>();
	case pad_handler::keyboard:
		return std::make_unique<keyboard_pad_handler>();
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
