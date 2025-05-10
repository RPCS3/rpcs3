#pragma once

#include <string>
#include <QObject>

#include "Emu/localized_string_id.h"
#include "Emu/Io/pad_types.h"

/**
 * Localized emucore string collection class
 * Due to special characters this file should stay in UTF-8 format
 */
class localized_emu : public QObject
{
	Q_OBJECT

public:
	localized_emu() = default;

	static QString translated_pad_button(pad_button btn);
	static QString translated_mouse_button(int btn);

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
		case localized_string_id::RSX_OVERLAYS_TROPHY_BRONZE: return tr("You have earned a bronze trophy.\n%0", "Trophy text").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_TROPHY_SILVER: return tr("You have earned a silver trophy.\n%0", "Trophy text").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_TROPHY_GOLD: return tr("You have earned a gold trophy.\n%0", "Trophy text").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_TROPHY_PLATINUM: return tr("You have earned a platinum trophy.\n%0", "Trophy text").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_COMPILING_SHADERS: return tr("Compiling shaders");
		case localized_string_id::RSX_OVERLAYS_COMPILING_PPU_MODULES: return tr("Compiling PPU Modules");
		case localized_string_id::RSX_OVERLAYS_MSG_DIALOG_YES: return tr("Yes", "Message Dialog");
		case localized_string_id::RSX_OVERLAYS_MSG_DIALOG_NO: return tr("No", "Message Dialog");
		case localized_string_id::RSX_OVERLAYS_MSG_DIALOG_CANCEL: return tr("Back", "Message Dialog");
		case localized_string_id::RSX_OVERLAYS_MSG_DIALOG_OK: return tr("OK", "Message Dialog");
		case localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_TITLE: return tr("Save Dialog", "Save Dialog");
		case localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_DELETE: return tr("Delete Save", "Save Dialog");
		case localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_LOAD: return tr("Load Save", "Save Dialog");
		case localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_SAVE: return tr("Save", "Save Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ACCEPT: return tr("Enter", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_CANCEL: return tr("Back", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_SPACE: return tr("Space", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_BACKSPACE: return tr("Backspace", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_SHIFT: return tr("Shift", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ENTER_TEXT: return tr("[Enter Text]", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ENTER_PASSWORD: return tr("[Enter Password]", "OSK Dialog");
		case localized_string_id::RSX_OVERLAYS_MEDIA_DIALOG_TITLE: return tr("Select media", "Media dialog");
		case localized_string_id::RSX_OVERLAYS_MEDIA_DIALOG_TITLE_PHOTO_IMPORT: return tr("Select photo to import", "Media dialog");
		case localized_string_id::RSX_OVERLAYS_MEDIA_DIALOG_EMPTY: return tr("No media found.", "Media dialog");
		case localized_string_id::RSX_OVERLAYS_LIST_SELECT: return tr("Enter", "Enter Dialog List");
		case localized_string_id::RSX_OVERLAYS_LIST_CANCEL: return tr("Back", "Cancel Dialog List");
		case localized_string_id::RSX_OVERLAYS_LIST_DENY: return tr("Deny", "Deny Dialog List");
		case localized_string_id::RSX_OVERLAYS_PRESSURE_INTENSITY_TOGGLED_OFF: return tr("Pressure intensity mode of player %0 disabled", "Pressure intensity toggled off").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_PRESSURE_INTENSITY_TOGGLED_ON: return tr("Pressure intensity mode of player %0 enabled", "Pressure intensity toggled on").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_ANALOG_LIMITER_TOGGLED_OFF: return tr("Analog limiter of player %0 disabled", "Analog limiter toggled off").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_ANALOG_LIMITER_TOGGLED_ON: return tr("Analog limiter of player %0 enabled", "Analog limiter toggled on").arg(std::forward<Args>(args)...);
		case localized_string_id::RSX_OVERLAYS_MOUSE_AND_KEYBOARD_EMULATED: return tr("Mouse and keyboard are now used as emulated devices.", "Mouse and keyboard emulated");
		case localized_string_id::RSX_OVERLAYS_MOUSE_AND_KEYBOARD_PAD: return tr("Mouse and keyboard are now used as pad.", "Mouse and keyboard pad");
		case localized_string_id::CELL_GAME_ERROR_BROKEN_GAMEDATA: return tr("ERROR: Game data is corrupted. The application will continue.", "Game Error");
		case localized_string_id::CELL_GAME_ERROR_BROKEN_HDDGAME: return tr("ERROR: HDD boot game is corrupted. The application will continue.", "Game Error");
		case localized_string_id::CELL_GAME_ERROR_BROKEN_EXIT_GAMEDATA: return tr("ERROR: Game data is corrupted. The application will be terminated.", "Game Error");
		case localized_string_id::CELL_GAME_ERROR_BROKEN_EXIT_HDDGAME: return tr("ERROR: HDD boot game is corrupted. The application will be terminated.", "Game Error");
		case localized_string_id::CELL_GAME_ERROR_NOSPACE: return tr("ERROR: Not enough available space. The application will continue.\nSpace needed: %0 KB", "Game Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_GAME_ERROR_NOSPACE_EXIT: return tr("ERROR: Not enough available space. The application will be terminated.\nSpace needed: %0 KB", "Game Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_GAME_ERROR_DIR_NAME: return tr("Directory name: %0", "Game Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_GAME_DATA_EXIT_BROKEN: return tr("There has been an error!\n\nPlease remove the game data for this title.", "Game Error");
		case localized_string_id::CELL_HDD_GAME_EXIT_BROKEN: return tr("There has been an error!\n\nPlease reinstall the HDD boot game.", "Game Error");
		case localized_string_id::CELL_HDD_GAME_CHECK_NOSPACE: return tr("Not enough space to create HDD boot game.\nSpace Needed: %0 KB", "HDD Game Check Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_HDD_GAME_CHECK_BROKEN: return tr("HDD boot game %0 is corrupt!", "HDD Game Check Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_HDD_GAME_CHECK_NODATA: return tr("HDD boot game %0 could not be found!", "HDD Game Check Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_HDD_GAME_CHECK_INVALID: return tr("Error: %0", "HDD Game Check Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_GAMEDATA_CHECK_NOSPACE: return tr("Not enough space to create game data.\nSpace Needed: %0 KB", "Gamedata Check Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_GAMEDATA_CHECK_BROKEN: return tr("The game data in %0 is corrupt!", "Gamedata Check Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_GAMEDATA_CHECK_NODATA: return tr("The game data in %0 could not be found!", "Gamedata Check Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_GAMEDATA_CHECK_INVALID: return tr("Error: %0", "Gamedata Check Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010001: return tr("The resource is temporarily unavailable.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010002: return tr("Invalid argument or flag.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010003: return tr("The feature is not yet implemented.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010004: return tr("Memory allocation failed.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010005: return tr("The resource with the specified identifier does not exist.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010006: return tr("The file does not exist.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010007: return tr("The file is in an unrecognized format / The file is not a valid ELF file.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
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
		case localized_string_id::CELL_MSG_DIALOG_ERROR_80010039: return tr("Overflow occurred.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001003A: return tr("Filesystem not mounted.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001003B: return tr("Not SData.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001003C: return tr("Incorrect version in sys_load_param.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001003D: return tr("Pointer is null.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_8001003E: return tr("Pointer is null.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_MSG_DIALOG_ERROR_DEFAULT: return tr("An error has occurred.\n(%0)", "Error code").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_OSK_DIALOG_TITLE: return tr("On Screen Keyboard", "OSK Dialog");
		case localized_string_id::CELL_OSK_DIALOG_BUSY: return tr("The Home Menu can't be opened while the On Screen Keyboard is busy!", "OSK Dialog");
		case localized_string_id::CELL_SAVEDATA_CB_BROKEN: return tr("Error - Save data corrupted", "Savedata Error");
		case localized_string_id::CELL_SAVEDATA_CB_FAILURE: return tr("Error - Failed to save or load", "Savedata Error");
		case localized_string_id::CELL_SAVEDATA_CB_NO_DATA: return tr("Error - Save data cannot be found", "Savedata Error");
		case localized_string_id::CELL_SAVEDATA_CB_NO_SPACE: return tr("Error - Insufficient free space\n\nSpace needed: %0 KB", "Savedata Error").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_SAVEDATA_NO_DATA: return tr("There is no saved data.", "Savedata entry info");
		case localized_string_id::CELL_SAVEDATA_NEW_SAVED_DATA_TITLE: return tr("New Saved Data", "Savedata Dialog");
		case localized_string_id::CELL_SAVEDATA_NEW_SAVED_DATA_SUB_TITLE: return tr("Select to create a new entry", "Savedata Dialog");
		case localized_string_id::CELL_SAVEDATA_SAVE_CONFIRMATION: return tr("Do you want to save this data?", "Savedata Dialog");
		case localized_string_id::CELL_SAVEDATA_DELETE_CONFIRMATION: return tr("Do you really want to delete this data?\n\n%0", "Savedata entry info").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_SAVEDATA_DELETE_SUCCESS: return tr("Successfully removed data!\n\n%0", "Savedata entry info").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_SAVEDATA_DELETE: return tr("Delete this data?\n\n%0", "Savedata entry info").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_SAVEDATA_LOAD: return tr("Load this data?\n\n%0", "Savedata entry info").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_SAVEDATA_OVERWRITE: return tr("Do you want to overwrite the saved data?\n\n%0", "Savedata entry info").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_SAVEDATA_AUTOSAVE: return tr("Saving...");
		case localized_string_id::CELL_SAVEDATA_AUTOLOAD: return tr("Loading...");
		case localized_string_id::CELL_CROSS_CONTROLLER_MSG: return tr("Start [%0] on the PS Vita system.\nIf you have not installed [%0], go to [Remote Play] on the PS Vita system and start [Cross-Controller] from the LiveArea™ screen.", "Cross-Controller message").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_CROSS_CONTROLLER_FW_MSG: return tr("If your system software version on the PS Vita system is earlier than 1.80, you must update the system software to the latest version.", "Cross-Controller firmware message");
		case localized_string_id::CELL_NP_RECVMESSAGE_DIALOG_TITLE: return tr("Select Message", "RECVMESSAGE_DIALOG");
		case localized_string_id::CELL_NP_RECVMESSAGE_DIALOG_TITLE_INVITE: return tr("Select Invite", "RECVMESSAGE_DIALOG");
		case localized_string_id::CELL_NP_RECVMESSAGE_DIALOG_TITLE_ADD_FRIEND: return tr("Add Friend", "RECVMESSAGE_DIALOG");
		case localized_string_id::CELL_NP_RECVMESSAGE_DIALOG_FROM: return tr("From:", "RECVMESSAGE_DIALOG");
		case localized_string_id::CELL_NP_RECVMESSAGE_DIALOG_SUBJECT: return tr("Subject:", "RECVMESSAGE_DIALOG");
		case localized_string_id::CELL_NP_SENDMESSAGE_DIALOG_TITLE: return tr("Select Message To Send", "SENDMESSAGE_DIALOG");
		case localized_string_id::CELL_NP_SENDMESSAGE_DIALOG_TITLE_INVITE: return tr("Send Invite", "SENDMESSAGE_DIALOG");
		case localized_string_id::CELL_NP_SENDMESSAGE_DIALOG_TITLE_ADD_FRIEND: return tr("Add Friend", "SENDMESSAGE_DIALOG");
		case localized_string_id::CELL_NP_SENDMESSAGE_DIALOG_CONFIRMATION: return tr("Send message to %0 ?\n\nSubject:", "SENDMESSAGE_DIALOG").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_NP_SENDMESSAGE_DIALOG_CONFIRMATION_INVITE: return tr("Send invite to %0 ?\n\nSubject:", "SENDMESSAGE_DIALOG").arg(std::forward<Args>(args)...);
		case localized_string_id::CELL_NP_SENDMESSAGE_DIALOG_CONFIRMATION_ADD_FRIEND: return tr("Send friend request to %0 ?\n\nSubject:", "SENDMESSAGE_DIALOG").arg(std::forward<Args>(args)...);
		case localized_string_id::RECORDING_ABORTED: return tr("Recording aborted!");
		case localized_string_id::RPCN_NO_ERROR: return tr("RPCN: No Error");
		case localized_string_id::RPCN_ERROR_INVALID_INPUT: return tr("RPCN: Invalid Input (Wrong Host/Port)");
		case localized_string_id::RPCN_ERROR_WOLFSSL: return tr("RPCN Connection Error: WolfSSL Error");
		case localized_string_id::RPCN_ERROR_RESOLVE: return tr("RPCN Connection Error: Resolve Error");
		case localized_string_id::RPCN_ERROR_BINDING: return tr("RPCN Connection Error: Failed to bind to given binding IP");
		case localized_string_id::RPCN_ERROR_CONNECT: return tr("RPCN Connection Error");
		case localized_string_id::RPCN_ERROR_LOGIN_ERROR: return tr("RPCN Login Error: Identification Error");
		case localized_string_id::RPCN_ERROR_ALREADY_LOGGED: return tr("RPCN Login Error: User Already Logged In");
		case localized_string_id::RPCN_ERROR_INVALID_LOGIN: return tr("RPCN Login Error: Invalid Username");
		case localized_string_id::RPCN_ERROR_INVALID_PASSWORD: return tr("RPCN Login Error: Invalid Password");
		case localized_string_id::RPCN_ERROR_INVALID_TOKEN: return tr("RPCN Login Error: Invalid Token");
		case localized_string_id::RPCN_ERROR_INVALID_PROTOCOL_VERSION: return tr("RPCN Misc Error: Protocol Version Error (outdated RPCS3?)");
		case localized_string_id::RPCN_ERROR_UNKNOWN: return tr("RPCN: Unknown Error");
		case localized_string_id::RPCN_SUCCESS_LOGGED_ON: return tr("Successfully logged on RPCN!");
		case localized_string_id::RPCN_FRIEND_REQUEST_RECEIVED: return tr("RPCN: Received friend request: %0", "RCPN Friends").arg(std::forward<Args>(args)...);
		case localized_string_id::RPCN_FRIEND_ADDED: return tr("RPCN: Friend added: %0", "RCPN Friends").arg(std::forward<Args>(args)...);
		case localized_string_id::RPCN_FRIEND_LOST: return tr("RPCN: Friend removed: %0", "RCPN Friends").arg(std::forward<Args>(args)...);
		case localized_string_id::RPCN_FRIEND_LOGGED_IN: return tr("RPCN: %0 logged in", "RCPN Friends").arg(std::forward<Args>(args)...);
		case localized_string_id::RPCN_FRIEND_LOGGED_OUT: return tr("RPCN: %0 logged out", "RCPN Friends").arg(std::forward<Args>(args)...);
		case localized_string_id::HOME_MENU_TITLE: return tr("Home Menu");
		case localized_string_id::HOME_MENU_EXIT_GAME: return tr("Exit Game");
		case localized_string_id::HOME_MENU_RESUME: return tr("Resume Game");
		case localized_string_id::HOME_MENU_FRIENDS: return tr("Friends");
		case localized_string_id::HOME_MENU_FRIENDS_REQUESTS: return tr("Pending Friend Requests");
		case localized_string_id::HOME_MENU_FRIENDS_BLOCKED: return tr("Blocked Users");
		case localized_string_id::HOME_MENU_FRIENDS_STATUS_ONLINE: return tr("Online");
		case localized_string_id::HOME_MENU_FRIENDS_STATUS_OFFLINE: return tr("Offline");
		case localized_string_id::HOME_MENU_FRIENDS_STATUS_BLOCKED: return tr("Blocked");
		case localized_string_id::HOME_MENU_FRIENDS_REQUEST_SENT: return tr("You sent a friend request");
		case localized_string_id::HOME_MENU_FRIENDS_REQUEST_RECEIVED: return tr("Sent you a friend request");
		case localized_string_id::HOME_MENU_FRIENDS_BLOCK_USER_MSG: return tr("Block this user?\n\n%0").arg(std::forward<Args>(args)...);
		case localized_string_id::HOME_MENU_FRIENDS_UNBLOCK_USER_MSG: return tr("Unblock this user?\n\n%0").arg(std::forward<Args>(args)...);
		case localized_string_id::HOME_MENU_FRIENDS_REMOVE_USER_MSG: return tr("Remove this user?\n\n%0").arg(std::forward<Args>(args)...);
		case localized_string_id::HOME_MENU_FRIENDS_ACCEPT_REQUEST_MSG: return tr("Accept Request?\n\n%0").arg(std::forward<Args>(args)...);
		case localized_string_id::HOME_MENU_FRIENDS_CANCEL_REQUEST_MSG: return tr("Cancel Request?\n\n%0").arg(std::forward<Args>(args)...);
		case localized_string_id::HOME_MENU_FRIENDS_REJECT_REQUEST_MSG: return tr("Reject Request?\n\n%0").arg(std::forward<Args>(args)...);
		case localized_string_id::HOME_MENU_FRIENDS_REJECT_REQUEST: return tr("Reject Request");
		case localized_string_id::HOME_MENU_FRIENDS_NEXT_LIST: return tr("Next list");
		case localized_string_id::HOME_MENU_RESTART: return tr("Restart Game");
		case localized_string_id::HOME_MENU_SETTINGS: return tr("Settings");
		case localized_string_id::HOME_MENU_SETTINGS_SAVE: return tr("Save custom configuration?");
		case localized_string_id::HOME_MENU_SETTINGS_SAVE_BUTTON: return tr("Save");
		case localized_string_id::HOME_MENU_SETTINGS_DISCARD: return tr("Discard the current settings' changes?");
		case localized_string_id::HOME_MENU_SETTINGS_DISCARD_BUTTON: return tr("Discard");
		case localized_string_id::HOME_MENU_SETTINGS_AUDIO: return tr("Audio");
		case localized_string_id::HOME_MENU_SETTINGS_AUDIO_MASTER_VOLUME: return tr("Master Volume", "Audio");
		case localized_string_id::HOME_MENU_SETTINGS_AUDIO_BACKEND: return tr("Audio Backend", "Audio");
		case localized_string_id::HOME_MENU_SETTINGS_AUDIO_BUFFERING: return tr("Enable Buffering", "Audio");
		case localized_string_id::HOME_MENU_SETTINGS_AUDIO_BUFFER_DURATION: return tr("Desired Audio Buffer Duration", "Audio");
		case localized_string_id::HOME_MENU_SETTINGS_AUDIO_TIME_STRETCHING: return tr("Enable Time Stretching", "Audio");
		case localized_string_id::HOME_MENU_SETTINGS_AUDIO_TIME_STRETCHING_THRESHOLD: return tr("Time Stretching Threshold", "Audio");
		case localized_string_id::HOME_MENU_SETTINGS_VIDEO: return tr("Video");
		case localized_string_id::HOME_MENU_SETTINGS_VIDEO_FRAME_LIMIT: return tr("Frame Limit", "Video");
		case localized_string_id::HOME_MENU_SETTINGS_VIDEO_ANISOTROPIC_OVERRIDE: return tr("Anisotropic Filter Override", "Video");
		case localized_string_id::HOME_MENU_SETTINGS_VIDEO_OUTPUT_SCALING: return tr("Output Scaling", "Video");
		case localized_string_id::HOME_MENU_SETTINGS_VIDEO_RCAS_SHARPENING: return tr("FidelityFX CAS Sharpening Intensity", "Video");
		case localized_string_id::HOME_MENU_SETTINGS_VIDEO_STRETCH_TO_DISPLAY: return tr("Stretch To Display Area", "Video");
		case localized_string_id::HOME_MENU_SETTINGS_INPUT: return tr("Input");
		case localized_string_id::HOME_MENU_SETTINGS_INPUT_BACKGROUND_INPUT: return tr("Background Input Enabled", "Input");
		case localized_string_id::HOME_MENU_SETTINGS_INPUT_KEEP_PADS_CONNECTED: return tr("Keep Pads Connected", "Input");
		case localized_string_id::HOME_MENU_SETTINGS_INPUT_SHOW_PS_MOVE_CURSOR: return tr("Show PS Move Cursor", "Input");
		case localized_string_id::HOME_MENU_SETTINGS_INPUT_CAMERA_FLIP: return tr("Camera Flip", "Input");
		case localized_string_id::HOME_MENU_SETTINGS_INPUT_PAD_MODE: return tr("Pad Handler Mode", "Input");
		case localized_string_id::HOME_MENU_SETTINGS_INPUT_PAD_SLEEP: return tr("Pad Handler Sleep", "Input");
		case localized_string_id::HOME_MENU_SETTINGS_INPUT_FAKE_MOVE_ROTATION_CONE_H: return tr("Fake PS Move Rotation Cone (Horizontal)", "Input");
		case localized_string_id::HOME_MENU_SETTINGS_INPUT_FAKE_MOVE_ROTATION_CONE_V: return tr("Fake PS Move Rotation Cone (Vertical)", "Input");
		case localized_string_id::HOME_MENU_SETTINGS_ADVANCED: return tr("Advanced");
		case localized_string_id::HOME_MENU_SETTINGS_ADVANCED_PREFERRED_SPU_THREADS: return tr("Preferred SPU Threads", "Advanced");
		case localized_string_id::HOME_MENU_SETTINGS_ADVANCED_MAX_CPU_PREEMPTIONS: return tr("Max Power Saving CPU-Preemptions", "Advanced");
		case localized_string_id::HOME_MENU_SETTINGS_ADVANCED_ACCURATE_RSX_RESERVATION_ACCESS: return tr("Accurate RSX reservation access", "Advanced");
		case localized_string_id::HOME_MENU_SETTINGS_ADVANCED_SLEEP_TIMERS_ACCURACY: return tr("Sleep Timers Accuracy", "Advanced");
		case localized_string_id::HOME_MENU_SETTINGS_ADVANCED_MAX_SPURS_THREADS: return tr("Max SPURS Threads", "Advanced");
		case localized_string_id::HOME_MENU_SETTINGS_ADVANCED_DRIVER_WAKE_UP_DELAY: return tr("Driver Wake-Up Delay", "Advanced");
		case localized_string_id::HOME_MENU_SETTINGS_ADVANCED_VBLANK_FREQUENCY: return tr("VBlank Frequency", "Advanced");
		case localized_string_id::HOME_MENU_SETTINGS_ADVANCED_VBLANK_NTSC: return tr("VBlank NTSC Fixup", "Advanced");
		case localized_string_id::HOME_MENU_SETTINGS_OVERLAYS: return tr("Overlays");
		case localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_TROPHY_POPUPS: return tr("Show Trophy Popups", "Overlays");
		case localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_RPCN_POPUPS: return tr("Show RPCN Popups", "Overlays");
		case localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_SHADER_COMPILATION_HINT: return tr("Show Shader Compilation Hint", "Overlays");
		case localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_PPU_COMPILATION_HINT: return tr("Show PPU Compilation Hint", "Overlays");
		case localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_AUTO_SAVE_LOAD_HINT: return tr("Show Autosave/Autoload Hint", "Overlays");
		case localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_PRESSURE_INTENSITY_TOGGLE_HINT: return tr("Show Pressure Intensity Toggle Hint", "Overlays");
		case localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_ANALOG_LIMITER_TOGGLE_HINT: return tr( "Show Analog Limiter Toggle Hint", "Overlays");
		case localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_MOUSE_AND_KB_TOGGLE_HINT: return tr("Show Mouse And Keyboard Toggle Hint", "Overlays");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY: return tr("Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_ENABLE: return tr("Enable Performance Overlay", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_ENABLE_FRAMERATE_GRAPH: return tr("Enable Framerate Graph", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_ENABLE_FRAMETIME_GRAPH: return tr("Enable Frametime Graph", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_DETAIL_LEVEL: return tr("Detail level", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FRAMERATE_DETAIL_LEVEL: return tr("Framerate Graph Detail Level", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FRAMETIME_DETAIL_LEVEL: return tr("Frametime Graph Detail Level", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FRAMERATE_DATAPOINT_COUNT: return tr("Framerate Datapoints", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FRAMETIME_DATAPOINT_COUNT: return tr("Frametime Datapoints", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_UPDATE_INTERVAL: return tr("Metrics Update Interval", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_POSITION: return tr("Position", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_CENTER_X: return tr("Center Horizontally", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_CENTER_Y: return tr("Center Vertically", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_MARGIN_X: return tr("Horizontal Margin", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_MARGIN_Y: return tr("Vertical Margin", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FONT_SIZE: return tr("Font Size", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_OPACITY: return tr("Opacity", "Performance Overlay");
		case localized_string_id::HOME_MENU_SETTINGS_DEBUG: return tr("Debug");
		case localized_string_id::HOME_MENU_SETTINGS_DEBUG_OVERLAY: return tr("Debug Overlay", "Debug");
		case localized_string_id::HOME_MENU_SETTINGS_DEBUG_INPUT_OVERLAY: return tr("Input Debug Overlay", "Debug");
		case localized_string_id::HOME_MENU_SETTINGS_DEBUG_DISABLE_VIDEO_OUTPUT: return tr("Disable Video Output", "Debug");
		case localized_string_id::HOME_MENU_SETTINGS_DEBUG_TEXTURE_LOD_BIAS: return tr("Texture LOD Bias Addend", "Debug");
		case localized_string_id::HOME_MENU_SCREENSHOT: return tr("Take Screenshot");
		case localized_string_id::HOME_MENU_SAVESTATE: return tr("SaveState");
		case localized_string_id::HOME_MENU_SAVESTATE_SAVE: return tr("Save Emulation State");
		case localized_string_id::HOME_MENU_SAVESTATE_AND_EXIT: return tr("Save Emulation State And Exit");
		case localized_string_id::HOME_MENU_RELOAD_SAVESTATE: return tr("Reload Last Emulation State");
		case localized_string_id::HOME_MENU_RECORDING: return tr("Start/Stop Recording");
		case localized_string_id::HOME_MENU_TROPHIES: return tr("Trophies");
		case localized_string_id::HOME_MENU_TROPHY_LIST_TITLE: return tr("Trophy Progress: %0").arg(std::forward<Args>(args)...);
		case localized_string_id::HOME_MENU_TROPHY_LOCKED_TITLE: return tr("Locked trophy: %0").arg(std::forward<Args>(args)...);
		case localized_string_id::HOME_MENU_TROPHY_HIDDEN_TITLE: return tr("Hidden trophy");
		case localized_string_id::HOME_MENU_TROPHY_HIDDEN_DESCRIPTION: return tr("This trophy is hidden");
		case localized_string_id::HOME_MENU_TROPHY_SHOW_HIDDEN_TROPHIES: return tr("Show hidden trophies");
		case localized_string_id::HOME_MENU_TROPHY_HIDE_HIDDEN_TROPHIES: return tr("Hide hidden trophies");
		case localized_string_id::HOME_MENU_TROPHY_PLATINUM_RELEVANT: return tr("Platinum relevant");
		case localized_string_id::HOME_MENU_TROPHY_GRADE_BRONZE: return tr("Bronze", "Trophy type");
		case localized_string_id::HOME_MENU_TROPHY_GRADE_SILVER: return tr("Silver", "Trophy type");
		case localized_string_id::HOME_MENU_TROPHY_GRADE_GOLD: return tr("Gold", "Trophy type");
		case localized_string_id::HOME_MENU_TROPHY_GRADE_PLATINUM: return tr("Platinum", "Trophy type");
		case localized_string_id::AUDIO_MUTED: return tr("Audio muted", "Audio");
		case localized_string_id::AUDIO_UNMUTED: return tr("Audio unmuted", "Audio");
		case localized_string_id::AUDIO_CHANGED: return tr("Volume changed to %0", "Audio").arg(std::forward<Args>(args)...);
		case localized_string_id::PROGRESS_DIALOG_PROGRESS: return tr("Progress:");
		case localized_string_id::PROGRESS_DIALOG_PROGRESS_ANALYZING: return tr("Progress: analyzing...");
		case localized_string_id::PROGRESS_DIALOG_REMAINING: return tr("remaining");
		case localized_string_id::PROGRESS_DIALOG_DONE: return tr("done");
		case localized_string_id::PROGRESS_DIALOG_FILE: return tr("file");
		case localized_string_id::PROGRESS_DIALOG_MODULE: return tr("module");
		case localized_string_id::PROGRESS_DIALOG_OF: return tr("of");
		case localized_string_id::PROGRESS_DIALOG_PLEASE_WAIT: return tr("Please wait");
		case localized_string_id::PROGRESS_DIALOG_STOPPING_PLEASE_WAIT: return tr("Stopping. Please wait...");
		case localized_string_id::PROGRESS_DIALOG_SAVESTATE_PLEASE_WAIT: return tr("Creating savestate. Please wait...");
		case localized_string_id::PROGRESS_DIALOG_SCANNING_PPU_EXECUTABLE: return tr("Scanning PPU Executable...");
		case localized_string_id::PROGRESS_DIALOG_ANALYZING_PPU_EXECUTABLE: return tr("Analyzing PPU Executable...");
		case localized_string_id::PROGRESS_DIALOG_SCANNING_PPU_MODULES: return tr("Scanning PPU Modules...");
		case localized_string_id::PROGRESS_DIALOG_LOADING_PPU_MODULES: return tr("Loading PPU Modules...");
		case localized_string_id::PROGRESS_DIALOG_COMPILING_PPU_MODULES: return tr("Compiling PPU Modules...");
		case localized_string_id::PROGRESS_DIALOG_LINKING_PPU_MODULES: return tr("Linking PPU Modules...");
		case localized_string_id::PROGRESS_DIALOG_APPLYING_PPU_CODE: return tr("Applying PPU Code...");
		case localized_string_id::PROGRESS_DIALOG_BUILDING_SPU_CACHE: return tr("Building SPU Cache...");
		case localized_string_id::EMULATION_PAUSED_RESUME_WITH_START: return tr("Press and hold the START button to resume");
		case localized_string_id::EMULATION_RESUMING: return tr("Resuming...!");
		case localized_string_id::EMULATION_FROZEN: return tr("The PS3 application has likely crashed, you can close it.");
		case localized_string_id::SAVESTATE_FAILED_DUE_TO_SAVEDATA: return tr("SaveState failed: Game saving is in progress, wait until finished.");
		case localized_string_id::SAVESTATE_FAILED_DUE_TO_VDEC: return tr("SaveState failed: VDEC-base video/cutscenes are in order, wait for them to end or enable libvdec.sprx.");
		case localized_string_id::SAVESTATE_FAILED_DUE_TO_MISSING_SPU_SETTING: return tr("SaveState failed: Failed to lock SPU state, enabling SPU-Compatible mode may fix it.");
		case localized_string_id::SAVESTATE_FAILED_DUE_TO_SPU: return tr("SaveState failed: Failed to lock SPU state, using SPU ASMJIT will fix it.");
		case localized_string_id::INVALID: return tr("Invalid");
		default: return tr("Unknown");
		}
	}
};
