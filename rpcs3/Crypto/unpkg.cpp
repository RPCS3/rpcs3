#include "stdafx.h"
#include "unpkg.h"
#include <wx/progdlg.h>

// Decryption.
bool CheckHeader(wxFile& pkg_f, PKGHeader* m_header)
{
	if (m_header->pkg_magic != 0x7F504B47) {
		ConLog.Error("PKG: Not a package file!");
		return false;
	}

	switch (m_header->pkg_type)
	{
	case PKG_RELEASE_TYPE_DEBUG:   break;
	case PKG_RELEASE_TYPE_RELEASE: break;
	default:
		ConLog.Error("PKG: Unknown PKG type!");
		return false;
	}

	switch (m_header->pkg_platform)
	{
	case PKG_PLATFORM_TYPE_PS3: break;
	case PKG_PLATFORM_TYPE_PSP: break;
	default:
		ConLog.Error("PKG: Unknown PKG type!");
		return false;
	}

	if (m_header->header_size != PKG_HEADER_SIZE) {
		ConLog.Error("PKG: Wrong header size!");
		return false;
	}

	if (m_header->pkg_size != pkg_f.Length()) {
		ConLog.Error("PKG: File size mismatch.");
		return false;
	}

	if (m_header->data_size + m_header->data_offset + 0x60 != pkg_f.Length()) {
		ConLog.Error("PKG: Data size mismatch.");
		return false;
	}

	return true;
}

bool LoadHeader(wxFile& pkg_f, PKGHeader* m_header)
{
	pkg_f.Seek(0);
	
	if (pkg_f.Read(m_header, sizeof(PKGHeader)) != sizeof(PKGHeader)) {
		ConLog.Error("PKG: Package file is too short!");
		return false;
	}

	if (!CheckHeader(pkg_f, m_header))
		return false;

	return true;
}

int Decrypt(wxFile& pkg_f, wxFile& dec_pkg_f, PKGHeader* m_header)
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
				be_t<u64> lo = *(be_t<u64>*)&iv[8] + 1;

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
bool LoadEntries(wxFile& dec_pkg_f, PKGHeader* m_header, PKGEntry *m_entries)
{
	dec_pkg_f.Seek(0);
	dec_pkg_f.Read(m_entries, sizeof(PKGEntry) * m_header->file_count);
	
	if (m_entries->name_offset / sizeof(PKGEntry) != m_header->file_count) {
		ConLog.Error("PKG: Entries are damaged!");
		return false;
	}

	return true;
}

bool UnpackEntry(wxFile& dec_pkg_f, const PKGEntry& entry, std::string dir)
{
	u8 buf[BUF_SIZE];

	dec_pkg_f.Seek(entry.name_offset);
	dec_pkg_f.Read(buf, entry.name_size);
	buf[entry.name_size] = 0;
	
	switch (entry.type & (0xff))
	{
		case PKG_FILE_ENTRY_NPDRM:
		case PKG_FILE_ENTRY_NPDRMEDAT:
		case PKG_FILE_ENTRY_SDAT:
		case PKG_FILE_ENTRY_REGULAR:
		{
			wxFile out;
			out.Create(dir + buf);
			dec_pkg_f.Seek(entry.file_offset);

			for (u64 size = 0; size < entry.file_size; ) {
				size += dec_pkg_f.Read(buf, BUF_SIZE);
				if (size > entry.file_size)
					out.Write(buf, BUF_SIZE - (size - entry.file_size));
				else
					out.Write(buf, BUF_SIZE);
			}
			out.Close();
		}
		break;
			
		case PKG_FILE_ENTRY_FOLDER:
			wxMkdir(dir + buf);
		break;
	}
	return true;
}

int Unpack(wxFile& pkg_f, std::string src, std::string dst)
{
	PKGHeader* m_header = (PKGHeader*) malloc (sizeof(PKGHeader));

	wxFile dec_pkg_f;
	std::string decryptedFile = wxGetCwd().ToStdString() + "/dev_hdd1/" + src + ".dec";

	dec_pkg_f.Create(decryptedFile, true);
	
	if (Decrypt(pkg_f, dec_pkg_f, m_header) < 0)
		return -1;
	
	dec_pkg_f.Close();

	wxFile n_dec_pkg_f(decryptedFile, wxFile::read);

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
