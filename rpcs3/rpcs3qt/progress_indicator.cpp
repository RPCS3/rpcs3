#include "progress_indicator.h"

#ifdef HAS_QT_WIN_STUFF
#include <QCoreApplication>
#include <QWinTaskbarProgress>
#elif HAVE_QTDBUS
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#endif

progress_indicator::progress_indicator(int minimum, int maximum)
{
#ifdef HAS_QT_WIN_STUFF
	m_tb_button = std::make_unique<QWinTaskbarButton>();
	m_tb_button->progress()->setRange(minimum, maximum);
	m_tb_button->progress()->setVisible(false);
#else
	m_minimum = minimum;
	m_maximum = maximum;
#if HAVE_QTDBUS
	update_progress(0, true, false);
#endif
#endif
}

progress_indicator::~progress_indicator()
{
#ifdef HAS_QT_WIN_STUFF
	// QWinTaskbarProgress::hide() will crash if the application is already about to close, even if the object is not null.
	if (!QCoreApplication::closingDown())
	{
		m_tb_button->progress()->hide();
	}
#elif HAVE_QTDBUS
	update_progress(0, false, false);
#endif
}

void progress_indicator::show(QWindow* window)
{
#ifdef HAS_QT_WIN_STUFF
	m_tb_button->setWindow(window);
	m_tb_button->progress()->show();
#else
	Q_UNUSED(window);
#endif
}

void progress_indicator::hide()
{
#ifdef HAS_QT_WIN_STUFF
	m_tb_button->progress()->hide();
#endif
}

int progress_indicator::value() const
{
#ifdef HAS_QT_WIN_STUFF
	return m_tb_button->progress()->value();
#else
	return m_value;
#endif
}

void progress_indicator::set_value(int value)
{
#ifdef HAS_QT_WIN_STUFF
	m_tb_button->progress()->setValue(std::clamp(value, m_tb_button->progress()->minimum(), m_tb_button->progress()->maximum()));
#else
	m_value = std::clamp(value, m_minimum, m_maximum);
#if HAVE_QTDBUS
	update_progress(m_value, true, false);
#endif
#endif
}

void progress_indicator::set_range(int minimum, int maximum)
{
#ifdef HAS_QT_WIN_STUFF
	m_tb_button->progress()->setRange(minimum, maximum);
#else
	m_minimum = minimum;
	m_maximum = maximum;
#endif
}

void progress_indicator::reset()
{
#ifdef HAS_QT_WIN_STUFF
	m_tb_button->progress()->reset();
#else
	m_value = m_minimum;
#if HAVE_QTDBUS
	update_progress(m_value, false, false);
#endif
#endif
}

void progress_indicator::signal_failure()
{
#ifdef HAS_QT_WIN_STUFF
	m_tb_button->progress()->stop();
#elif HAVE_QTDBUS
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
