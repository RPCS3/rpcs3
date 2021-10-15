#include "localized.h"

QString Localized::GetVerboseTimeByMs(quint64 elapsed_ms, bool show_days) const
{
	const quint64 elapsed_seconds = (elapsed_ms / 1000) + ((elapsed_ms % 1000) > 0 ? 1 : 0);
	quint64 hours = elapsed_seconds / 3600;

	quint64 days = 0;
	if (show_days)
	{
		days = hours / 24;
		hours = hours % 24;
	}
	const quint64 minutes = (elapsed_seconds % 3600) / 60;
	const quint64 seconds = (elapsed_seconds % 3600) % 60;

	QString str_days    = tr("%Ln day(s)", "", days);
	QString str_hours   = tr("%Ln hour(s)", "", hours);
	QString str_minutes = tr("%Ln minute(s)", "", minutes);
	QString str_seconds = tr("%Ln second(s)", "", seconds);

	if (days != 0)
	{
		if (hours == 0)
			return str_days;

		return tr("%0 and %1", "Days and hours").arg(str_days).arg(str_hours);
	}

	if (hours != 0)
	{
		if (minutes == 0)
			return str_hours;

		return tr("%0 and %1", "Hours and minutes").arg(str_hours).arg(str_minutes);
	}

	if (minutes != 0)
	{
		if (seconds != 0)
			return tr("%0 and %1", "Minutes and seconds").arg(str_minutes).arg(str_seconds);

		return str_minutes;
	}

	return str_seconds;
}
