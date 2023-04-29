#pragma once

#include "flow_widget.h"

#include <QDialog>
#include <QFutureWatcher>
#include <QPixmap>
#include <QSize>
#include <QLabel>
#include <QEvent>
#include <QThread>
#include <array>

class screenshot_manager_dialog : public QDialog
{
	Q_OBJECT

public:
	screenshot_manager_dialog(QWidget* parent = nullptr);
	~screenshot_manager_dialog();

	bool eventFilter(QObject* watched, QEvent* event) override;

Q_SIGNALS:
	void signal_entry_parsed(const QString& path);
	void signal_icon_preview(const QString& path);

public Q_SLOTS:
	void update_icon(const QPixmap& pixmap);

private Q_SLOTS:
	void add_entry(const QString& path);
	void show_preview(const QString& path);

protected:
	void showEvent(QShowEvent* event) override;

private:
	void reload();

	bool m_abort_parsing = false;
	const std::array<int, 1> m_parsing_threads{0};
	QFutureWatcher<void> m_parsing_watcher;
	flow_widget* m_flow_widget = nullptr;

	QSize m_icon_size;
	QPixmap m_placeholder;
};

class screenshot_item : public flow_widget_item
{
	Q_OBJECT

public:
	screenshot_item(QWidget* parent);
	virtual ~screenshot_item();

	QString icon_path;
	QSize icon_size;
	QLabel* label{};

private:
	std::unique_ptr<QThread> m_thread;

Q_SIGNALS:
	void signal_icon_update(const QPixmap& pixmap);
};
