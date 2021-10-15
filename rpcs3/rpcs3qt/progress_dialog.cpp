#include "progress_dialog.h"

#include <QApplication>
#include <QLabel>

#ifdef _WIN32
#include <QWinTHumbnailToolbutton>
#elif HAVE_QTDBUS
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#endif

progress_dialog::progress_dialog(const QString& windowTitle, const QString& labelText, const QString& cancelButtonText, int minimum, int maximum, bool delete_on_close, QWidget* parent, Qt::WindowFlags flags)
	: QProgressDialog(labelText, cancelButtonText, minimum, maximum, parent, flags)
{
	setWindowTitle(windowTitle);
	setMinimumSize(QLabel("This is the very length of the progressdialog due to hidpi reasons.").sizeHint().width(), sizeHint().height());
	setValue(0);
	setWindowModality(Qt::WindowModal);

	if (delete_on_close)
	{
		connect(this, &QProgressDialog::canceled, this, &QProgressDialog::deleteLater);
	}

#ifdef _WIN32
	// Try to find a window handle first
	QWindow* handle = windowHandle();

	for (QWidget* ancestor = this; !handle && ancestor;)
	{
		if (ancestor = static_cast<QWidget*>(ancestor->parent()))
		{
			handle = ancestor->windowHandle();
		}
	}

	m_tb_button = std::make_unique<QWinTaskbarButton>();
	m_tb_button->setWindow(handle);
	m_tb_progress = m_tb_button->progress();
	m_tb_progress->setRange(minimum, maximum);
	m_tb_progress->show();
#elif HAVE_QTDBUS
	UpdateProgress(0, true);
#endif
}

progress_dialog::~progress_dialog()
{
#ifdef _WIN32
	// QWinTaskbarProgress::hide() will crash if the application is already about to close, even if the object is not null.
	if (!QCoreApplication::closingDown())
	{
		m_tb_progress->hide();
	}
#elif HAVE_QTDBUS
	QDBusMessage message = QDBusMessage::createSignal(
		QStringLiteral("/"),
		QStringLiteral("com.canonical.Unity.LauncherEntry"),
		QStringLiteral("Update"));
	QVariantMap properties;
	properties.insert(QStringLiteral("urgent"), false);
	properties.insert(QStringLiteral("progress"), 0);
	properties.insert(QStringLiteral("progress-visible"), false);
	message << QStringLiteral("application://rpcs3.desktop") << properties;
	QDBusConnection::sessionBus().send(message);
#endif
}

void progress_dialog::SetRange(int min, int max)
{
#ifdef _WIN32
	m_tb_progress->setRange(min, max);
#endif

	QProgressDialog::setRange(min, max);
}

void progress_dialog::SetValue(int progress)
{
	const int value = std::clamp(progress, minimum(), maximum());

#ifdef _WIN32
	m_tb_progress->setValue(value);
#elif HAVE_QTDBUS
	UpdateProgress(value, true);
#endif

	QProgressDialog::setValue(value);
}

void progress_dialog::SignalFailure() const
{
#ifdef _WIN32
	m_tb_progress->stop();
#elif HAVE_QTDBUS
	QDBusMessage message = QDBusMessage::createSignal(
		QStringLiteral("/"),
		QStringLiteral("com.canonical.Unity.LauncherEntry"),
		QStringLiteral("Update"));
	QVariantMap properties;
	properties.insert(QStringLiteral("urgent"), true);
	message << QStringLiteral("application://rpcs3.desktop") << properties;
	QDBusConnection::sessionBus().send(message);
#endif

	QApplication::beep();
}

#ifdef HAVE_QTDBUS
void progress_dialog::UpdateProgress(int progress, bool progress_visible)
{
	QDBusMessage message = QDBusMessage::createSignal(
		QStringLiteral("/"),
		QStringLiteral("com.canonical.Unity.LauncherEntry"),
		QStringLiteral("Update"));
	QVariantMap properties;
	// Progress takes a value from 0.0 to 0.1
	properties.insert(QStringLiteral("progress"), 1. * progress / maximum());
	properties.insert(QStringLiteral("progress-visible"), progress_visible);
	message << QStringLiteral("application://rpcs3.desktop") << properties;
	QDBusConnection::sessionBus().send(message);
}
#endif
