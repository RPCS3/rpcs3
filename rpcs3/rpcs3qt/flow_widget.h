#pragma once

#include "util/types.hpp"
#include "flow_widget_item.h"
#include "flow_layout.h"

#include <set>
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

	void set_multi_selection_enabled(bool enabled) { m_allow_multi_selection = enabled; }

	std::vector<flow_widget_item*>& items() { return m_widgets; }
	std::set<flow_widget_item*> selected_items() const;
	QScrollArea* scroll_area() const { return m_scroll_area; }

	void paintEvent(QPaintEvent* event) override;

Q_SIGNALS:
	void ItemSelectionChanged(int index);

private Q_SLOTS:
	void on_item_focus();
	void on_navigate(flow_navigation value);

protected:
	void select_items(const std::set<flow_widget_item*>& selected_items, flow_widget_item* current_item = nullptr);
	void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
	s64 find_item(const flow_layout::position& pos);
	flow_layout::position find_item(flow_widget_item* item);
	flow_layout::position find_next_item(flow_layout::position current_pos, flow_navigation value);

	void update_selection(flow_widget_item* current_item);

	flow_layout* m_flow_layout{};
	QScrollArea* m_scroll_area{};
	std::vector<flow_widget_item*> m_widgets;
	std::set<s64> m_selected_items;
	s64 m_last_selected_item = -1;
	bool m_allow_multi_selection = false;
};
