#pragma once

#include "Utilities/File.h"
#include "util/logs.hpp"

#include "custom_dock_widget.h"
#include "find_dialog.h"

#include <memory>

#include <QTabWidget>
#include <QPlainTextEdit>
#include <QActionGroup>

class gui_settings;

class log_frame : public custom_dock_widget
{
	Q_OBJECT

public:
	explicit log_frame(std::shared_ptr<gui_settings> guiSettings, QWidget *parent = nullptr);

	/** Repaint log colors after new stylesheet was applied */
	void RepaintTextColors();

public Q_SLOTS:
	/** Loads from settings. Public so that main_window can call this easily. */
	void LoadSettings();

Q_SIGNALS:
	void LogFrameClosed();
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event) override;
	bool eventFilter(QObject* object, QEvent* event) override;
private Q_SLOTS:
	void UpdateUI();
private:
	void SetLogLevel(logs::level lev);
	void SetTTYLogging(bool val);

	void CreateAndConnectActions();

	QTabWidget* m_tabWidget = nullptr;

	std::unique_ptr<find_dialog> m_find_dialog;

	QList<QColor> m_color;
	QColor m_color_stack;
	QPlainTextEdit* m_log = nullptr;
	QString m_old_log_text;
	QString m_old_tty_text;
	ullong m_log_counter{};
	ullong m_tty_counter{};
	bool m_stack_log{};
	bool m_stack_tty{};

	fs::file m_tty_file;
	QWidget* m_tty_container = nullptr;
	QPlainTextEdit* m_tty = nullptr;
	QLineEdit* m_tty_input = nullptr;
	int m_tty_channel = -1;

	QAction* m_clearAct = nullptr;
	QAction* m_clearTTYAct = nullptr;

	QActionGroup* m_logLevels = nullptr;
	QAction* m_nothingAct = nullptr;
	QAction* m_fatalAct = nullptr;
	QAction* m_errorAct = nullptr;
	QAction* m_todoAct = nullptr;
	QAction* m_successAct = nullptr;
	QAction* m_warningAct = nullptr;
	QAction* m_noticeAct = nullptr;
	QAction* m_traceAct = nullptr;

	QAction* m_stackAct_log = nullptr;
	QAction* m_stackAct_tty = nullptr;

	QAction* m_TTYAct = nullptr;

	QActionGroup* m_tty_channel_acts = nullptr;

	std::shared_ptr<gui_settings> m_gui_settings;
};
