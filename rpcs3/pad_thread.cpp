#include "pad_thread.h"
#include "../ds4_pad_handler.h"
#ifdef _WIN32
#include "../xinput_pad_handler.h"
#include "../mm_joystick_handler.h"
#elif HAVE_LIBEVDEV
#include "../evdev_joystick_handler.h"
#endif
#include "../keyboard_pad_handler.h"
#include "../Emu/Io/Null/NullPadHandler.h"


pad_thread::pad_thread(void *_curthread, void *_curwindow) : curthread(_curthread), curwindow(_curwindow)
{

}

pad_thread::~pad_thread()
{
	active = false;
	thread->join();

	handlers.clear();
}

void pad_thread::Init(const u32 max_connect)
{
	std::memset(&m_info, 0, sizeof(m_info));
	m_info.max_connect = std::min(max_connect, (u32)7); // max 7 pads
	m_info.now_connect = 0;

	g_cfg_input.load();

	std::shared_ptr<keyboard_pad_handler> keyptr;

	//Always have a Null Pad Handler
	std::shared_ptr<NullPadHandler> nullpad = std::make_shared<NullPadHandler>();
	handlers.emplace(pad_handler::null, nullpad);

	for (u32 i = 0; i < m_info.max_connect; i++)
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
#ifdef _MSC_VER
			case pad_handler::xinput:
				cur_pad_handler = std::make_shared<xinput_pad_handler>();
				break;
#endif
#ifdef _WIN32
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

		m_pads.push_back(std::make_shared<Pad>(
			CELL_PAD_STATUS_DISCONNECTED,
			CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
			CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_ACTUATOR,
			CELL_PAD_DEV_TYPE_STANDARD));

		if (cur_pad_handler->bindPadToDevice(m_pads.back(), g_cfg_input.player[i]->device.to_string()) == false)
		{
			//Failed to bind the device to cur_pad_handler so binds to NullPadHandler
			LOG_ERROR(GENERAL, "Failed to bind device %s to handler %s", g_cfg_input.player[i]->device.to_string(), handler_type.to_string());
			nullpad->bindPadToDevice(m_pads.back(), g_cfg_input.player[i]->device.to_string());
		}
	}

	thread = std::make_shared<std::thread>(&pad_thread::ThreadFunc, this);
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

void pad_thread::ThreadFunc()
{
	active = true;
	while (active)
	{
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
