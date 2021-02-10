#pragma once

#include "util/types.hpp"
#include "Emu/Io/KeyboardHandler.h"

#include <QKeyEvent>
#include <QWindow>

class basic_keyboard_handler final : public KeyboardHandlerBase, public QObject
{
public:
	virtual void Init(const u32 max_connect) override;

	explicit basic_keyboard_handler();

	void SetTargetWindow(QWindow* target);
	bool eventFilter(QObject* obj, QEvent* ev) override;
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);
	s32 getUnmodifiedKey(QKeyEvent* event);
	void LoadSettings();
private:
	QWindow* m_target = nullptr;
};
