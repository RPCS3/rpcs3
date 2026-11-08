#include "stdafx.h"
#include "ZAR_ISO.h"
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
	// ZArchive 0.1.x footer: 6 sections (offset + size), a SHA-256 hash, the total size, the version and the magic
	constexpr u64 ZAR_FOOTER_SIZE = 144;
	constexpr u32 ZAR_FOOTER_MAGIC = 0x169f52d6;
	constexpr u32 ZAR_FOOTER_VERSION = 0x61bf3a01;

	// Limits guarding the hierarchy built out of an archive and the one packed into an archive
	constexpr u32 ZAR_MAX_DEPTH = 128;
	constexpr u64 ZAR_MAX_NODES = 1'000'000;

	constexpr std::string_view ZAR_IMAGE_NAME = "disc.iso";
	constexpr std::string_view ZAR_DKEY_NAME = "disc.dkey";
	constexpr std::string_view ZAR_KEY_NAME = "disc.key";

	class zar_file final : public fs::file_base
	{
	public:
		zar_file(std::shared_ptr<ZArchiveReader> reader, ZArchiveNodeHandle node, std::string name, s64 mtime)
			: m_reader(std::move(reader)), m_node(node), m_name(std::move(name)), m_mtime(mtime)
		{
			m_size = m_reader->GetFileSize(m_node);
		}

		fs::stat_t get_stat() override
		{
			return fs::stat_t
			{
				.is_directory = false,
				.is_symlink = false,
				.is_writable = false,
				.size = m_size,
				.atime = m_mtime,
				.mtime = m_mtime,
				.ctime = m_mtime,
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
			if (offset >= m_size || !size)
			{
				return 0;
			}

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
			const s64 new_pos =
				whence == fs::seek_set ? offset :
				whence == fs::seek_cur ? offset + m_pos :
				whence == fs::seek_end ? offset + m_size : -1;

			if (new_pos < 0)
			{
				fs::g_tls_error = fs::error::inval;
				return umax;
			}

			m_pos = new_pos;
			return m_pos;
		}

		u64 size() override
		{
			return m_size;
		}

	private:
		std::shared_ptr<ZArchiveReader> m_reader;
		ZArchiveNodeHandle m_node = ZARCHIVE_INVALID_NODE;
		std::string m_name;
		u64 m_pos = 0;
		u64 m_size = 0;
		s64 m_mtime = 0;
	};

	bool is_valid_entry_name(std::string_view name)
	{
		return !name.empty() && name != "." && name != ".." && name.find_first_of("/\\"sv) == umax && name.find('\0') == umax;
	}

	std::filesystem::path to_fs_path(const std::string& path)
	{
		return std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t*>(path.data()), path.size()));
	}
}

bool is_iso_container(const std::string& path)
{
	if (!zar_iso_container::has_zar_extension(path))
	{
		return false;
	}

	const fs::file file(path);

	if (!file || file.size() <= ZAR_FOOTER_SIZE)
	{
		return false;
	}

	std::array<u8, ZAR_FOOTER_SIZE> footer {};

	if (file.read_at(file.size() - footer.size(), footer.data(), footer.size()) != footer.size())
	{
		return false;
	}

	return read_from_ptr<be_t<u64>>(footer, 128) == file.size() &&
		read_from_ptr<be_t<u32>>(footer, 136) == ZAR_FOOTER_VERSION &&
		read_from_ptr<be_t<u32>>(footer, 140) == ZAR_FOOTER_MAGIC;
}

std::shared_ptr<iso_container> open_iso_container(const std::string& path)
{
	if (!zar_iso_container::has_zar_extension(path))
	{
		return nullptr;
	}

	std::string error;
	std::shared_ptr<zar_iso_container> container = zar_iso_container::open(path, &error);

	if (!container)
	{
		zar_log.error("Failed to open '%s': %s", path, error);
	}

	return container;
}

bool zar_iso_container::has_zar_extension(std::string_view path)
{
	constexpr std::string_view ext = ".zar";

	return path.size() >= ext.size() && std::equal(ext.begin(), ext.end(), path.end() - ext.size(), [](char a, char b)
	{
		return a == utils::tolower(b);
	});
}

zar_iso_container::zar_iso_container(const std::string& path, std::shared_ptr<ZArchiveReader> reader, s64 mtime)
	: m_path(path), m_reader(std::move(reader)), m_mtime(mtime)
{
}

std::shared_ptr<zar_iso_container> zar_iso_container::open(const std::string& path, std::string* error)
{
	const auto fail = [error](std::string message) -> std::shared_ptr<zar_iso_container>
	{
		if (error)
		{
			*error = std::move(message);
		}

		return nullptr;
	};

	fs::stat_t stat {};

	if (!has_zar_extension(path) || !fs::get_stat(path, stat) || stat.is_directory)
	{
		return fail("not a .zar file");
	}

	std::shared_ptr<ZArchiveReader> reader(ZArchiveReader::OpenFromFile(to_fs_path(path)));

	if (!reader)
	{
		return fail("invalid or unsupported ZArchive");
	}

	std::shared_ptr<zar_iso_container> container(new zar_iso_container(path, std::move(reader), stat.mtime));
	ZArchiveReader& zar = *container->m_reader;

	const ZArchiveNodeHandle image_node = zar.LookUp(ZAR_IMAGE_NAME, true, false);
	const bool has_image = image_node != ZARCHIVE_INVALID_NODE;
	const bool has_folder = zar.LookUp("PS3_DISC.SFB", true, false) != ZARCHIVE_INVALID_NODE && zar.LookUp("PS3_GAME/PARAM.SFO", true, false) != ZARCHIVE_INVALID_NODE;

	if (has_image == has_folder)
	{
		return fail(has_image ?
			"ambiguous disc archive: contains both disc.iso and a disc folder layout" :
			"not a PS3 disc archive: expected disc.iso or PS3_DISC.SFB + PS3_GAME/PARAM.SFO");
	}

	if (!has_image)
	{
		return container;
	}

	// The standard identifier ("CD001") follows the type of the first volume descriptor
	std::array<char, 5> magic {};

	if (zar.ReadFromFile(image_node, ISO_DESCRIPTORS_OFFSET + 1, magic.size(), magic.data()) != magic.size() || std::string_view(magic.data(), magic.size()) != "CD001")
	{
		return fail("disc.iso is not an ISO 9660 image");
	}

	container->m_image_node = image_node;

	for (const std::string_view key_name : {ZAR_DKEY_NAME, ZAR_KEY_NAME})
	{
		const ZArchiveNodeHandle key_node = zar.LookUp(key_name, true, false);

		if (key_node == ZARCHIVE_INVALID_NODE)
		{
			continue;
		}

		if (container->m_key_node != umax)
		{
			return fail("archive contains both disc.dkey and disc.key");
		}

		if (const u64 key_size = zar.GetFileSize(key_node); key_size != 16 && key_size != 32)
		{
			return fail(fmt::format("%s has invalid size %llu (expected 16 raw bytes or 32 hex characters)", key_name, key_size));
		}

		container->m_key_node = key_node;
		container->m_key_name = key_name;
	}

	return container;
}

fs::file zar_iso_container::open_node(u32 node, std::string_view name) const
{
	if (node == ZARCHIVE_INVALID_NODE || !m_reader->IsFile(node))
	{
		fs::g_tls_error = fs::error::noent;
		return {};
	}

	return fs::file(std::make_unique<zar_file>(m_reader, node, fmt::format("%s::%s", m_path, name), m_mtime));
}

fs::file zar_iso_container::open_image() const
{
	return open_node(m_image_node, ZAR_IMAGE_NAME);
}

fs::file zar_iso_container::open_key() const
{
	return open_node(m_key_node, m_key_name);
}

fs::file zar_iso_container::open_file(const iso_fs_node& node) const
{
	return open_node(node.metadata.container_node, node.metadata.name);
}

bool zar_iso_container::form_hierarchy(iso_fs_node& root) const
{
	root = {};
	root.metadata.name = ".";
	root.metadata.time = m_mtime;
	root.metadata.is_directory = true;
	root.metadata.extents.push_back({0, ISO_SECTOR_SIZE});

	u64 node_count = 1;

	return form_hierarchy(root, "", 0, node_count);
}

bool zar_iso_container::form_hierarchy(iso_fs_node& parent, const std::string& parent_path, u32 depth, u64& node_count) const
{
	const ZArchiveNodeHandle dir_node = m_reader->LookUp(parent_path, false, true);

	if (dir_node == ZARCHIVE_INVALID_NODE)
	{
		return false;
	}

	const u32 count = m_reader->GetDirEntryCount(dir_node);

	// The entries plus "." and ".."
	node_count += count + 2;

	if (depth > ZAR_MAX_DEPTH || node_count > ZAR_MAX_NODES)
	{
		zar_log.error("form_hierarchy: '%s' exceeds the limits (depth=%u, nodes=%llu)", m_path, depth, node_count);
		return false;
	}

	parent.metadata.container_node = dir_node;
	parent.children.reserve(count + 2);

	// Listed like on an image, where every directory holds both
	for (const std::string_view name : {"."sv, ".."sv})
	{
		auto& node = parent.children.emplace_back(std::make_unique<iso_fs_node>());
		node->metadata.name = name;
		node->metadata.time = m_mtime;
		node->metadata.is_directory = true;
		node->metadata.extents.push_back({0, ISO_SECTOR_SIZE});
	}

	for (u32 i = 0; i < count; i++)
	{
		ZArchiveReader::DirEntry entry {};

		if (!m_reader->GetDirEntry(dir_node, i, entry) || !is_valid_entry_name(entry.name) || entry.isFile == entry.isDirectory)
		{
			zar_log.error("form_hierarchy: Invalid entry in '%s' at '%s' (index %u)", m_path, parent_path, i);
			return false;
		}

		const std::string path = parent_path.empty() ? std::string(entry.name) : fmt::format("%s/%s", parent_path, entry.name);

		auto& node = parent.children.emplace_back(std::make_unique<iso_fs_node>());
		node->metadata.name = entry.name;
		node->metadata.time = m_mtime;
		node->metadata.is_directory = entry.isDirectory;

		if (entry.isDirectory)
		{
			node->metadata.extents.push_back({0, ISO_SECTOR_SIZE});

			if (!form_hierarchy(*node, path, depth + 1, node_count))
			{
				return false;
			}

			continue;
		}

		node->metadata.container_node = m_reader->LookUp(path, true, false);

		if (node->metadata.container_node == ZARCHIVE_INVALID_NODE)
		{
			zar_log.error("form_hierarchy: Failed to look up '%s' in '%s'", path, m_path);
			return false;
		}

		node->metadata.extents.push_back({0, m_reader->GetFileSize(node->metadata.container_node)});
	}

	return true;
}

namespace
{
	struct zar_writer_context
	{
		std::string output_path;
		fs::pending_file output;
		bool opened = false;
		std::string error;

		bool failed() const { return !error.empty(); }
	};

	void zar_new_output_file(const int32_t part_index, void* context_ptr)
	{
		auto& context = *static_cast<zar_writer_context*>(context_ptr);

		// ZArchive 0.1.x creates a single output file, announced with -1
		if (context.opened || part_index > 0)
		{
			context.error = "multi-part ZArchive output is not supported";
			return;
		}

		context.opened = context.output.open(context.output_path);

		if (!context.opened)
		{
			context.error = fmt::format("failed to create output file '%s': %s", context.output_path, fs::g_tls_error);
		}
	}

	void zar_write_output_data(const void* data, size_t length, void* context_ptr)
	{
		auto& context = *static_cast<zar_writer_context*>(context_ptr);

		if (context.failed() || !context.opened || !length)
		{
			return;
		}

		if (context.output.file.write(data, length) != length)
		{
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

	// Sets the result to a failure and returns false, for the helpers below to bail out in a single statement
	bool fail(zar_compression_result& result, zar_compression_status status, std::string error)
	{
		result.status = status;
		result.error = std::move(error);
		return false;
	}

	bool collect_folder_entries(const std::string& root, std::vector<zar_pack_entry>& entries, zar_compression_result& result)
	{
		struct pending_dir
		{
			std::string disk_path;
			std::string archive_path;
			u32 depth = 0;
		};

		std::vector<pending_dir> pending;
		pending.push_back({root, {}, 0});

		while (!pending.empty())
		{
			const pending_dir current = std::move(pending.back());
			pending.pop_back();

			if (current.depth > ZAR_MAX_DEPTH)
			{
				return fail(result, zar_compression_status::invalid_source, fmt::format("directory depth exceeds %u", ZAR_MAX_DEPTH));
			}

			fs::dir directory(current.disk_path);

			if (!directory)
			{
				return fail(result, zar_compression_status::invalid_source, fmt::format("failed to open directory '%s': %s", current.disk_path, fs::g_tls_error));
			}

			for (const fs::dir_entry& entry : directory)
			{
				if (entry.name == "." || entry.name == "..")
				{
					continue;
				}

				if (!is_valid_entry_name(entry.name))
				{
					return fail(result, zar_compression_status::invalid_source, fmt::format("unsupported file name: '%s'", entry.name));
				}

				if (entry.is_symlink)
				{
					return fail(result, zar_compression_status::invalid_source, fmt::format("symbolic links are not supported: '%s'", entry.name));
				}

				if (entries.size() >= ZAR_MAX_NODES)
				{
					return fail(result, zar_compression_status::invalid_source, fmt::format("folder contains more than %u entries", ZAR_MAX_NODES));
				}

				std::string disk_path = fmt::format("%s/%s", current.disk_path, entry.name);
				std::string archive_path = current.archive_path.empty() ? entry.name : fmt::format("%s/%s", current.archive_path, entry.name);

				if (entry.is_directory)
				{
					pending.push_back({disk_path, archive_path, current.depth + 1});
					entries.push_back({std::move(disk_path), std::move(archive_path), true, 0});
					continue;
				}

				if (std::numeric_limits<u64>::max() - result.input_size < entry.size)
				{
					return fail(result, zar_compression_status::invalid_source, "folder size overflow");
				}

				result.input_size += entry.size;
				entries.push_back({std::move(disk_path), std::move(archive_path), false, entry.size});
			}
		}

		// Parents must be created before their children. A stable order also makes archives reproducible
		std::sort(entries.begin(), entries.end(), [](const zar_pack_entry& a, const zar_pack_entry& b)
		{
			const usz depth_a = std::count(a.archive_path.begin(), a.archive_path.end(), '/');
			const usz depth_b = std::count(b.archive_path.begin(), b.archive_path.end(), '/');

			if (depth_a != depth_b)
			{
				return depth_a < depth_b;
			}

			if (a.is_directory != b.is_directory)
			{
				return a.is_directory;
			}

			return a.archive_path < b.archive_path;
		});

		return true;
	}

	bool append_file(ZArchiveWriter& writer, zar_writer_context& context, const zar_pack_entry& entry, u64& processed, u64 total,
		const zar_compression_progress_cb& progress, zar_compression_result& result)
	{
		if (!writer.StartNewFile(entry.archive_path.c_str()))
		{
			return fail(result, zar_compression_status::archive_error, fmt::format("failed to create '%s' in the archive (duplicate path or invalid parent)", entry.archive_path));
		}

		const fs::file input(entry.source_path);

		if (!input)
		{
			return fail(result, zar_compression_status::io_error, fmt::format("failed to open '%s': %s", entry.source_path, fs::g_tls_error));
		}

		std::vector<u8> buffer(4 * 1024 * 1024);

		for (u64 file_read = 0; file_read < entry.size;)
		{
			if (progress && !progress(processed, total, entry.archive_path))
			{
				return fail(result, zar_compression_status::cancelled, "compression cancelled");
			}

			const u64 to_read = std::min<u64>(buffer.size(), entry.size - file_read);

			if (input.read(buffer.data(), to_read) != to_read)
			{
				return fail(result, zar_compression_status::io_error, fmt::format("short read from '%s'", entry.source_path));
			}

			writer.AppendData(buffer.data(), to_read);

			if (context.failed())
			{
				return fail(result, zar_compression_status::io_error, context.error);
			}

			file_read += to_read;
			processed += to_read;
		}

		return true;
	}

	bool verify_file(ZArchiveReader& reader, const zar_pack_entry& entry, const zar_compression_progress_cb& progress, zar_compression_result& result)
	{
		const ZArchiveNodeHandle node = reader.LookUp(entry.archive_path, true, false);

		if (node == ZARCHIVE_INVALID_NODE || reader.GetFileSize(node) != entry.size)
		{
			return fail(result, zar_compression_status::archive_error, fmt::format("verification failed: '%s' is missing or has a wrong size", entry.archive_path));
		}

		const fs::file source(entry.source_path);

		if (!source || source.size() != entry.size)
		{
			return fail(result, zar_compression_status::io_error, fmt::format("verification failed: '%s' changed or cannot be reopened", entry.source_path));
		}

		constexpr u64 chunk_size = 1024 * 1024;
		std::vector<u8> source_buffer(chunk_size);
		std::vector<u8> archive_buffer(chunk_size);

		for (u64 offset = 0; offset < entry.size;)
		{
			if (progress && !progress(entry.size, entry.size, "Verifying: " + entry.archive_path))
			{
				return fail(result, zar_compression_status::cancelled, "compression cancelled");
			}

			const u64 chunk = std::min<u64>(chunk_size, entry.size - offset);

			if (source.read_at(offset, source_buffer.data(), chunk) != chunk || reader.ReadFromFile(node, offset, chunk, archive_buffer.data()) != chunk ||
				std::memcmp(source_buffer.data(), archive_buffer.data(), chunk) != 0)
			{
				return fail(result, zar_compression_status::archive_error, fmt::format("verification failed: data mismatch in '%s' at offset 0x%llx", entry.archive_path, offset));
			}

			offset += chunk;
		}

		return true;
	}

	bool verify_archive(const std::string& output_path, const std::vector<zar_pack_entry>& entries, const zar_compression_progress_cb& progress, zar_compression_result& result)
	{
		const std::unique_ptr<ZArchiveReader> reader(ZArchiveReader::OpenFromFile(to_fs_path(output_path)));

		if (!reader)
		{
			return fail(result, zar_compression_status::archive_error, "verification failed: the output cannot be reopened");
		}

		for (const zar_pack_entry& entry : entries)
		{
			if (entry.is_directory)
			{
				if (reader->LookUp(entry.archive_path, false, true) == ZARCHIVE_INVALID_NODE)
				{
					return fail(result, zar_compression_status::archive_error, fmt::format("verification failed: directory '%s' is missing", entry.archive_path));
				}

				continue;
			}

			if (!verify_file(*reader, entry, progress, result))
			{
				return false;
			}
		}

		return true;
	}

	bool collect_image_entries(const std::string& source_path, std::vector<zar_pack_entry>& entries, zar_compression_result& result)
	{
		iso_archive archive(source_path);

		if (!archive.is_valid() || !archive.is_file("PS3_DISC.SFB") || !archive.is_file("PS3_GAME/PARAM.SFO"))
		{
			return fail(result, zar_compression_status::invalid_source, "the image is not a valid PS3 disc image (missing PS3_DISC.SFB or PS3_GAME/PARAM.SFO)");
		}

		result.input_size = fs::file(source_path).size();
		entries.push_back({source_path, std::string(ZAR_IMAGE_NAME), false, result.input_size});

		std::string key_path;

		switch (iso_file_decryption::find_key(source_path, &key_path))
		{
		case iso_type_status::REDUMP_ISO:
			break;
		case iso_type_status::ERROR_PROCESSING_KEY:
			return fail(result, zar_compression_status::invalid_key, fmt::format("the key file found for the image is invalid: '%s'", key_path));
		default:
			return true;
		}

		// The key is stored as found, a 16 bytes one being binary and a 32 bytes one an hex string
		const u64 key_size = fs::file(key_path).size();

		entries.push_back({key_path, std::string(key_size == 16 ? ZAR_KEY_NAME : ZAR_DKEY_NAME), false, key_size});
		result.embedded_key = std::move(key_path);
		return true;
	}
}

zar_compression_result compress_ps3_disc_to_zar(const std::string& source_path, const std::string& output_path, const zar_compression_progress_cb& progress)
{
	zar_compression_result result {};

	if (source_path.empty() || output_path.empty())
	{
		fail(result, zar_compression_status::invalid_source, "source and output paths must not be empty");
		return result;
	}

	if (zar_iso_container::has_zar_extension(source_path))
	{
		fail(result, zar_compression_status::invalid_source, "source is already a ZArchive");
		return result;
	}

	if (fs::exists(output_path))
	{
		fail(result, zar_compression_status::output_exists, fmt::format("output file already exists: '%s'", output_path));
		return result;
	}

	std::vector<zar_pack_entry> entries;

	if (fs::is_dir(source_path))
	{
		if (!fs::is_file(source_path + "/PS3_DISC.SFB") || !fs::is_file(source_path + "/PS3_GAME/PARAM.SFO"))
		{
			fail(result, zar_compression_status::invalid_source, "folder is not a PS3 disc folder layout (missing PS3_DISC.SFB or PS3_GAME/PARAM.SFO)");
			return result;
		}

		if (!collect_folder_entries(source_path, entries, result))
		{
			return result;
		}
	}
	else if (fs::is_file(source_path) && is_iso_file(source_path))
	{
		if (!collect_image_entries(source_path, entries, result))
		{
			return result;
		}
	}
	else
	{
		fail(result, zar_compression_status::invalid_source, "source is not a PS3 disc folder or image");
		return result;
	}

	zar_writer_context context {.output_path = output_path};
	ZArchiveWriter writer(zar_new_output_file, zar_write_output_data, &context);

	if (context.failed() || !context.opened)
	{
		fail(result, zar_compression_status::io_error, context.failed() ? context.error : "failed to create the output file");
		return result;
	}

	u64 processed = 0;

	for (const zar_pack_entry& entry : entries)
	{
		if (entry.is_directory)
		{
			if (!writer.MakeDir(entry.archive_path.c_str(), false))
			{
				fail(result, zar_compression_status::archive_error, fmt::format("failed to create directory '%s' in the archive", entry.archive_path));
				return result;
			}

			continue;
		}

		if (!append_file(writer, context, entry, processed, result.input_size, progress, result))
		{
			return result;
		}
	}

	writer.Finalize();

	if (context.failed())
	{
		fail(result, zar_compression_status::io_error, context.error);
		return result;
	}

	result.output_size = context.output.file.size();
	context.output.file.sync();

	if (!context.output.commit(false))
	{
		fail(result, zar_compression_status::io_error, fmt::format("failed to commit output file '%s': %s", output_path, fs::g_tls_error));
		return result;
	}

	if (!verify_archive(output_path, entries, progress, result))
	{
		fs::remove_file(output_path);
		return result;
	}

	result.status = zar_compression_status::success;
	return result;
}
