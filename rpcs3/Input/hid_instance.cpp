#include "stdafx.h"
#include "hid_instance.h"
#include "util/logs.hpp"
#include "Emu/System.h"
#include "Utilities/Thread.h"

#if defined(__APPLE__)
#include "3rdparty/hidapi/hidapi/mac/hidapi_darwin.h"
#endif

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
	if (thread_ctrl::is_main())
	{
		error_code = hid_init();
		hid_darwin_set_open_exclusive(0);
	}
	else
	{
		Emu.BlockingCallFromMainThread([&error_code]()
		{
			error_code = hid_init();
			hid_darwin_set_open_exclusive(0);
		}, false);
	}
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
	hid_device_info* devs = nullptr;
#if defined(__APPLE__)
	if (thread_ctrl::is_main())
	{
		std::lock_guard lock(g_hid_mutex);
		devs = hid_enumerate(vid, pid);
	}
	else
	{
		Emu.BlockingCallFromMainThread([&]()
		{
			std::lock_guard lock(g_hid_mutex);
			devs = hid_enumerate(vid, pid);
		}, false);
	}
#else
	std::lock_guard lock(g_hid_mutex);
	devs = hid_enumerate(vid, pid);
#endif
	return devs;
}

void hid_instance::free_enumeration(hid_device_info* devs)
{
	if (!devs) return;

#if defined(__APPLE__)
	if (thread_ctrl::is_main())
	{
		std::lock_guard lock(g_hid_mutex);
		hid_free_enumeration(devs);
	}
	else
	{
		Emu.BlockingCallFromMainThread([&]()
		{
			std::lock_guard lock(g_hid_mutex);
			hid_free_enumeration(devs);
		}, false);
	}
#else
	std::lock_guard lock(g_hid_mutex);
	hid_free_enumeration(devs);
#endif
}

hid_device* hid_instance::open_path(const char* path)
{
	hid_device* dev = nullptr;
#if defined(__APPLE__)
	if (thread_ctrl::is_main())
	{
		std::lock_guard lock(g_hid_mutex);
		dev = hid_open_path(path);
	}
	else
	{
		Emu.BlockingCallFromMainThread([&]()
		{
			std::lock_guard lock(g_hid_mutex);
			dev = hid_open_path(path);
		}, false);
	}
#else
	std::lock_guard lock(g_hid_mutex);
	dev = hid_open_path(path);
#endif
	return dev;
}

void hid_instance::close(hid_device* dev)
{
	if (!dev) return;

#if defined(__APPLE__)
	if (thread_ctrl::is_main())
	{
		std::lock_guard lock(g_hid_mutex);
		hid_close(dev);
	}
	else
	{
		Emu.BlockingCallFromMainThread([&]()
		{
			std::lock_guard lock(g_hid_mutex);
			hid_close(dev);
		}, false);
	}
#else
	std::lock_guard lock(g_hid_mutex);
	hid_close(dev);
#endif
}
