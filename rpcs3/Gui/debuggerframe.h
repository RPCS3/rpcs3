#ifndef DEBUGGERFRAME_H
#define DEBUGGERFRAME_H

#include <QDockWidget>

class DebuggerFrame : public QDockWidget
{
    Q_OBJECT

public:
    explicit DebuggerFrame(QWidget *parent = 0);
signals:
	void DebugFrameClosed();
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event);
};

#endif // DEBUGGERFRAME_H
