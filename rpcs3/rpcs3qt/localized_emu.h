#pragma once

#include <string>
#include <QObject>

#include "Emu/localized_string.h"

/**
 * Localized emucore string collection class
 * Due to special characters this file should stay in UTF-8 format
 */
class localized_emu : public QObject
{
	Q_OBJECT

public:
	localized_emu();

	static std::string get_string(localized_string_id id);
	static std::u32string get_u32string(localized_string_id id);

private:
	static QString translated(localized_string_id id);
};
