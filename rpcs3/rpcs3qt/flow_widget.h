#pragma once

#include <QWidget>

class flow_layout;

class flow_widget_item : public QWidget
{
public:
	using QWidget::QWidget;
	void paintEvent(QPaintEvent* event) override;

	bool got_visible{};
	std::function<void()> cb_on_first_visibility{};
};

class flow_widget : public QWidget
{
public:
	flow_widget(QWidget* parent);
	virtual ~flow_widget();

	void add_widget(flow_widget_item* widget);
	void clear();

	QList<flow_widget_item*>& items();

private:
	flow_layout* m_flow_layout{};
	QList<flow_widget_item*> m_widgets{};
};
