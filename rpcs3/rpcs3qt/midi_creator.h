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
	virtual ~midi_creator();

	QString get_none() const;
	std::string set_device(u32 num, const midi_device& device);
	void parse_devices(std::string_view list);
	void refresh_list();
	const QStringList& get_midi_list() const;
	const std::array<midi_device, max_midi_devices>& get_selection_list() const;

	// Wait for the async rtmidi initialization to finish
	static void wait_for_init();

private:
	QStringList m_midi_list;
	std::array<midi_device, max_midi_devices> m_sel_list;
};
