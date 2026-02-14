#pragma once

#include "util/types.hpp"
#include "Utilities/Thread.h"
#include "Utilities/mutex.h"
#include <hidapi.h>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <shared_mutex>
#include <chrono>
#include <array>

static constexpr usz MAX_WIIMOTES = 4;
static constexpr usz MAX_WIIMOTE_IR_POINTS = 4;

struct wiimote_ir_point
{
	u16 x = 1023;
	u16 y = 1023;
	u8 size = 0;
};

enum class wiimote_button : u16
{
	None = 0,
	Left = 0x0001,
	Right = 0x0002,
	Down = 0x0004,
	Up = 0x0008,
	Plus = 0x0010,
	Two = 0x0100,
	One = 0x0200,
	B = 0x0400,
	A = 0x0800,
	Minus = 0x1000,
	Home = 0x8000
};

struct wiimote_guncon_mapping
{
	wiimote_button trigger = wiimote_button::B;
	wiimote_button a1 = wiimote_button::A;
	wiimote_button a2 = wiimote_button::Minus;
	wiimote_button a3 = wiimote_button::Left;
	wiimote_button b1 = wiimote_button::One;
	wiimote_button b2 = wiimote_button::Two;
	wiimote_button b3 = wiimote_button::Home;
	wiimote_button c1 = wiimote_button::Plus;
	wiimote_button c2 = wiimote_button::Right;

	// Secondary mappings (optional, e.g. D-Pad acting as buttons)
	wiimote_button b1_alt = wiimote_button::Up;
	wiimote_button b2_alt = wiimote_button::Down;
};

struct wiimote_state
{
	u16 buttons = 0;
	s16 acc_x = 0;
	s16 acc_y = 0;
	s16 acc_z = 0;
	std::array<wiimote_ir_point, MAX_WIIMOTE_IR_POINTS> ir {};
	bool connected = false;
};

class wiimote_device
{
public:
	wiimote_device();
	~wiimote_device();

	bool open(hid_device_info* info);
	void close();

	bool update();
	const wiimote_state& get_state() const { return m_state; }
	const std::string& get_path() const { return m_path; }
	const std::wstring& get_serial() const { return m_serial; }

private:
	hid_device* m_handle = nullptr;
	std::string m_path;
	std::wstring m_serial;
	wiimote_state m_state {};
	std::chrono::steady_clock::time_point m_last_ir_check;

	bool initialize_ir();
	bool verify_ir_health();
	bool write_reg(u32 addr, const std::vector<u8>& data);
};

class wiimote_handler
{
public:
	wiimote_handler();
	~wiimote_handler();

	static wiimote_handler* get_instance();

	void start();
	void stop();

	std::vector<wiimote_state> get_states();
	usz get_device_count();

	void set_mapping(const wiimote_guncon_mapping& mapping);
	wiimote_guncon_mapping get_mapping() const;

private:
	std::unique_ptr<named_thread<std::function<void()>>> m_thread;
	atomic_t<bool> m_running{false};
	std::vector<std::unique_ptr<wiimote_device>> m_devices;
	shared_mutex m_mutex;
	wiimote_guncon_mapping m_mapping {};

	void thread_proc();
	void load_config();
	void save_config();
};
