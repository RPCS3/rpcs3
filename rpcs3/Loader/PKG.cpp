#include "stdafx.h"
#include "PKG.h"
#include "scetool/aes.h"
#include "scetool/sha1.h"

#include <wx/progdlg.h>

PKGLoader::PKGLoader(wxFile& f) : pkg_f(f)
{
}

bool PKGLoader::Install(std::string dest, bool show)
{
	// Initial checks
	if (!pkg_f.IsOpened())
		return false;

	dest = wxGetCwd() + dest;
	if (!dest.empty() && dest.back() != '/')
		dest += '/';

	if(!LoadHeader(show))
		return false;

	std::string titleID = std::string(m_header.title_id).substr(7, 9);
	std::string decryptedFile = wxGetCwd() + "/dev_hdd1/" + titleID + ".dec";

	if (wxDirExists(dest+titleID)) {
		wxMessageDialog d_overwrite(NULL, "Another installation was found. Do you want to overwrite it?", "PKG Decrypter / Installer", wxYES_NO|wxCENTRE);
		if (d_overwrite.ShowModal() != wxID_YES) {
			ConLog.Error("PKG Loader: Another installation found in: %s", wxString(titleID).wx_str());
			return false;
		}
		// TODO: Remove the following two lines and remove the folder dest+titleID
		ConLog.Error("PKG Loader: Another installation found in: %s", wxString(titleID).wx_str());
		return false;
	}
	if (!wxMkdir(dest+titleID)) {
		ConLog.Error("PKG Loader: Could not make the installation directory: %s", wxString(titleID).wx_str());
		return false;
	}

	// Decrypt the PKG file
	wxFile out;
	out.Create(decryptedFile, true);
	Decrypt(out);
	out.Close();

	// Unpack the decrypted file
	wxFile dec(decryptedFile, wxFile::read);
	LoadEntries(dec);
	wxProgressDialog pdlg("PKG Decrypter / Installer", "Please wait, unpacking...", m_entries.size(), 0, wxPD_AUTO_HIDE | wxPD_APP_MODAL);

	for (const PKGEntry& entry : m_entries)
	{
		UnpackEntry(dec, entry, dest+titleID+'/');
		pdlg.Update(pdlg.GetValue() + 1);
	}
	pdlg.Update(m_entries.size());

	// Delete decrypted file
	dec.Close();
	wxRemoveFile(decryptedFile);
	ConLog.Write("PKG Loader: Package successfully installed in: /dev_hdd0/game/%s", wxString(titleID.c_str()).wx_str());
	return true;
}

bool PKGLoader::Close()
{
	return pkg_f.Close();
}

bool PKGLoader::LoadHeader(bool show)
{
	pkg_f.Seek(0);
	if (pkg_f.Read(&m_header, sizeof(PKGHeader)) != sizeof(PKGHeader)) {
		ConLog.Error("PKG Loader: Package file is too short!");
		return false;
	}

	if (!CheckHeader())
		return false;

	return true;
}

bool PKGLoader::CheckHeader()
{
	if (m_header.pkg_magic != 0x7F504B47) {
		ConLog.Error("PKG Loader: Not a package file!");
		return false;
	}

	switch (m_header.pkg_type)
	{
	case PKG_RELEASE_TYPE_DEBUG:   break;
	case PKG_RELEASE_TYPE_RELEASE: break;
	default:
		ConLog.Error("PKG Loader: Unknown PKG type!");
		return false;
	}

	switch (m_header.pkg_platform)
	{
	case PKG_PLATFORM_TYPE_PS3: break;
	case PKG_PLATFORM_TYPE_PSP: break;
	default:
		ConLog.Error("PKG Loader: Unknown PKG type!");
		return false;
	}

	if (m_header.header_size != PKG_HEADER_SIZE) {
		ConLog.Error("PKG Loader: Wrong header size!");
		return false;
	}

	if (m_header.pkg_size != pkg_f.Length()) {
		ConLog.Error("PKG Loader: File size mismatch.");
		return false;
	}

	if (m_header.data_size + m_header.data_offset + 0x60 != pkg_f.Length()) {
		ConLog.Error("PKG Loader: Data size mismatch.");
		return false;
	}

	return true;
}

bool PKGLoader::LoadEntries(wxFile& dec)
{
	m_entries.resize(m_header.file_count);

	dec.Seek(0);
	dec.Read(&m_entries[0], sizeof(PKGEntry) * m_header.file_count);
	
	if (m_entries[0].name_offset / sizeof(PKGEntry) != m_header.file_count) {
		ConLog.Error("PKG Loader: Entries are damaged!");
		return false;
	}

	return true;
}

bool PKGLoader::UnpackEntry(wxFile& dec, const PKGEntry& entry, std::string dir)
{
	u8 buf[BUF_SIZE];

	dec.Seek(entry.name_offset);
	dec.Read(buf, entry.name_size);
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
			dec.Seek(entry.file_offset);

			for (u64 size = 0; size < entry.file_size; ) {
				size += dec.Read(buf, BUF_SIZE);
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

void PKGLoader::Decrypt(wxFile& out)
{
	aes_context c;
	u8 iv[HASH_LEN];
	u8 buf[BUF_SIZE];
	u8 ctr[BUF_SIZE];
	
	// Debug key
	u8 key[0x40];
	memset(key, 0, 0x40);
	memcpy(key+0x00, &m_header.qa_digest[0], 8); // &data[0x60]
	memcpy(key+0x08, &m_header.qa_digest[0], 8); // &data[0x60]
	memcpy(key+0x10, &m_header.qa_digest[8], 8); // &data[0x68]
	memcpy(key+0x18, &m_header.qa_digest[8], 8); // &data[0x68]

	pkg_f.Seek(m_header.data_offset);
	u32 parts = (m_header.data_size + BUF_SIZE - 1) / BUF_SIZE;

	wxProgressDialog pdlg("PKG Decrypter / Installer", "Please wait, decrypting...", parts, 0, wxPD_AUTO_HIDE | wxPD_APP_MODAL);	
	
	memcpy(iv, m_header.klicensee, sizeof(iv));
	aes_setkey_enc(&c, PKG_AES_KEY, 128);
	
	for (u32 i=0; i<parts; i++)
	{
		memset(buf, 0, sizeof(buf));
		u32 length = pkg_f.Read(buf, BUF_SIZE);
		u32 bits = (length + HASH_LEN - 1) / HASH_LEN;
		
		if (m_header.pkg_type == PKG_RELEASE_TYPE_DEBUG)
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

		if (m_header.pkg_type == PKG_RELEASE_TYPE_RELEASE)
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
		
		out.Write(buf, length);
		pdlg.Update(i);
	}
	pdlg.Update(parts);
}
