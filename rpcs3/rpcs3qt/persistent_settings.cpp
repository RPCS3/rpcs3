#include "persistent_settings.h"

persistent_settings::persistent_settings(QObject* parent) : settings(parent)
{
	// Don't use the .ini file ending for now, as it will be confused for a regular gui_settings file.
	m_settings = new QSettings(ComputeSettingsDir() + gui::persistent::persistent_file_name + ".dat", QSettings::Format::IniFormat, parent);
}

void persistent_settings::SetPlaytime(const QString& serial, const qint64& elapsed)
{
	m_playtime[serial] = std::max<qint64>(0, elapsed);
	SetValue(gui::persistent::playtime, serial, m_playtime[serial]);
}

qint64 persistent_settings::GetPlaytime(const QString& serial)
{
	return std::max<qint64>(0, m_playtime[serial]);
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
