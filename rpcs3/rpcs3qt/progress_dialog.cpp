#include "progress_dialog.h"

#include <QLabel>

#ifdef _WIN32
#include <QWinTHumbnailToolbar>
#include <QWinTHumbnailToolbutton>
#elif HAVE_QTDBUS
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#endif

progress_dialog::progress_dialog(const QString &windowTitle, const QString &labelText, const QString &cancelButtonText, int minimum, int maximum, bool delete_on_close, QWidget *parent, Qt::WindowFlags flags)
	: QProgressDialog(labelText, cancelButtonText, minimum, maximum, parent, flags)
{
	setWindowTitle(windowTitle);
	setFixedSize(QLabel("This is the very length of the progressdialog due to hidpi reasons.").sizeHint().width(), sizeHint().height());
	setValue(0);
	setWindowModality(Qt::WindowModal);

	if (delete_on_close)
	{
		connect(this, &QProgressDialog::canceled, this, &QProgressDialog::deleteLater);
	}

#ifdef _WIN32
	m_tb_button = std::make_unique<QWinTaskbarButton>();
	m_tb_button->setWindow(parent ? parent->windowHandle() : windowHandle());
	m_tb_progress = m_tb_button->progress();
	m_tb_progress->setRange(minimum, maximum);
	m_tb_progress->setVisible(true);
#elif HAVE_QTDBUS
	UpdateProgress(0);
#endif
}

progress_dialog::~progress_dialog()
{
#ifdef _WIN32
	m_tb_progress->hide();
#elif HAVE_QTDBUS
	UpdateProgress(0);
#endif
}

void progress_dialog::SetValue(int progress)
{
	const int value = std::clamp(progress, minimum(), maximum());

#ifdef _WIN32
	m_tb_progress->setValue(value);
#elif HAVE_QTDBUS
	UpdateProgress(value);
#endif

	QProgressDialog::setValue(value);
}

void progress_dialog::SignalFailure()
{
#ifdef _WIN32
	m_tb_progress->stop();
#endif
	// TODO: Implement an equivalent for Linux, if possible
}

#ifdef HAVE_QTDBUS
void progress_dialog::UpdateProgress(int progress, bool disable)
{
	QDBusMessage message = QDBusMessage::createSignal(
		QStringLiteral("/"),
		QStringLiteral("com.canonical.Unity.LauncherEntry"),
		QStringLiteral("Update"));
	QVariantMap properties;
	if (disable)
		properties.insert(QStringLiteral("progress-visible"), false);
	else
		properties.insert(QStringLiteral("progress-visible"), true);
	//Progress takes a value from 0.0 to 0.1
	properties.insert(QStringLiteral("progress"), 1. * progress / maximum());
	message << QStringLiteral("application://rpcs3.desktop") << properties;
	QDBusConnection::sessionBus().send(message);
}
#endif
