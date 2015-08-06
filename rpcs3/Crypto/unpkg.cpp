#include "stdafx.h"
#include "Utilities/Log.h"
#include "Utilities/File.h"
#include "utils.h"
#include "aes.h"
#include "sha1.h"
#include "key_vault.h"
#include "unpkg.h"
#include "restore_new.h"
#pragma warning(push)
#pragma message("TODO: remove wx dependency: <wx/progdlg.h>")
#pragma warning(disable : 4996)
#include <wx/progdlg.h>
#pragma warning(pop)
#include "define_new_memleakdetect.h"

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
bool Unpack(const fs::file& pkg_f, std::string dir)
{
	// Save current file offset (probably zero)
	const u64 start_offset = pkg_f.seek(0, from_cur);

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

	aes_context c;

	u8 iv[HASH_LEN];
	be_t<u64>& hi = (be_t<u64>&)iv[0];
	be_t<u64>& lo = (be_t<u64>&)iv[8];

	// Allocate buffers with BUF_SIZE size or more if required
	const u64 buffer_size = std::max<u64>(BUF_SIZE, sizeof(PKGEntry) * header.file_count);

	const std::unique_ptr<u8[]> buf(new u8[buffer_size]), ctr(new u8[buffer_size]);

	// Debug key
	u8 key[0x40] = {};
	memcpy(key + 0x00, &header.qa_digest[0], 8); // &data[0x60]
	memcpy(key + 0x08, &header.qa_digest[0], 8); // &data[0x60]
	memcpy(key + 0x10, &header.qa_digest[8], 8); // &data[0x68]
	memcpy(key + 0x18, &header.qa_digest[8], 8); // &data[0x68]

	// Define decryption subfunction (`psp` arg selects the key for specific block)
	auto decrypt = [&](u64 offset, u64 size, bool psp)
	{
		// Initialize buffer
		std::memset(buf.get(), 0, size);

		// Read the data
		pkg_f.seek(start_offset + header.data_offset + offset);
		size = pkg_f.read(buf.get(), size);
		const u64 bits = (size + HASH_LEN - 1) / HASH_LEN;

		if (header.pkg_type == PKG_RELEASE_TYPE_DEBUG)
		{
			for (u64 j = 0; j < bits; j++)
			{
				u8 hash[0x14];
				sha1(key, 0x40, hash);
				*(u64*)&buf[j * HASH_LEN + 0] ^= *(u64*)&hash[0];
				*(u64*)&buf[j * HASH_LEN + 8] ^= *(u64*)&hash[8];
				*(be_t<u64>*)&key[0x38] += 1;
			}
		}

		if (header.pkg_type == PKG_RELEASE_TYPE_RELEASE)
		{
			// Set decryption key
			aes_setkey_enc(&c, psp ? PKG_AES_KEY2 : PKG_AES_KEY, 128);

			// Initialize `iv` for the specific position
			memcpy(iv, header.klicensee, sizeof(iv));
			if (lo + offset / HASH_LEN < lo) hi++;
			lo += offset / HASH_LEN;

			for (u64 j = 0; j < bits; j++)
			{
				aes_crypt_ecb(&c, AES_ENCRYPT, iv, ctr.get() + j * HASH_LEN);

				if (!++lo)
				{
					hi++;
				}
			}

			for (u64 j = 0; j < size; j++)
			{
				buf[j] ^= ctr[j];
			}
		}
	};

	wxProgressDialog pdlg("PKG Decrypter / Installer", "Please wait, decrypting...", header.file_count, 0, wxPD_AUTO_HIDE | wxPD_APP_MODAL);

	decrypt(0, header.file_count * sizeof(PKGEntry), header.pkg_platform == PKG_PLATFORM_TYPE_PSP);

	std::vector<PKGEntry> entries(header.file_count);

	std::memcpy(entries.data(), buf.get(), entries.size() * sizeof(PKGEntry));

	for (s32 i = 0; i < entries.size(); i++)
	{
		const PKGEntry& entry = entries[i];

		const bool is_psp = (entry.type & PKG_FILE_ENTRY_PSP) != 0;

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

			if (fs::is_file(path))
			{
				LOG_WARNING(LOADER, "PKG Loader: '%s' is overwritten", path);
			}

			if (fs::file out{ path, o_write | o_create | o_trunc })
			{
				for (u64 pos = 0; pos < entry.file_size; pos += BUF_SIZE)
				{
					const u64 block_size = std::min<u64>(BUF_SIZE, entry.file_size - pos);

					decrypt(entry.file_offset + pos, block_size, is_psp);

					out.write(buf.get(), block_size);
				}
			}
			else
			{
				LOG_ERROR(LOADER, "PKG Loader: Could not create file '%s'", path);
				return false;
			}

			break;
		}

		case PKG_FILE_ENTRY_FOLDER:
		{
			const std::string path = dir + name;

			if (!fs::is_dir(path) && !fs::create_dir(path))
			{
				LOG_ERROR(LOADER, "PKG Loader: Could not create directory: %s", path);
				return false;
			}

			break;
		}

		default:
		{
			LOG_ERROR(LOADER, "PKG Loader: unknown PKG file entry: 0x%x", entry.type);
			return false;
		}
		}

		pdlg.Update(i + 1);
	}

	return true;
}
