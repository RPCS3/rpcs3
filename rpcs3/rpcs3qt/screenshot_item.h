#pragma once

#include "flow_widget_item.h"
#include <QLabel>
#include <QThread>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QKeyEvent>

class screenshot_item : public flow_widget_item
{
	Q_OBJECT

public:
	screenshot_item(QWidget* parent, QSize icon_size, const QString& icon_path, const QPixmap& placeholder);
	~screenshot_item() override;

	void polish_style() override;

private:
	QLabel* m_label{};
	QString m_icon_path;
	QSize m_icon_size;
	std::unique_ptr<QThread> m_thread;

	void show_context_menu(const QPoint& global_pos);
	void delete_screenshot();

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* ev) override;
	void mouseDoubleClickEvent(QMouseEvent* ev) override;
	void contextMenuEvent(QContextMenuEvent* ev) override;
	void keyPressEvent(QKeyEvent* ev) override;

Q_SIGNALS:
	void signal_icon_update(const QPixmap& pixmap);
	void signal_icon_preview(const QString& path);
	void signal_screenshot_deleted();

public Q_SLOTS:
	void update_icon(const QPixmap& pixmap);
};
