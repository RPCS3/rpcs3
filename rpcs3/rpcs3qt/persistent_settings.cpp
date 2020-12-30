#include "persistent_settings.h"

#include "util/logs.hpp"

LOG_CHANNEL(cfg_log, "CFG");

persistent_settings::persistent_settings(QObject* parent) : settings(parent)
{
	// Don't use the .ini file ending for now, as it will be confused for a regular gui_settings file.
	m_settings.reset(new QSettings(ComputeSettingsDir() + gui::persistent::persistent_file_name + ".dat", QSettings::Format::IniFormat, parent));
}

void persistent_settings::SetPlaytime(const QString& serial, quint64 playtime)
{
	m_playtime[serial] = playtime;
	SetValue(gui::persistent::playtime, serial, playtime);
}

void persistent_settings::AddPlaytime(const QString& serial, quint64 elapsed)
{
	const quint64 playtime = GetValue(gui::persistent::playtime, serial, 0).toULongLong();
	SetPlaytime(serial, playtime + elapsed);
}

quint64 persistent_settings::GetPlaytime(const QString& serial)
{
	return m_playtime[serial];
}

void persistent_settings::SetLastPlayed(const QString& serial, const QString& date)
{
	m_last_played[serial] = date;
	SetValue(gui::persistent::last_played, serial, date);
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

	bool is_valid_user;
	const u32 user_id = user.toInt(&is_valid_user);

	// Set user if valid
	if (is_valid_user && user_id > 0)
	{
		return user;
	}

	cfg_log.fatal("Could not parse user setting: '%s' = '%d'.", user.toStdString(), user_id);
	return QString();
}
