#include "pad_thread.h"
#include "rpcs3qt/gamepads_settings_dialog.h"
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
	m_info.max_connect = max_connect;
	m_info.now_connect = std::min(max_connect, (u32)7); // max 7 pads

	input_cfg.load();

	std::shared_ptr<keyboard_pad_handler> keyptr;

	//Always have a Null Pad Handler
	std::shared_ptr<NullPadHandler> nullpad = std::make_shared<NullPadHandler>();
	handlers.emplace(pad_handler::null, nullpad);

	for (u32 i = 0; i < m_info.now_connect; i++)
	{
		std::shared_ptr<PadHandlerBase> cur_pad_handler;

		if (handlers.count(input_cfg.player_input[i]) != 0)
		{
			cur_pad_handler = handlers[input_cfg.player_input[i]];
		}
		else
		{
			switch (input_cfg.player_input[i])
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
			}
			handlers.emplace(input_cfg.player_input[i], cur_pad_handler);
		}
		cur_pad_handler->Init();

		m_pads.push_back(std::make_shared<Pad>(
			CELL_PAD_STATUS_DISCONNECTED,
			CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
			CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_ACTUATOR,
			CELL_PAD_DEV_TYPE_STANDARD));

		if (cur_pad_handler->bindPadToDevice(m_pads.back(), input_cfg.player_device[i]->to_string()) == false)
		{
			//Failed to bind the device to cur_pad_handler so binds to NullPadHandler
			LOG_ERROR(GENERAL, "Failed to bind device %s to handler %s", input_cfg.player_device[i]->to_string(), input_cfg.player_input[i].to_string());
			nullpad->bindPadToDevice(m_pads.back(), input_cfg.player_device[i]->to_string());
		}
	}

	thread = std::make_shared<std::thread>(&pad_thread::ThreadFunc, this);
}

void pad_thread::SetRumble(const u32 pad, u8 largeMotor, bool smallMotor) {
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
		for (auto& cur_pad_handler : handlers)
		{
			cur_pad_handler.second->ThreadProc();
		}
		std::this_thread::sleep_for(1ms);
	}
}
