#include "stdafx.h"
#include "hid_instance.h"
#include "util/logs.hpp"
#include "Emu/System.h"

#if defined(__APPLE__)
#include <hidapi/hidapi_darwin.h>
#endif

LOG_CHANNEL(hid_log, "HID");

std::mutex g_hid_mutex;

hid_instance::~hid_instance()
{
	std::lock_guard lock(m_hid_mutex);

	// Only exit HIDAPI once on exit. HIDAPI uses a global state internally...
	if (m_initialized)
	{
		hid_log.notice("Exiting HIDAPI...");

		if (hid_exit() != 0)
		{
			hid_log.error("hid_exit failed!");
		}
	}
}

bool hid_instance::initialize()
{
	std::lock_guard lock(m_hid_mutex);

	// Only init HIDAPI once. HIDAPI uses a global state internally...
	if (m_initialized)
	{
		return true;
	}

	hid_log.notice("Initializing HIDAPI ...");

#if defined(__APPLE__)
	int error_code = 0;
	Emu.BlockingCallFromMainThread([&error_code]()
	{
		error_code = hid_init();
		hid_darwin_set_open_exclusive(0);
	}, false);
#else
	const int error_code = hid_init();
#endif
	if (error_code != 0)
	{
		hid_log.fatal("hid_init error %d: %s", error_code, hid_error(nullptr));
		return false;
	}

	m_initialized = true;
	return true;
}
