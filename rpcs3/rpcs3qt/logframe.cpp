#ifdef QT_UI

#include "logframe.h"

LogFrame::LogFrame(QWidget *parent) : QDockWidget(tr("Log"), parent)
{
	tabWidget = new QTabWidget;

	log = new QTextEdit(tabWidget);
	QPalette logPalette = log->palette();
	logPalette.setColor(QPalette::Base, Qt::black);
	log->setPalette(logPalette);
	log->setReadOnly(true);

	tty = new QTextEdit(tabWidget);
	QPalette ttyPalette = log->palette();
	ttyPalette.setColor(QPalette::Base, Qt::black);
	ttyPalette.setColor(QPalette::Text, Qt::white);
	tty->setPalette(ttyPalette);
	tty->setReadOnly(true);

	tabWidget->addTab(log, tr("Log"));
	tabWidget->addTab(tty, tr("TTY"));

	setWidget(tabWidget);

	// Check for updates every ~10 ms
	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &LogFrame::Update);
	timer->start(10);
}

void LogFrame::Update()
{

}

#endif // QT_UI
