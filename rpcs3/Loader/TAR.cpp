#include "stdafx.h"

#include "Emu/VFS.h"
#include "Emu/System.h"

#include "Crypto/unself.h"

#include "TAR.h"

#include "util/asm.hpp"

#include <charconv>

LOG_CHANNEL(tar_log, "TAR");

tar_object::tar_object(const fs::file& file)
	: m_file(file)
{
}

TARHeader tar_object::read_header(u64 offset) const
{
	TARHeader header{};

	if (m_file.seek(offset) != offset)
	{
		return header;
	}

	if (!m_file.read(header))
	{
		std::memset(&header, 0, sizeof(header));
	}

	return header;
}

u64 octal_text_to_u64(std::string_view sv)
{
	u64 i = -1;
	const auto ptr = std::from_chars(sv.data(), sv.data() + sv.size(), i, 8).ptr;

	// Range must be terminated with either NUL or space
	if (ptr == sv.data() + sv.size() || (*ptr && *ptr != ' '))
	{
		i = -1;
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

fs::file tar_object::get_file(const std::string& path)
{
	if (!m_file) return fs::file();

	if (auto it = m_map.find(path); it != m_map.end())
	{
		u64 size = 0;
		std::memcpy(&size, it->second.second.size, sizeof(size));
		std::vector<u8> buf(size);
		m_file.seek(it->second.first);
		m_file.read(buf, size);
		return fs::make_stream(std::move(buf));
	}
	else //continue scanning from last file entered
	{
		const u64 max_size = m_file.size();

		while (largest_offset < max_size)
		{
			TARHeader header = read_header(largest_offset);

			u64 size = -1;

			std::string filename;

			if (std::memcmp(header.magic, "ustar", 5) == 0)
			{
				const std::string_view size_sv{header.size, std::size(header.size)};

				size = octal_text_to_u64(size_sv);

				// Check for overflows and if surpasses file size
				if ((header.name[0] || header.prefix[0]) && size + 512 > size && max_size >= size + 512 && max_size - size - 512 >= largest_offset)
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
					m_map.insert_or_assign(filename, std::make_pair(largest_offset + 512, header));
				}
				else
				{
					// Invalid
					size = -1;
					tar_log.error("tar_object::get_file() failed to convert header.size=%s, filesize=0x%x", size_sv, max_size);
				}
			}
			else
			{
				tar_log.trace("tar_object::get_file() failed to parse header: offset=0x%x, filesize=0x%x", largest_offset, max_size);
			}

			if (size == umax)
			{
				largest_offset += 512;
				continue;
			}

			// Advance offset to next block
			largest_offset += utils::align(size, 512) + 512;

			if (!path.empty() && path == filename)
			{
				// Path is equal, read file and advance offset to start of next block
				std::vector<u8> buf(size);

				if (m_file.read(buf, size))
				{
					return fs::make_stream(std::move(buf));
				}

				tar_log.error("tar_object::get_file() failed to read file entry %s (size=0x%x)", filename, size);
				largest_offset -= utils::align(size, 512);
			}
		}

		return fs::file();
	}
}

bool tar_object::extract(std::string vfs_mp)
{
	if (!m_file) return false;

	get_file(""); // Make sure we have scanned all files

	for (auto& iter : m_map)
	{
		const TARHeader& header = iter.second.second;
		const std::string& name = iter.first;

		std::string result = name;

		if (!vfs_mp.empty())
		{
			result = fmt::format("/%s/%s", vfs_mp, result);
		}
		else
		{
			result.insert(result.begin(), '/');
		}

		result = vfs::get(result);

		if (result.empty())
		{
			tar_log.error("Path of entry is not mounted: '%s' (vfs_mp='%s')", name, vfs_mp);
			return false;
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
			// Create the directories which should have been mount points if vfs_mp is not empty
			if (!vfs_mp.empty() && !fs::create_path(fs::get_parent_dir(result)))
			{
				tar_log.error("TAR Loader: failed to create directory for file %s (%s)", name, fs::g_tls_error);
				return false;
			}

			auto data = get_file(name).release();

			fs::file file(result, fs::rewrite);

			if (file)
			{
				file.write(static_cast<fs::container_stream<std::vector<u8>>*>(data.get())->obj);
				file.close();

				if (mtime != umax && !fs::utime(result, atime, mtime))
				{
					tar_log.error("TAR Loader: fs::utime failed on %s (%s)", result, fs::g_tls_error);
					return false;
				}

				tar_log.notice("TAR Loader: written file %s", name);
				break;
			}

			const auto old_error = fs::g_tls_error;
			tar_log.error("TAR Loader: failed to write file %s (%s) (fs::exists=%s)", name, old_error, fs::exists(result));
			return false;
		}

		case '5':
		{
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

std::vector<u8> tar_object::save_directory(const std::string& src_dir, std::vector<u8>&& init, const process_func& func, std::string full_path)
{
	const std::string& target_path = full_path.empty() ? src_dir : full_path;

	fs::stat_t stat{};
	if (!fs::stat(target_path, stat))
	{
		return std::move(init);
	}

	u32 count = 0;

	if (stat.is_directory)
	{
		bool has_items = false;

		for (auto& entry : fs::dir(target_path))
		{
			if (entry.name.find_first_not_of('.') == umax) continue;

			init = save_directory(src_dir, std::move(init), func, target_path + '/' + entry.name);
			has_items = true;
		}

		if (has_items)
		{
			return std::move(init);
		}
	}

	auto write_octal = [](char* ptr, u64 i)
	{
		if (!i)
		{
			*ptr = '0';
			return;
		}

		ptr += utils::aligned_div(std::bit_width(i), 3) - 1;

		for (; i; ptr--, i /= 8)
		{
			*ptr = static_cast<char>('0' + (i % 8));
		}
	};

	std::string saved_path{target_path.data() + src_dir.size(), target_path.size() - src_dir.size()};

	const u64 old_size = init.size();
	init.resize(old_size + sizeof(TARHeader));

	if (!stat.is_directory)
	{
		fs::file fd(target_path);

		if (func)
		{
			// Use custom function for file saving if provided
			// Allows for example to compress PNG files as JPEG in the TAR itself 
			if (!func(fd, saved_path, std::move(init)))
			{
				// Revert (this entry should not be included if func returns false)
				init.resize(old_size);
				return std::move(init);
			}
		}
		else
		{
			const u64 old_size2 = init.size();
			init.resize(init.size() + stat.size);
			ensure(fd.read(init.data() + old_size2, stat.size) == stat.size);
		}

		// Align
		init.resize(utils::align(init.size(), 512));

		fd.close();
		fs::utime(target_path, stat.atime, stat.mtime);
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

	std::memcpy(init.data() + old_size, &header, sizeof(header));
	return std::move(init);
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

	const bool ok = tar.extract("/tar_extract");

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
