#include "stdafx.h"
#include "hid_instance.h"
#include "util/logs.hpp"
#include "Emu/System.h"
#include "Utilities/Thread.h"

#if defined(__APPLE__)
#include "3rdparty/hidapi/hidapi/mac/hidapi_darwin.h"
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

hid_device_info* hid_instance::enumerate(u16 vid, u16 pid)
{
	std::lock_guard lock(g_hid_mutex);
#if defined(__APPLE__)
	hid_device_info* devs = nullptr;
	Emu.BlockingCallFromMainThread([&]() { devs = hid_enumerate(vid, pid); }, false);
	return devs;
#else
	return hid_enumerate(vid, pid);
#endif
}

void hid_instance::free_enumeration(hid_device_info* devs)
{
	if (!devs) return;

	std::lock_guard lock(g_hid_mutex);
#if defined(__APPLE__)
	Emu.BlockingCallFromMainThread([&]() { hid_free_enumeration(devs); }, false);
#else
	hid_free_enumeration(devs);
#endif
}

hid_device* hid_instance::open_path(const char* path)
{
	std::lock_guard lock(g_hid_mutex);
#if defined(__APPLE__)
	if (!thread_ctrl::is_main())
	{
		hid_device* dev = nullptr;
		Emu.BlockingCallFromMainThread([&]() { dev = hid_open_path(path); }, false);
		return dev;
	}
#endif
	return hid_open_path(path);
}

void hid_instance::close(hid_device* dev)
{
	if (!dev) return;

	std::lock_guard lock(g_hid_mutex);
#if defined(__APPLE__)
	Emu.BlockingCallFromMainThread([&]() { hid_close(dev); }, false);
#else
	hid_close(dev);
#endif
}
