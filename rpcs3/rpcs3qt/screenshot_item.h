#pragma once

#include "flow_widget_item.h"
#include <QLabel>
#include <QThread>

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
