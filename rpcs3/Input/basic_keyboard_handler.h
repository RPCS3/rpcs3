#pragma once

#include "util/types.hpp"
#include "Emu/Io/KeyboardHandler.h"

#include <QKeyEvent>
#include <QWindow>

class basic_keyboard_handler final : public KeyboardHandlerBase, public QObject
{
	using KeyboardHandlerBase::KeyboardHandlerBase;

public:
	void Init(keyboard_consumer& consumer, const u32 max_connect) override;

	void SetTargetWindow(QWindow* target);
	bool eventFilter(QObject* watched, QEvent* event) override;
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);
	static s32 getUnmodifiedKey(QKeyEvent* event);

private:
	void LoadSettings(Keyboard& keyboard);

	QWindow* m_target = nullptr;
};
