#include "stdafx.h"
#include "ZAR.h"
#include "ISO.h"
#include "util/cctype.hpp"

#include <zarchive/zarchivereader.h>
#include <zarchive/zarchivewriter.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string_view>
#include <vector>

LOG_CHANNEL(zar_log, "ZAR");

namespace
{
	class zar_file final : public fs::file_base
	{
	public:
		zar_file(std::shared_ptr<ZArchiveReader> reader, ZArchiveNodeHandle node, std::string name, s64 source_mtime)
			: m_reader(std::move(reader)), m_node(node), m_name(std::move(name)), m_source_mtime(source_mtime)
		{
			m_size = m_reader ? m_reader->GetFileSize(m_node) : 0;
		}

		fs::stat_t get_stat() override
		{
			return fs::stat_t
			{
				.is_directory = false,
				.is_symlink = false,
				.is_writable = false,
				.size = m_size,
				.atime = m_source_mtime,
				.mtime = m_source_mtime,
				.ctime = m_source_mtime,
			};
		}

		bool trunc(u64) override
		{
			fs::g_tls_error = fs::error::readonly;
			return false;
		}

		u64 read(void* buffer, u64 size) override
		{
			const u64 result = read_at(m_pos, buffer, size);
			m_pos += result;
			return result;
		}

		u64 read_at(u64 offset, void* buffer, u64 size) override
		{
			if (!m_reader || offset >= m_size || !size)
				return 0;

			const u64 wanted = std::min(size, m_size - offset);
			const u64 result = m_reader->ReadFromFile(m_node, offset, wanted, buffer);
			if (result != wanted)
			{
				zar_log.error("Short read from '%s': offset=%llu requested=%llu read=%llu", m_name, offset, wanted, result);
			}
			return result;
		}

		u64 write(const void*, u64) override
		{
			fs::g_tls_error = fs::error::readonly;
			return 0;
		}

		u64 seek(s64 offset, fs::seek_mode whence) override
		{
			const s64 next = whence == fs::seek_set ? offset :
				whence == fs::seek_cur ? static_cast<s64>(m_pos) + offset :
				whence == fs::seek_end ? static_cast<s64>(m_size) + offset : -1;

			if (next < 0)
			{
				fs::g_tls_error = fs::error::inval;
				return umax;
			}

			m_pos = static_cast<u64>(next);
			return m_pos;
		}

		u64 size() override { return m_size; }

	private:
		std::shared_ptr<ZArchiveReader> m_reader;
		ZArchiveNodeHandle m_node = ZARCHIVE_INVALID_NODE;
		std::string m_name;
		u64 m_pos = 0;
		u64 m_size = 0;
		s64 m_source_mtime = 0;
	};


	class zar_dir final : public fs::dir_base
	{
	public:
		explicit zar_dir(std::vector<fs::dir_entry> entries)
			: m_entries(std::move(entries))
		{
		}

		bool read(fs::dir_entry& entry) override
		{
			if (m_pos >= m_entries.size())
				return false;

			entry = m_entries[m_pos++];
			return true;
		}

		void rewind() override
		{
			m_pos = 0;
		}

	private:
		std::vector<fs::dir_entry> m_entries;
		usz m_pos = 0;
	};

	bool iequals_ext(std::string_view value, std::string_view ext)
	{
		if (value.size() < ext.size()) return false;
		value.remove_prefix(value.size() - ext.size());
		return std::equal(value.begin(), value.end(), ext.begin(), ext.end(), [](char a, char b)
		{
			return utils::tolower(a) == utils::tolower(b);
		});
	}

	bool preflight_zar(const std::string& path, std::string* error)
	{
		fs::file file(path, fs::read);
		if (!file || file.size() <= 144)
		{
			if (error) *error = "archive is too small for a ZArchive footer";
			return false;
		}

		std::array<u8, 144> footer{};
		if (file.read_at(file.size() - footer.size(), footer.data(), footer.size()) != footer.size())
		{
			if (error) *error = "failed to read ZArchive footer";
			return false;
		}

		const u64 total_size = read_from_ptr<be_t<u64>>(footer, 128);
		const u32 version = read_from_ptr<be_t<u32>>(footer, 136);
		const u32 magic = read_from_ptr<be_t<u32>>(footer, 140);
		if (magic != 0x169f52d6)
		{
			if (error) *error = "invalid ZArchive footer magic";
			return false;
		}
		if (version != 0x61bf3a01)
		{
			if (error) *error = fmt::format("unsupported ZArchive version 0x%08x", version);
			return false;
		}
		if (total_size != file.size())
		{
			if (error) *error = "ZArchive footer size does not match the physical file size";
			return false;
		}

		const u64 data_end = file.size() - footer.size();
		for (u32 section = 0; section < 6; section++)
		{
			const u64 offset = read_from_ptr<be_t<u64>>(footer, section * 16);
			const u64 section_size = read_from_ptr<be_t<u64>>(footer, section * 16 + 8);
			if (offset > data_end || section_size > data_end - offset)
			{
				if (error) *error = fmt::format("ZArchive section %u is outside the file bounds", section);
				return false;
			}
		}

		// CompressionOffsetRecord is 40 bytes and FileDirectoryEntry is 16 bytes in ZArchive 0.1.x.
		if (read_from_ptr<be_t<u64>>(footer, 16 + 8) % 40 || read_from_ptr<be_t<u64>>(footer, 48 + 8) % 16)
		{
			if (error) *error = "ZArchive index sections have invalid alignment";
			return false;
		}
		return true;
	}

	void set_error(std::string* out, std::string message)
	{
		if (out) *out = std::move(message);
	}
}

zar_disc_container::zar_disc_container(std::string source_path, std::shared_ptr<ZArchiveReader> reader, zar_disc_layout layout,
	u32 iso_node, u32 key_node, std::string iso_name, std::string key_name, s64 source_mtime)
	: m_source_path(std::move(source_path))
	, m_reader(std::move(reader))
	, m_layout(layout)
	, m_iso_node(iso_node)
	, m_key_node(key_node)
	, m_iso_name(std::move(iso_name))
	, m_key_name(std::move(key_name))
	, m_source_mtime(source_mtime)
{
}

bool zar_disc_container::has_zar_extension(const std::string& path)
{
	return iequals_ext(path, ".zar");
}

std::shared_ptr<zar_disc_container> zar_disc_container::open(const std::string& path, std::string* error)
{
	if (!has_zar_extension(path))
	{
		set_error(error, "not a .zar file");
		return {};
	}
	if (!fs::is_file(path))
	{
		set_error(error, "archive does not exist or is not a regular file");
		return {};
	}

	if (!preflight_zar(path, error))
		return {};

	fs::stat_t source_stat{};
	const s64 source_mtime = fs::get_stat(path, source_stat) ? source_stat.mtime : 0;

	std::shared_ptr<ZArchiveReader> reader(ZArchiveReader::OpenFromFile(std::filesystem::path(reinterpret_cast<const char8_t*>(path.c_str()))));
	if (!reader)
	{
		set_error(error, "invalid or unsupported ZArchive structure");
		return {};
	}

	const std::string iso_name = "disc.iso";
	const ZArchiveNodeHandle iso_node = reader->LookUp(iso_name, true, false);
	const bool has_iso = iso_node != ZARCHIVE_INVALID_NODE && reader->IsFile(iso_node);

	const ZArchiveNodeHandle sfb_node = reader->LookUp("PS3_DISC.SFB", true, false);
	const ZArchiveNodeHandle game_node = reader->LookUp("PS3_GAME", false, true);
	const ZArchiveNodeHandle param_node = reader->LookUp("PS3_GAME/PARAM.SFO", true, false);
	const bool has_jb = sfb_node != ZARCHIVE_INVALID_NODE && reader->IsFile(sfb_node) &&
		game_node != ZARCHIVE_INVALID_NODE && reader->IsDirectory(game_node) &&
		param_node != ZARCHIVE_INVALID_NODE && reader->IsFile(param_node);

	if (has_iso == has_jb)
	{
		set_error(error, has_iso ?
			"ambiguous disc archive: contains both disc.iso and a JB layout" :
			"not an RPCS3 PS3 disc archive: expected disc.iso or PS3_DISC.SFB + PS3_GAME/PARAM.SFO");
		return {};
	}

	u32 key_node = ZARCHIVE_INVALID_NODE;
	std::string key_name;
	if (has_iso)
	{
		for (const std::string_view candidate : {std::string_view("disc.dkey"), std::string_view("disc.key")})
		{
			const ZArchiveNodeHandle node = reader->LookUp(candidate, true, false);
			if (node == ZARCHIVE_INVALID_NODE)
				continue;
			if (!reader->IsFile(node))
			{
				set_error(error, std::string(candidate) + " exists but is not a regular file");
				return {};
			}
			if (key_node != ZARCHIVE_INVALID_NODE)
			{
				set_error(error, "archive contains both disc.dkey and disc.key; keep only one key");
				return {};
			}
			const u64 key_size = reader->GetFileSize(node);
			if (key_size != 16 && key_size != 32)
			{
				set_error(error, fmt::format("%s has invalid size %llu (expected 16 raw bytes or 32 hex characters)", candidate, key_size));
				return {};
			}
			key_node = node;
			key_name = candidate;
		}

		const u64 iso_size = reader->GetFileSize(iso_node);
		if (iso_size < 32768 + 6)
		{
			set_error(error, "embedded ISO is truncated");
			return {};
		}

		std::array<char, 5> pvd_magic{};
		if (reader->ReadFromFile(iso_node, 32768 + 1, pvd_magic.size(), pvd_magic.data()) != pvd_magic.size() ||
			std::string_view(pvd_magic.data(), pvd_magic.size()) != "CD001")
		{
			set_error(error, "embedded file is not a valid ISO9660 PS3 disc image (missing CD001 PVD)");
			return {};
		}
	}

	return std::shared_ptr<zar_disc_container>(new zar_disc_container(path, std::move(reader), has_iso ? zar_disc_layout::iso : zar_disc_layout::jb,
		has_iso ? iso_node : ZARCHIVE_INVALID_NODE, key_node, has_iso ? iso_name : std::string{}, key_name, source_mtime));
}

fs::file zar_disc_container::open_iso() const
{
	if (!m_reader || m_iso_node == ZARCHIVE_INVALID_NODE) return {};
	return fs::file(std::make_unique<zar_file>(m_reader, m_iso_node, m_source_path + "::" + m_iso_name, m_source_mtime));
}

fs::file zar_disc_container::open_key() const
{
	if (!m_reader || m_key_node == ZARCHIVE_INVALID_NODE) return {};
	return fs::file(std::make_unique<zar_file>(m_reader, m_key_node, m_source_path + "::" + m_key_name, m_source_mtime));
}

u64 zar_disc_container::iso_size() const
{
	return m_reader && m_iso_node != ZARCHIVE_INVALID_NODE ? m_reader->GetFileSize(m_iso_node) : 0;
}

fs::file zar_disc_container::open_file(u32 node, const std::string& display_name) const
{
	if (!m_reader || node == ZARCHIVE_INVALID_NODE || !m_reader->IsFile(node)) return {};
	return fs::file(std::make_unique<zar_file>(m_reader, node, m_source_path + "::" + display_name, m_source_mtime));
}

fs::file zar_disc_container::open_file(const std::string& path) const
{
	if (!m_reader)
		return {};

	const u32 node = lookup(path, true, false);
	if (node == ZARCHIVE_INVALID_NODE || !m_reader->IsFile(node))
	{
		fs::g_tls_error = fs::error::noent;
		return {};
	}

	return fs::file(std::make_unique<zar_file>(m_reader, node, m_source_path + "::" + path, m_source_mtime));
}

std::unique_ptr<fs::dir_base> zar_disc_container::open_dir(const std::string& path) const
{
	if (!m_reader)
		return nullptr;

	const u32 node = lookup(path, false, true);
	if (node == ZARCHIVE_INVALID_NODE || !m_reader->IsDirectory(node))
	{
		fs::g_tls_error = fs::error::noent;
		return nullptr;
	}

	const u32 count = m_reader->GetDirEntryCount(node);
	std::vector<fs::dir_entry> entries;
	entries.reserve(count);

	for (u32 i = 0; i < count; i++)
	{
		ZArchiveReader::DirEntry in{};
		if (!m_reader->GetDirEntry(node, i, in) || in.name.empty())
		{
			zar_log.error("Invalid directory entry in '%s::%s' at index %u", m_source_path, path, i);
			fs::g_tls_error = fs::error::inval;
			return nullptr;
		}

		fs::dir_entry out{};
		out.name = std::string(in.name);
		out.is_directory = in.isDirectory;
		out.is_symlink = false;
		out.is_writable = false;
		out.size = in.isFile ? in.size : ISO_SECTOR_SIZE;
		out.atime = m_source_mtime;
		out.mtime = m_source_mtime;
		out.ctime = m_source_mtime;
		entries.emplace_back(std::move(out));
	}

	return std::make_unique<zar_dir>(std::move(entries));
}

bool zar_disc_container::stat(const std::string& path, fs::stat_t& info) const
{
	if (!m_reader)
		return false;

	const u32 node = lookup(path, true, true);
	if (node == ZARCHIVE_INVALID_NODE)
	{
		fs::g_tls_error = fs::error::noent;
		return false;
	}

	const bool is_dir = m_reader->IsDirectory(node);
	const bool is_file_node = m_reader->IsFile(node);
	if (is_dir == is_file_node)
	{
		fs::g_tls_error = fs::error::inval;
		return false;
	}

	info = fs::stat_t
	{
		.is_directory = is_dir,
		.is_symlink = false,
		.is_writable = false,
		.size = is_file_node ? m_reader->GetFileSize(node) : ISO_SECTOR_SIZE,
		.atime = m_source_mtime,
		.mtime = m_source_mtime,
		.ctime = m_source_mtime,
	};
	return true;
}

u64 zar_disc_container::file_size(u32 node) const
{
	return m_reader && node != ZARCHIVE_INVALID_NODE && m_reader->IsFile(node) ? m_reader->GetFileSize(node) : 0;
}

u32 zar_disc_container::lookup(const std::string& path, bool allow_file, bool allow_directory) const
{
	return m_reader ? m_reader->LookUp(path, allow_file, allow_directory) : ZARCHIVE_INVALID_NODE;
}

bool zar_disc_container::is_file(u32 node) const
{
	return m_reader && node != ZARCHIVE_INVALID_NODE && m_reader->IsFile(node);
}

bool zar_disc_container::is_directory(u32 node) const
{
	return m_reader && node != ZARCHIVE_INVALID_NODE && m_reader->IsDirectory(node);
}

u32 zar_disc_container::dir_entry_count(u32 node) const
{
	return is_directory(node) ? m_reader->GetDirEntryCount(node) : 0;
}

bool zar_disc_container::dir_entry(u32 node, u32 index, zar_dir_entry& entry) const
{
	if (!is_directory(node) || index >= m_reader->GetDirEntryCount(node)) return false;
	ZArchiveReader::DirEntry in{};
	if (!m_reader->GetDirEntry(node, index, in) || in.name.empty()) return false;
	const std::string name(in.name);
	entry = {.name = name, .node = ZARCHIVE_INVALID_NODE, .is_file = in.isFile, .is_directory = in.isDirectory, .size = 0};
	return true;
}

namespace
{
	struct zar_writer_context
	{
		explicit zar_writer_context(std::string output)
			: output_path(std::move(output))
		{
		}

		std::string output_path;
		fs::pending_file output;
		bool opened = false;
		bool failed = false;
		std::string error;
	};

	void zar_new_output_file(const int32_t part_index, void* context_ptr)
	{
		auto& context = *static_cast<zar_writer_context*>(context_ptr);

		// ZArchive 0.1.x currently creates one output stream and calls this with -1.
		if (context.opened || part_index > 0)
		{
			context.failed = true;
			context.error = "multi-part ZArchive output is not supported";
			return;
		}

		context.opened = context.output.open(context.output_path);
		if (!context.opened)
		{
			context.failed = true;
			context.error = fmt::format("failed to create output file '%s': %s", context.output_path, fs::g_tls_error);
		}
	}

	void zar_write_output_data(const void* data, size_t length, void* context_ptr)
	{
		auto& context = *static_cast<zar_writer_context*>(context_ptr);
		if (context.failed || !context.opened || !length)
		{
			return;
		}

		if (context.output.file.write(data, length) != length)
		{
			context.failed = true;
			context.error = fmt::format("failed while writing '%s': %s", context.output_path, fs::g_tls_error);
		}
	}

	struct zar_pack_entry
	{
		std::string source_path;
		std::string archive_path;
		bool is_directory = false;
		u64 size = 0;
	};

	bool is_valid_archive_name(std::string_view name)
	{
		return !name.empty() && name != "." && name != ".." && name.find('/') == std::string_view::npos && name.find('\\') == std::string_view::npos && name.find('\0') == std::string_view::npos;
	}

	bool collect_jb_entries(const std::string& root, std::vector<zar_pack_entry>& entries, u64& total_size, std::string& error)
	{
		struct pending_dir
		{
			std::string disk_path;
			std::string archive_path;
			u32 depth = 0;
		};

		std::vector<pending_dir> pending;
		pending.push_back({root, {}, 0});
		constexpr u32 max_depth = 128;
		constexpr usz max_entries = 1'000'000;

		while (!pending.empty())
		{
			pending_dir current = std::move(pending.back());
			pending.pop_back();

			if (current.depth > max_depth)
			{
				error = fmt::format("JB directory depth exceeds %u", max_depth);
				return false;
			}

			fs::dir directory(current.disk_path);
			if (!directory)
			{
				error = fmt::format("failed to open directory '%s': %s", current.disk_path, fs::g_tls_error);
				return false;
			}

			for (auto&& entry : directory)
			{
				if (entry.name == "." || entry.name == "..")
				{
					continue;
				}

				if (!is_valid_archive_name(entry.name))
				{
					error = fmt::format("unsupported path component in JB folder: '%s'", entry.name);
					return false;
				}

				if (entry.is_symlink)
				{
					error = fmt::format("symbolic links are not supported in JB archives: '%s'", entry.name);
					return false;
				}

				if (entries.size() >= max_entries)
				{
					error = fmt::format("JB folder contains more than %u entries", max_entries);
					return false;
				}

				const std::string disk_path = current.disk_path + (current.disk_path.ends_with('/') ? "" : "/") + entry.name;
				const std::string archive_path = current.archive_path.empty() ? entry.name : current.archive_path + "/" + entry.name;

				if (entry.is_directory)
				{
					entries.push_back({disk_path, archive_path, true, 0});
					pending.push_back({disk_path, archive_path, current.depth + 1});
				}
				else
				{
					entries.push_back({disk_path, archive_path, false, entry.size});
					if (std::numeric_limits<u64>::max() - total_size < entry.size)
					{
						error = "JB folder size overflow";
						return false;
					}
					total_size += entry.size;
				}
			}
		}

		// Parents must be created before children/files. Stable ordering also makes archives reproducible.
		std::sort(entries.begin(), entries.end(), [](const zar_pack_entry& a, const zar_pack_entry& b)
		{
			const usz depth_a = std::count(a.archive_path.begin(), a.archive_path.end(), '/');
			const usz depth_b = std::count(b.archive_path.begin(), b.archive_path.end(), '/');
			if (depth_a != depth_b) return depth_a < depth_b;
			if (a.is_directory != b.is_directory) return a.is_directory > b.is_directory;
			return a.archive_path < b.archive_path;
		});

		return true;
	}

	bool append_file_to_archive(ZArchiveWriter& writer, const std::string& disk_path, const std::string& archive_path,
		u64 expected_size, u64& processed, u64 total, const zar_compression_progress_cb& progress, std::string& error,
		zar_writer_context& output)
	{
		if (!writer.StartNewFile(archive_path.c_str()))
		{
			error = fmt::format("failed to create '%s' in ZArchive (duplicate/case-colliding path or invalid parent)", archive_path);
			return false;
		}

		fs::file input(disk_path, fs::read);
		if (!input)
		{
			error = fmt::format("failed to open input file '%s': %s", disk_path, fs::g_tls_error);
			return false;
		}

		constexpr usz buffer_size = 4 * 1024 * 1024;
		std::vector<u8> buffer(buffer_size);
		u64 file_read = 0;

		while (file_read < expected_size)
		{
			if (progress && !progress(processed, total, archive_path))
			{
				error = "compression cancelled";
				return false;
			}

			const u64 to_read = std::min<u64>(buffer.size(), expected_size - file_read);
			const u64 read = input.read(buffer.data(), to_read);
			if (read != to_read)
			{
				error = fmt::format("short read from '%s': expected %llu bytes, read %llu", disk_path, to_read, read);
				return false;
			}

			writer.AppendData(buffer.data(), static_cast<size_t>(read));
			if (output.failed)
			{
				error = output.error;
				return false;
			}

			file_read += read;
			processed += read;
		}

		return true;
	}

	std::string key_archive_name(const std::string& key_path)
	{
		fs::file key(key_path, fs::read);
		return key && key.size() == 16 ? "disc.key" : "disc.dkey";
	}


	bool verify_archived_file(ZArchiveReader& reader, const std::string& source_path, const std::string& archive_path,
		u64 expected_size, const zar_compression_progress_cb& progress, std::string& error)
	{
		const ZArchiveNodeHandle node = reader.LookUp(archive_path, true, false);
		if (node == ZARCHIVE_INVALID_NODE || !reader.IsFile(node))
		{
			error = fmt::format("verification failed: missing file '%s' in output archive", archive_path);
			return false;
		}
		if (reader.GetFileSize(node) != expected_size)
		{
			error = fmt::format("verification failed: size mismatch for '%s'", archive_path);
			return false;
		}

		fs::file source(source_path, fs::read);
		if (!source || source.size() != expected_size)
		{
			error = fmt::format("verification failed: source file changed or cannot be reopened: '%s'", source_path);
			return false;
		}

		constexpr usz verify_buffer_size = 1024 * 1024;
		std::vector<u8> source_buffer(verify_buffer_size);
		std::vector<u8> archive_buffer(verify_buffer_size);
		u64 offset = 0;

		while (offset < expected_size)
		{
			if (progress && !progress(expected_size, expected_size, "Verifying: " + archive_path))
			{
				error = "compression cancelled during verification";
				return false;
			}

			const u64 chunk = std::min<u64>(verify_buffer_size, expected_size - offset);
			if (source.read_at(offset, source_buffer.data(), chunk) != chunk)
			{
				error = fmt::format("verification failed: short read from source '%s'", source_path);
				return false;
			}
			if (reader.ReadFromFile(node, offset, chunk, archive_buffer.data()) != chunk)
			{
				error = fmt::format("verification failed: short/decompression read from '%s'", archive_path);
				return false;
			}
			if (std::memcmp(source_buffer.data(), archive_buffer.data(), static_cast<usz>(chunk)) != 0)
			{
				error = fmt::format("verification failed: data mismatch in '%s' at offset 0x%llx", archive_path, offset);
				return false;
			}
			offset += chunk;
		}
		return true;
	}

	bool verify_created_archive(const std::string& output_path, bool is_iso, const std::string& source_path,
		const std::string& key_path, const std::vector<zar_pack_entry>& entries, const zar_compression_progress_cb& progress,
		std::string& error)
	{
		std::unique_ptr<ZArchiveReader> reader(ZArchiveReader::OpenFromFile(std::filesystem::path(reinterpret_cast<const char8_t*>(output_path.c_str()))));
		if (!reader)
		{
			error = "verification failed: output cannot be reopened as ZArchive";
			return false;
		}

		if (is_iso)
		{
			fs::file source(source_path, fs::read);
			if (!source)
			{
				error = fmt::format("verification failed: cannot reopen source ISO '%s'", source_path);
				return false;
			}
			if (!verify_archived_file(*reader, source_path, "disc.iso", source.size(), progress, error))
				return false;

			if (!key_path.empty())
			{
				fs::file key(key_path, fs::read);
				if (!key)
				{
					error = fmt::format("verification failed: cannot reopen key '%s'", key_path);
					return false;
				}
				if (!verify_archived_file(*reader, key_path, key_archive_name(key_path), key.size(), progress, error))
					return false;
			}
			return true;
		}

		for (const zar_pack_entry& entry : entries)
		{
			if (entry.is_directory)
			{
				const ZArchiveNodeHandle node = reader->LookUp(entry.archive_path, false, true);
				if (node == ZARCHIVE_INVALID_NODE || !reader->IsDirectory(node))
				{
					error = fmt::format("verification failed: missing directory '%s' in output archive", entry.archive_path);
					return false;
				}
				continue;
			}

			if (!verify_archived_file(*reader, entry.source_path, entry.archive_path, entry.size, progress, error))
				return false;
		}
		return true;
	}
}

zar_compression_result compress_ps3_disc_to_zar(const std::string& source_path, const std::string& output_path,
	const zar_compression_progress_cb& progress)
{
	zar_compression_result result{};

	if (source_path.empty() || output_path.empty())
	{
		result.status = zar_compression_status::invalid_source;
		result.error = "source and output paths must not be empty";
		return result;
	}

	if (zar_disc_container::has_zar_extension(source_path))
	{
		result.status = zar_compression_status::invalid_source;
		result.error = "source is already a ZArchive";
		return result;
	}

	if (fs::is_file(output_path))
	{
		result.status = zar_compression_status::output_exists;
		result.error = fmt::format("output file already exists: '%s'", output_path);
		return result;
	}

	const bool is_jb = fs::is_dir(source_path);
	const bool is_iso = !is_jb && fs::is_file(source_path) && is_iso_file(source_path);

	if (!is_jb && !is_iso)
	{
		result.status = zar_compression_status::invalid_source;
		result.error = "source is not a valid PS3 JB folder or ISO image";
		return result;
	}

	std::vector<zar_pack_entry> entries;
	std::string key_path;

	if (is_jb)
	{
		const std::string sfb = source_path + (source_path.ends_with('/') ? "" : "/") + "PS3_DISC.SFB";
		const std::string param = source_path + (source_path.ends_with('/') ? "" : "/") + "PS3_GAME/PARAM.SFO";
		if (!fs::is_file(sfb) || !fs::is_file(param))
		{
			result.status = zar_compression_status::invalid_source;
			result.error = "folder is not a PS3 disc JB layout (missing PS3_DISC.SFB or PS3_GAME/PARAM.SFO)";
			return result;
		}

		if (!collect_jb_entries(source_path, entries, result.input_size, result.error))
		{
			result.status = zar_compression_status::invalid_source;
			return result;
		}
	}
	else
	{
		iso_archive archive(source_path);
		if (!archive.is_valid() || !archive.is_file("PS3_DISC.SFB") || !archive.is_file("PS3_GAME/PARAM.SFO"))
		{
			result.status = zar_compression_status::invalid_source;
			result.error = "ISO is not a valid PS3 disc image (missing PS3_DISC.SFB or PS3_GAME/PARAM.SFO)";
			return result;
		}

		fs::file iso(source_path, fs::read);
		if (!iso)
		{
			result.status = zar_compression_status::io_error;
			result.error = fmt::format("failed to open ISO '%s': %s", source_path, fs::g_tls_error);
			return result;
		}
		result.input_size = iso.size();

		const iso_type_status key_status = iso_file_decryption::find_key(source_path, &key_path);
		if (key_status == iso_type_status::ERROR_PROCESSING_KEY)
		{
			result.status = zar_compression_status::invalid_key;
			result.error = fmt::format("matching key file is invalid: '%s'", key_path);
			return result;
		}
		if (key_status != iso_type_status::REDUMP_ISO)
		{
			key_path.clear();
		}

		if (!key_path.empty())
		{
			fs::file key(key_path, fs::read);
			if (!key || (key.size() != 16 && key.size() != 32))
			{
				result.status = zar_compression_status::invalid_key;
				result.error = fmt::format("key file has invalid size or cannot be opened: '%s'", key_path);
				return result;
			}
			result.embedded_key = key_path;
		}
	}

	zar_writer_context context(output_path);
	ZArchiveWriter writer(zar_new_output_file, zar_write_output_data, &context);
	if (context.failed || !context.opened)
	{
		result.status = zar_compression_status::io_error;
		result.error = context.error.empty() ? "failed to initialize ZArchive output" : context.error;
		return result;
	}

	u64 processed = 0;
	std::string error;

	if (is_iso)
	{
		if (!append_file_to_archive(writer, source_path, "disc.iso", result.input_size, processed, result.input_size, progress, error, context))
		{
			result.status = error == "compression cancelled" ? zar_compression_status::cancelled : zar_compression_status::archive_error;
			result.error = std::move(error);
			return result;
		}

		if (!key_path.empty())
		{
			fs::file key(key_path, fs::read);
			const u64 key_size = key ? key.size() : 0;
			const std::string archive_name = key_archive_name(key_path);
			if (!append_file_to_archive(writer, key_path, archive_name, key_size, processed, result.input_size + key_size, {}, error, context))
			{
				result.status = zar_compression_status::archive_error;
				result.error = std::move(error);
				return result;
			}
		}
	}
	else
	{
		for (const zar_pack_entry& entry : entries)
		{
			if (progress && !progress(processed, result.input_size, entry.archive_path))
			{
				result.status = zar_compression_status::cancelled;
				result.error = "compression cancelled";
				return result;
			}

			if (entry.is_directory)
			{
				if (!writer.MakeDir(entry.archive_path.c_str(), false))
				{
					result.status = zar_compression_status::archive_error;
					result.error = fmt::format("failed to create directory '%s' in ZArchive", entry.archive_path);
					return result;
				}
				continue;
			}

			if (!append_file_to_archive(writer, entry.source_path, entry.archive_path, entry.size, processed, result.input_size, progress, error, context))
			{
				result.status = error == "compression cancelled" ? zar_compression_status::cancelled : zar_compression_status::archive_error;
				result.error = std::move(error);
				return result;
			}
		}
	}

	if (progress && !progress(result.input_size, result.input_size, {}))
	{
		result.status = zar_compression_status::cancelled;
		result.error = "compression cancelled";
		return result;
	}

	writer.Finalize();
	if (context.failed)
	{
		result.status = zar_compression_status::io_error;
		result.error = context.error;
		return result;
	}

	result.output_size = context.output.file.size();
	context.output.file.sync();
	if (!context.output.commit(false))
	{
		result.status = fs::is_file(output_path) ? zar_compression_status::output_exists : zar_compression_status::io_error;
		result.error = fmt::format("failed to commit output file '%s': %s", output_path, fs::g_tls_error);
		return result;
	}

	std::string verify_error;
	if (!verify_created_archive(output_path, is_iso, source_path, key_path, entries, progress, verify_error))
	{
		fs::remove_file(output_path);
		result.status = verify_error.starts_with("compression cancelled") ? zar_compression_status::cancelled : zar_compression_status::archive_error;
		result.error = std::move(verify_error);
		return result;
	}

	result.status = zar_compression_status::success;
	return result;
}
