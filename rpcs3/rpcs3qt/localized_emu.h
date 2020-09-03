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
	localized_emu() = default;

	template <typename... Args>
	static std::string get_string(localized_string_id id, Args&&... args)
	{
		return translated(id, std::forward<Args>(args)...).toStdString();
	}

	template <typename... Args>
	static std::u32string get_u32string(localized_string_id id, Args&&... args)
	{
		return translated(id, std::forward<Args>(args)...).toStdU32String();
	}

private:
	template <typename... Args>
	static QString translated(localized_string_id id, Args&&... args)
	{
		switch (id)
		{
		case localized_string_id::RSX_OVERLAYS_TROPHY_BRONZE: return tr("You have earned the bronze trophy\n%0").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_TROPHY_SILVER: return tr("You have earned the silver trophy\n%0").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_TROPHY_GOLD: return tr("You have earned the gold trophy\n%0").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_TROPHY_PLATINUM: return tr("You have earned the platinum trophy\n%0").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_COMPILING_SHADERS: return tr("Compiling shaders");
		case localized_string_id::RSX_OVERLAYS_MSG_DIALOG_YES: return tr("Yes");
		case localized_string_id::RSX_OVERLAYS_MSG_DIALOG_NO: return tr("No");
		case localized_string_id::RSX_OVERLAYS_MSG_DIALOG_CANCEL: return tr("Cancel");
		case localized_string_id::RSX_OVERLAYS_MSG_DIALOG_OK: return tr("OK");
		case localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_TITLE: return tr("Save Dialog");
		case localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_DELETE: return tr("Delete Save");
		case localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_LOAD: return tr("Load Save");
		case localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_SAVE: return tr("Save");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ACCEPT: return tr("Accept");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_CANCEL: return tr("Cancel");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_SPACE: return tr("Space");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_BACKSPACE: return tr("Backspace");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_SHIFT: return tr("Shift");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ENTER_TEXT: return tr("[Enter Text]");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ENTER_PASSWORD: return tr("[Enter Password]");
		case localized_string_id::RSX_OVERLAYS_LIST_SELECT: return tr("Select");
		case localized_string_id::RSX_OVERLAYS_LIST_CANCEL: return tr("Cancel");

		case localized_string_id::INVALID: return tr("Invalid");
		default: return tr("Unknown");
		}
	}
};
