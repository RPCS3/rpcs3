#pragma once

#include <QWidget>
#include <QKeyEvent>

#include <functional>

enum class flow_navigation
{
	up,
	down,
	left,
	right,
	home,
	end,
	page_up,
	page_down
};

class flow_widget_item : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(bool hover MEMBER m_hover) // Stylesheet workaround for descendants with parent pseudo state
	Q_PROPERTY(bool selected MEMBER selected) // Stylesheet workaround for descendants with parent pseudo state

public:
	flow_widget_item(QWidget* parent);
	virtual ~flow_widget_item();

	virtual void polish_style();

	void paintEvent(QPaintEvent* event) override;
	void focusInEvent(QFocusEvent* event) override;
	void focusOutEvent(QFocusEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	bool event(QEvent* event) override;

	bool got_visible{};
	bool selected{};
	std::function<void()> cb_on_first_visibility{};

protected:
	bool m_hover{};

Q_SIGNALS:
	void navigate(flow_navigation value);
	void focused();
};
