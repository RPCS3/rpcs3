#pragma once

#include "util/types.hpp"
#include "Emu/Io/MouseHandler.h"

#include <QWindow>
#include <QMouseEvent>
#include <QWheelEvent>

namespace cfg
{
	class string;
}

static const std::map<std::string, Qt::MouseButton> qt_mouse_button_map
{
	{ "NoButton",      Qt::MouseButton::NoButton },
	{ "LeftButton",    Qt::MouseButton::LeftButton },
	{ "RightButton",   Qt::MouseButton::RightButton },
	{ "MiddleButton",  Qt::MouseButton::MiddleButton },
	{ "BackButton",    Qt::MouseButton::BackButton },
	{ "ForwardButton", Qt::MouseButton::ForwardButton },
	{ "TaskButton",    Qt::MouseButton::TaskButton },
	{ "ExtraButton4",  Qt::MouseButton::ExtraButton4 },
	{ "ExtraButton5",  Qt::MouseButton::ExtraButton5 },
	{ "ExtraButton6",  Qt::MouseButton::ExtraButton6 },
	{ "ExtraButton7",  Qt::MouseButton::ExtraButton7 },
	{ "ExtraButton8",  Qt::MouseButton::ExtraButton8 },
	{ "ExtraButton9",  Qt::MouseButton::ExtraButton9 },
	{ "ExtraButton10", Qt::MouseButton::ExtraButton10 },
	{ "ExtraButton11", Qt::MouseButton::ExtraButton11 },
	{ "ExtraButton12", Qt::MouseButton::ExtraButton12 },
	{ "ExtraButton13", Qt::MouseButton::ExtraButton13 },
	{ "ExtraButton14", Qt::MouseButton::ExtraButton14 },
	{ "ExtraButton15", Qt::MouseButton::ExtraButton15 },
	{ "ExtraButton16", Qt::MouseButton::ExtraButton16 },
	{ "ExtraButton17", Qt::MouseButton::ExtraButton17 },
	{ "ExtraButton18", Qt::MouseButton::ExtraButton18 },
	{ "ExtraButton19", Qt::MouseButton::ExtraButton19 },
	{ "ExtraButton20", Qt::MouseButton::ExtraButton20 },
	{ "ExtraButton21", Qt::MouseButton::ExtraButton21 },
	{ "ExtraButton22", Qt::MouseButton::ExtraButton22 },
	{ "ExtraButton23", Qt::MouseButton::ExtraButton23 },
	{ "ExtraButton24", Qt::MouseButton::ExtraButton24 }
};

class basic_mouse_handler final : public MouseHandlerBase, public QObject
{
	using MouseHandlerBase::MouseHandlerBase;

public:
	void Init(const u32 max_connect) override;

	void SetTargetWindow(QWindow* target);
	void MouseButtonDown(QMouseEvent* event);
	void MouseButtonUp(QMouseEvent* event);
	void MouseScroll(QWheelEvent* event);
	void MouseMove(QMouseEvent* event);

	bool eventFilter(QObject* obj, QEvent* ev) override;
private:
	QWindow* m_target = nullptr;
	bool get_mouse_lock_state() const;
	static Qt::MouseButton get_mouse_button(const cfg::string& button);

	std::map<u8, Qt::MouseButton> m_buttons;
};
