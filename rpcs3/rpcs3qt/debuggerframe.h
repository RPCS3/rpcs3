#ifndef DEBUGGERFRAME_H
#define DEBUGGERFRAME_H

#include <QDockWidget>

class DebuggerFrame : public QDockWidget
{
    Q_OBJECT

public:
        explicit DebuggerFrame(QWidget *parent = 0);
};

#endif // DEBUGGERFRAME_H
