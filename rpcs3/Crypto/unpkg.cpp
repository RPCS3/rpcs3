#include "stdafx.h"
#include "Utilities/Log.h"
#include "Utilities/File.h"
#include "utils.h"
#include "aes.h"
#include "sha1.h"
#include "key_vault.h"
#include "unpkg.h"

static bool CheckHeader(const fs::file& pkg_f, PKGHeader& header)
{
	if (header.pkg_magic != 0x7F504B47)
	{
		LOG_ERROR(LOADER, "PKG: Not a package file!");
		return false;
	}

	switch (const u16 type = header.pkg_type)
	{
	case PKG_RELEASE_TYPE_DEBUG:   break;
	case PKG_RELEASE_TYPE_RELEASE: break;
	default:
	{
		LOG_ERROR(LOADER, "PKG: Unknown PKG type (0x%x)", type);
		return false;
	}
	}

	switch (const u16 platform = header.pkg_platform)
	{
	case PKG_PLATFORM_TYPE_PS3: break;
	case PKG_PLATFORM_TYPE_PSP: break;
	default:
	{
		LOG_ERROR(LOADER, "PKG: Unknown PKG platform (0x%x)", platform);
		return false;
	}
	}

	if (header.header_size != PKG_HEADER_SIZE && header.header_size != PKG_HEADER_SIZE2)
	{
		LOG_ERROR(LOADER, "PKG: Wrong header size (0x%x)", header.header_size);
		return false;
	}

	if (header.pkg_size > pkg_f.size())
	{
		LOG_ERROR(LOADER, "PKG: File size mismatch (pkg_size=0x%llx)", header.pkg_size);
		return false;
	}

	if (header.data_size + header.data_offset > header.pkg_size)
	{
		LOG_ERROR(LOADER, "PKG: Data size mismatch (data_size=0x%llx, data_offset=0x%llx, file_size=0x%llx)", header.data_size, header.data_offset, header.pkg_size);
		return false;
	}

	return true;
}

// PKG Decryption
bool UnpackPKG(const fs::file& pkg_f, const std::string& dir, volatile f64& progress)
{
	const std::size_t BUF_SIZE = 8192 * 1024; // 8 MB

	// Save current file offset (probably zero)
	const u64 start_offset = pkg_f.seek(0, fsm::cur);

	// Get basic PKG information
	PKGHeader header;

	if (pkg_f.read(&header, sizeof(PKGHeader)) != sizeof(PKGHeader))
	{
		LOG_ERROR(LOADER, "PKG: Package file is too short!");
		return false;
	}

	if (!CheckHeader(pkg_f, header))
	{
		return false;
	}

	// Allocate buffer with BUF_SIZE size or more if required
	const std::unique_ptr<u128[]> buf(new u128[std::max<u64>(BUF_SIZE, sizeof(PKGEntry) * header.file_count) / sizeof(u128)]);

	// Define decryption subfunction (`psp` arg selects the key for specific block)
	auto decrypt = [&](u64 offset, u64 size, bool psp) -> u64
	{
		pkg_f.seek(start_offset + header.data_offset + offset);

		// Read the data and set available size
		const u64 read = pkg_f.read(buf.get(), size);

		// Get block count
		const u64 blocks = (read + 15) / 16;

		if (header.pkg_type == PKG_RELEASE_TYPE_DEBUG)
		{
			// Debug key
			be_t<u64> input[8] =
			{
				header.qa_digest[0],
				header.qa_digest[0],
				header.qa_digest[1],
				header.qa_digest[1],
			};

			for (u64 i = 0; i < blocks; i++)
			{
				// Initialize "debug key" for current position
				input[7] = offset / 16 + i;

				u128 key;
				
				sha1(reinterpret_cast<const u8*>(input), sizeof(input), reinterpret_cast<u8*>(&key));

				buf[i] ^= key;
			}
		}

		if (header.pkg_type == PKG_RELEASE_TYPE_RELEASE)
		{
			aes_context ctx;

			// Set decryption key
			aes_setkey_enc(&ctx, psp ? PKG_AES_KEY2 : PKG_AES_KEY, 128);

			// Initialize "release key" for start position
			be_t<u128> input = header.klicensee.value() + offset / 16;

			// Increment "release key" for every block
			for (u64 i = 0; i < blocks; i++, input++)
			{
				u128 key;

				aes_crypt_ecb(&ctx, AES_ENCRYPT, reinterpret_cast<const u8*>(&input), reinterpret_cast<u8*>(&key));

				buf[i] ^= key;
			}
		}

		// Return the amount of data written in buf
		return read;
	};

	LOG_SUCCESS(LOADER, "PKG: Installing in %s (%d entries)...", dir, header.file_count);

	decrypt(0, header.file_count * sizeof(PKGEntry), header.pkg_platform == PKG_PLATFORM_TYPE_PSP);

	std::vector<PKGEntry> entries(header.file_count);

	std::memcpy(entries.data(), buf.get(), entries.size() * sizeof(PKGEntry));

	for (const auto& entry : entries)
	{
		const bool is_psp = (entry.type & PKG_FILE_ENTRY_PSP) != 0;

		if (entry.name_size > 256)
		{
			LOG_ERROR(LOADER, "PKG: Name size is too big (0x%x)", entry.name_size);
			continue;
		}

		decrypt(entry.name_offset, entry.name_size, is_psp);

		const std::string name(reinterpret_cast<char*>(buf.get()), entry.name_size);

		switch (entry.type & 0xff)
		{
		case PKG_FILE_ENTRY_NPDRM:
		case PKG_FILE_ENTRY_NPDRMEDAT:
		case PKG_FILE_ENTRY_SDAT:
		case PKG_FILE_ENTRY_REGULAR:
		case PKG_FILE_ENTRY_UNK1:
		{
			const std::string path = dir + name;

			const bool did_overwrite = fs::is_file(path);

			if (fs::file out{ path, fom::write | fom::create | fom::trunc })
			{
				for (u64 pos = 0; pos < entry.file_size; pos += BUF_SIZE)
				{
					const u64 block_size = std::min<u64>(BUF_SIZE, entry.file_size - pos);

					if (decrypt(entry.file_offset + pos, block_size, is_psp) != block_size)
					{
						LOG_ERROR(LOADER, "PKG: Failed to extract file %s", path);
						break;
					}

					if (out.write(buf.get(), block_size) != block_size)
					{
						LOG_ERROR(LOADER, "PKG: Failed to write file %s", path);
						break;
					}

					progress += (block_size + 0.0) / header.data_size;
				}

				if (did_overwrite)
				{
					LOG_SUCCESS(LOADER, "PKG: %s file overwritten", name);
				}
				else
				{
					LOG_SUCCESS(LOADER, "PKG: %s file created", name);
				}
			}
			else
			{
				LOG_ERROR(LOADER, "PKG: Could not create file %s", path);
			}

			break;
		}

		case PKG_FILE_ENTRY_FOLDER:
		{
			const std::string path = dir + name;

			if (fs::create_dir(path))
			{
				LOG_SUCCESS(LOADER, "PKG: %s directory created", name);
			}
			else if (fs::is_dir(path))
			{
				LOG_SUCCESS(LOADER, "PKG: %s directory already exists", name);
			}
			else
			{
				LOG_ERROR(LOADER, "PKG: Could not create directory %s", path);
			}

			break;
		}

		default:
		{
			LOG_ERROR(LOADER, "PKG: Unknown PKG entry type (0x%x) %s", entry.type, name);
		}
		}
	}

	return true;
}
