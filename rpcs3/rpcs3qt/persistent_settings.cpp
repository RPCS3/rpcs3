#include "persistent_settings.h"

#include "util/logs.hpp"
#include "Emu/system_utils.hpp"

LOG_CHANNEL(cfg_log, "CFG");

persistent_settings::persistent_settings(QObject* parent) : settings(parent)
{
	// Don't use the .ini file ending for now, as it will be confused for a regular gui_settings file.
	m_settings = std::make_unique<QSettings>(ComputeSettingsDir() + gui::persistent::persistent_file_name + ".dat", QSettings::Format::IniFormat, parent);
}

void persistent_settings::SetPlaytime(const QString& serial, quint64 playtime, bool sync)
{
	m_playtime[serial] = playtime;
	SetValue(gui::persistent::playtime, serial, playtime, sync);
}

void persistent_settings::AddPlaytime(const QString& serial, quint64 elapsed, bool sync)
{
	const quint64 playtime = GetValue(gui::persistent::playtime, serial, 0).toULongLong();
	SetPlaytime(serial, playtime + elapsed, sync);
}

quint64 persistent_settings::GetPlaytime(const QString& serial)
{
	return m_playtime[serial];
}

void persistent_settings::SetLastPlayed(const QString& serial, const QString& date, bool sync)
{
	m_last_played[serial] = date;
	SetValue(gui::persistent::last_played, serial, date, sync);
}

QString persistent_settings::GetLastPlayed(const QString& serial)
{
	return m_last_played[serial];
}

QString persistent_settings::GetCurrentUser(const QString& fallback) const
{
	// Load user
	QString user = GetValue(gui::persistent::active_user).toString();
	if (user.isEmpty())
	{
		user = fallback;
	}

	// Set user if valid
	if (rpcs3::utils::check_user(user.toStdString()) > 0)
	{
		return user;
	}

	cfg_log.fatal("Could not parse user setting: '%s'.", user);
	return QString();
}
