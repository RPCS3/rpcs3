#include "localized.h"

Localized::Localized()
{
}

QString Localized::GetVerboseTimeByMs(qint64 elapsed_ms, bool show_days) const
{
	if (elapsed_ms <= 0)
	{
		return tr("Never played");
	}

	const qint64 elapsed_seconds = (elapsed_ms / 1000) + ((elapsed_ms % 1000) > 0 ? 1 : 0);
	qint64 hours = elapsed_seconds / 3600;

	const qint64 days = hours / 24;
	if(show_days)
		hours = hours % 24;
	const qint64 minutes = (elapsed_seconds % 3600) / 60;
	const qint64 seconds = (elapsed_seconds % 3600) % 60;

	QString str_days = tr("%Ln day(s)", "", days);
	QString str_hours = tr("%Ln hour(s)", "", hours);
	QString str_minutes = tr("%Ln minute(s)", "", minutes);
	QString str_seconds = tr("%Ln second(s)", "", seconds);

	if (show_days && days > 0)
	{
		if (hours == 0)
			return str_days;
		else
			return tr("%0 and %1", "Days and hours").arg(str_days).arg(str_hours);
	}
	else
	{
		if (hours > 0)
		{
			if (minutes == 0)
				return str_hours;
			else
				return tr("%0 and %1", "Hours and minutes").arg(str_hours).arg(str_minutes);
		}
		else
		{
			if (minutes > 0)
			{
				if (seconds != 0)
					return tr("%0 and %1", "Minutes and seconds").arg(str_minutes).arg(str_seconds);
				else
					return str_minutes;
			}
			else
			{
				return str_seconds;
			}
		}
	}
}
