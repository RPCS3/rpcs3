#pragma once

#include "util/types.hpp"
#include "Utilities/Thread.h"
#include "wiimote_types.h"
#include "Utilities/mutex.h"
#include <hidapi.h>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <shared_mutex>
#include <chrono>
#include <array>

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

	bool is_running() const { return m_running; }

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
