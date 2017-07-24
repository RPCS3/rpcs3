#pragma once

#include "Utilities/File.h"
#include "Utilities/Log.h"

#include "gui_settings.h"

#include <memory>

#include <QDockWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QActionGroup>
#include <QTimer>

class log_frame : public QDockWidget
{
	Q_OBJECT

public:
	explicit log_frame(std::shared_ptr<gui_settings> guiSettings, QWidget *parent = nullptr);

	/** Loads from settings. Public so that main_window can call this easily. */
	void LoadSettings();
Q_SIGNALS:
	void LogFrameClosed();
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event);
private Q_SLOTS:
	void UpdateUI();
private:
	void SetLogLevel(logs::level lev);
	void SetTTYLogging(bool val);

	void CreateAndConnectActions();

	QTabWidget *tabWidget;
	QTextEdit *log;
	QTextEdit *tty;
	QString m_old_text;
	ullong m_log_counter;
	bool m_stack_log;

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

	QAction* stackAct;

	QAction* TTYAct;

	std::shared_ptr<gui_settings> xgui_settings;
};
