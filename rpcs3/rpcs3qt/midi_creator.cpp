#include "stdafx.h"
#include "midi_creator.h"

#include "Utilities/StrFmt.h"
#include "Utilities/StrUtil.h"

#include <rtmidi_c.h>

LOG_CHANNEL(cfg_log, "CFG");

midi_creator::midi_creator()
{
}

// We need to recreate the localized string because the midi creator is currently only created once.
QString midi_creator::get_none()
{
	return tr("None", "Midi device");
}

void midi_creator::refresh_list()
{
	m_midi_list.clear();
	m_midi_list.append(get_none());

	const auto deleter = [](RtMidiWrapper* ptr) { if (ptr) rtmidi_in_free(ptr); };
	std::unique_ptr<RtMidiWrapper, decltype(deleter)> midi_in(rtmidi_in_create_default());
	ensure(midi_in);

	cfg_log.notice("MIDI: Using %s api", rtmidi_api_name(rtmidi_in_get_current_api(midi_in.get())));

	if (!midi_in->ok)
	{
		cfg_log.error("Could not get MIDI in ptr: %s", midi_in->msg);
		return;
	}

	const s32 port_count = rtmidi_get_port_count(midi_in.get());

	if (port_count == -1)
	{
		cfg_log.error("Could not get MIDI port count: %s", midi_in->msg);
		return;
	}

	for (s32 port_number = 0; port_number < port_count; port_number++)
	{
		char buf[128]{};
		s32 size = sizeof(buf);
		if (rtmidi_get_port_name(midi_in.get(), port_number, buf, &size) == -1)
		{
			cfg_log.error("Error getting midi port name for port %d: %s", port_number, midi_in->msg);
			continue;
		}

		cfg_log.notice("Found midi device with name: %s", buf);
		m_midi_list.append(QString::fromUtf8(buf));
	}
}

QStringList midi_creator::get_midi_list() const
{
	return m_midi_list;
}

std::array<midi_device, max_midi_devices> midi_creator::get_selection_list() const
{
	return m_sel_list;
}

std::string midi_creator::set_device(u32 num, const midi_device& device)
{
	ensure(num < m_sel_list.size());

	m_sel_list[num] = device;

	if (device.name == get_none().toStdString())
	{
		m_sel_list[num].name.clear();
	}

	std::string result;

	for (const midi_device& device : m_sel_list)
	{
		fmt::append(result, "%s@@@", device);
	}

	return result;
}

void midi_creator::parse_devices(const std::string& list)
{
	m_sel_list = {};

	const std::vector<std::string> devices_list = fmt::split(list, { "@@@" });
	for (usz index = 0; index < std::min(m_sel_list.size(), devices_list.size()); index++)
	{
		m_sel_list[index] = midi_device::from_string(devices_list[index]);
	}
}
