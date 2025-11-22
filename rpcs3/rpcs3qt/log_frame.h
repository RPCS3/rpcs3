#pragma once

#include "Utilities/File.h"
#include "util/logs.hpp"

#include "custom_dock_widget.h"
#include "find_dialog.h"

#include <memory>

#include <QFuture>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QActionGroup>

class gui_settings;

class log_frame : public custom_dock_widget
{
	Q_OBJECT

public:
	explicit log_frame(std::shared_ptr<gui_settings> _gui_settings, QWidget* parent = nullptr);

	/** Repaint log colors after new stylesheet was applied */
	void RepaintTextColors();

public Q_SLOTS:
	/** Loads from settings. Public so that main_window can call this easily. */
	void LoadSettings();

Q_SIGNALS:
	void LogFrameClosed();
	void PerformGoToOnDebugger(const QString& text_argument, bool is_address, bool test_only = false, std::shared_ptr<bool> signal_accepted = nullptr);
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event) override;
	bool eventFilter(QObject* object, QEvent* event) override;
private Q_SLOTS:
	void UpdateUI();
private:
	void show_disk_usage(const std::vector<std::pair<std::string, u64>>& vfs_disk_usage, u64 cache_disk_usage);
	void SetLogLevel(logs::level lev) const;
	void SetTTYLogging(bool val) const;

	void CreateAndConnectActions();

	QTabWidget* m_tabWidget = nullptr;

	std::unique_ptr<find_dialog> m_find_dialog;

	QTimer* m_timer = nullptr;
	QFuture<void> m_disk_usage_future;

	std::vector<QColor> m_color;
	QColor m_color_stack;
	QPlainTextEdit* m_log = nullptr;
	std::string m_old_log_text;
	QString m_old_tty_text;
	QString m_log_text;
	std::string m_tty_buf;
	usz m_tty_limited_read = 0;
	usz m_log_counter{};
	usz m_tty_counter{};
	bool m_stack_log{};
	bool m_stack_tty{};
	bool m_ansi_tty{};
	logs::level m_old_log_level{};

	fs::file m_tty_file;
	QWidget* m_tty_container = nullptr;
	QPlainTextEdit* m_tty = nullptr;
	QLineEdit* m_tty_input = nullptr;
	int m_tty_channel = -1;

	QAction* m_clear_act = nullptr;
	QAction* m_clear_tty_act = nullptr;
	QAction* m_show_disk_usage_act = nullptr;
	QAction* m_perform_goto_on_debugger = nullptr;
	QAction* m_perform_goto_thread_on_debugger = nullptr;
	QAction* m_perform_show_in_mem_viewer = nullptr;

	QActionGroup* m_log_level_acts = nullptr;
	QAction* m_nothing_act = nullptr;
	QAction* m_fatal_act = nullptr;
	QAction* m_error_act = nullptr;
	QAction* m_todo_act = nullptr;
	QAction* m_success_act = nullptr;
	QAction* m_warning_act = nullptr;
	QAction* m_notice_act = nullptr;
	QAction* m_trace_act = nullptr;

	QAction* m_stack_act_log = nullptr;
	QAction* m_stack_act_tty = nullptr;
	QAction* m_ansi_act_tty = nullptr;
	QAction* m_stack_act_err = nullptr;

	QAction* m_show_prefix_act = nullptr;

	QAction* m_tty_act = nullptr;

	QActionGroup* m_tty_channel_acts = nullptr;

	std::shared_ptr<gui_settings> m_gui_settings;
};
