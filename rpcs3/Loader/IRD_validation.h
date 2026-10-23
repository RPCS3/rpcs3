#pragma once

#include "util/types.hpp"

#include <string>
#include <vector>

// Status of a single file of a game, compared against the IRD of the disc the game comes from
enum class disc_file_status
{
	MATCH,         // The game holds the file and its MD5 hash matches the one the IRD holds
	MATCH_REBUILT, // Same as above, but only once the file is rebuilt (see "check_content")
	MISMATCH,      // The game holds the file but its content differs from the one of the disc
	MISSING,       // The IRD lists the file but the game does not hold it
	NOT_REQUIRED   // The game holds the file but the IRD does not list it
};

// One file of a disc content validation report
struct disc_file_entry
{
	std::string path; // Path of the file relative to the root of the disc (e.g. "/PS3_GAME/USRDIR/EBOOT.BIN")
	u64 size = 0;     // Size of the file (the one on the disc, or the one of the game for a file not on the disc)
	disc_file_status status = disc_file_status::MATCH;
};

// Enum returned by validating the content of a game against an IRD file
enum class disc_check_status
{
	PASSED,               // Every file the IRD lists is there and matches, the firmware update aside
	FAILED,               // At least one file of the game is missing or does not match
	ABORTED,              // Validation aborted by the user
	ERROR_NOT_A_PS3_GAME, // No "PS3_GAME/PARAM.SFO" file found
	ERROR_OPENING_ISO,    // The ISO file could not be opened or recognized
	ERROR_KEY_MISSING,    // The ISO file is encrypted and no key to read it back was found
	ERROR_KEY_INVALID,    // A key file was found for the ISO file, but it belongs to another disc
	ERROR_PARSING_IRD     // The IRD file could not be read
};

// Report filled in by validating the content of a game against an IRD file
struct disc_check_report
{
	disc_check_status status = disc_check_status::FAILED;

	bool is_iso = false;          // Whether the game is held by an ISO image instead of a JB folder
	bool ird_crc_valid = false;   // Whether the CRC32 stored in the IRD matches its content
	bool serial_mismatch = false; // Whether the IRD belongs to a different game

	// Whether the image turned out to be encrypted with no key file for it, and was read back through the key
	// the IRD file stores
	bool decrypted_with_ird_key = false;

	std::string game_id;           // TITLE_ID read from the PARAM.SFO of the game
	std::string game_title;        // TITLE read from the PARAM.SFO of the game
	std::string ird_game_id;       // Game the IRD belongs to
	std::string ird_game_name;     // Name of the disc the IRD was made from
	std::string ird_game_version;  // VERSION of the disc
	std::string ird_app_version;   // APP_VER of the disc
	std::string ird_update_version;// PS3_SYSTEM_VER of the disc

	u32 matched = 0;        // Files matching the disc (rebuilt ones included)
	u32 rebuilt = 0;        // Files matching the disc only once rebuilt
	u32 mismatched = 0;     // Files whose content differs from the one of the disc
	u32 missing = 0;        // Files of the disc the game does not hold
	u32 missing_update = 0; // Of those, the ones belonging to the firmware update of the disc
	u32 not_required = 0;   // Files of the game the disc does not hold

	std::vector<disc_file_entry> entries; // Only the files needing the attention of the user
};
