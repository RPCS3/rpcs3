#include "stdafx.h"
#include "microphone_creator.h"

#include "Utilities/StrUtil.h"

#include "al.h"
#include "alc.h"

LOG_CHANNEL(cfg_log, "CFG");

microphone_creator::microphone_creator()
{
	setObjectName("microphone_creator");
}

// We need to recreate the localized string because the microphone creator is currently only created once.
QString microphone_creator::get_none()
{
	return tr("None", "Microphone device");
}

void microphone_creator::refresh_list()
{
	m_microphone_list.clear();
	m_microphone_list.append(get_none());

	if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT") == AL_TRUE)
	{
		if (const char* devices = alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER))
		{
			while (devices && *devices != 0)
			{
				cfg_log.notice("Found microphone: '%s'", devices);
				m_microphone_list.append(devices);
				devices += strlen(devices) + 1;
			}
		}
	}
	else
	{
		// Without enumeration we can only use one device
		cfg_log.error("OpenAl extension ALC_ENUMERATION_EXT not supported. The microphone list will only contain the default microphone.");

		if (const char* device = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER))
		{
			cfg_log.notice("Found default microphone: '%s'", device);
			m_microphone_list.append(device);
		}
	}
}

QStringList microphone_creator::get_microphone_list() const
{
	return m_microphone_list;
}

std::array<std::string, 4> microphone_creator::get_selection_list() const
{
	return m_sel_list;
}

std::string microphone_creator::set_device(u32 num, const QString& text)
{
	ensure(num < m_sel_list.size());

	if (text == get_none())
		m_sel_list[num].clear();
	else
		m_sel_list[num] = text.toStdString();

	return m_sel_list[0] + "@@@" + m_sel_list[1] + "@@@" + m_sel_list[2] + "@@@" + m_sel_list[3] + "@@@";
}

void microphone_creator::parse_devices(const std::string& list)
{
	m_sel_list = {};

	const std::vector<std::string> devices_list = fmt::split(list, { "@@@" });
	for (usz index = 0; index < std::min(m_sel_list.size(), devices_list.size()); index++)
	{
		m_sel_list[index] = devices_list[index];
	}
}
