#pragma once

#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"
#include "stdafx.h"
#include "Emu/System.h"

#include <QWindow>
#include <QKeyEvent>

class keyboard_pad_handler final : public QObject, public PadHandlerBase
{
public:
	bool Init() override;

	keyboard_pad_handler();

	void SetTargetWindow(QWindow* target);
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);

	bool eventFilter(QObject* obj, QEvent* ev) override;

	std::vector<std::string> ListDevices() override;
	bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) override;
	void ThreadProc() override;
	void ConfigController(const std::string& device) override;

	const std::string GetKeyName(const QKeyEvent* keyEvent);
	const std::string GetKeyName(const u32& keyCode);
	const u32 GetKeyCode(const std::string& keyName);

protected:
	void Key(const u32 code, bool pressed, u16 value = 255);

private:
	QWindow* m_target = nullptr;
	std::vector<std::shared_ptr<Pad>> bindings;
};
