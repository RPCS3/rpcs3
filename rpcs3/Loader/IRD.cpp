#include "stdafx.h"

#include "IRD.h"
#include "ISO.h"

#include "Crypto/utils.h"
#include "Utilities/File.h"

#include <zlib.h>

LOG_CHANNEL(ird_log, "IRD");

// Magic every IRD file begins with, once uncompressed
static constexpr std::array<u8, 4> s_ird_magic = {'3', 'I', 'R', 'D'};

static constexpr u8 s_ird_min_version = 6;
static constexpr u8 s_ird_max_version = 9;

namespace
{
	// Little endian cursor over the uncompressed IRD data: every read is bounds checked, so a truncated file
	// stops the parsing instead of running past the end of the buffer
	struct ird_reader
	{
		const std::vector<u8>& data;
		usz pos = 0;
		bool error = false;

		bool has_room(usz size)
		{
			if (error || pos + size > data.size())
			{
				error = true;
				return false;
			}

			return true;
		}

		template <typename T>
		T read()
		{
			le_t<T> value{};

			if (!has_room(sizeof(T)))
			{
				return value;
			}

			std::memcpy(&value, data.data() + pos, sizeof(T));
			pos += sizeof(T);

			return value;
		}

		// The bytes right where they lie in the buffer, for whatever is read once and not kept
		const u8* read_ptr(usz size)
		{
			if (!has_room(size))
			{
				return nullptr;
			}

			const u8* value = data.data() + pos;
			pos += size;

			return value;
		}

		std::vector<u8> read_bytes(usz size)
		{
			if (!has_room(size))
			{
				return {};
			}

			std::vector<u8> value(data.begin() + pos, data.begin() + pos + size);
			pos += size;

			return value;
		}

		// Fixed size string, stripped of the padding the dumping tools leave in it
		std::string read_string(usz size)
		{
			if (!has_room(size))
			{
				return {};
			}

			std::string value(reinterpret_cast<const char*>(data.data() + pos), size);
			pos += size;

			while (!value.empty() && (value.back() == '\0' || value.back() == ' '))
			{
				value.pop_back();
			}

			return value;
		}

		// Length prefixed string, as it is written by ".NET BinaryWriter": the length comes as a 7 bit encoded
		// integer (7 bits of payload per byte, the 8th bit telling whether another byte follows)
		std::string read_prefixed_string()
		{
			usz size = 0;

			for (u32 shift = 0; shift < 32; shift += 7)
			{
				const u8 byte = read<u8>();

				if (error)
				{
					return {};
				}

				size |= static_cast<usz>(byte & 0x7f) << shift;

				if (!(byte & 0x80))
				{
					break;
				}
			}

			if (!has_room(size))
			{
				return {};
			}

			std::string value(reinterpret_cast<const char*>(data.data() + pos), size);
			pos += size;

			return value;
		}

		void skip(usz size)
		{
			if (has_room(size))
			{
				pos += size;
			}
		}
	};
} // namespace

// Decompresses one gzip member, and stops right at the end of it.
// NOTE: the shared "unzip()" helper is deliberately not used here. It keeps inflating while there is input left,
//       and an IRD file can carry padding past the end of its gzip stream (some writers leave a block of 0x10
//       bytes behind): on one of those, inflate reports the end of the stream over and over without ever
//       consuming that tail, and the helper grows its output buffer forever
static std::vector<u8> ird_ungzip(const std::vector<u8>& data)
{
	// A gzip member is never shorter than its header plus its trailer
	if (data.size() < 18)
	{
		return {};
	}

	std::vector<u8> out(std::max<usz>(data.size() * 6, 0x10000));

	z_stream zs{};

	if (::inflateInit2(&zs, 16 + 15) != Z_OK)
	{
		return {};
	}

	zs.avail_in = ::narrow<uInt>(data.size());
	zs.next_in = const_cast<u8*>(data.data());
	zs.avail_out = ::narrow<uInt>(out.size());
	zs.next_out = out.data();

	for (int res = Z_OK; res != Z_STREAM_END;)
	{
		res = ::inflate(&zs, Z_NO_FLUSH);

		if (res == Z_STREAM_END)
		{
			break;
		}

		// Either the input ran out before the end of the stream (i.e. it is truncated) or inflate itself gave up
		if ((res != Z_OK && res != Z_BUF_ERROR) || zs.avail_in == 0 || zs.avail_out != 0)
		{
			::inflateEnd(&zs);
			return {};
		}

		// Only the output ran out: make room and go on
		const usz written = zs.next_out - out.data();

		out.resize(out.size() + 0x100000);

		zs.next_out = out.data() + written;
		zs.avail_out = ::narrow<uInt>(out.size() - written);
	}

	out.resize(zs.next_out - out.data());

	::inflateEnd(&zs);

	return out;
}

// Flattens the file system tree the ISO header describes, pairing every file with the MD5 hash the IRD holds
// for the sector the file starts at
static void ird_flatten_tree(const iso_fs_node& node, const std::string& parent_path, const std::map<u64, std::string>& hashes, std::vector<ird_file_entry>& files)
{
	for (const auto& child : node.children)
	{
		const iso_fs_metadata& meta = child->metadata;

		if (meta.name == "." || meta.name == "..")
		{
			continue;
		}

		const std::string path = parent_path + "/" + meta.name;

		if (meta.is_directory)
		{
			ird_flatten_tree(*child, path, hashes, files);
			continue;
		}

		if (meta.extents.empty())
		{
			continue;
		}

		const u64 sector = ::at32(meta.extents, 0).start;
		const auto hash = hashes.find(sector);

		files.push_back(ird_file_entry
		{
			.path = path,
			.size = meta.size(),
			.sector = sector,
			.md5 = hash == hashes.cend() ? std::string{} : hash->second
		});
	}
}

ird_file::ird_file(const std::string& path)
{
	m_path = path;

	const fs::file file(path);

	if (!file)
	{
		ird_log.error("ird_file: Failed to open file: '%s'", path);
		m_status = ird_parse_status::ERROR_OPENING_FILE;
		return;
	}

	const std::vector<u8> raw = file.to_vector<u8>();

	if (raw.size() < s_ird_magic.size())
	{
		ird_log.error("ird_file: File too small to be an IRD: '%s'", path);
		m_status = ird_parse_status::ERROR_NOT_AN_IRD;
		return;
	}

	// An IRD file is normally gzipped: only a file already exposing the magic is read as-is
	if (std::memcmp(raw.data(), s_ird_magic.data(), s_ird_magic.size()) == 0)
	{
		m_status = parse(raw);
	}
	else
	{
		const std::vector<u8> data = ird_ungzip(raw);

		if (data.size() < s_ird_magic.size())
		{
			ird_log.error("ird_file: Failed to decompress file: '%s'", path);
			m_status = ird_parse_status::ERROR_NOT_AN_IRD;
			return;
		}

		m_status = parse(data);
	}

	if (m_status == ird_parse_status::OK)
	{
		ird_log.notice("ird_file: Loaded IRD v%d for '%s' (%s): %d files, CRC32 %s", m_version, m_game_id, m_game_name,
			m_files.size(), m_crc_valid ? "valid" : "invalid");
	}
}

ird_parse_status ird_file::parse(const std::vector<u8>& data)
{
	ird_reader reader{data};

	if (const std::vector<u8> magic = reader.read_bytes(s_ird_magic.size());
		magic.size() != s_ird_magic.size() || std::memcmp(magic.data(), s_ird_magic.data(), s_ird_magic.size()) != 0)
	{
		ird_log.error("ird_file: Wrong magic on file: '%s'", m_path);
		return ird_parse_status::ERROR_NOT_AN_IRD;
	}

	m_version = reader.read<u8>();

	if (m_version < s_ird_min_version || m_version > s_ird_max_version)
	{
		ird_log.error("ird_file: Unsupported IRD version %d on file: '%s'", m_version, m_path);
		return ird_parse_status::ERROR_BAD_VERSION;
	}

	m_game_id = reader.read_string(9);
	m_game_name = reader.read_prefixed_string();
	m_update_version = reader.read_string(4);
	m_game_version = reader.read_string(5);
	m_app_version = reader.read_string(5);

	if (m_version == 7)
	{
		// Version 7 only: an extra id nothing else refers to
		reader.skip(4);
	}

	// The ISO header (the ECMA-119 structures the disc begins with) and the ISO footer, both gzipped
	const u32 header_size = reader.read<u32>();
	const std::vector<u8> header_data = reader.read_bytes(header_size);
	const std::vector<u8> header = ird_ungzip(header_data);
	const u32 footer_size = reader.read<u32>();
	reader.skip(footer_size); // The footer holds no data the validation needs

	// The gzip trailer of a block tells how big its content is: a header decompressed short would silently turn
	// the files it does not reach any more into missing ones, so it is caught right here
	if (header_size >= 4 && header.size() != read_from_ptr<le_t<u32>>(header_data, header_size - 4))
	{
		ird_log.error("ird_file: Failed to decompress the ISO header of file: '%s'", m_path);
		return ird_parse_status::ERROR_TRUNCATED;
	}

	// MD5 hash of each encrypted region of the disc: not needed to validate a JB folder, whose files are always
	// decrypted, but it has to be walked over to reach the file hashes
	reader.skip(reader.read<u8>() * 16ull);

	const u32 file_count = reader.read<u32>();

	if (reader.error)
	{
		ird_log.error("ird_file: Truncated file: '%s'", m_path);
		return ird_parse_status::ERROR_TRUNCATED;
	}

	// MD5 hash of each file of the disc, keyed by the sector the file starts at
	std::map<u64, std::string> hashes;

	for (u32 i = 0; i < file_count; i++)
	{
		const u64 sector = reader.read<u64>();
		const u8* hash = reader.read_ptr(16);

		if (!hash)
		{
			ird_log.error("ird_file: Truncated file hash table: '%s'", m_path);
			return ird_parse_status::ERROR_TRUNCATED;
		}

		bytes_to_hex(hashes[sector], hash, 16);
	}

	reader.skip(4); // Extra config and attachments (both unused)

	if (m_version >= 9)
	{
		reader.skip(115); // PIC
	}

	// "Data1" is the D1 of the disc (i.e. the key material), "Data2" its D2
	if (const std::vector<u8> data = reader.read_bytes(32); data.size() == 32)
	{
		std::memcpy(m_disc_key.data(), data.data(), m_disc_key.size());
		std::memcpy(m_disc_id.data(), data.data() + m_disc_key.size(), m_disc_id.size());
	}

	if (m_version < 9)
	{
		reader.skip(115); // PIC (up to version 8 it comes after the keys)
	}

	if (m_version > 7)
	{
		reader.skip(4); // Unique identifier
	}

	const usz crc_pos = reader.pos;
	const u32 crc = reader.read<u32>();

	if (reader.error)
	{
		ird_log.error("ird_file: Truncated file: '%s'", m_path);
		return ird_parse_status::ERROR_TRUNCATED;
	}

	m_crc_valid = crc == ::crc32(::crc32(0, nullptr, 0), data.data(), ::narrow<uInt>(crc_pos));

	if (!m_crc_valid)
	{
		// A wrong CRC32 does not make the hashes unusable, so the parsing goes on: it is just reported
		ird_log.warning("ird_file: Wrong CRC32 on file: '%s'", m_path);
	}

	if (!parse_iso_header(header, hashes))
	{
		return ird_parse_status::ERROR_PARSING_HEADER;
	}

	return ird_parse_status::OK;
}

bool ird_file::parse_iso_header(const std::vector<u8>& header, const std::map<u64, std::string>& hashes)
{
	if (header.empty())
	{
		ird_log.error("ird_file: Empty ISO header on file: '%s'", m_path);
		return false;
	}

	// The header holds the ECMA-119 structures at the very same sectors they lie at on the disc, so the ISO file
	// system parser can walk it as if it were the disc itself
	fs::file stream = fs::make_stream<std::vector<u8>>(std::vector<u8>(header));
	iso_fs_node root{};

	if (!iso_parse_file_system(stream, root, m_path))
	{
		return false;
	}

	m_files.reserve(hashes.size());

	ird_flatten_tree(root, "", hashes, m_files);

	if (m_files.empty())
	{
		ird_log.error("ird_file: No file found on the ISO header of file: '%s'", m_path);
		return false;
	}

	// Report the files in the order they lie on the disc, the way the dumping tools list them
	std::sort(m_files.begin(), m_files.end(), [](const ird_file_entry& lhs, const ird_file_entry& rhs)
	{
		return lhs.sector < rhs.sector;
	});

	m_disc_size = m_files.back().sector * ISO_SECTOR_SIZE + m_files.back().size;

	return true;
}
