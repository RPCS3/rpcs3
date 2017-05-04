#ifndef LOGFRAME_H
#define LOGFRAME_H

#include "Utilities/File.h"
#include "Utilities/Log.h"

#include "GuiSettings.h"

#include <memory>

#include <QDockWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QActionGroup>
#include <QTimer>

class LogFrame : public QDockWidget
{
	Q_OBJECT

public:
	explicit LogFrame(std::shared_ptr<GuiSettings> guiSettings, QWidget *parent = nullptr);

	/** Loads from settings. Public so that mainwindow can call this easily. */
	void LoadSettings();
signals:
	void LogFrameClosed();
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event);
private slots:
	void UpdateUI();
	void OpenLogContextMenu(const QPoint& point);
private:
	void SetLogLevel(logs::level lev);
	void SetTTYLogging(bool val);

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

	std::shared_ptr<GuiSettings> xGuiSettings;
};

#endif // LOGFRAME_H
