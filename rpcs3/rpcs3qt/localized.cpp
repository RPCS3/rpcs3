#include "localized.h"
#include "Loader/PSF.h"

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

std::string Localized::GetStringFromU32(const u32& key, const std::map<u32, QString>& map, bool combined)
{
	QStringList string;

	if (combined)
	{
		for (const auto& item : map)
		{
			if (key & item.first)
			{
				string << item.second;
			}
		}
	}
	else
	{
		if (map.find(key) != map.end())
		{
			string << ::at32(map, key);
		}
	}

	if (string.isEmpty())
	{
		string << tr("Unknown");
	}

	return string.join(", ").toStdString();
}

Localized::resolution::resolution()
	: mode({
		{ psf::resolution_flag::_480p,      tr("480p") },
		{ psf::resolution_flag::_576p,      tr("576p") },
		{ psf::resolution_flag::_720p,      tr("720p") },
		{ psf::resolution_flag::_1080p,     tr("1080p") },
		{ psf::resolution_flag::_480p_16_9, tr("480p 16:9") },
		{ psf::resolution_flag::_576p_16_9, tr("576p 16:9") },
	})
{
}

Localized::sound::sound()
	: format({
		{ psf::sound_format_flag::lpcm_2,   tr("LPCM 2.0") },
		{ psf::sound_format_flag::lpcm_5_1, tr("LPCM 5.1") },
		{ psf::sound_format_flag::lpcm_7_1, tr("LPCM 7.1") },
		{ psf::sound_format_flag::ac3,      tr("Dolby Digital 5.1") },
		{ psf::sound_format_flag::dts,      tr("DTS 5.1") },
	})
{
}

Localized::title_t::title_t()
	: titles({
		{ "vsh/module/vsh.self", tr("The PS3 Interface (XMB, or VSH)") },
	})
{
}
