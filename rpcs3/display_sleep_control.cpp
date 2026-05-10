#include "display_sleep_control.h"

#ifdef _WIN32
#include <windows.h>

#elif defined(__APPLE__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <IOKit/pwr_mgt/IOPMLib.h>
#pragma GCC diagnostic pop

static IOPMAssertionID s_pm_assertion = kIOPMNullAssertionID;

#elif defined(HAVE_QTDBUS)
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QDBusReply>
#include "util/types.hpp"
static u32 s_dbus_cookie = 0;
#endif

bool display_sleep_control_supported()
{
#if defined(_WIN32) || defined(__APPLE__)
	return true;
#elif defined(HAVE_QTDBUS)
	for (const char* service : { "org.freedesktop.ScreenSaver", "org.mate.ScreenSaver" })
	{
		QDBusInterface interface(service, "/ScreenSaver", service, QDBusConnection::sessionBus());
		if (interface.isValid())
		{
			return true;
		}
	}
	return false;
#else
	return false;
#endif
}

void enable_display_sleep(bool enabled)
{
	if (!display_sleep_control_supported())
	{
		return;
	}

#ifdef _WIN32
	SetThreadExecutionState(enabled ? ES_CONTINUOUS : (ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED));
#elif defined(__APPLE__)
	if (enabled && s_pm_assertion != kIOPMNullAssertionID)
	{
		IOPMAssertionRelease(s_pm_assertion);
		s_pm_assertion = kIOPMNullAssertionID;
	}
	else if (!enabled)
	{
#pragma GCC diagnostic push
// Necessary as some of those values are macro using old casts
#pragma GCC diagnostic ignored "-Wold-style-cast"
		IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleDisplaySleep, kIOPMAssertionLevelOn, CFSTR("Game running"), &s_pm_assertion);
#pragma GCC diagnostic pop
	}
#elif defined(HAVE_QTDBUS)
	if (enabled && s_dbus_cookie != 0)
	{
		for (const char* service : { "org.freedesktop.ScreenSaver", "org.mate.ScreenSaver" })
		{
			QDBusInterface interface(service, "/ScreenSaver", service, QDBusConnection::sessionBus());
			if (interface.isValid())
			{
				interface.call("UnInhibit", s_dbus_cookie);
				break;
			}
		}
		s_dbus_cookie = 0;
	}
	else if (!enabled)
	{
		for (const char* service : { "org.freedesktop.ScreenSaver", "org.mate.ScreenSaver" })
		{
			QDBusInterface interface(service, "/ScreenSaver", service, QDBusConnection::sessionBus());
			if (interface.isValid())
			{
				QDBusReply<u32> reply = interface.call("Inhibit", "rpcs3", "Game running");
				if (reply.isValid())
				{
					s_dbus_cookie = reply.value();
				}
				break;
			}
		}
	}
#endif
}
