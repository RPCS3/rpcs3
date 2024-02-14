#pragma once

#include "Emu/Io/MouseHandler.h"
#include "Emu/RSX/display.h"

#ifdef _WIN32
#include <windows.h>
#endif

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

private:
	u32 m_index = 0;
	std::string m_device_name;
	void* m_handle{};
	display_handle_t m_window_handle{};
	int m_window_width{};
	int m_window_height{};
	int m_pos_x{};
	int m_pos_y{};
	float m_mouse_acceleration = 1.0f;
	raw_mouse_handler* m_handler{};
};

class raw_mouse_handler final : public MouseHandlerBase
{
	using MouseHandlerBase::MouseHandlerBase;

public:
	virtual ~raw_mouse_handler();

	void Init(const u32 max_connect) override;

#ifdef _WIN32
	void handle_native_event(const MSG& msg);
#endif

private:
	void enumerate_devices(u32 max_connect);

	std::map<void*, raw_mouse> m_raw_mice;
};
