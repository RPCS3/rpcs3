#include "stdafx.h"

#include "ISO.h"
#include "Emu/VFS.h"

#include <codecvt>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stack>

LOG_CHANNEL(sys_log, "SYS");

bool is_file_iso(const std::string& path)
{
	if (path.empty()) return false;
	if (fs::is_dir(path)) return false;

	return is_file_iso(fs::file(path));
}

bool is_file_iso(const fs::file& file)
{
	if (!file) return false;
	if (file.size() < 32768 + 6) return false;

	file.seek(32768);

	char magic[5];
	file.read_at(32768 + 1, magic, 5);

	return magic[0] == 'C' && magic[1] == 'D'
		&& magic[2] == '0' && magic[3] == '0'
		&& magic[4] == '1';
}

const int ISO_BLOCK_SIZE = 2048;

template<typename T>
inline T read_both_endian_int(fs::file& file)
{
	T out;

	if (std::endian::little == std::endian::native)
	{
		out = file.read<T>();
		file.seek(sizeof(T), fs::seek_cur);
	}
	else
	{
		file.seek(sizeof(T), fs::seek_cur);
		out = file.read<T>();
	}

	return out;
}

// assumed that directory_entry is at file head
std::optional<iso_fs_metadata> iso_read_directory_entry(fs::file& file, bool names_in_ucs2 = false)
{
	const auto start_pos = file.pos();
	const u8 entry_length = file.read<u8>();

	if (entry_length == 0) return std::nullopt;

	file.seek(1, fs::seek_cur);
	const u32 start_sector = read_both_endian_int<u32>(file);
	const u32 file_size = read_both_endian_int<u32>(file);

	std::tm file_date = {};
	file_date.tm_year = file.read<u8>();
	file_date.tm_mon = file.read<u8>() - 1;
	file_date.tm_mday = file.read<u8>();
	file_date.tm_hour = file.read<u8>();
	file_date.tm_min = file.read<u8>();
	file_date.tm_sec = file.read<u8>();
	const s16 timezone_value = file.read<u8>();
	const s16 timezone_offset = (timezone_value - 50) * 15 * 60;

	const std::time_t date_time = std::mktime(&file_date) + timezone_offset;

	const u8 flags = file.read<u8>();

	// 2nd flag bit indicates whether a given fs node is a directory
	const bool is_directory = flags & 0b00000010;
	const bool has_more_extents = flags & 0b10000000;

	file.seek(6, fs::seek_cur);

	const u8 file_name_length = file.read<u8>();

	std::string file_name;
	file.read(file_name, file_name_length);

	if (file_name_length == 1 && file_name[0] == 0)
	{
		file_name = ".";
	}
	else if (file_name == "\1")
	{
		file_name = "..";
	}
	else if (names_in_ucs2) // for strings in joliet descriptor
	{
		// characters are stored in big endian format.
		std::u16string utf16;
		utf16.resize(file_name_length / 2);

		const u16* raw = reinterpret_cast<const u16*>(file_name.data());
		for (size_t i = 0; i < utf16.size(); ++i, raw++)
		{
			utf16[i] = *reinterpret_cast<const be_t<u16>*>(raw);
		}

		file_name = utf16_to_utf8(utf16);
	}

	if (file_name.ends_with(";1"))
	{
		file_name.erase(file_name.end() - 2, file_name.end());
	}

	if (file_name_length > 1 && file_name.ends_with("."))
	{
		file_name.pop_back();
	}

	// skip the rest of the entry.
	file.seek(entry_length + start_pos);

	return iso_fs_metadata
	{
		.name = std::move(file_name),
		.time = date_time,
		.is_directory = is_directory,
		.has_multiple_extents = has_more_extents,
		.extents =
		{
			iso_extent_info
			{
				.start = start_sector,
				.size = file_size
			}
		}
	};
}

void iso_form_hierarchy(fs::file& file, iso_fs_node& node, bool use_ucs2_decoding = false, const std::string& parent_path = "")
{
	if (!node.metadata.is_directory) return;

	std::vector<usz> multi_extent_node_indices;

	// assuming the directory spans a single extent
	const auto& directory_extent = node.metadata.extents[0];

	file.seek(directory_extent.start * ISO_BLOCK_SIZE);

	const u64 end_pos = directory_extent.size + (directory_extent.start * ISO_BLOCK_SIZE);

	while(file.pos() < end_pos)
	{
		auto entry = iso_read_directory_entry(file, use_ucs2_decoding);
		if (!entry)
		{
			const u64 new_sector = (file.pos() / ISO_BLOCK_SIZE) + 1;
			file.seek(new_sector * ISO_BLOCK_SIZE);
			continue;
		}

		bool extent_added = false;

		// find previous extent and merge into it, otherwise we push this node's index
		for (usz index : multi_extent_node_indices)
		{
			auto& selected_node = ::at32(node.children, index);
			if (selected_node->metadata.name == entry->name)
			{
				// merge into selected_node
				selected_node->metadata.extents.push_back(entry->extents[0]);

				extent_added = true;
			}
		}

		if (extent_added) continue;

		if (entry->has_multiple_extents)
		{
			// haven't pushed entry to node.children yet so node.children::size() == entry_index
			multi_extent_node_indices.push_back(node.children.size());
		}

		node.children.push_back(std::make_unique<iso_fs_node>(iso_fs_node{
			.metadata = std::move(*entry)
		}));
	}

	for (auto& child_node : node.children)
	{
		if (child_node->metadata.name != "." && child_node->metadata.name != "..")
		{
			iso_form_hierarchy(file, *child_node, use_ucs2_decoding, parent_path + "/" + node.metadata.name);
		}
	}
}

u64 iso_fs_metadata::size() const
{
	u64 total_size = 0;
	for (const auto& extent : extents)
	{
		total_size += extent.size;
	}

	return total_size;
}

iso_archive::iso_archive(const std::string& path)
{
	m_path = path;
	m_file = fs::file(path);

	if (!is_file_iso(m_file))
	{
		// not iso... TODO: throw something??
		return;
	}

	u8 descriptor_type = -2;
	bool use_ucs2_decoding = false;

	do
	{
		const auto descriptor_start = m_file.pos();

		descriptor_type = m_file.read<u8>();

		// 1 = primary vol descriptor, 2 = joliet SVD
		if (descriptor_type == 1 || descriptor_type == 2)
		{
			use_ucs2_decoding = descriptor_type == 2;

			// skip the rest of descriptor's data
			m_file.seek(155, fs::seek_cur);

			m_root = iso_fs_node
			{
				.metadata = iso_read_directory_entry(m_file, use_ucs2_decoding).value(),
			};
		}

		m_file.seek(descriptor_start + ISO_BLOCK_SIZE);
	}
	while (descriptor_type != 255);

	iso_form_hierarchy(m_file, m_root, use_ucs2_decoding);
}

iso_fs_node* iso_archive::retrieve(const std::string& passed_path)
{
	if (passed_path.empty()) return nullptr;

	const std::string path = std::filesystem::path(passed_path).string();
	const std::string_view path_sv = path;

	size_t start = 0;
	size_t end = path_sv.find_first_of(fs::delim);

	std::stack<iso_fs_node*> search_stack;
	search_stack.push(&m_root);

	do
	{
		if (search_stack.empty()) return nullptr;
		const auto* top_entry = search_stack.top();

		if (end == umax)
		{
			end = path.size();
		}

		const std::string_view path_component = path_sv.substr(start, end-start);

		bool found = false;

		if (path_component == ".")
		{
			found = true;
		}
		else if (path_component == "..")
		{
			search_stack.pop();
			found = true;
		}
		else
		{
			for (const auto& entry : top_entry->children)
			{
				if (entry->metadata.name == path_component)
				{
					search_stack.push(entry.get());

					found = true;
					break;
				}
			}
		}

		if (!found) return nullptr;

		start = end + 1;
		end = path_sv.find_first_of(fs::delim, start);
	}
	while (start < path.size());

	if (search_stack.empty()) return nullptr;

	return search_stack.top();
}

bool iso_archive::exists(const std::string& path)
{
	return retrieve(path) != nullptr;
}

bool iso_archive::is_file(const std::string& path)
{
	const auto file_node = retrieve(path);
	if (!file_node) return false;

	return !file_node->metadata.is_directory;
}

iso_file iso_archive::open(const std::string& path)
{
	return iso_file(fs::file(m_path), *ensure(retrieve(path)));
}

psf::registry iso_archive::open_psf(const std::string& path)
{
	auto* archive_file = retrieve(path);
	if (!archive_file) return psf::registry();

	const fs::file psf_file(std::make_unique<iso_file>(fs::file(m_path), *archive_file));

	return psf::load_object(psf_file, path);
}

iso_file::iso_file(fs::file&& iso_handle, const iso_fs_node& node)
	: m_file(std::move(iso_handle)), m_meta(node.metadata)
{
	m_file.seek(ISO_BLOCK_SIZE * node.metadata.extents[0].start);
}

fs::stat_t iso_file::get_stat()
{
	return fs::stat_t
	{
		.is_directory = false,
		.is_symlink = false,
		.is_writable = false,
		.size = size(),
		.atime = m_meta.time,
		.mtime = m_meta.time,
		.ctime = m_meta.time
	};
}

bool iso_file::trunc(u64 /*length*/)
{
	fs::g_tls_error = fs::error::readonly;
	return false;
}

std::pair<u64, iso_extent_info> iso_file::get_extent_pos(u64 pos) const
{
	ensure(!m_meta.extents.empty());

	auto it = m_meta.extents.begin();

	while (pos >= it->size && it != m_meta.extents.end() - 1)
	{
		pos -= it->size;

		it++;
	}

	return {pos, *it};
}

// assumed valid and in bounds.
u64 iso_file::file_offset(u64 pos) const
{
	const auto [local_pos, extent] = get_extent_pos(pos);

	return (extent.start * ISO_BLOCK_SIZE) + local_pos;
}

u64 iso_file::local_extent_remaining(u64 pos) const
{
	const auto [local_pos, extent] = get_extent_pos(pos);

	return extent.size - local_pos;
}

u64 iso_file::local_extent_size(u64 pos) const
{
	return get_extent_pos(pos).second.size;
}

u64 iso_file::read(void* buffer, u64 size)
{
	const auto r = read_at(m_pos, buffer, size);
	m_pos += r;
	return r;
}

u64 iso_file::read_at(u64 offset, void* buffer, u64 size)
{
	const u64 local_remaining = local_extent_remaining(offset);

	const u64 total_read = m_file.read_at(file_offset(offset), buffer, std::min(size, local_remaining));

	const auto total_size = this->size();

	if (size > total_read && (offset + total_read) < total_size)
	{
		const u64 second_total_read = read_at(offset + total_read, reinterpret_cast<u8*>(buffer) + total_read, size - total_read);

		return total_read + second_total_read;
	}

	return total_read;
}

u64 iso_file::write(const void* /*buffer*/, u64 /*size*/)
{
	fs::g_tls_error = fs::error::readonly;
	return 0;
}

u64 iso_file::seek(s64 offset, fs::seek_mode whence)
{
	const s64 total_size = size();
	const s64 new_pos =
		whence == fs::seek_set ? offset :
		whence == fs::seek_cur ? offset + m_pos :
		whence == fs::seek_end ? offset + total_size : -1;

	if (new_pos < 0)
	{
		fs::g_tls_error = fs::error::inval;
		return -1;
	}

	const u64 result = m_file.seek(file_offset(m_pos));
	if (result == umax) return umax;

	m_pos = new_pos;
	return m_pos;
}

u64 iso_file::size()
{
	u64 extent_sizes = 0;
	for (const auto& extent : m_meta.extents)
	{
		extent_sizes += extent.size;
	}

	return extent_sizes;
}

void iso_file::release()
{
	m_file.release();
}

bool iso_dir::read(fs::dir_entry& entry)
{
	if (m_pos < m_node.children.size())
	{
		const auto& selected = m_node.children[m_pos].get()->metadata;

		entry.name = selected.name;
		entry.atime = selected.time;
		entry.mtime = selected.time;
		entry.ctime = selected.time;
		entry.is_directory = selected.is_directory;
		entry.is_symlink = false;
		entry.is_writable = false;
		entry.size = selected.size();

		m_pos++;

		return true;
	}

	return false;
}

bool iso_device::stat(const std::string& path, fs::stat_t& info)
{
	const auto relative_path = std::filesystem::relative(std::filesystem::path(path), std::filesystem::path(fs_prefix)).string();

	const auto node = m_archive.retrieve(relative_path);
	if (!node)
	{
		fs::g_tls_error = fs::error::noent;
		return false;
	}

	const auto& meta = node->metadata;

	info = fs::stat_t
	{
		.is_directory = meta.is_directory,
		.is_symlink = false,
		.is_writable = false,
		.size = meta.size(),
		.atime = meta.time,
		.mtime = meta.time,
		.ctime = meta.time
	};

	return true;
}

bool iso_device::statfs(const std::string& path, fs::device_stat& info)
{
	const auto relative_path = std::filesystem::relative(std::filesystem::path(path), std::filesystem::path(fs_prefix)).string();

	const auto node = m_archive.retrieve(relative_path);
	if (!node)
	{
		fs::g_tls_error = fs::error::noent;
		return false;
	}

	const auto& meta = node->metadata;
	const u64 size = meta.size();

	info = fs::device_stat
	{
		.block_size = size,
		.total_size = size,
		.total_free = 0,
		.avail_free = 0
	};

	return false;
}

std::unique_ptr<fs::file_base> iso_device::open(const std::string& path, bs_t<fs::open_mode> mode)
{
	const auto relative_path = std::filesystem::relative(std::filesystem::path(path), std::filesystem::path(fs_prefix)).string();

	const auto node = m_archive.retrieve(relative_path);
	if (!node)
	{
		fs::g_tls_error = fs::error::noent;
		return nullptr;
	}

	if (node->metadata.is_directory)
	{
		fs::g_tls_error = fs::error::isdir;
		return nullptr;
	}

	return std::make_unique<iso_file>(fs::file(iso_path), *node);
}

std::unique_ptr<fs::dir_base> iso_device::open_dir(const std::string& path)
{
	const auto relative_path = std::filesystem::relative(std::filesystem::path(path), std::filesystem::path(fs_prefix)).string();

	const auto node = m_archive.retrieve(relative_path);
	if (!node)
	{
		fs::g_tls_error = fs::error::noent;
		return nullptr;
	}

	if (!node->metadata.is_directory)
	{
		// fs::dir::open -> ::readdir should return ENOTDIR when path is
		// pointing to a file instead of a folder, which translates to error::unknown.
		// doing the same here.
		fs::g_tls_error = fs::error::unknown;
		return nullptr;
	}

	return std::make_unique<iso_dir>(*node);
}

void iso_dir::rewind()
{
	m_pos = 0;
}

void load_iso(const std::string& path)
{
	sys_log.notice("Loading iso '%s'", path);

	fs::set_virtual_device("iso_overlay_fs_dev", stx::make_shared<iso_device>(path));

	vfs::mount("/dev_bdvd/"sv, iso_device::virtual_device_name + "/");
}

void unload_iso()
{
	sys_log.notice("Unoading iso");

	fs::set_virtual_device("iso_overlay_fs_dev", stx::shared_ptr<iso_device>());
}
