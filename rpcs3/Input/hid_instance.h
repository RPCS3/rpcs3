#pragma once

#include "util/types.hpp"
#include "util/logs.hpp"
#include <mutex>
#include <hidapi.h>

LOG_CHANNEL(hid_log, "HID");

struct hid_instance
{
public:
	hid_instance() = default;
	~hid_instance();

	static hid_instance& get_instance()
	{
		static hid_instance instance {};
		return instance;
	}

	bool initialize();

	static hid_device_info* enumerate(u16 vid, u16 pid);
	static void free_enumeration(hid_device_info* devs);
	static hid_device* open_path(const char* path);
	static void close(hid_device* dev);

private:
	bool m_initialized = false;
	std::mutex m_hid_mutex;
};

extern std::mutex g_hid_mutex;
