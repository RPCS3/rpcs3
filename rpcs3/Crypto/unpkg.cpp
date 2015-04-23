#include "stdafx.h"
#include "Utilities/Log.h"
#include "Utilities/rFile.h"
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
bool CheckHeader(const rfile_t& pkg_f, PKGHeader* m_header)
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

	if (m_header->header_size != PKG_HEADER_SIZE)
	{
		LOG_ERROR(LOADER, "PKG: Wrong header size (0x%x)", m_header->header_size);
		return false;
	}

	if (m_header->pkg_size != pkg_f.size())
	{
		LOG_ERROR(LOADER, "PKG: File size mismatch (pkg_size=0x%x)", m_header->pkg_size);
		//return false;
	}

	if (m_header->data_size + m_header->data_offset + 0x60 != pkg_f.size())
	{
		LOG_ERROR(LOADER, "PKG: Data size mismatch (data_size=0x%x, offset=0x%x)", m_header->data_size, m_header->data_offset);
		//return false;
	}

	return true;
}

bool LoadHeader(const rfile_t& pkg_f, PKGHeader* m_header)
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

int Decrypt(const rfile_t& pkg_f, const rfile_t& dec_pkg_f, PKGHeader* m_header)
{
	if (!LoadHeader(pkg_f, m_header))
	{
		return -1;
	}

	aes_context c;
	u8 iv[HASH_LEN];
	u8 buf[BUF_SIZE];
	u8 ctr[BUF_SIZE];
	
	// Debug key
	u8 key[0x40];
	memset(key, 0, 0x40);
	memcpy(key+0x00, &m_header->qa_digest[0], 8); // &data[0x60]
	memcpy(key+0x08, &m_header->qa_digest[0], 8); // &data[0x60]
	memcpy(key+0x10, &m_header->qa_digest[8], 8); // &data[0x68]
	memcpy(key+0x18, &m_header->qa_digest[8], 8); // &data[0x68]

	pkg_f.seek(m_header->data_offset);
	u32 parts = (m_header->data_size + BUF_SIZE - 1) / BUF_SIZE;

	wxProgressDialog pdlg("PKG Decrypter / Installer", "Please wait, decrypting...", parts, 0, wxPD_AUTO_HIDE | wxPD_APP_MODAL);

	memcpy(iv, m_header->klicensee, sizeof(iv));
	aes_setkey_enc(&c, PKG_AES_KEY, 128);
	
	for (u32 i=0; i<parts; i++)
	{
		memset(buf, 0, sizeof(buf));
		u32 length = pkg_f.read(buf, BUF_SIZE);
		u32 bits = (length + HASH_LEN - 1) / HASH_LEN;
		
		if (m_header->pkg_type == PKG_RELEASE_TYPE_DEBUG)
		{
			for (u32 j=0; j<bits; j++)
			{
				u8 hash[0x14];
				sha1(key, 0x40, hash);
				*(u64*)&buf[j*HASH_LEN + 0] ^= *(u64*)&hash[0];
				*(u64*)&buf[j*HASH_LEN + 8] ^= *(u64*)&hash[8];
				*(be_t<u64>*)&key[0x38] += 1;
			}
		}

		if (m_header->pkg_type == PKG_RELEASE_TYPE_RELEASE)
		{
			for (u32 j=0; j<bits; j++)
			{
				aes_crypt_ecb(&c, AES_ENCRYPT, iv, ctr+j*HASH_LEN);

				be_t<u64> hi = *(be_t<u64>*)&iv[0];
				be_t<u64> lo = *(be_t<u64>*)&iv[8];
				lo++;

				if (lo == 0)
					hi += 1;

				*(be_t<u64>*)&iv[0] = hi;
				*(be_t<u64>*)&iv[8] = lo;
			}
		
			for (u32 j=0; j<length; j++) {
				buf[j] ^= ctr[j];
			}
		}		
		dec_pkg_f.write(buf, length);
		pdlg.Update(i);
	}
	pdlg.Update(parts);
	return 0;
}

// Unpacking.
bool LoadEntries(const rfile_t& dec_pkg_f, PKGHeader* m_header, PKGEntry* m_entries)
{
	dec_pkg_f.seek(0);
	dec_pkg_f.read(m_entries, sizeof(PKGEntry) * m_header->file_count);
	
	if (m_entries->name_offset / sizeof(PKGEntry) != m_header->file_count)
	{
		LOG_ERROR(LOADER, "PKG: Entries are damaged!");
		return false;
	}

	return true;
}

bool UnpackEntry(const rfile_t& dec_pkg_f, const PKGEntry& entry, std::string dir)
{
	char buf[BUF_SIZE];

	dec_pkg_f.seek(entry.name_offset);
	dec_pkg_f.read(buf, entry.name_size);
	buf[entry.name_size] = 0;
	
	switch (entry.type.data() >> 24)
	{
	case PKG_FILE_ENTRY_NPDRM:
	case PKG_FILE_ENTRY_NPDRMEDAT:
	case PKG_FILE_ENTRY_SDAT:
	case PKG_FILE_ENTRY_REGULAR:
	{
		auto path = dir + std::string(buf, entry.name_size);

		if (rIsFile(path))
		{
			LOG_WARNING(LOADER, "PKG Loader: '%s' is overwritten", path);
		}

		rfile_t out;

		if (out.open(path, o_write | o_create | o_trunc))
		{
			dec_pkg_f.seek(entry.file_offset);

			for (u64 size = 0; size < entry.file_size;)
			{
				size += dec_pkg_f.read(buf, BUF_SIZE);
				if (size > entry.file_size)
					out.write(buf, BUF_SIZE - (size - entry.file_size));
				else
					out.write(buf, BUF_SIZE);
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
		auto path = dir + std::string(buf, entry.name_size);
		if (!rIsDir(path) && !rMkPath(path))
		{
			LOG_ERROR(LOADER, "PKG Loader: Could not create directory: %s", path.c_str());
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

int Unpack(const rfile_t& pkg_f, std::string src, std::string dst)
{
	PKGHeader* m_header = (PKGHeader*) malloc (sizeof(PKGHeader));

	// TODO: This shouldn't use current dir
	std::string decryptedFile = "./dev_hdd1/" + src + ".dec";

	rfile_t dec_pkg_f(decryptedFile, o_read | o_write | o_create | o_trunc);
	
	if (Decrypt(pkg_f, dec_pkg_f, m_header) < 0)
	{
		return -1;
	}

	dec_pkg_f.seek(0);

	std::vector<PKGEntry> m_entries;
	m_entries.resize(m_header->file_count);

	auto m_entries_ptr = m_entries.data();

	if (!LoadEntries(dec_pkg_f, m_header, m_entries_ptr))
	{
		return -1;
	}

	wxProgressDialog pdlg("PKG Decrypter / Installer", "Please wait, unpacking...", m_entries.size(), 0, wxPD_AUTO_HIDE | wxPD_APP_MODAL);

	for (const auto& entry : m_entries)
	{
		UnpackEntry(dec_pkg_f, entry, dst + src + "/");
		pdlg.Update(pdlg.GetValue() + 1);
	}

	pdlg.Update(m_entries.size());

	dec_pkg_f.close();
	rRemoveFile(decryptedFile);

	return 0;
}
