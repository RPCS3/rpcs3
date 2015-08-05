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

// Decryption.
bool CheckHeader(const fs::file& pkg_f, PKGHeader* m_header)
{
	if (m_header->pkg_magic != 0x7F504B47)
	{
		LOG_ERROR(LOADER, "PKG: Not a package file!");
		return false;
	}

	switch (const u16 type = m_header->pkg_type)
	{
	case PKG_RELEASE_TYPE_DEBUG:   break;
	case PKG_RELEASE_TYPE_RELEASE: break;
	default:
	{
		LOG_ERROR(LOADER, "PKG: Unknown PKG type (0x%x)", type);
		return false;
	}
	}

	switch (const u16 platform = m_header->pkg_platform)
	{
	case PKG_PLATFORM_TYPE_PS3: break;
	case PKG_PLATFORM_TYPE_PSP: break;
	default:
	{
		LOG_ERROR(LOADER, "PKG: Unknown PKG platform (0x%x)", platform);
		return false;
	}
	}

	if (m_header->header_size != PKG_HEADER_SIZE && m_header->header_size != PKG_HEADER_SIZE2)
	{
		LOG_ERROR(LOADER, "PKG: Wrong header size (0x%x)", m_header->header_size);
		return false;
	}

	if (m_header->pkg_size != pkg_f.size())
	{
		LOG_ERROR(LOADER, "PKG: File size mismatch (pkg_size=0x%llx)", m_header->pkg_size);
		//return false;
	}

	if (m_header->data_size + m_header->data_offset > m_header->pkg_size)
	{
		LOG_ERROR(LOADER, "PKG: Data size mismatch (data_size=0x%llx, data_offset=0x%llx, file_size=0x%llx)", m_header->data_size, m_header->data_offset, m_header->pkg_size);
		//return false;
	}

	return true;
}

bool LoadHeader(const fs::file& pkg_f, PKGHeader* m_header)
{
	pkg_f.seek(0);
	
	if (pkg_f.read(m_header, sizeof(PKGHeader)) != sizeof(PKGHeader))
	{
		LOG_ERROR(LOADER, "PKG: Package file is too short!");
		return false;
	}

	if (!CheckHeader(pkg_f, m_header))
		return false;

	return true;
}

bool Decrypt(const fs::file& pkg_f, const fs::file& dec_pkg_f, PKGHeader* m_header, std::vector<PKGEntry>& entries)
{
	if (!LoadHeader(pkg_f, m_header))
	{
		return false;
	}

	aes_context c;

	u8 iv[HASH_LEN];
	be_t<u64>& hi = (be_t<u64>&)iv[0];
	be_t<u64>& lo = (be_t<u64>&)iv[8];

	auto buf = std::make_unique<u8[]>(BUF_SIZE);
	auto ctr = std::make_unique<u8[]>(BUF_SIZE);

	dec_pkg_f.trunc(pkg_f.size());
	
	// Debug key
	u8 key[0x40] = {};
	memcpy(key+0x00, &m_header->qa_digest[0], 8); // &data[0x60]
	memcpy(key+0x08, &m_header->qa_digest[0], 8); // &data[0x60]
	memcpy(key+0x10, &m_header->qa_digest[8], 8); // &data[0x68]
	memcpy(key+0x18, &m_header->qa_digest[8], 8); // &data[0x68]

	auto decrypt = [&](std::size_t offset, std::size_t size, bool psp)
	{
		if (size > BUF_SIZE)
		{
			buf = std::make_unique<u8[]>(size);
			ctr = std::make_unique<u8[]>(size);
		}

		std::memset(buf.get(), 0, size);

		pkg_f.seek(m_header->data_offset + offset);
		size = pkg_f.read(buf.get(), size);
		const u32 bits = (size + HASH_LEN - 1) / HASH_LEN;

		if (m_header->pkg_type == PKG_RELEASE_TYPE_DEBUG)
		{
			for (u32 j = 0; j < bits; j++)
			{
				u8 hash[0x14];
				sha1(key, 0x40, hash);
				*(u64*)&buf[j * HASH_LEN + 0] ^= *(u64*)&hash[0];
				*(u64*)&buf[j * HASH_LEN + 8] ^= *(u64*)&hash[8];
				*(be_t<u64>*)&key[0x38] += 1;
			}
		}

		if (m_header->pkg_type == PKG_RELEASE_TYPE_RELEASE)
		{
			aes_setkey_enc(&c, psp ? PKG_AES_KEY2 : PKG_AES_KEY, 128);

			memcpy(iv, m_header->klicensee, sizeof(iv));
			if (lo + offset / HASH_LEN < lo) hi++;
			lo += offset / HASH_LEN;

			for (u32 j = 0; j < bits; j++)
			{
				aes_crypt_ecb(&c, AES_ENCRYPT, iv, ctr.get() + j * HASH_LEN);

				if (!++lo)
				{
					hi++;
				}
			}

			for (u32 j = 0; j < size; j++)
			{
				buf[j] ^= ctr[j];
			}
		}

		dec_pkg_f.seek(offset);
		dec_pkg_f.write(buf.get(), size);
	};

	wxProgressDialog pdlg("PKG Decrypter / Installer", "Please wait, decrypting...", m_header->file_count, 0, wxPD_AUTO_HIDE | wxPD_APP_MODAL);

	decrypt(0, m_header->file_count * sizeof(PKGEntry), m_header->pkg_platform == PKG_PLATFORM_TYPE_PSP);

	entries.resize(m_header->file_count);
	dec_pkg_f.seek(0);
	dec_pkg_f.read(entries.data(), m_header->file_count * sizeof(PKGEntry));
	
	for (s32 i = 0; i < entries.size(); i++)
	{
		const bool is_psp = (entries[i].type & PKG_FILE_ENTRY_PSP) != 0;

		decrypt(entries[i].name_offset, entries[i].name_size, is_psp);

		for (u64 j = 0; j < entries[i].file_size; j += BUF_SIZE)
		{
			decrypt(entries[i].file_offset + j, std::min<u64>(entries[i].file_size - j, BUF_SIZE), is_psp);
		}

		pdlg.Update(i + 1);
	}

	return true;
}

// Unpacking.
bool UnpackEntry(const fs::file& dec_pkg_f, const PKGEntry& entry, std::string dir)
{
	auto buf = std::make_unique<char[]>(BUF_SIZE);

	dec_pkg_f.seek(entry.name_offset);
	dec_pkg_f.read(buf.get(), entry.name_size);
	buf[entry.name_size] = 0;
	
	switch (entry.type & 0xff)
	{
	case PKG_FILE_ENTRY_NPDRM:
	case PKG_FILE_ENTRY_NPDRMEDAT:
	case PKG_FILE_ENTRY_SDAT:
	case PKG_FILE_ENTRY_REGULAR:
	case PKG_FILE_ENTRY_UNK1:
	{
		const std::string path = dir + std::string(buf.get(), entry.name_size);

		if (fs::is_file(path))
		{
			LOG_WARNING(LOADER, "PKG Loader: '%s' is overwritten", path);
		}

		fs::file out(path, o_write | o_create | o_trunc);

		if (out)
		{
			dec_pkg_f.seek(entry.file_offset);

			for (u64 i = 0; i < entry.file_size; i += BUF_SIZE)
			{
				const u64 size = std::min<u64>(BUF_SIZE, entry.file_size - i);

				dec_pkg_f.read(buf.get(), size);
				out.write(buf.get(), size);
			}

			return true;
		}
		else
		{
			LOG_ERROR(LOADER, "PKG Loader: Could not create file '%s'", path);
			return false;
		}
	}

	case PKG_FILE_ENTRY_FOLDER:
	{
		const std::string path = dir + std::string(buf.get(), entry.name_size);

		if (!fs::is_dir(path) && !fs::create_dir(path))
		{
			LOG_ERROR(LOADER, "PKG Loader: Could not create directory: %s", path);
			return false;
		}

		return true;
	}

	default:
	{
		LOG_ERROR(LOADER, "PKG Loader: unknown PKG file entry: 0x%x", entry.type);
		return false;
	}
	}
}

bool Unpack(const fs::file& pkg_f, std::string src, std::string dst)
{
	PKGHeader* m_header = (PKGHeader*) malloc (sizeof(PKGHeader));

	// TODO: This shouldn't use current dir
	std::string decryptedFile = "./dev_hdd1/" + src + ".dec";

	fs::file dec_pkg_f(decryptedFile, o_read | o_write | o_create | o_trunc);
	
	std::vector<PKGEntry> m_entries;

	if (!Decrypt(pkg_f, dec_pkg_f, m_header, m_entries))
	{
		return false;
	}

	wxProgressDialog pdlg("PKG Decrypter / Installer", "Please wait, unpacking...", m_entries.size(), 0, wxPD_AUTO_HIDE | wxPD_APP_MODAL);

	for (const auto& entry : m_entries)
	{
		UnpackEntry(dec_pkg_f, entry, dst + src + "/");
		pdlg.Update(pdlg.GetValue() + 1);
	}

	pdlg.Update(m_entries.size());

	dec_pkg_f.close();
	fs::remove_file(decryptedFile);

	return true;
}
