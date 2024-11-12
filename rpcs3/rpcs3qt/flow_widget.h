#pragma once

#include "util/types.hpp"
#include "flow_widget_item.h"
#include "flow_layout.h"

#include <QWidget>
#include <QScrollArea>
#include <QPaintEvent>

class flow_widget : public QWidget
{
	Q_OBJECT

public:
	flow_widget(QWidget* parent);
	virtual ~flow_widget();

	void add_widget(flow_widget_item* widget);
	void clear();

	std::vector<flow_widget_item*>& items() { return m_widgets; }
	flow_widget_item* selected_item() const;
	QScrollArea* scroll_area() const { return m_scroll_area; }

	void paintEvent(QPaintEvent* event) override;

Q_SIGNALS:
	void ItemSelectionChanged(int index);

private Q_SLOTS:
	void on_item_focus();
	void on_navigate(flow_navigation value);

protected:
	void select_item(flow_widget_item* item);
	void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
	s64 find_item(const flow_layout::position& pos);
	flow_layout::position find_item(flow_widget_item* item);
	flow_layout::position find_next_item(flow_layout::position current_pos, flow_navigation value);

	flow_layout* m_flow_layout{};
	QScrollArea* m_scroll_area{};
	std::vector<flow_widget_item*> m_widgets;
	s64 m_selected_index = -1;
};
