#pragma once

#include "qt_video_source.h"
#include <QLabel>
#include <QEnterEvent>
#include <QPixmap>

class video_label : public QLabel, public qt_video_source
{
public:
	video_label(QWidget* parent = nullptr);
	virtual ~video_label();

	void set_thumbnail(const QPixmap& pxmap);
	void set_active(bool active) override;

	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	QPixmap m_current_pixmap;
};
