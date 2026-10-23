#pragma once

#include "util/types.hpp"

#include <array>
#include <map>
#include <string>
#include <vector>

// One file of the disc file system described by an IRD file
struct ird_file_entry
{
	std::string path; // Path of the file inside the disc, '/' delimited (e.g. "/PS3_GAME/USRDIR/EBOOT.BIN")
	u64 size = 0;     // Size of the file in bytes (sum of every extent it spans)
	u64 sector = 0;   // First data sector of the file (it is the key of the MD5 table stored in the IRD)
	std::string md5;  // MD5 hash (lowercase hex) of the file as it is stored on the original disc
};

// Enum returned by parsing an IRD file
enum class ird_parse_status
{
	OK,
	ERROR_OPENING_FILE,
	ERROR_NOT_AN_IRD,     // Wrong magic (the file is neither a raw nor a gzipped IRD)
	ERROR_BAD_VERSION,    // IRD version out of the supported range
	ERROR_TRUNCATED,      // The file ends in the middle of a field
	ERROR_PARSING_HEADER  // The ISO9660 header stored in the IRD could not be parsed
};

/*
- An IRD (ISO ReBuild Data) file stores the file system header of a PS3 disc (the very same ECMA-119 volume
  descriptors and directory records the disc begins with) together with the MD5 hash of each of its files.
  That is what makes it possible to validate a game, held either by a "JB folder" or by an ISO image, file by
  file against the disc it comes from, without owning that disc.

- It also carries the "D1" of the disc, i.e. what a Redump ".dkey" file holds already derived, so an encrypted
  image checked against an IRD can be read back even when no key file exists for it.

- The whole file is usually gzipped: only when the magic is found as-is the file is read uncompressed.

- Supported IRD versions: 6 to 9 (version 9 is the one every current dumping tool writes)
*/
class ird_file
{
private:
	ird_parse_status m_status = ird_parse_status::ERROR_OPENING_FILE;
	bool m_crc_valid = false;
	u8 m_version = 0;
	std::string m_path;
	std::string m_game_id;
	std::string m_game_name;
	std::string m_update_version;
	std::string m_game_version;
	std::string m_app_version;
	u64 m_disc_size = 0;
	std::array<u8, 16> m_disc_key{}; // "D1", the field of the disc the decryption key is derived from
	std::array<u8, 16> m_disc_id{};  // "D2"
	std::vector<ird_file_entry> m_files;

	ird_parse_status parse(const std::vector<u8>& data);
	bool parse_iso_header(const std::vector<u8>& header, const std::map<u64, std::string>& hashes);

public:
	ird_file(const std::string& path);

	ird_parse_status get_status() const { return m_status; }
	bool is_valid() const { return m_status == ird_parse_status::OK; }

	// Whether the CRC32 stored at the end of the IRD matches its content
	bool is_crc_valid() const { return m_crc_valid; }

	u8 get_version() const { return m_version; }
	const std::string& get_path() const { return m_path; }
	const std::string& get_game_id() const { return m_game_id; }
	const std::string& get_game_name() const { return m_game_name; }
	const std::string& get_update_version() const { return m_update_version; }
	const std::string& get_game_version() const { return m_game_version; }
	const std::string& get_app_version() const { return m_app_version; }
	u64 get_disc_size() const { return m_disc_size; }

	// The "D1" of the disc: the very field a 3k3y image carries in its watermark and a ".dkey" file holds
	// already derived, so an encrypted image whose key file is missing can still be read back through it
	const std::array<u8, 16>& get_disc_key() const { return m_disc_key; }
	const std::array<u8, 16>& get_disc_id() const { return m_disc_id; }

	// Every file of the disc, ordered by its first data sector (i.e. in the order it lies on the disc)
	const std::vector<ird_file_entry>& get_files() const { return m_files; }
};
