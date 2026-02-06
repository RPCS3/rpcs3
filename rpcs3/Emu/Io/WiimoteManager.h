#pragma once

#include "util/types.hpp"
#include "Utilities/Thread.h"
#include "Utilities/mutex.h"
#include <hidapi/hidapi.h>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <shared_mutex>
#include <chrono>

struct WiimoteIRPoint
{
	u16 x = 1023;
	u16 y = 1023;
	u8 size = 0;
};

enum class WiimoteButton : u16
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

struct WiimoteGunConMapping
{
	WiimoteButton trigger = WiimoteButton::B;
	WiimoteButton a1 = WiimoteButton::A;
	WiimoteButton a2 = WiimoteButton::Minus;
	WiimoteButton a3 = WiimoteButton::Left;
	WiimoteButton b1 = WiimoteButton::One;
	WiimoteButton b2 = WiimoteButton::Two;
	WiimoteButton b3 = WiimoteButton::Home;
	WiimoteButton c1 = WiimoteButton::Plus;
	WiimoteButton c2 = WiimoteButton::Right;

	// Secondary mappings (optional, e.g. D-Pad acting as buttons)
	WiimoteButton b1_alt = WiimoteButton::Up;
	WiimoteButton b2_alt = WiimoteButton::Down;
};

struct WiimoteState
{
	u16 buttons = 0;
	s16 acc_x = 0, acc_y = 0, acc_z = 0;
	WiimoteIRPoint ir[4];
	bool connected = false;
};

class WiimoteDevice
{
public:
	WiimoteDevice(hid_device_info* info);
	~WiimoteDevice();

	bool update();
	const WiimoteState& get_state() const { return m_state; }
	std::string get_path() const { return m_path; }
	std::wstring get_serial() const { return m_serial; }

private:
	hid_device* m_handle = nullptr;
	std::string m_path;
	std::wstring m_serial;
	WiimoteState m_state;

	bool initialize_ir();
};

class WiimoteManager
{
public:
	WiimoteManager();
	~WiimoteManager();

	static WiimoteManager* get_instance();

	void start();
	void stop();

	std::vector<WiimoteState> get_states();
	size_t get_device_count();

	void set_mapping(const WiimoteGunConMapping& mapping);
	WiimoteGunConMapping get_mapping() const;

private:
	std::thread m_thread;
	atomic_t<bool> m_running{false};
	std::vector<std::unique_ptr<WiimoteDevice>> m_devices;
	shared_mutex m_mutex;
	WiimoteGunConMapping m_mapping;

	void thread_proc();
	void load_config();
	void save_config();
};
