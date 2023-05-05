#pragma once

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

	QList<flow_widget_item*>& items();
	flow_widget_item* selected_item() const;
	QScrollArea* scroll_area() const;

	void paintEvent(QPaintEvent* event) override;

Q_SIGNALS:
	void ItemSelectionChanged(int index);

private Q_SLOTS:
	void on_item_focus();
	void on_navigate(flow_navigation value);

protected:
	void select_item(flow_widget_item* item);

private:
	int find_item(const flow_layout::position& pos);
	flow_layout::position find_item(flow_widget_item* item);
	flow_layout::position find_next_item(flow_layout::position current_pos, flow_navigation value);

	flow_layout* m_flow_layout{};
	QScrollArea* m_scroll_area{};
	QList<flow_widget_item*> m_widgets{};
	int m_selected_index = -1;
};
