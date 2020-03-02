#include "display_sleep_control.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#elif defined(__APPLE__)
#include <IOKit/pwr_mgt/IOPMLib.h>

static IOPMAssertionID s_pm_assertion = kIOPMNullAssertionID;

#elif defined(HAVE_QTDBUS)
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QDBusReply>
#include "Utilities/types.h"
static u32 s_dbus_cookie = 0;
#endif

bool display_sleep_control_supported()
{
#if defined(_WIN32) || defined(__APPLE__)
	return true;
#elif defined(HAVE_QTDBUS)
	QDBusInterface interface("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver", QDBusConnection::sessionBus());
	return interface.isValid();
#else
	return false;
#endif
}

void enable_display_sleep()
{
#ifdef _WIN32
	SetThreadExecutionState(ES_CONTINUOUS);
#elif defined(__APPLE__)
	if (s_pm_assertion != kIOPMNullAssertionID)
	{
		IOPMAssertionRelease(s_pm_assertion);
		s_pm_assertion = kIOPMNullAssertionID;
	}
#elif defined(HAVE_QTDBUS)
	if (s_dbus_cookie != 0)
	{
		QDBusInterface interface("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver", QDBusConnection::sessionBus());
		interface.call("UnInhibit", s_dbus_cookie);
		s_dbus_cookie = 0;
	}
#endif
}

void disable_display_sleep()
{
#ifdef _WIN32
	SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
#elif defined(__APPLE__)
	IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleDisplaySleep, kIOPMAssertionLevelOn, CFSTR("Game running"), &s_pm_assertion);
#elif defined(HAVE_QTDBUS)
	QDBusInterface interface("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver", QDBusConnection::sessionBus());
	QDBusReply<u32> reply = interface.call("Inhibit", "rpcs3", "Game running");
	if (reply.isValid())
	{
		s_dbus_cookie = reply.value();
	}
#endif
}
