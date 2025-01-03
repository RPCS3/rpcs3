#pragma once

#include <string>
#include <QThread>

struct EmuCallbacks;
class gs_frame;

class main_application
{
public:
	virtual bool Init() = 0;

	static void InitializeEmulator(const std::string& user, bool show_gui);

	void SetActiveUser(const std::string& user)
	{
		m_active_user = user;
	}

protected:
	virtual QThread* get_thread() = 0;

	void OnEmuSettingsChange();

	EmuCallbacks CreateCallbacks();

	std::string m_active_user;
	gs_frame* m_game_window = nullptr;
};
