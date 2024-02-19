#pragma once

#include "util/types.hpp"
#include "Emu/Io/midi_config_types.h"

#include <QObject>
#include <QStringList>

class midi_creator : public QObject
{
	Q_OBJECT

public:
	midi_creator();
	QString get_none();
	std::string set_device(u32 num, const midi_device& device);
	void parse_devices(const std::string& list);
	void refresh_list();
	QStringList get_midi_list() const;
	std::array<midi_device, max_midi_devices> get_selection_list() const;

private:
	QStringList m_midi_list;
	std::array<midi_device, max_midi_devices> m_sel_list;
};
