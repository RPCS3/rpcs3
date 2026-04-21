#pragma once

#include "flow_widget_item.h"
#include <QLabel>
#include <QThread>
#include <QMouseEvent>

class screenshot_item : public flow_widget_item
{
	Q_OBJECT

public:
	screenshot_item(QWidget* parent, QSize icon_size, const QString& icon_path, const QPixmap& placeholder);
	virtual ~screenshot_item();

private:
	QLabel* m_label{};
	QString m_icon_path;
	QSize m_icon_size;
	std::unique_ptr<QThread> m_thread;

protected:
	void mouseDoubleClickEvent(QMouseEvent* ev) override;

Q_SIGNALS:
	void signal_icon_update(const QPixmap& pixmap);
	void signal_icon_preview(const QString& path);

public Q_SLOTS:
	void update_icon(const QPixmap& pixmap);
};
