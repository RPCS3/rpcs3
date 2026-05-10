#include "stdafx.h"
#include "video_label.h"

video_label::video_label(QWidget* parent)
	: QLabel(parent), qt_video_source()
{
}

video_label::~video_label()
{
}

void video_label::set_thumbnail(const QPixmap& pxmap)
{
	m_current_pixmap = pxmap;
}

void video_label::set_active(bool active)
{
	if (active)
	{
		set_image_change_callback([this](const QVideoFrame& frame)
		{
			if (const QPixmap pixmap = get_movie_image(frame); get_active() && !pixmap.isNull())
			{
				setPixmap(pixmap);
			}
		});
		start_movie();
	}
	else
	{
		set_image_change_callback({});
		stop_movie();
		setPixmap(m_current_pixmap);
	}
}

void video_label::enterEvent(QEnterEvent* event)
{
	set_active(true);

	QLabel::enterEvent(event);
}

void video_label::leaveEvent(QEvent* event)
{
	set_active(false);

	QLabel::leaveEvent(event);
}
