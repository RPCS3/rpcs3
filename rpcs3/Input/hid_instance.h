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

private:
	bool m_initialized = false;
	std::mutex m_hid_mutex;
};

extern std::mutex g_hid_mutex;
