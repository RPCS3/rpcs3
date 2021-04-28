#pragma once

#include <QPlainTextEdit>
#include <QDropEvent>

#include <memory>

class LogHighlighter;
class gui_settings;
class find_dialog;

class log_viewer : public QWidget
{
	Q_OBJECT

public:
	explicit log_viewer(std::shared_ptr<gui_settings> gui_settings);
	void show_log() const;

private Q_SLOTS:
	void show_context_menu(const QPoint& pos);

private:
	bool is_valid_file(const QMimeData& md, bool save = false);

	std::shared_ptr<gui_settings> m_gui_settings;
	QString m_path_last;
	QPlainTextEdit* m_log_text;
	LogHighlighter* m_log_highlighter;
	std::unique_ptr<find_dialog> m_find_dialog;

protected:
	void dropEvent(QDropEvent* ev) override;
	void dragEnterEvent(QDragEnterEvent* ev) override;
	void dragMoveEvent(QDragMoveEvent* ev) override;
	void dragLeaveEvent(QDragLeaveEvent* ev) override;
	bool eventFilter(QObject* object, QEvent* event) override;
};
