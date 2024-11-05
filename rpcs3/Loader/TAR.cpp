#include "stdafx.h"

#include "Emu/VFS.h"
#include "Emu/System.h"

#include "Crypto/unself.h"

#include "TAR.h"

#include "util/asm.hpp"

#include "util/serialization_ext.hpp"

#include <charconv>
#include <span>

LOG_CHANNEL(tar_log, "TAR");

fs::file make_file_view(const fs::file& file, u64 offset, u64 size);

// File constructor
tar_object::tar_object(const fs::file& file)
	: m_file(std::addressof(file))
	, m_ar(nullptr)
	, m_ar_tar_start(umax)
{
	ensure(*m_file);
}

// Stream (pipe-like) constructor
tar_object::tar_object(utils::serial& ar)
	: m_file(nullptr)
	, m_ar(std::addressof(ar))
	, m_ar_tar_start(ar.pos)
{
}

TARHeader tar_object::read_header(u64 offset) const
{
	TARHeader header{};

	if (m_ar)
	{
		ensure(m_ar->pos == m_ar_tar_start + offset);
		m_ar->serialize(header);
		return header;
	}

	if (m_file->read_at(offset, &header, sizeof(header)) != sizeof(header))
	{
		std::memset(&header, 0, sizeof(header));
	}

	return header;
}

u64 octal_text_to_u64(std::string_view sv)
{
	u64 i = umax;
	const auto ptr = std::from_chars(sv.data(), sv.data() + sv.size(), i, 8).ptr;

	// Range must be terminated with either NUL or space
	if (ptr == sv.data() + sv.size() || (*ptr && *ptr != ' '))
	{
		i = umax;
	}

	return i;
}

std::vector<std::string> tar_object::get_filenames()
{
	std::vector<std::string> vec;

	get_file("");

	for (auto it = m_map.cbegin(); it != m_map.cend(); ++it)
	{
		vec.push_back(it->first);
	}

	return vec;
}

std::unique_ptr<utils::serial> tar_object::get_file(const std::string& path, std::string* new_file_path)
{
	std::unique_ptr<utils::serial> m_out;

	auto emplace_single_entry = [&](usz& offset, const usz max_size) -> std::pair<usz, std::string>
	{
		if (offset >= max_size)
		{
			return {};
		}

		TARHeader header = read_header(offset);
		offset += 512;

		u64 size = umax;

		std::string filename;

		if (std::memcmp(header.magic, "ustar", 5) == 0)
		{
			const std::string_view size_sv{header.size, std::size(header.size)};

			size = octal_text_to_u64(size_sv);

			// Check for overflows and if surpasses file size
			if ((header.name[0] || header.prefix[0]) && ~size >= 512 && max_size >= size && max_size - size >= offset)
			{
				// Cache size in native u64 format
				static_assert(sizeof(size) < sizeof(header.size));
				std::memcpy(header.size, &size, 8);

				std::string_view prefix_name{header.prefix, std::size(header.prefix)};
				std::string_view name{header.name, std::size(header.name)};

				prefix_name = prefix_name.substr(0, prefix_name.find_first_of('\0'));
				name = name.substr(0, name.find_first_of('\0'));

				filename += prefix_name;
				filename += name;

				// Save header and offset
				m_map.insert_or_assign(filename, std::make_pair(offset, std::move(header)));

				if (new_file_path)
				{
					*new_file_path = filename;
				}

				return { size, std::move(filename) };
			}

			tar_log.error("tar_object::get_file() failed to convert header.size=%s, filesize=0x%x", size_sv, max_size);
		}
		else
		{
			tar_log.notice("tar_object::get_file() failed to parse header: offset=0x%x, filesize=0x%x, header_first16=0x%016x", offset, max_size, read_from_ptr<be_t<u128>>(reinterpret_cast<const u8*>(&header)));
		}

		return { size, {} };
	};

	if (auto it = m_map.find(path); it != m_map.end())
	{
		u64 size = 0;
		std::memcpy(&size, it->second.second.size, sizeof(size));

		if (m_file)
		{
			m_out = std::make_unique<utils::serial>();
			m_out->set_reading_state();
			m_out->m_file_handler = make_uncompressed_serialization_file_handler(make_file_view(*m_file, it->second.first, size));
		}
		else
		{
			m_out = std::make_unique<utils::serial>();
			*m_out = std::move(*m_ar);
			m_out->m_max_data = m_ar_tar_start + it->second.first + size;
		}

		return m_out;
	}
	else if (m_ar && path.empty())
	{
		const u64 size = emplace_single_entry(largest_offset, m_ar->get_size(umax) - m_ar_tar_start).first;

		// Advance offset to next block
		largest_offset += utils::align(size, 512);
	}
	// Continue scanning from last file entered
	else if (m_file)
	{
		const u64 max_size = m_file->size();

		while (largest_offset < max_size)
		{
			const auto [size, filename] = emplace_single_entry(largest_offset, max_size);

			if (size == umax)
			{
				continue;
			}

			// Advance offset to next block
			largest_offset += utils::align(size, 512);

			if (!path.empty() && path == filename)
			{
				// Path is equal, return handle to the file data
				return get_file(path);
			}
		}
	}

	return m_out;
}

bool tar_object::extract(const std::string& prefix_path, bool is_vfs)
{
	std::vector<u8> filedata_buffer(0x80'0000);
	std::span<u8> filedata_span{filedata_buffer.data(), filedata_buffer.size()};

	auto iter = m_map.begin();

	auto get_next = [&](bool is_first)
	{
		if (m_ar)
		{
			ensure(!is_first || m_map.empty()); // Must be empty on first call
			std::string name_iter;
			get_file("", &name_iter); // Get next entry
			return m_map.find(name_iter);
		}
		else if (is_first)
		{
			get_file(""); // Scan entries
			return m_map.begin();
		}
		else
		{
			return std::next(iter);
		}
	};

	for (iter = get_next(true); iter != m_map.end(); iter = get_next(false))
	{
		const TARHeader& header = iter->second.second;
		const std::string& name = iter->first;

		// Backwards compatibility measure
		const bool should_ignore = name.find(reinterpret_cast<const char*>(u8"＄")) != umax;

		std::string result = name;

		if (!prefix_path.empty())
		{
			result = prefix_path + '/' + result;
		}
		else
		{
			// Must be VFS here
			is_vfs = true;
			result.insert(result.begin(), '/');
		}

		if (is_vfs)
		{
			result = vfs::get(result);

			if (result.empty())
			{
				tar_log.error("Path of entry is not mounted: '%s' (prefix_path='%s')", name, prefix_path);
				return false;
			}
		}

		u64 mtime = octal_text_to_u64({header.mtime, std::size(header.mtime)});

		// Let's use it for optional atime
		u64 atime = octal_text_to_u64({header.padding, 12});

		// This is a fake timestamp, it can be invalid
		if (atime == umax)
		{
			// Set to mtime if not provided
			atime = mtime;
		}

		switch (header.filetype)
		{
		case '\0':
		case '0':
		{
			// Create the directories which should have been mount points if prefix_path is not empty
			if (!prefix_path.empty() && !fs::create_path(fs::get_parent_dir(result)))
			{
				tar_log.error("TAR Loader: failed to create directory for file %s (%s)", name, fs::g_tls_error);
				return false;
			}

			// For restoring m_ar->m_max_data
			usz restore_limit = umax;

			if (!m_file)
			{
				// Restore m_ar (remove limit)
				restore_limit = m_ar->m_max_data;
			}

			std::unique_ptr<utils::serial> file_data = get_file(name);

			fs::file file;

			if (should_ignore)
			{
				file = fs::make_stream<std::vector<u8>>();
			}
			else
			{
				file.open(result, fs::rewrite);
			}

			if (file && file_data)
			{
				while (true)
				{
					const usz unread_size = file_data->try_read(filedata_span);

					if (unread_size == 0)
					{
						file.write(filedata_span.data(), should_ignore ? 0 : filedata_span.size());
						continue;
					}

					// Tail data

					if (usz read_size = filedata_span.size() - unread_size)
					{
						ensure(file_data->try_read(filedata_span.first(read_size)) == 0);
						file.write(filedata_span.data(), should_ignore ? 0 : read_size);
					}

					break;
				}

				file.close();


				file_data->seek_pos(m_ar_tar_start + largest_offset, true);

				if (!m_file)
				{
					// Restore m_ar
					*m_ar = std::move(*file_data);
					m_ar->m_max_data = restore_limit;
				}

				if (should_ignore)
				{
					break;
				}

				if (mtime != umax && !fs::utime(result, atime, mtime))
				{
					tar_log.error("TAR Loader: fs::utime failed on %s (%s)", result, fs::g_tls_error);
					return false;
				}

				tar_log.notice("TAR Loader: written file %s", name);
				break;
			}

			if (!m_file)
			{
				// Restore m_ar
				*m_ar = std::move(*file_data);
				m_ar->m_max_data = restore_limit;
			}

			const auto old_error = fs::g_tls_error;
			tar_log.error("TAR Loader: failed to write file %s (%s) (fs::exists=%s)", name, old_error, fs::exists(result));
			return false;
		}

		case '5':
		{
			if (should_ignore)
			{
				break;
			}

			if (!fs::create_path(result))
			{
				tar_log.error("TAR Loader: failed to create directory %s (%s)", name, fs::g_tls_error);
				return false;
			}

			if (mtime != umax && !fs::utime(result, atime, mtime))
			{
				tar_log.error("TAR Loader: fs::utime failed on %s (%s)", result, fs::g_tls_error);
				return false;
			}

			break;
		}

		default:
			tar_log.error("TAR Loader: unknown file type: 0x%x", header.filetype);
			return false;
		}
	}
	return true;
}

void tar_object::save_directory(const std::string& target_path, utils::serial& ar, const process_func& func, std::vector<fs::dir_entry>&& entries, bool has_evaluated_results, usz src_dir_pos)
{
	const bool is_null = ar.m_file_handler && ar.m_file_handler->is_null();
	const bool reuse_entries = !is_null || has_evaluated_results;

	if (reuse_entries)
	{
		ensure(!entries.empty());
	}

	auto write_octal = [](char* ptr, u64 i)
	{
		if (!i)
		{
			*ptr = '0';
			return;
		}

		ptr += utils::aligned_div(static_cast<u32>(std::bit_width(i)), 3) - 1;

		for (; i; ptr--, i /= 8)
		{
			*ptr = static_cast<char>('0' + (i % 8));
		}
	};

	auto save_file = [&](const fs::stat_t& file_stat, const std::string& file_name)
	{
		if (!file_stat.size)
		{
			return;
		}

		if (is_null && !func)
		{
			ar.pos += utils::align(file_stat.size, 512);
			return;
		}

		if (fs::file fd{file_name})
		{
			const u64 old_pos = ar.pos;
			const usz old_size = ar.data.size();

			if (func)
			{
				std::string saved_path{&::at32(file_name, src_dir_pos), file_name.size() - src_dir_pos};

				// Use custom function for file saving if provided
				// Allows for example to compress PNG files as JPEG in the TAR itself
				if (!func(fd, saved_path, ar))
				{
					// Revert (this entry should not be included if func returns false)

					if (is_null)
					{
						ar.pos = old_pos;
						return;
					}

					ar.data.resize(old_size);
					ar.seek_end();
					return;
				}

				if (is_null)
				{
					// Align
					ar.pos += utils::align(ar.pos - old_pos, 512);
					return;
				}
			}
			else
			{
				constexpr usz transfer_block_size = 0x100'0000;

				for (usz read_index = 0; read_index < file_stat.size; read_index += transfer_block_size)
				{
					const usz read_size = std::min<usz>(transfer_block_size, file_stat.size - read_index);

					// Read file data
					const usz buffer_tail = ar.data.size();
					ar.data.resize(buffer_tail + read_size);
					ensure(fd.read_at(read_index, ar.data.data() + buffer_tail, read_size) == read_size);

					// Set position to the end of data, so breathe() would work correctly
					ar.seek_end();

					// Allow flushing to file if needed
					ar.breathe();
				}
			}

			// Align
			const usz diff = ar.pos - old_pos;
			ar.data.resize(ar.data.size() + utils::align(diff, 512) - diff);
			ar.seek_end();

			fd.close();
			ensure(fs::utime(file_name, file_stat.atime, file_stat.mtime));
		}
		else
		{
			ensure(false);
		}
	};

	auto save_header = [&](const fs::stat_t& stat, const std::string& name)
	{
		static_assert(sizeof(TARHeader) == 512);

		std::string_view saved_path{name.size() == src_dir_pos ? name.c_str() : &::at32(name, src_dir_pos), name.size() - src_dir_pos};

		if (is_null)
		{
			ar.pos += sizeof(TARHeader);
			return;
		}

		if (usz pos = saved_path.find_first_not_of(fs::delim); pos != umax)
		{
			saved_path = saved_path.substr(pos, saved_path.size());
		}
		else
		{
			// Target the destination directory, I do not know if this is compliant with TAR format
			saved_path = "/"sv;
		}

		TARHeader header{};
		std::memcpy(header.magic, "ustar ", 6);

		// Prefer saving to name field as much as we can
		// If it doesn't fit, save 100 characters at name and 155 characters preceding to it at max
		const u64 prefix_size = std::clamp<usz>(saved_path.size(), 100, 255) - 100;
		std::memcpy(header.prefix, saved_path.data(), prefix_size);
		const u64 name_size = std::min<usz>(saved_path.size(), 255) - prefix_size;
		std::memcpy(header.name, saved_path.data() + prefix_size, name_size);

		write_octal(header.size, stat.is_directory ? 0 : stat.size);
		write_octal(header.mtime, stat.mtime);
		write_octal(header.padding, stat.atime);
		header.filetype = stat.is_directory ? '5' : '0';

		ar(header);
		ar.breathe();
	};

	fs::stat_t stat{};

	if (src_dir_pos == umax)
	{
		// First call, get source directory string size so it can be cut from entry paths
		src_dir_pos = target_path.size();
	}

	if (has_evaluated_results)
	{
		// Save from cached data by previous call
		for (auto&& entry : entries)
		{
			ensure(entry.name.starts_with(target_path));
			save_header(entry, entry.name);

			if (!entry.is_directory)
			{
				save_file(entry, entry.name);
			}
		}
	}
	else
	{
		if (entries.empty())
		{
			if (!fs::get_stat(target_path, stat))
			{
				return;
			}

			save_header(stat, target_path);

			// Optimization: avoid saving to list if this is not an evaluation call
			if (is_null)
			{
				static_cast<fs::stat_t&>(entries.emplace_back()) = stat;
				entries.back().name = target_path;
			}
		}
		else
		{
			stat = entries.back();
			save_header(stat, entries.back().name);
		}

		if (stat.is_directory)
		{
			bool exists = false;

			for (auto&& entry : fs::dir(target_path))
			{
				exists = true;

				if (entry.name.find_first_not_of('.') == umax || entry.name.starts_with(reinterpret_cast<const char*>(u8"＄")))
				{
					continue;
				}

				entry.name = target_path.ends_with('/') ? target_path + entry.name : target_path + '/' + entry.name;

				if (!entry.is_directory)
				{
					save_header(entry, entry.name);
					save_file(entry, entry.name);

					// TAR is an old format which does not depend on previous data so memory ventilation is trivial here
					ar.breathe();

					entries.emplace_back(std::move(entry));
				}
				else
				{
					if (!is_null)
					{
						// Optimization: avoid saving to list if this is not an evaluation call
						entries.clear();
					}

					entries.emplace_back(std::move(entry));
					save_directory(::as_rvalue(entries.back().name), ar, func, std::move(entries), false, src_dir_pos);
				}
			}

			ensure(exists);
		}
		else
		{
			fs::dir_entry entry{};
			entry.name = target_path;
			static_cast<fs::stat_t&>(entry) = stat;

			save_file(entry, entry.name);
		}

		ar.breathe();
	}
}

bool extract_tar(const std::string& file_path, const std::string& dir_path, fs::file file)
{
	tar_log.notice("Extracting '%s' to directory '%s'...", file_path, dir_path);

	if (!file)
	{
		file.open(file_path);
	}

	if (!file)
	{
		tar_log.error("Error opening file '%s' (%s)", file_path, fs::g_tls_error);
		return false;
	}

	std::vector<fs::file> vec;

	if (SCEDecrypter self_dec(file); self_dec.LoadHeaders())
	{
		// Encrypted file, decrypt
		self_dec.LoadMetadata(SCEPKG_ERK, SCEPKG_RIV);

		if (!self_dec.DecryptData())
		{
			tar_log.error("Failed to decrypt TAR.");
			return false;
		}

		vec = self_dec.MakeFile();
		if (vec.size() < 3)
		{
			tar_log.error("Failed to decrypt TAR.");
			return false;
		}
	}
	else
	{
		// Not an encrypted file
		tar_log.warning("TAR is not encrypted, it may not be valid for this tool. Encrypted TAR are known to be found in PS3 Firmware files only.");
	}

	if (!vfs::mount("/tar_extract", dir_path))
	{
		tar_log.error("Failed to mount '%s'", dir_path);
		return false;
	}

	tar_object tar(vec.empty() ? file : vec[2]);

	const bool ok = tar.extract("/tar_extract", true);

	if (ok)
	{
		tar_log.success("Extraction complete!");
	}
	else
	{
		tar_log.error("TAR contents are invalid.");
	}

	// Unmount
	Emu.Init();
	return ok;
}
