﻿#include "localized.h"

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
			return tr("%n day(s)", "", days);
		}
		if (days == 1)
		{
			return tr("%0 day and %n hour(s)", "", hours).arg(days);
		}
		return tr("%0 days and %n hour(s)", "", hours).arg(days);
	}

	const qint64 minutes = (elapsed_seconds % 3600) / 60;
	const qint64 seconds = (elapsed_seconds % 3600) % 60;

	if (hours <= 0)
	{
		if (minutes <= 0)
		{
			return tr("%n second(s)", "", seconds);
		}

		if (seconds <= 0)
		{
			return tr("%n minute(s)", "", minutes);
		}

		if (minutes == 1)
		{
			return tr("%0 minute and %n second(s)", "", seconds).arg(minutes);
		}
		
		return tr("%0 minutes and %n second(s)", "", seconds).arg(minutes);
	}

	if (minutes <= 0)
	{
		return tr("%n hour(s)", "", hours);
	}

	if (hours == 1)
	{
		return tr("%0 hour and %n minute(s)", "", minutes).arg(hours);
	}
	
	return tr("%0 hours and %n minute(s)", "", minutes).arg(hours);
}
