#pragma once

#include "util/types.hpp"
#include "Utilities/File.h"

#include <string>
#include <vector>

// Enum identifying the content file type
enum class content_file_type
{
	ISO,
	PSN_CONTENT,
	PSN_DLC,
	PSN_UPDATE
};

// Enum returned by calculating hash
enum class content_hash_status
{
	INITIALIZED,
	COMPLETED,
	ABORTED
};

// Enum returned by checking integrity
enum class content_integrity_status
{
	NO_MATCH,
	FOUND_MATCH,
	ERROR_OPENING_DB,
	ERROR_PARSING_DB
};

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
	PASSED,               // Every file the IRD lists is there and matches
	FAILED,               // At least one file is missing or does not match
	ABORTED,              // Validation aborted by the user
	ERROR_NOT_A_PS3_GAME, // No "PS3_GAME/PARAM.SFO" file found
	ERROR_OPENING_ISO,    // The ISO file could not be opened or recognized
	ERROR_ISO_ENCRYPTED,  // The ISO file is encrypted and no key to read it back was found
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

	u32 matched = 0;      // Files matching the disc (rebuilt ones included)
	u32 rebuilt = 0;      // Files matching the disc only once rebuilt
	u32 mismatched = 0;   // Files whose content differs from the one of the disc
	u32 missing = 0;      // Files of the disc the game does not hold
	u32 not_required = 0; // Files of the game the disc does not hold

	std::vector<disc_file_entry> entries; // Only the files needing the attention of the user
};

// Content validation class
class content_validation
{
private:
	std::string m_path;
	std::string m_name;
	u64 m_size = 0;
	u64 m_bytes_read = 0;
	u16 m_count = 0; // Set only by set_count()
	content_hash_status m_status = content_hash_status::INITIALIZED;

	// Read buffer of hash_content_file(), kept between files rather than built anew for each of them
	std::vector<u8> m_hash_buffer;

	// MD5 hash of a single file of a game: whenever the file is shorter than the size it has on the disc
	// ("disc_size"), the missing tail is hashed as zeros, the way the padding of "PS3UPDAT.PUP" is handled by the
	// dumping tools ("disc_size" of 0 hashes the file as it is). Progress information is updated as it goes
	bool hash_content_file(const fs::file& file, u64 disc_size, std::string& hash);

public:
	static content_integrity_status check_integrity(content_file_type file_type, std::string_view hash, std::string* game_name = nullptr);

	const std::string& get_path() const { return m_path; }
	const std::string& get_name() const	{ return m_name; }
	u64 get_size() const { return m_size; }
	u64 get_bytes_read() const { return m_bytes_read; }
	u16 get_count() const { return m_count; }
	content_hash_status get_status() const { return m_status; }

	void set_count(u16 count) { m_count = count; }
	void abort_hash() { m_status = content_hash_status::ABORTED; }

	bool init_hash(const std::string& path);
	content_hash_status calculate_hash(std::string& hash);

	/*
	  Validates a game against the IRD file of the disc it comes from, comparing the MD5 hash of each of its files.

	  - The game can be held either by a JB folder (the one holding the "PS3_GAME" directory, or that very
	    directory) or by an ISO image: the two are told apart on their own.

	  - An encrypted image (3k3y, Redump) is decrypted while reading it. Should no key file be found for it, the
	    key stored in the IRD is used, so that a ".dkey" is never actually needed; an image that stays unreadable
	    is refused right away instead of being read through only to report every one of its files as invalid.

	  - Only a folder, which a ripping tool regularly strips, is given the benefit of the doubt: a file it does
	    not hold is rebuilt in memory (never written to the drive) whenever the game alone determines its content,
	    i.e. the license file, an empty file, and the zero padded tail of a file shorter than the one of the disc.
	    An image is meant to hold the disc as it is, so nothing of it is ever rebuilt.
	*/
	disc_check_status check_content(const std::string& game_path, const std::string& ird_path, disc_check_report& report);
};
