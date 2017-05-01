#ifndef LOGFRAME_H
#define LOGFRAME_H

#include "Utilities/File.h"
#include "Utilities/Log.h"

#include <QDockWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QActionGroup>
#include <QTimer>

class LogFrame : public QDockWidget
{
	Q_OBJECT

public:
	explicit LogFrame(QWidget *parent = 0);

	/** Set log level. Used exclusively to pass in the initial settings. */
	void SetLogLevel(logs::level lev);
	/** Another method just for initial settings. */
	void SetTTYLogging(bool val);
signals:
	void LogFrameClosed();
	/** Again, I'd love to use the actual enum type, but it can't be registered as a meta type.*/
	void LogLevelChanged(uint lev);
	void TTYChanged(bool newVal);
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event);
private slots:
	void UpdateUI();
	void OpenLogContextMenu(const QPoint& point);
private:
	void CreateAndConnectActions();

	QTabWidget *tabWidget;
	QTextEdit *log;
	QTextEdit *tty;

	fs::file tty_file;

	QAction* clearAct;

	QActionGroup* logLevels;
	QAction* nothingAct;
	QAction* fatalAct;
	QAction* errorAct;
	QAction* todoAct;
	QAction* successAct;
	QAction* warningAct;
	QAction* noticeAct;
	QAction* traceAct;

	QAction* TTYAct;
};

#endif // LOGFRAME_H
