#pragma once

#include "settings.h"

namespace gui
{
	namespace persistent
	{
		// File name
		const QString persistent_file_name = "persistent_settings";

		// Entry names
		const QString playtime    = "Playtime";
		const QString last_played = "LastPlayed";
		const QString notes       = "Notes";
		const QString titles      = "Titles";

		// Date format
		const QString last_played_date_format_old                  = "MMMM d yyyy";
		const QString last_played_date_format_new                  = "MMMM d, yyyy";
		const QString last_played_date_with_time_of_day_format     = "MMMM d, yyyy HH:mm";
		const Qt::DateFormat last_played_date_format               = Qt::DateFormat::ISODate;

		// GUI Saves
		const gui_save save_notes  = gui_save(savedata, "notes",       QVariantMap());
		const gui_save active_user = gui_save(users,    "active_user", "");
	}
}

// Provides a persistent settings class that won't be affected by settings dialog changes
class persistent_settings : public settings
{
	Q_OBJECT

public:
	explicit persistent_settings(QObject* parent = nullptr);

	QString GetCurrentUser(const QString& fallback) const;

public Q_SLOTS:
	void SetPlaytime(const QString& serial, quint64 playtime, bool sync);
	void AddPlaytime(const QString& serial, quint64 elapsed, bool sync);
	quint64 GetPlaytime(const QString& serial);

	void SetLastPlayed(const QString& serial, const QString& date, bool sync);
	QString GetLastPlayed(const QString& serial);
private:
	std::map<QString, quint64> m_playtime;
	std::map<QString, QString> m_last_played;
};
