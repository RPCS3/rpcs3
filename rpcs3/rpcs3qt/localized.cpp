#include "localized.h"

Localized::Localized()
{
}

QString Localized::GetVerboseTimeByMs(qint64 elapsed_ms) const
{
	if (elapsed_ms <= 0)
	{
		return "";
	}

	const qint64 elapsed_seconds = (elapsed_ms / 1000) + ((elapsed_ms % 1000) > 0 ? 1 : 0);

	const qint64 hours   = elapsed_seconds / 3600;
	const qint64 minutes = (elapsed_seconds % 3600) / 60;
	const qint64 seconds = (elapsed_seconds % 3600) % 60;

	// For anyone who was wondering why there need to be so many cases:
	// 1. Using variables won't work for future localization due to varying sentence structure in different languages.
	// 2. The provided Qt functionality only works if localization is already enabled
	// 3. The provided Qt functionality only works for single variables

	if (hours <= 0)
	{
		if (hours <= 0)
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
