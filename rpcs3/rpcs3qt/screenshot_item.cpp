#include "screenshot_item.h"
#include "qt_utils.h"
#include "Utilities/Thread.h"

#include <QVBoxLayout>

screenshot_item::screenshot_item(QWidget* parent, QSize icon_size, const QString& icon_path, const QPixmap& placeholder)
	: flow_widget_item(parent)
	, m_icon_path(icon_path)
	, m_icon_size(icon_size)
{
	setToolTip(icon_path);

	cb_on_first_visibility = [this]()
	{
		m_thread.reset(QThread::create([this]()
		{
			thread_base::set_name("Screenshot item");

			const QPixmap src_icon = QPixmap(m_icon_path);
			if (src_icon.isNull()) return;

			const QPixmap pixmap = gui::utils::get_aligned_pixmap(src_icon, m_icon_size, 1.0, Qt::SmoothTransformation, gui::utils::align_h::center, gui::utils::align_v::center);
			Q_EMIT signal_icon_update(pixmap);
		}));
		m_thread->start();
	};

	m_label = new QLabel(this);
	m_label->setPixmap(placeholder);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_label);
	setLayout(layout);

	connect(this, &screenshot_item::signal_icon_update, this, &screenshot_item::update_icon, Qt::ConnectionType::QueuedConnection);
}

screenshot_item::~screenshot_item()
{
	if (m_thread && m_thread->isRunning())
	{
		m_thread->wait();
	}
}

void screenshot_item::update_icon(const QPixmap& pixmap)
{
	if (m_label)
	{
		m_label->setPixmap(pixmap);
	}
}

void screenshot_item::mouseDoubleClickEvent(QMouseEvent* ev)
{
	flow_widget_item::mouseDoubleClickEvent(ev);

	if (!ev) return;

	if (ev->button() == Qt::LeftButton)
	{
		Q_EMIT signal_icon_preview(m_icon_path);
	}
}
