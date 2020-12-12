#pragma once

#include "stdafx.h"

#include <QString>
#include <QStringList>
#include <QObject>

#include <array>

class microphone_creator : public QObject
{
	Q_OBJECT

public:
	microphone_creator();
	QString get_none();
	std::string set_device(u32 num, const QString& text);
	void parse_devices(const std::string& list);
	void refresh_list();
	QStringList get_microphone_list();
	std::array<std::string, 4> get_selection_list();

private:
	QStringList m_microphone_list;
	std::array<std::string, 4> m_sel_list;
};
