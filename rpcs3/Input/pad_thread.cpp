#include "stdafx.h"
#include "pad_thread.h"
#include "ds3_pad_handler.h"
#include "ds4_pad_handler.h"
#ifdef _WIN32
#include "xinput_pad_handler.h"
#include "mm_joystick_handler.h"
#elif HAVE_LIBEVDEV
#include "evdev_joystick_handler.h"
#endif
#include "keyboard_pad_handler.h"
#include "Emu/Io/Null/NullPadHandler.h"
#include "Emu/Io/PadHandler.h"
#include "Emu/Io/pad_config.h"

LOG_CHANNEL(input_log, "Input");

namespace pad
{
	atomic_t<pad_thread*> g_current = nullptr;
	std::recursive_mutex g_pad_mutex;
	std::string g_title_id;
}

struct pad_setting
{
	u32 port_status;
	u32 device_capability;
	u32 device_type;
};

pad_thread::pad_thread(void *_curthread, void *_curwindow, std::string_view title_id) : curthread(_curthread), curwindow(_curwindow)
{
	pad::g_title_id = title_id;
	Init();

	thread = std::make_shared<std::thread>(&pad_thread::ThreadFunc, this);
	pad::g_current = this;
}

pad_thread::~pad_thread()
{
	pad::g_current = nullptr;
	active = false;
	thread->join();

	handlers.clear();
}

void pad_thread::Init()
{
	std::lock_guard lock(pad::g_pad_mutex);

	// Cache old settings if possible
	std::vector<pad_setting> pad_settings;
	for (u32 i = 0; i < CELL_PAD_MAX_PORT_NUM; i++) // max 7 pads
	{
		if (!m_pads[i])
		{
			pad_settings.push_back({ CELL_PAD_STATUS_DISCONNECTED, CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_ACTUATOR, CELL_PAD_DEV_TYPE_STANDARD });
		}
		else
		{
			pad_settings.push_back({ m_pads[i]->m_port_status, m_pads[i]->m_device_capability, m_pads[i]->m_device_type });
		}
	}

	m_info.now_connect = 0;

	handlers.clear();

	g_cfg_input.load(pad::g_title_id);

	std::shared_ptr<keyboard_pad_handler> keyptr;

	// Always have a Null Pad Handler
	std::shared_ptr<NullPadHandler> nullpad = std::make_shared<NullPadHandler>();
	handlers.emplace(pad_handler::null, nullpad);

	for (u32 i = 0; i < CELL_PAD_MAX_PORT_NUM; i++) // max 7 pads
	{
		std::shared_ptr<PadHandlerBase> cur_pad_handler;

		const auto &handler_type = g_cfg_input.player[i]->handler;

		if (handlers.count(handler_type) != 0)
		{
			cur_pad_handler = handlers[handler_type];
		}
		else
		{
			switch (handler_type)
			{
			case pad_handler::keyboard:
				keyptr = std::make_shared<keyboard_pad_handler>();
				keyptr->moveToThread(static_cast<QThread*>(curthread));
				keyptr->SetTargetWindow(static_cast<QWindow*>(curwindow));
				cur_pad_handler = keyptr;
				break;
			case pad_handler::ds3:
				cur_pad_handler = std::make_shared<ds3_pad_handler>();
				break;
			case pad_handler::ds4:
				cur_pad_handler = std::make_shared<ds4_pad_handler>();
				break;
#ifdef _WIN32
			case pad_handler::xinput:
				cur_pad_handler = std::make_shared<xinput_pad_handler>();
				break;
			case pad_handler::mm:
				cur_pad_handler = std::make_shared<mm_joystick_handler>();
				break;
#endif
#ifdef HAVE_LIBEVDEV
			case pad_handler::evdev:
				cur_pad_handler = std::make_shared<evdev_joystick_handler>();
				break;
#endif
			default:
				break;
			}
			handlers.emplace(handler_type, cur_pad_handler);
		}
		cur_pad_handler->Init();

		m_pads[i] = std::make_shared<Pad>(CELL_PAD_STATUS_DISCONNECTED, pad_settings[i].device_capability, pad_settings[i].device_type);

		if (cur_pad_handler->bindPadToDevice(m_pads[i], g_cfg_input.player[i]->device.to_string()) == false)
		{
			// Failed to bind the device to cur_pad_handler so binds to NullPadHandler
			input_log.error("Failed to bind device %s to handler %s", g_cfg_input.player[i]->device.to_string(), handler_type.to_string());
			nullpad->bindPadToDevice(m_pads[i], g_cfg_input.player[i]->device.to_string());
		}
	}
}

void pad_thread::SetRumble(const u32 pad, u8 largeMotor, bool smallMotor)
{
	if (pad > m_pads.size())
		return;

	if (m_pads[pad]->m_vibrateMotors.size() >= 2)
	{
		m_pads[pad]->m_vibrateMotors[0].m_value = largeMotor;
		m_pads[pad]->m_vibrateMotors[1].m_value = smallMotor ? 255 : 0;
	}
}

void pad_thread::Reset(std::string_view title_id)
{
	pad::g_title_id = title_id;
	reset = active.load();
}

void pad_thread::SetEnabled(bool enabled)
{
	is_enabled = enabled;
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

void pad_thread::ThreadFunc()
{
	active = true;
	while (active)
	{
		if (!is_enabled)
		{
			std::this_thread::sleep_for(1ms);
			continue;
		}

		if (reset && reset.exchange(false))
		{
			Init();
		}

		u32 connected_devices = 0;

		for (auto& cur_pad_handler : handlers)
		{
			cur_pad_handler.second->ThreadProc();
			connected_devices += cur_pad_handler.second->connected_devices;
		}

		m_info.now_connect = connected_devices + num_ldd_pad;

		// The following section is only reached when a dialog was closed and the pads are still intercepted.
		// As long as any of the listed buttons is pressed, cellPadGetData will ignore all input (needed for Hotline Miami).
		// ignore_input was added because if we keep the pads intercepted, then some games will enter the menu due to unexpected system interception (tested with Ninja Gaiden Sigma).
		if (!(m_info.system_info & CELL_PAD_INFO_INTERCEPTED) && m_info.ignore_input)
		{
			bool any_button_pressed = false;

			for (const auto& pad : m_pads)
			{
				if (pad->m_port_status & CELL_PAD_STATUS_CONNECTED)
				{
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

					if (any_button_pressed)
					{
						break;
					}
				}
			}

			if (!any_button_pressed)
			{
				m_info.ignore_input = false;
			}
		}

		std::this_thread::sleep_for(1ms);
	}
}

s32 pad_thread::AddLddPad()
{
	// Look for first null pad
	for (u32 i = 0; i < CELL_PAD_MAX_PORT_NUM; i++)
	{
		if (g_cfg_input.player[i]->handler == pad_handler::null)
		{
			m_pads[i]->ldd = true;
			num_ldd_pad++;
			return i;
		}
	}

	return -1;
}

void pad_thread::UnregisterLddPad(u32 handle)
{
	m_pads[handle]->ldd = false;
	num_ldd_pad--;
}
