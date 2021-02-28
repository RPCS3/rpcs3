#include "microphone_creator.h"

#include "Utilities/StrFmt.h"
#include "Utilities/StrUtil.h"

#include "3rdparty/OpenAL/include/alext.h"

constexpr auto qstr = QString::fromStdString;

microphone_creator::microphone_creator()
{
	setObjectName("microphone_creator");
	refresh_list();
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

	if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") == AL_TRUE)
	{
		if (const char* devices = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER); devices != nullptr)
		{
			while (*devices != 0)
			{
				m_microphone_list.append(qstr(devices));
				devices += strlen(devices) + 1;
			}
		}
	}
	else
	{
		// Without enumeration we can only use one device
		if (const char* device = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER); device != nullptr)
		{
			m_microphone_list.append(qstr(device));
		}
	}
}

QStringList microphone_creator::get_microphone_list()
{
	return m_microphone_list;
}

std::array<std::string, 4> microphone_creator::get_selection_list()
{
	return m_sel_list;
}

std::string microphone_creator::set_device(u32 num, const QString& text)
{
	if (text == get_none())
		m_sel_list[num - 1] = "";
	else
		m_sel_list[num - 1] = text.toStdString();

	const std::string final_list = m_sel_list[0] + "@@@" + m_sel_list[1] + "@@@" + m_sel_list[2] + "@@@" + m_sel_list[3] + "@@@";
	return final_list;
}

void microphone_creator::parse_devices(const std::string& list)
{
	for (u32 index = 0; index < 4; index++)
	{
		m_sel_list[index] = "";
	}

	const auto devices_list = fmt::split(list, { "@@@" });
	for (u32 index = 0; index < std::min<u32>(4, ::size32(devices_list)); index++)
	{
		m_sel_list[index] = devices_list[index];
	}
}
