#include "stdafx.h"
#include "flow_widget.h"

#include <QApplication>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QStyleOption>
#include <QPainter>

flow_widget::flow_widget(QWidget* parent)
	: QWidget(parent)
{
	m_flow_layout = new flow_layout();

	QWidget* widget = new QWidget(this);
	widget->setLayout(m_flow_layout);
	widget->setObjectName("flow_widget_content");
	widget->setFocusProxy(this);

	m_scroll_area = new QScrollArea(this);
	m_scroll_area->setWidget(widget);
	m_scroll_area->setWidgetResizable(true);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_scroll_area);
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
		m_widgets.push_back(widget);
		m_flow_layout->addWidget(widget);

		connect(widget, &flow_widget_item::navigate, this, &flow_widget::on_navigate);
		connect(widget, &flow_widget_item::focused, this, &flow_widget::on_item_focus);
	}
}

void flow_widget::clear()
{
	m_widgets.clear();
	m_flow_layout->clear();
}

std::set<flow_widget_item*> flow_widget::selected_items() const
{
	std::set<flow_widget_item*> items;

	for (s64 index : m_selected_items)
	{
		if (index >= 0 && static_cast<usz>(index) < m_widgets.size())
		{
			items.insert(::at32(m_widgets, index));

			if (!m_allow_multi_selection)
			{
				return items;
			}
		}
	}

	return items;
}

void flow_widget::paintEvent(QPaintEvent* /*event*/)
{
	// Needed for stylesheets to apply to QWidgets
	QStyleOption option;
	option.initFrom(this);
	QPainter painter(this);
	style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
}

s64 flow_widget::find_item(const flow_layout::position& pos)
{
	if (!pos)
	{
		return -1;
	}

	const auto& positions = m_flow_layout->positions();

	for (s64 i = 0; i < positions.size(); i++)
	{
		if (const auto& other = ::at32(positions, i); other.row == pos.row && other.col == pos.col)
		{
			return i;
		}
	}

	return -1;
}

flow_layout::position flow_widget::find_item(flow_widget_item* item)
{
	if (item)
	{
		const auto& item_list = m_flow_layout->item_list();
		const auto& positions = m_flow_layout->positions();
		ensure(item_list.size() == positions.size());

		for (s64 i = 0; i < item_list.size(); i++)
		{
			if (const auto& layout_item = ::at32(item_list, i); layout_item && layout_item->widget() == item)
			{
				return ::at32(positions, i);
			}
		}
	}

	return flow_layout::position{ .row = -1, .col = - 1};
}

flow_layout::position flow_widget::find_next_item(flow_layout::position current_pos, flow_navigation value)
{
	if (current_pos && m_flow_layout->rows() > 0 && m_flow_layout->cols() > 0)
	{
		switch (value)
		{
		case flow_navigation::up:
			// Go up one row.
			if (current_pos.row > 0)
			{
				current_pos.row--;
			}
			break;
		case flow_navigation::down:
			// Go down one row. Beware of last row which might have less columns.
			for (const auto& pos : m_flow_layout->positions())
			{;
				if (pos.col != current_pos.col) continue;
				if (pos.row == current_pos.row + 1)
				{
					current_pos.row = pos.row;
					break;
				}
			}
			break;
		case flow_navigation::left:
			// Go left one column.
			if (current_pos.col > 0)
			{
				current_pos.col--;
			}
			break;
		case flow_navigation::right:
			// Go right one column. Beware of last row which might have less columns.
			for (const auto& pos : m_flow_layout->positions())
			{
				if (pos.row > current_pos.row) break;
				if (pos.row < current_pos.row) continue;
				if (pos.col == current_pos.col + 1)
				{
					current_pos.col = pos.col;
					break;
				}
			}
			break;
		case flow_navigation::home:
			// Go to leftmost column.
			current_pos.col = 0;
			break;
		case flow_navigation::end:
			// Go to last column. Beware of last row which might have less columns.
			for (const auto& pos : m_flow_layout->positions())
			{
				if (pos.row > current_pos.row) break;
				if (pos.row < current_pos.row) continue;
				current_pos.col = std::max(current_pos.col, pos.col);
			}
			break;
		case flow_navigation::page_up:
			// Go to top row.
			current_pos.row = 0;
			break;
		case flow_navigation::page_down:
			// Go to bottom row. Beware of last row which might have less columns.
			for (const auto& pos : m_flow_layout->positions())
			{
				if (pos.col != current_pos.col) continue;
				current_pos.row = std::max(current_pos.row, pos.row);
			}
			break;
		}
	}

	return current_pos;
}

void flow_widget::select_items(const std::set<flow_widget_item*>& selected_items, flow_widget_item* current_item)
{
	m_selected_items.clear();

	for (flow_widget_item* item : selected_items)
	{
		const flow_layout::position selected_pos = find_item(item);
		const s64 selected_index = find_item(selected_pos);

		if (selected_index < 0 || static_cast<usz>(selected_index) >= items().size())
		{
			continue;
		}

		m_selected_items.insert(selected_index);

		if (!m_allow_multi_selection)
		{
			break;
		}
	}

	for (usz i = 0; i < items().size(); i++)
	{
		if (flow_widget_item* item = items().at(i))
		{
			// We need to polish the widgets in order to re-apply any stylesheet changes for the selected property.
			item->selected = m_selected_items.contains(i);
			item->polish_style();
		}
	}

	if (m_selected_items.empty())
	{
		m_last_selected_item = -1;
	}
	else
	{
		if (!m_selected_items.contains(m_last_selected_item))
		{
			m_last_selected_item = *m_selected_items.cbegin();
		}

		s64 selected_item = m_last_selected_item;
		s64 focused_item = selected_item;

		if (current_item)
		{
			if (const flow_layout::position selected_pos = find_item(current_item))
			{
				focused_item = find_item(selected_pos);

				if (current_item->selected)
				{
					selected_item = focused_item;
				}
			}
		}

		// Make sure we see the focused widget
		m_scroll_area->ensureWidgetVisible(::at32(items(), focused_item));

		Q_EMIT ItemSelectionChanged(selected_item);
	}
}

void flow_widget::update_selection(flow_widget_item* current_item)
{
	std::set<flow_widget_item*> selected_items;
	const Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();

	if (current_item)
	{
		if (!m_allow_multi_selection || !current_item->selected || modifiers != Qt::ControlModifier)
		{
			selected_items.insert(current_item);
		}
	}

	if (m_allow_multi_selection)
	{
		flow_layout::position selected_pos;
		flow_layout::position last_selected_pos;

		if (modifiers == Qt::ShiftModifier && m_last_selected_item >= 0 && static_cast<usz>(m_last_selected_item) < items().size())
		{
			selected_pos = find_item(current_item);
			last_selected_pos = find_item(::at32(m_widgets, m_last_selected_item));
		}

		flow_layout::position pos_min = last_selected_pos;
		flow_layout::position pos_max = selected_pos;

		if (pos_min.row > pos_max.row)
		{
			std::swap(pos_min, pos_max);
		}

		// Check if the item is between the last and the current selection
		const auto item_between = [this, &pos_min, &pos_max](flow_widget_item* item)
		{
			if (!pos_min || !pos_max) return false;

			const flow_layout::position pos = find_item(item);
			if (!pos) return false; // pos invalid
			if (pos.row < pos_min.row || pos.row > pos_max.row) return false; // not in any relevant row
			if (pos.row > pos_min.row && pos.row < pos_max.row) return true; // in a row between the items -> match

			const bool in_min_row = pos.row == pos_min.row && pos.col >= pos_min.col;
			const bool in_max_row = pos.row == pos_max.row && pos.col <= pos_max.col;

			if (pos_min.row == pos_max.row) return in_min_row && in_max_row; // in the only row at a relevant col -> match

			return in_min_row || in_max_row; // in either min or max row at a relevant col -> match
		};

		for (usz i = 0; i < items().size(); i++)
		{
			if (flow_widget_item* item = items().at(i))
			{
				if (item == current_item) continue;

				if (modifiers == Qt::ControlModifier && item->selected)
				{
					selected_items.insert(item);
				}
				else if (modifiers == Qt::ShiftModifier && item_between(item))
				{
					selected_items.insert(item);
				}
			}
		}
	}

	select_items(selected_items, current_item);
}

void flow_widget::on_item_focus()
{
	update_selection(static_cast<flow_widget_item*>(QObject::sender()));
}

void flow_widget::on_navigate(flow_navigation value)
{
	const flow_layout::position selected_pos = find_next_item(find_item(static_cast<flow_widget_item*>(QObject::sender())), value);
	const s64 selected_index = find_item(selected_pos);

	if (selected_index >= 0 && static_cast<usz>(selected_index) < items().size())
	{
		if (flow_widget_item* item = items().at(selected_index))
		{
			item->setFocus();
		}
	}
}

void flow_widget::mouseDoubleClickEvent(QMouseEvent* ev)
{
	if (!ev) return;

	// Qt's itemDoubleClicked signal doesn't distinguish between mouse buttons and there is no simple way to get the pressed button.
	// So we have to ignore this event when another button is pressed.
	if (ev->button() != Qt::LeftButton)
	{
		ev->ignore();
		return;
	}

	QWidget::mouseDoubleClickEvent(ev);
}
