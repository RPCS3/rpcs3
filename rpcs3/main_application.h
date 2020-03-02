#pragma once

#include <string>
#include <QWindow>

struct EmuCallbacks;

class main_application
{
public:
	virtual void Init() = 0;

	static bool InitializeEmulator(const std::string& user, bool force_init, bool show_gui);

protected:
	virtual QThread* get_thread() = 0;

	EmuCallbacks CreateCallbacks();

	QWindow* m_game_window = nullptr; // (Currently) only needed so that pad handlers have a valid target for event filtering.
};
