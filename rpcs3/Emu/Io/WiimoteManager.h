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

private:
	std::thread m_thread;
	atomic_t<bool> m_running{false};
	std::vector<std::unique_ptr<WiimoteDevice>> m_devices;
	shared_mutex m_mutex;

	void thread_proc();
};
