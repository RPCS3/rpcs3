#include <QString>
#include "localized_emu.h"

localized_emu::localized_emu()
{
}

QString localized_emu::translated(localized_string_id id)
{
	switch (id)
	{
	case localized_string_id::RSX_OVERLAYS_TROPHY_BRONZE: return tr("You have earned the bronze trophy\n");
	case localized_string_id::RSX_OVERLAYS_TROPHY_SILVER: return tr("You have earned the silver trophy\n");
	case localized_string_id::RSX_OVERLAYS_TROPHY_GOLD: return tr("You have earned the gold trophy\n");
	case localized_string_id::RSX_OVERLAYS_TROPHY_PLATINUM: return tr("You have earned the platinum trophy\n");
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

std::string localized_emu::get_string(localized_string_id id)
{
	return translated(id).toStdString();
}

std::u32string localized_emu::get_u32string(localized_string_id id)
{
	return translated(id).toStdU32String();
}
