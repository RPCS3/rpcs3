#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include "Emu/Io/pad_types.h"
#include "Emu/Io/pad_config.h"
#include "Emu/Io/pad_config_types.h"
#include "Utilities/Timer.h"
#include "Utilities/Thread.h"

class PadHandlerBase;
class gui_settings;

class gui_pad_thread
{
public:
	gui_pad_thread();
	virtual ~gui_pad_thread();

	void update_settings(const std::shared_ptr<gui_settings>& settings);

	static std::shared_ptr<PadHandlerBase> GetHandler(pad_handler type);
	static void InitPadConfig(cfg_pad& cfg, pad_handler type, std::shared_ptr<PadHandlerBase>& handler);

	static void reset()
	{
		m_reset = true;
	}

protected:
	bool init();
	void run();

	void process_input();

	enum class mouse_button
	{
		none,
		left,
		right,
		middle
	};

	enum class mouse_wheel
	{
		none,
		vertical,
		horizontal
	};

	void send_key_event(u32 key, bool pressed);
	void send_mouse_button_event(mouse_button btn, bool pressed);
	void send_mouse_wheel_event(mouse_wheel wheel, int delta);
	void send_mouse_move_event(int delta_x, int delta_y);

#ifdef __linux__
	int m_uinput_fd = -1;
	void emit_event(int type, int code, int val);
#endif

	std::shared_ptr<PadHandlerBase> m_handler;
	std::shared_ptr<Pad> m_pad;

	std::unique_ptr<named_thread<std::function<void()>>> m_thread;
	atomic_t<bool> m_allow_global_input = false;
	static atomic_t<bool> m_reset;

	std::array<bool, static_cast<u32>(pad_button::pad_button_max_enum)> m_last_button_state{};

	steady_clock::time_point m_timestamp;
	steady_clock::time_point m_initial_timestamp;
	Timer m_input_timer;

	static constexpr u64 auto_repeat_ms_interval_default = 200;
	pad_button m_last_auto_repeat_button = pad_button::pad_button_max_enum;
	std::map<pad_button, u64> m_auto_repeat_buttons = {
		{ pad_button::dpad_up, auto_repeat_ms_interval_default },
		{ pad_button::dpad_down, auto_repeat_ms_interval_default },
		{ pad_button::dpad_left, auto_repeat_ms_interval_default },
		{ pad_button::dpad_right, auto_repeat_ms_interval_default },
		{ pad_button::rs_up, 1 },   // used for wheel scrolling by default
		{ pad_button::rs_down, 1 }, // used for wheel scrolling by default
		{ pad_button::rs_left, 1 }, // used for wheel scrolling by default
		{ pad_button::rs_right, 1 } // used for wheel scrolling by default
	};

	// Mouse movement should just work without delays
	std::map<pad_button, u64> m_mouse_move_buttons = {
		{ pad_button::ls_up, 1 },
		{ pad_button::ls_down, 1 },
		{ pad_button::ls_left, 1 },
		{ pad_button::ls_right, 1 }
	};
	pad_button m_mouse_boost_button = pad_button::L2;
	bool m_boost_mouse = false;
	float m_mouse_delta_x = 0.0f;
	float m_mouse_delta_y = 0.0f;
#ifdef __APPLE__
	float m_mouse_abs_x = 0.0f;
	float m_mouse_abs_y = 0.0f;
#endif
};
