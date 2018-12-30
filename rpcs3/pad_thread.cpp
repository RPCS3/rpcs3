#include "pad_thread.h"
#include "ds4_pad_handler.h"
#ifdef _WIN32
#include "xinput_pad_handler.h"
#include "mm_joystick_handler.h"
#elif HAVE_LIBEVDEV
#include "evdev_joystick_handler.h"
#endif
#include "keyboard_pad_handler.h"
#include "Emu/Io/Null/NullPadHandler.h"

namespace pad
{
	atomic_t<pad_thread*> g_current = nullptr;
	std::recursive_mutex g_pad_mutex;
}

struct pad_setting
{
	u32 port_status;
	u32 device_capability;
	u32 device_type;
};

pad_thread::pad_thread(void *_curthread, void *_curwindow) : curthread(_curthread), curwindow(_curwindow)
{
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
		if (i >= m_pads.size())
		{
			pad_settings.push_back({ CELL_PAD_STATUS_DISCONNECTED, CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_ACTUATOR, CELL_PAD_DEV_TYPE_STANDARD });
		}
		else
		{
			pad_settings.push_back({ m_pads[i]->m_port_status, m_pads[i]->m_device_capability, m_pads[i]->m_device_type });
		}
	}

	const u32 system_info = m_info.system_info;
	std::memset(&m_info, 0, sizeof(m_info));
	m_info.now_connect = 0;
	m_info.system_info |= system_info;

	m_pads.clear();
	handlers.clear();

	g_cfg_input.load();

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
				keyptr->moveToThread((QThread *)curthread);
				keyptr->SetTargetWindow((QWindow *)curwindow);
				cur_pad_handler = keyptr;
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

		m_pads.push_back(std::make_shared<Pad>(CELL_PAD_STATUS_DISCONNECTED, pad_settings[i].device_capability, pad_settings[i].device_type));

		if (cur_pad_handler->bindPadToDevice(m_pads.back(), g_cfg_input.player[i]->device.to_string()) == false)
		{
			// Failed to bind the device to cur_pad_handler so binds to NullPadHandler
			LOG_ERROR(GENERAL, "Failed to bind device %s to handler %s", g_cfg_input.player[i]->device.to_string(), handler_type.to_string());
			nullpad->bindPadToDevice(m_pads.back(), g_cfg_input.player[i]->device.to_string());
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

void pad_thread::Reset()
{
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
		u32 connected = 0;
		for (auto& cur_pad_handler : handlers)
		{
			cur_pad_handler.second->ThreadProc();
			connected += cur_pad_handler.second->connected;
		}
		m_info.now_connect = connected;
		std::this_thread::sleep_for(1ms);
	}
}
