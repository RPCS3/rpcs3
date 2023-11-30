#include "flow_widget_item.h"

#include <QStyle>
#include <QStyleOption>
#include <QPainter>

flow_widget_item::flow_widget_item(QWidget* parent) : QWidget(parent)
{
	setAttribute(Qt::WA_Hover); // We need to enable the hover attribute to ensure that hover events are handled.
}

flow_widget_item::~flow_widget_item()
{
}

void flow_widget_item::polish_style()
{
	style()->unpolish(this);
	style()->polish(this);
}

void flow_widget_item::paintEvent(QPaintEvent* /*event*/)
{
	// Needed for stylesheets to apply to QWidgets
	QStyleOption option;
	option.initFrom(this);
	QPainter painter(this);
	style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);

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

void flow_widget_item::focusInEvent(QFocusEvent* event)
{
	QWidget::focusInEvent(event);

	// We need to polish the widgets in order to re-apply any stylesheet changes for the focus property.
	polish_style();

	Q_EMIT focused();
}

void flow_widget_item::focusOutEvent(QFocusEvent* event)
{
	QWidget::focusOutEvent(event);

	// We need to polish the widgets in order to re-apply any stylesheet changes for the focus property.
	polish_style();
}

void flow_widget_item::keyPressEvent(QKeyEvent* event)
{
	if (!event)
	{
		return;
	}

	switch (event->key())
	{
	case Qt::Key_Left:
		Q_EMIT navigate(flow_navigation::left);
		return;
	case Qt::Key_Right:
		Q_EMIT navigate(flow_navigation::right);
		return;
	case Qt::Key_Up:
		Q_EMIT navigate(flow_navigation::up);
		return;
	case Qt::Key_Down:
		Q_EMIT navigate(flow_navigation::down);
		return;
	case Qt::Key_Home:
		Q_EMIT navigate(flow_navigation::home);
		return;
	case Qt::Key_End:
		Q_EMIT navigate(flow_navigation::end);
		return;
	case Qt::Key_PageUp:
		Q_EMIT navigate(flow_navigation::page_up);
		return;
	case Qt::Key_PageDown:
		Q_EMIT navigate(flow_navigation::page_down);
		return;
	default:
		break;
	}

	QWidget::keyPressEvent(event);
}

bool flow_widget_item::event(QEvent* event)
{
	bool hover_changed = false;

	switch (event->type())
	{
	case QEvent::HoverEnter:
		hover_changed = setProperty("hover", "true");
		break;
	case QEvent::HoverLeave:
		hover_changed = setProperty("hover", "false");
		break;
	default:
		break;
	}

	if (hover_changed)
	{
		// We need to polish the widgets in order to re-apply any stylesheet changes for the custom hover property.
		// :hover does not work if we add descendants in the qss, so we need to use a custom property.
		polish_style();
	}

	return QWidget::event(event);
}
