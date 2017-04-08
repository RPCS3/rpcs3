#ifndef LOGFRAME_H
#define LOGFRAME_H

#include <QDockWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>

class LogFrame : public QDockWidget
{
	Q_OBJECT

public:
	explicit LogFrame(QWidget *parent = 0);

private slots:
	void Update();

private:
	QTabWidget *tabWidget;
	QTextEdit *log;
	QTextEdit *tty;
};

#endif // LOGFRAME_H
