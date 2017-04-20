#include "logframe.h"

#include <QMenu>
#include <QActionGroup>

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

	CreateActions();
	log->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(log, &QWidget::customContextMenuRequested, this, &LogFrame::OpenLogContextMenu);
	// Check for updates every ~10 ms
	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &LogFrame::Update);
	timer->start(10);
}

void LogFrame::Update()
{

}

void LogFrame::CreateActions()
{
	clearAct = new QAction(tr("Clear"), this);

	logLevels = new QActionGroup(this);
	nothingAct = new QAction(tr("Nothing"), logLevels);
	fatalAct = new QAction(tr("Fatal"), logLevels);
	errorAct = new QAction(tr("Error"), logLevels);
	todoAct = new QAction(tr("Todo"), logLevels);
	successAct = new QAction(tr("Success"), logLevels);
	warningAct = new QAction(tr("Warning"), logLevels);
	noticeAct = new QAction(tr("Notice"), logLevels);
	traceAct = new QAction(tr("Trace"), logLevels);
	TTYAct = new QAction(tr("TTY"), this);
	// Errors are log default
	nothingAct->setCheckable(true);
	fatalAct->setCheckable(true);
	errorAct->setCheckable(true);
	todoAct->setCheckable(true);
	successAct->setCheckable(true);
	warningAct->setCheckable(true);
	noticeAct->setCheckable(true);
	traceAct->setCheckable(true);
	TTYAct->setCheckable(true);

	TTYAct->setChecked(true);
	errorAct->setChecked(true);
}

void LogFrame::OpenLogContextMenu(const QPoint& loc)
{
	QMenu* menu = log->createStandardContextMenu();
	menu->addAction(clearAct);
	menu->addSeparator();
	menu->addActions({ nothingAct, fatalAct, errorAct, todoAct, successAct, warningAct, noticeAct, traceAct });

	menu->addSeparator();
	menu->addAction(TTYAct);
	menu->exec(mapToGlobal(loc));
}

void LogFrame::closeEvent(QCloseEvent *event)
{
	QDockWidget::closeEvent(event);
	emit LogFrameClosed();
}
