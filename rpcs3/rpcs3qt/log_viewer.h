#pragma once

#include "find_dialog.h"

#include <QPlainTextEdit>
#include <QDropEvent>

#include <bitset>
#include <memory>

class LogHighlighter;
class gui_settings;

class log_viewer : public QWidget
{
	Q_OBJECT

public:
	explicit log_viewer(std::shared_ptr<gui_settings> gui_settings);
	void show_log();

private Q_SLOTS:
	void show_context_menu(const QPoint& pos);

private:
	void set_text_and_keep_position(const QString& text);
	void filter_log();
	bool is_valid_file(const QMimeData& md, bool save = false);

	std::shared_ptr<gui_settings> m_gui_settings;
	QString m_path_last;
	QString m_filter_term;
	QString m_full_log;
	QPlainTextEdit* m_log_text;
	LogHighlighter* m_log_highlighter;
	std::unique_ptr<find_dialog> m_find_dialog;
	std::bitset<32> m_log_levels = std::bitset<32>(0b11111111u);
	bool m_show_timestamps = true;
	bool m_show_threads = true;
	bool m_last_actions_only = false;

protected:
	void dropEvent(QDropEvent* ev) override;
	void dragEnterEvent(QDragEnterEvent* ev) override;
	void dragMoveEvent(QDragMoveEvent* ev) override;
	void dragLeaveEvent(QDragLeaveEvent* ev) override;
	bool eventFilter(QObject* object, QEvent* event) override;
};
