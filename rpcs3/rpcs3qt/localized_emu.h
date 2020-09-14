#pragma once

#include <string>
#include <QObject>

#include "Emu/localized_string_id.h"

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
		case localized_string_id::RSX_OVERLAYS_TROPHY_BRONZE: return tr("You have earned the bronze trophy\n%0", "Trophy text").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_TROPHY_SILVER: return tr("You have earned the silver trophy\n%0", "Trophy text").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_TROPHY_GOLD: return tr("You have earned the gold trophy\n%0", "Trophy text").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_TROPHY_PLATINUM: return tr("You have earned the platinum trophy\n%0", "Trophy text").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_COMPILING_SHADERS: return tr("Compiling shaders");
		case localized_string_id::RSX_OVERLAYS_MSG_DIALOG_YES: return tr("Yes", "Message Dialog");
		case localized_string_id::RSX_OVERLAYS_MSG_DIALOG_NO: return tr("No", "Message Dialog");
		case localized_string_id::RSX_OVERLAYS_MSG_DIALOG_CANCEL: return tr("Cancel", "Message Dialog");
		case localized_string_id::RSX_OVERLAYS_MSG_DIALOG_OK: return tr("OK", "Message Dialog");
		case localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_TITLE: return tr("Save Dialog", "Save Dialog");
		case localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_DELETE: return tr("Delete Save", "Save Dialog");
		case localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_LOAD: return tr("Load Save", "Save Dialog");
		case localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_SAVE: return tr("Save", "Save Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ACCEPT: return tr("Accept", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_CANCEL: return tr("Cancel", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_SPACE: return tr("Space", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_BACKSPACE: return tr("Backspace", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_SHIFT: return tr("Shift", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ENTER_TEXT: return tr("[Enter Text]", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ENTER_PASSWORD: return tr("[Enter Password]", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_LIST_SELECT: return tr("Select", "Save Dialog List");
		case localized_string_id::RSX_OVERLAYS_LIST_CANCEL: return tr("Cancel", "Save Dialog List");
		case localized_string_id::CELL_GAME_ERROR_BROKEN_GAMEDATA: return tr("ERROR: Game data is corrupted. The application will continue.", "Game Error");
		case localized_string_id::CELL_GAME_ERROR_BROKEN_HDDGAME: return tr("ERROR: HDD boot game is corrupted. The application will continue.", "Game Error");
		case localized_string_id::CELL_GAME_ERROR_BROKEN_EXIT_GAMEDATA: return tr("ERROR: Game data is corrupted. The application will be terminated.", "Game Error");
		case localized_string_id::CELL_GAME_ERROR_BROKEN_EXIT_HDDGAME: return tr("ERROR: HDD boot game is corrupted. The application will be terminated.", "Game Error");
		case localized_string_id::CELL_GAME_ERROR_NOSPACE: return tr("ERROR: Not enough available space. The application will continue.\nSpace needed: %0 KB", "Game Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_GAME_ERROR_NOSPACE_EXIT: return tr("ERROR: Not enough available space. The application will be terminated.\nSpace needed: %0 KB", "Game Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_GAME_ERROR_DIR_NAME: return tr("Directory name: %0", "Game Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_GAME_DATA_EXIT_BROKEN: return tr("There has been an error!\n\nPlease remove the game data for this title.", "Game Error");
		case localized_string_id::CELL_HDD_GAME_EXIT_BROKEN: return tr("There has been an error!\n\nPlease reinstall the HDD boot game.", "Game Error");
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010001: return tr("The resource is temporarily unavailable.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010002: return tr("Invalid argument or flag.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010003: return tr("The feature is not yet implemented.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010004: return tr("Memory allocation failed.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010005: return tr("The resource with the specified identifier does not exist.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010006: return tr("The file does not exist.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010007: return tr("The file is in unrecognized format / The file is not a valid ELF file.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010008: return tr("Resource deadlock is avoided.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010009: return tr("Operation not permitted.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001000A: return tr("The device or resource is busy.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001000B: return tr("The operation is timed out.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001000C: return tr("The operation is aborted.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001000D: return tr("Invalid memory access.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001000F: return tr("State of the target thread is invalid.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010010: return tr("Alignment is invalid.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010011: return tr("Shortage of the kernel resources.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010012: return tr("The file is a directory.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010013: return tr("Operation cancelled.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010014: return tr("Entry already exists.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010015: return tr("Port is already connected.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010016: return tr("Port is not connected.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010017: return tr("Failure in authorizing SELF. Program authentication fail.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010018: return tr("The file is not MSELF.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010019: return tr("System version error.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001001A: return tr("Fatal system error occurred while authorizing SELF. SELF auth failure.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001001B: return tr("Math domain violation.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001001C: return tr("Math range violation.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001001D: return tr("Illegal multi-byte sequence in input.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001001E: return tr("File position error.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001001F: return tr("Syscall was interrupted.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010020: return tr("File too large.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010021: return tr("Too many links.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010022: return tr("File table overflow.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010023: return tr("No space left on device.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010024: return tr("Not a TTY.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010025: return tr("Broken pipe.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010026: return tr("Read-only filesystem.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010027: return tr("Illegal seek.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010028: return tr("Arg list too long.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010029: return tr("Access violation.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001002A: return tr("Invalid file descriptor.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001002B: return tr("Filesystem mounting failed.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001002C: return tr("Too many files open.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001002D: return tr("No device.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001002E: return tr("Not a directory.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001002F: return tr("No such device or IO.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010030: return tr("Cross-device link error.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010031: return tr("Bad Message.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010032: return tr("In progress.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010033: return tr("Message size error.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010034: return tr("Name too long.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010035: return tr("No lock.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010036: return tr("Not empty.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010037: return tr("Not supported.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010038: return tr("File-system specific error.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010039: return tr("Overflow occured.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001003A: return tr("Filesystem not mounted.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001003B: return tr("Not SData.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001003C: return tr("Incorrect version in sys_load_param.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001003D: return tr("Pointer is null.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001003E: return tr("Pointer is null.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_DEFAULT: return tr("An error has occurred.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_OSK_DIALOG_TITLE: return tr("On Screen Keyboard", "OSK Dialog");
		case localized_string_id::CELL_SAVEDATA_CB_BROKEN: return tr("Error - Save data corrupted", "Savedata Error");
		case localized_string_id::CELL_SAVEDATA_CB_FAILURE: return tr("Error - Failed to save or load", "Savedata Error");
		case localized_string_id::CELL_SAVEDATA_CB_NO_DATA: return tr("Error - Save data cannot be found", "Savedata Error");
		case localized_string_id::CELL_SAVEDATA_CB_NO_SPACE: return tr("Error - Insufficient free space\n\nSpace needed: %0 KB", "Savedata Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_SAVEDATA_CREATE_CONFIRMATION: return tr("Create new Save Data?", "Savedata Dialog");
		case localized_string_id::CELL_SAVEDATA_DELETE_CONFIRMATION: return tr("Do you really want to delete this entry?\n\n%0", "Savedata entry info").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_SAVEDATA_DELETE_SUCCESS: return tr("Successfully removed entry!\n\n%0", "Savedata entry info").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_SAVEDATA_DELETE: return tr("Delete this entry?\n\n%0", "Savedata entry info").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_SAVEDATA_LOAD: return tr("Load this entry?\n\n%0", "Savedata entry info").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_SAVEDATA_OVERWRITE: return tr("Overwrite this entry?\n\n%0", "Savedata entry info").arg(std::forward<Args>(args)...);

		case localized_string_id::INVALID: return tr("Invalid");
		default: return tr("Unknown");
		}
	}
};
