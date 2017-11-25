#include "progress_dialog.h"

progress_dialog::progress_dialog(int min, int max, QWidget* parent, Qt::WindowFlags flags) : QProgressDialog(parent, flags)
{
#ifdef _WIN32
	m_tb_button = std::make_unique<QWinTaskbarButton>();
	m_tb_button->setWindow(parent->windowHandle());
	m_tb_progress = m_tb_button->progress();
	m_tb_progress->setRange(min, max);
	m_tb_progress->setVisible(true);
#elif HAVE_QTDBUS
	m_max = max;
	UpdateProgress(0);
#endif

	QProgressDialog::setRange(min, max);
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
#ifdef _WIN32
	m_tb_progress->setValue(progress);
#elif HAVE_QTDBUS
	UpdateProgress(progress);
#endif

	QProgressDialog::setValue(progress);
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
	properties.insert(QStringLiteral("progress"), (double)progress / (double)m_max);
	message << QStringLiteral("application://rpcs3.desktop") << properties;
	QDBusConnection::sessionBus().send(message);
}
#endif
