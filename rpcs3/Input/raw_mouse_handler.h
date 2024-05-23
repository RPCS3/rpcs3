#pragma once

#include "Emu/Io/MouseHandler.h"
#include "Emu/RSX/display.h"
#include "Utilities/Config.h"
#include "Utilities/mutex.h"
#include "Utilities/Thread.h"

#ifdef _WIN32
#include <windows.h>
#endif

static const std::map<std::string, int> raw_mouse_button_map
{
	{ "", 0 },
#ifdef _WIN32
	{ "Button 1", RI_MOUSE_BUTTON_1_UP },
	{ "Button 2", RI_MOUSE_BUTTON_2_UP },
	{ "Button 3", RI_MOUSE_BUTTON_3_UP },
	{ "Button 4", RI_MOUSE_BUTTON_4_UP },
	{ "Button 5", RI_MOUSE_BUTTON_5_UP },
#endif
};

class raw_mouse_handler;

class raw_mouse
{
public:
	raw_mouse() {}
	raw_mouse(u32 index, const std::string& device_name, void* handle, raw_mouse_handler* handler);
	virtual ~raw_mouse();

	void update_window_handle();
	display_handle_t window_handle() const { return m_window_handle; }

	void center_cursor();

#ifdef _WIN32
	void update_values(const RAWMOUSE& state);
#endif

	const std::string& device_name() const { return m_device_name; }
	u32 index() const { return m_index; }
	void set_index(u32 index) { m_index = index; }

private:
	static std::pair<int, int> get_mouse_button(const cfg::string& button);

	u32 m_index = 0;
	std::string m_device_name;
	void* m_handle{};
	display_handle_t m_window_handle{};
	int m_window_width{};
	int m_window_height{};
	int m_window_width_old{};
	int m_window_height_old{};
	int m_pos_x{};
	int m_pos_y{};
	float m_mouse_acceleration = 1.0f;
	raw_mouse_handler* m_handler{};
	std::map<u8, std::pair<int, int>> m_buttons;
};

class raw_mouse_handler final : public MouseHandlerBase
{
public:
	raw_mouse_handler(bool is_for_gui = false);
	virtual ~raw_mouse_handler();

	void Init(const u32 max_connect) override;

	const std::map<void*, raw_mouse>& get_mice() const { return m_raw_mice; };

	void set_mouse_press_callback(std::function<void(const std::string&, s32, bool)> cb)
	{
		m_mouse_press_callback = std::move(cb);
	}

	void mouse_press_callback(const std::string& device_name, s32 cell_code, bool pressed)
	{
		if (m_mouse_press_callback)
		{
			m_mouse_press_callback(device_name, cell_code, pressed);
		}
	}

	void update_devices();

#ifdef _WIN32
	void handle_native_event(const MSG& msg);
#endif

	shared_mutex m_raw_mutex;

private:
	u32 get_now_connect(std::set<u32>& connected_mice);
	std::map<void*, raw_mouse> enumerate_devices(u32 max_connect);

#ifdef _WIN32
	void register_raw_input_devices();
	void unregister_raw_input_devices();
	bool m_registered_raw_input_devices = false;
#endif

	bool m_is_for_gui = false;
	std::map<void*, raw_mouse> m_raw_mice;
	std::function<void(const std::string&, s32, bool)> m_mouse_press_callback;

	std::unique_ptr<named_thread<std::function<void()>>> m_thread;
};
