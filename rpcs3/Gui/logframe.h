#ifndef LOGFRAME_H
#define LOGFRAME_H

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

signals:
	void LogFrameClosed();
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event);
private slots:
	void Update();
	void OpenLogContextMenu(const QPoint& point);
private:
	void CreateActions();

	QTabWidget *tabWidget;
	QTextEdit *log;
	QTextEdit *tty;

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
