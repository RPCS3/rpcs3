#pragma once

#include "Utilities/File.h"
#include "Utilities/Log.h"

#include "gui_settings.h"

#include <memory>

#include <QDockWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QActionGroup>
#include <QTimer>

class log_frame : public QDockWidget
{
	Q_OBJECT

public:
	explicit log_frame(std::shared_ptr<gui_settings> guiSettings, QWidget *parent = nullptr);

	/** Loads from settings. Public so that main_window can call this easily. */
	void LoadSettings();

	/** Repaint log colors after new stylesheet was applied */
	void RepaintTextColors();

Q_SIGNALS:
	void LogFrameClosed();
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event);
private Q_SLOTS:
	void UpdateUI();
private:
	void SetLogLevel(logs::level lev);

	void CreateAndConnectActions();

	QList<QColor> m_color;
	QColor m_color_stack;
	QTextEdit *m_log;
	QTextEdit *m_tty;
	QLineEdit *m_tty_input;
	QString m_old_text;
	ullong m_log_counter;
	bool m_stack_log;
	bool m_log_tty;
	bool m_show_tty_in_log;

	fs::file m_tty_file;

	QAction* m_clearAct;
	QAction* m_clearTTYAct;

	QActionGroup* m_logLevels;
	QAction* m_nothingAct;
	QAction* m_fatalAct;
	QAction* m_errorAct;
	QAction* m_todoAct;
	QAction* m_successAct;
	QAction* m_warningAct;
	QAction* m_noticeAct;
	QAction* m_traceAct;

	QAction* m_stackAct;

	QAction* m_TTYAct;

	QAction* m_TTYLogAct;

	std::shared_ptr<gui_settings> xgui_settings;
};
