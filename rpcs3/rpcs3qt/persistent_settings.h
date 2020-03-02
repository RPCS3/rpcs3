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

		// Date format
		const QString last_played_date_format_old    = "MMMM d yyyy";
		const QString last_played_date_format_new    = "MMMM d, yyyy";
		const Qt::DateFormat last_played_date_format = Qt::DateFormat::ISODate;
	}
}

// Provides a persistent settings class that won't be affected by settings dialog changes
class persistent_settings : public settings
{
	Q_OBJECT

public:
	explicit persistent_settings(QObject* parent = nullptr);

public Q_SLOTS:
	void SetPlaytime(const QString& serial, const qint64& elapsed);
	qint64 GetPlaytime(const QString& serial);

	void SetLastPlayed(const QString& serial, const QString& date);
	QString GetLastPlayed(const QString& serial);
private:
	QMap<QString, qint64> m_playtime;
	QMap<QString, QString> m_last_played;
};
