#include "flow_widget.h"
#include "flow_layout.h"

#include <QScrollArea>
#include <QVBoxLayout>

flow_widget::flow_widget(QWidget* parent)
	: QWidget(parent)
{
	m_flow_layout = new flow_layout();

	QWidget* widget = new QWidget(this);
	widget->setLayout(m_flow_layout);

	QScrollArea* scrollArea = new QScrollArea(this);
	scrollArea->setWidget(widget);
	scrollArea->setWidgetResizable(true);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(scrollArea);
	layout->setContentsMargins(0, 0, 0, 0);
	setLayout(layout);
}

flow_widget::~flow_widget()
{
}

void flow_widget::add_widget(flow_widget_item* widget)
{
	if (widget)
	{
		m_widgets << widget;
		m_flow_layout->addWidget(widget);
	}
}

void flow_widget::clear()
{
	m_widgets.clear();
	m_flow_layout->clear();
}

QList<flow_widget_item*>& flow_widget::items()
{
	return m_widgets;
}

void flow_widget_item::paintEvent(QPaintEvent* event)
{
	QWidget::paintEvent(event);

	if (!got_visible && cb_on_first_visibility)
	{
		if (QWidget* widget = static_cast<QWidget*>(parent()))
		{
			if (widget->visibleRegion().intersects(geometry()))
			{
				got_visible = true;
				cb_on_first_visibility();
			}
		}
	}
}
