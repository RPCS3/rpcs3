#include "stdafx.h"
#include "utils.h"
#include "aes.h"
#include "sha1.h"
#include "key_vault.h"
#include "unpkg.h"
#include "restore_new.h"
#pragma warning(disable : 4996)
#include <wx/progdlg.h>
#include "define_new_memleakdetect.h"

#include "Utilities/Log.h"
#include "Utilities/rFile.h"

// Decryption.
bool CheckHeader(rFile& pkg_f, PKGHeader* m_header)
{
	if (m_header->pkg_magic != 0x7F504B47) {
		LOG_ERROR(LOADER, "PKG: Not a package file!");
		return false;
	}

	switch ((u32)m_header->pkg_type)
	{
	case PKG_RELEASE_TYPE_DEBUG:   break;
	case PKG_RELEASE_TYPE_RELEASE: break;
	default:
		LOG_ERROR(LOADER, "PKG: Unknown PKG type!");
		return false;
	}

	switch ((u32)m_header->pkg_platform)
	{
	case PKG_PLATFORM_TYPE_PS3: break;
	case PKG_PLATFORM_TYPE_PSP: break;
	default:
		LOG_ERROR(LOADER, "PKG: Unknown PKG type!");
		return false;
	}

	if (m_header->header_size != PKG_HEADER_SIZE) {
		LOG_ERROR(LOADER, "PKG: Wrong header size!");
		return false;
	}

	if (m_header->pkg_size != pkg_f.Length()) {
		LOG_WARNING(LOADER, "PKG: File size mismatch.");
		//return false;
	}

	if (m_header->data_size + m_header->data_offset + 0x60 != pkg_f.Length()) {
		LOG_WARNING(LOADER, "PKG: Data size mismatch.");
		//return false;
	}

	return true;
}

bool LoadHeader(rFile& pkg_f, PKGHeader* m_header)
{
	pkg_f.Seek(0);
	
	if (pkg_f.Read(m_header, sizeof(PKGHeader)) != sizeof(PKGHeader)) {
		LOG_ERROR(LOADER, "PKG: Package file is too short!");
		return false;
	}

	if (!CheckHeader(pkg_f, m_header))
		return false;

	return true;
}

int Decrypt(rFile& pkg_f, rFile& dec_pkg_f, PKGHeader* m_header)
{
	if (!LoadHeader(pkg_f, m_header))
		return -1;

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

	pkg_f.Seek(m_header->data_offset);
	u32 parts = (m_header->data_size + BUF_SIZE - 1) / BUF_SIZE;

	wxProgressDialog pdlg("PKG Decrypter / Installer", "Please wait, decrypting...", parts, 0, wxPD_AUTO_HIDE | wxPD_APP_MODAL);

	memcpy(iv, m_header->klicensee, sizeof(iv));
	aes_setkey_enc(&c, PKG_AES_KEY, 128);
	
	for (u32 i=0; i<parts; i++)
	{
		memset(buf, 0, sizeof(buf));
		u32 length = pkg_f.Read(buf, BUF_SIZE);
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
		dec_pkg_f.Write(buf, length);
		pdlg.Update(i);
	}
	pdlg.Update(parts);
	return 0;
}

// Unpacking.
bool LoadEntries(rFile& dec_pkg_f, PKGHeader* m_header, PKGEntry *m_entries)
{
	dec_pkg_f.Seek(0);
	dec_pkg_f.Read(m_entries, sizeof(PKGEntry) * m_header->file_count);
	
	if (m_entries->name_offset / sizeof(PKGEntry) != m_header->file_count) {
		LOG_ERROR(LOADER, "PKG: Entries are damaged!");
		return false;
	}

	return true;
}

bool UnpackEntry(rFile& dec_pkg_f, const PKGEntry& entry, std::string dir)
{
	char buf[BUF_SIZE];

	dec_pkg_f.Seek(entry.name_offset);
	dec_pkg_f.Read(buf, entry.name_size);
	buf[entry.name_size] = 0;
	
	switch (entry.type.data() >> 24)
	{
	case PKG_FILE_ENTRY_NPDRM:
	case PKG_FILE_ENTRY_NPDRMEDAT:
	case PKG_FILE_ENTRY_SDAT:
	case PKG_FILE_ENTRY_REGULAR:
	{
		rFile out;
		auto path = dir + std::string(buf, entry.name_size);
		if (rExists(path))
		{
			LOG_WARNING(LOADER, "PKG Loader: File is overwritten: %s", path.c_str());
		}

		if (out.Create(path, true /* overwriting */))
		{
			dec_pkg_f.Seek(entry.file_offset);

			for (u64 size = 0; size < entry.file_size;) {
				size += dec_pkg_f.Read(buf, BUF_SIZE);
				if (size > entry.file_size)
					out.Write(buf, BUF_SIZE - (size - entry.file_size));
				else
					out.Write(buf, BUF_SIZE);
			}
			out.Close();
			return true;
		}
		else
		{
			LOG_ERROR(LOADER, "PKG Loader: Could not create file: %s", path.c_str());
			return false;
		}
	}

	case PKG_FILE_ENTRY_FOLDER:
	{
		auto path = dir + std::string(buf, entry.name_size);
		if (!rExists(path) && !rMkdir(path))
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

int Unpack(rFile& pkg_f, std::string src, std::string dst)
{
	PKGHeader* m_header = (PKGHeader*) malloc (sizeof(PKGHeader));

	rFile dec_pkg_f;
	// TODO: This shouldn't use current dir
	std::string decryptedFile = "./dev_hdd1/" + src + ".dec";

	dec_pkg_f.Create(decryptedFile, true);
	
	if (Decrypt(pkg_f, dec_pkg_f, m_header) < 0)
		return -1;
	
	dec_pkg_f.Close();

	rFile n_dec_pkg_f(decryptedFile, rFile::read);

	std::vector<PKGEntry> m_entries;
	m_entries.resize(m_header->file_count);

	PKGEntry *m_entries_ptr = &m_entries[0];
	if (!LoadEntries(n_dec_pkg_f, m_header, m_entries_ptr))
		return -1;

	wxProgressDialog pdlg("PKG Decrypter / Installer", "Please wait, unpacking...", m_entries.size(), 0, wxPD_AUTO_HIDE | wxPD_APP_MODAL);

	for (const PKGEntry& entry : m_entries)
	{
		UnpackEntry(n_dec_pkg_f, entry, dst + src + "/");
		pdlg.Update(pdlg.GetValue() + 1);
	}
	pdlg.Update(m_entries.size());

	n_dec_pkg_f.Close();
	wxRemoveFile(decryptedFile);

	return 0;
}
