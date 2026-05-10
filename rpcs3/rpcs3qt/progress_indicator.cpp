#include "stdafx.h"
#include "progress_indicator.h"

#if HAVE_QTDBUS
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#endif

progress_indicator::progress_indicator(int minimum, int maximum)
{
	m_minimum = minimum;
	m_maximum = maximum;
#if HAVE_QTDBUS
	update_progress(0, true, false);
#endif
}

progress_indicator::~progress_indicator()
{
#if HAVE_QTDBUS
	update_progress(0, false, false);
#endif
}

int progress_indicator::value() const
{
	return m_value;
}

void progress_indicator::set_value(int value)
{
	m_value = std::clamp(value, m_minimum, m_maximum);
#if HAVE_QTDBUS
	update_progress(m_value, true, false);
#endif
}

void progress_indicator::set_range(int minimum, int maximum)
{
	m_minimum = minimum;
	m_maximum = maximum;
}

void progress_indicator::reset()
{
	m_value = m_minimum;
#if HAVE_QTDBUS
	update_progress(m_value, false, false);
#endif
}

void progress_indicator::signal_failure()
{
#if HAVE_QTDBUS
	update_progress(0, false, true);
#endif
}

#if HAVE_QTDBUS
void progress_indicator::update_progress(int progress, bool progress_visible, bool urgent)
{
	QVariantMap properties;
	properties.insert(QStringLiteral("urgent"), urgent);

	if (!urgent)
	{
		// Progress takes a value from 0.0 to 0.1
		properties.insert(QStringLiteral("progress"), 1. * progress / m_maximum);
		properties.insert(QStringLiteral("progress-visible"), progress_visible);
	}

	QDBusMessage message = QDBusMessage::createSignal(
		QStringLiteral("/"),
		QStringLiteral("com.canonical.Unity.LauncherEntry"),
		QStringLiteral("Update"));

	message << QStringLiteral("application://rpcs3.desktop") << properties;

	QDBusConnection::sessionBus().send(message);
}
#endif
