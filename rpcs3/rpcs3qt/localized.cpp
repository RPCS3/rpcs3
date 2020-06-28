#include "localized.h"

Localized::Localized()
{
}

QString Localized::GetVerboseTimeByMs(qint64 elapsed_ms, bool show_days) const
{
	if (elapsed_ms <= 0)
	{
		return "";
	}

	const qint64 elapsed_seconds = (elapsed_ms / 1000) + ((elapsed_ms % 1000) > 0 ? 1 : 0);

	qint64 hours = elapsed_seconds / 3600;

	// For anyone who was wondering why there need to be so many cases:
	// 1. Using variables won't work for future localization due to varying sentence structure in different languages.
	// 2. The provided Qt functionality only works if localization is already enabled
	// 3. The provided Qt functionality only works for single variables

	if (show_days && hours >= 24)
	{
		const qint64 days  = hours / 24;
		hours = hours % 24;

		if (hours <= 0)
		{
			if (days == 1)
			{
				return tr("%0 day").arg(days);
			}
			return tr("%0 days").arg(days);
		}
		if (days == 1 && hours == 1)
		{
			return tr("%0 day and %1 hour").arg(days).arg(hours);
		}
		if (days == 1)
		{
			return tr("%0 day and %1 hours").arg(days).arg(hours);
		}
		if (hours == 1)
		{
			return tr("%0 days and %1 hour").arg(days).arg(hours);
		}
		return tr("%0 days and %1 hours").arg(days).arg(hours);
	}

	const qint64 minutes = (elapsed_seconds % 3600) / 60;
	const qint64 seconds = (elapsed_seconds % 3600) % 60;

	if (hours <= 0)
	{
		if (minutes <= 0)
		{
			if (seconds == 1)
			{
				return tr("%0 second").arg(seconds);
			}
			return tr("%0 seconds").arg(seconds);
		}

		if (seconds <= 0)
		{
			if (minutes == 1)
			{
				return tr("%0 minute").arg(minutes);
			}
			return tr("%0 minutes").arg(minutes);
		}
		if (minutes == 1 && seconds == 1)
		{
			return tr("%0 minute and %1 second").arg(minutes).arg(seconds);
		}
		if (minutes == 1)
		{
			return tr("%0 minute and %1 seconds").arg(minutes).arg(seconds);
		}
		if (seconds == 1)
		{
			return tr("%0 minutes and %1 second").arg(minutes).arg(seconds);
		}
		return tr("%0 minutes and %1 seconds").arg(minutes).arg(seconds);
	}

	if (minutes <= 0)
	{
		if (hours == 1)
		{
			return tr("%0 hour").arg(hours);
		}
		return tr("%0 hours").arg(hours);
	}
	if (hours == 1 && minutes == 1)
	{
		return tr("%0 hour and %1 minute").arg(hours).arg(minutes);
	}
	if (hours == 1)
	{
		return tr("%0 hour and %1 minutes").arg(hours).arg(minutes);
	}
	if (minutes == 1)
	{
		return tr("%0 hours and %1 minute").arg(hours).arg(minutes);
	}
	return tr("%0 hours and %1 minutes").arg(hours).arg(minutes);
}
