#ifndef BASIC_KEYBOARD_HANDLER_H
#define BASIC_KEYBOARD_HANDLER_H

#include "stdafx.h"
#include "Emu/Io/KeyboardHandler.h"

#include <QObject>
#include <QKeyEvent>

class basic_keyboard_handler final : public QObject, public KeyboardHandlerBase
{
	Q_OBJECT
public:
	virtual void Init(const u32 max_connect) override;

	explicit basic_keyboard_handler(QObject* target = nullptr, QObject* parent = nullptr);

	bool eventFilter(QObject* obj, QEvent* ev);
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);
	void LoadSettings();
private:
	QObject* m_target;
};

#endif
