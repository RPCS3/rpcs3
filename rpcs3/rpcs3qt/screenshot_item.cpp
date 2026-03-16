#include "screenshot_item.h"
#include "qt_utils.h"

#include <QVBoxLayout>

screenshot_item::screenshot_item(QWidget* parent)
	: flow_widget_item(parent)
{
	cb_on_first_visibility = [this]()
	{
		m_thread.reset(QThread::create([this]()
		{
			const QPixmap pixmap = gui::utils::get_aligned_pixmap(icon_path, icon_size, 1.0, Qt::SmoothTransformation, gui::utils::align_h::center, gui::utils::align_v::center);
			Q_EMIT signal_icon_update(pixmap);
		}));
		m_thread->start();
	};

	label = new QLabel(this);
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(label);
	setLayout(layout);
}

screenshot_item::~screenshot_item()
{
	if (m_thread && m_thread->isRunning())
	{
		m_thread->wait();
	}
}
