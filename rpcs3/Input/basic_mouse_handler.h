#pragma once

#include "stdafx.h"
#include "Emu/Io/MouseHandler.h"

#include <QWindow>
#include <QMouseEvent>
#include <QWheelEvent>

class basic_mouse_handler final : public QObject, public MouseHandlerBase
{
	Q_OBJECT
public:
	virtual void Init(const u32 max_connect) override;

	basic_mouse_handler();

	void SetTargetWindow(QWindow* target);
	void MouseButtonDown(QMouseEvent* event);
	void MouseButtonUp(QMouseEvent* event);
	void MouseScroll(QWheelEvent* event);
	void MouseMove(QMouseEvent* event);

	bool eventFilter(QObject* obj, QEvent* ev) override;
private:
	QWindow* m_target = nullptr;
};
