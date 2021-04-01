#pragma once

#include <string>
#include <QWindow>

struct EmuCallbacks;

class main_application
{
public:
	virtual bool Init() = 0;

	static void InitializeEmulator(const std::string& user, bool show_gui);

	void SetActiveUser(std::string user)
	{
		m_active_user = user;
	}

protected:
	virtual QThread* get_thread() = 0;

	EmuCallbacks CreateCallbacks();

	std::string m_active_user;
	QWindow* m_game_window = nullptr; // (Currently) only needed so that pad handlers have a valid target for event filtering.
};
