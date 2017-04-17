#include "debuggerframe.h"

DebuggerFrame::DebuggerFrame(QWidget *parent) : QDockWidget(tr("Debugger"), parent)
{

}


void DebuggerFrame::closeEvent(QCloseEvent *event)
{
	QDockWidget::closeEvent(event);
	emit DebugFrameClosed();
}