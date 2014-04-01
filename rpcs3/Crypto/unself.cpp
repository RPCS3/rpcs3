#include "stdafx.h"
#include "unself.h"

SELFDecrypter::SELFDecrypter(vfsStream& s)
	: self_f(s), key_v(), data_buf_length(0)
{
}

bool SELFDecrypter::LoadHeaders(bool isElf32)
{
	// Read SCE header.
	self_f.Seek(0);
	sce_hdr.Load(self_f);

	// Check SCE magic.
	if (!sce_hdr.CheckMagic())
	{
		ConLog.Error("SELF: Not a SELF file!");
		return false;
	}

	// Read SELF header.
	self_hdr.Load(self_f);

	// Read the APP INFO.
	self_f.Seek(self_hdr.se_appinfooff);
	app_info.Load(self_f);

	// Read ELF header.
	self_f.Seek(self_hdr.se_elfoff);
	if (isElf32)
		elf32_hdr.Load(self_f);
	else
		elf64_hdr.Load(self_f);

	// Read ELF program headers.
	if (isElf32)
	{
		phdr32_arr.Clear();
		if(elf32_hdr.e_phoff == 0 && elf32_hdr.e_phnum)
		{
			ConLog.Error("SELF: ELF program header offset is null!");
			return false;
		}
		self_f.Seek(self_hdr.se_phdroff);
		for(u32 i = 0; i < elf32_hdr.e_phnum; ++i)
		{
			Elf32_Phdr* phdr = new Elf32_Phdr();
			phdr->Load(self_f);
			phdr32_arr.Move(phdr);
		}
	}
	else
	{
		phdr64_arr.Clear();
		if(elf64_hdr.e_phoff == 0 && elf64_hdr.e_phnum)
		{
			ConLog.Error("SELF: ELF program header offset is null!");
			return false;
		}
		self_f.Seek(self_hdr.se_phdroff);
		for(u32 i = 0; i < elf64_hdr.e_phnum; ++i)
		{
			Elf64_Phdr* phdr = new Elf64_Phdr();
			phdr->Load(self_f);
			phdr64_arr.Move(phdr);
		}
	}


	// Read section info.
	secinfo_arr.Clear();
	self_f.Seek(self_hdr.se_secinfoff);

	for(u32 i = 0; i < ((isElf32) ? elf32_hdr.e_phnum : elf64_hdr.e_phnum); ++i)
	{
		SectionInfo* sinfo = new SectionInfo();
		sinfo->Load(self_f);
		secinfo_arr.Move(sinfo);
	}

	// Read SCE version info.
	self_f.Seek(self_hdr.se_sceveroff);
	scev_info.Load(self_f);

	// Read control info.
	ctrlinfo_arr.Clear();
	self_f.Seek(self_hdr.se_controloff);

	u32 i = 0;
	while(i < self_hdr.se_controlsize)
	{
		ControlInfo* cinfo = new ControlInfo();
		cinfo->Load(self_f);
		i += cinfo->size;
		ctrlinfo_arr.Move(cinfo);
	}

	// Read ELF section headers.
	if (isElf32)
	{
		shdr32_arr.Clear();
		if(elf32_hdr.e_shoff == 0 && elf32_hdr.e_shnum)
		{
			ConLog.Warning("SELF: ELF section header offset is null!");
			return true;
		}
		self_f.Seek(self_hdr.se_shdroff);
		for(u32 i = 0; i < elf32_hdr.e_shnum; ++i)
		{
			Elf32_Shdr* shdr = new Elf32_Shdr();
			shdr->Load(self_f);
			shdr32_arr.Move(shdr);
		}
	}
	else
	{
		shdr64_arr.Clear();
		if(elf64_hdr.e_shoff == 0 && elf64_hdr.e_shnum)
		{
			ConLog.Warning("SELF: ELF section header offset is null!");
			return true;
		}
		self_f.Seek(self_hdr.se_shdroff);
		for(u32 i = 0; i < elf64_hdr.e_shnum; ++i)
		{
			Elf64_Shdr* shdr = new Elf64_Shdr();
			shdr->Load(self_f);
			shdr64_arr.Move(shdr);
		}
	}

	return true;
}

void SELFDecrypter::ShowHeaders(bool isElf32)
{
	ConLog.Write("SCE header");
	ConLog.Write("----------------------------------------------------");
	sce_hdr.Show();
	ConLog.Write("----------------------------------------------------");
	ConLog.Write("SELF header");
	ConLog.Write("----------------------------------------------------");
	self_hdr.Show();
	ConLog.Write("----------------------------------------------------");
	ConLog.Write("APP INFO");
	ConLog.Write("----------------------------------------------------");
	app_info.Show();
	ConLog.Write("----------------------------------------------------");
	ConLog.Write("ELF header");
	ConLog.Write("----------------------------------------------------");
	isElf32 ? elf32_hdr.Show() : elf64_hdr.Show();
	ConLog.Write("----------------------------------------------------");
	ConLog.Write("ELF program headers");
	ConLog.Write("----------------------------------------------------");
	for(unsigned int i = 0; i < ((isElf32) ? phdr32_arr.GetCount() : phdr64_arr.GetCount()); i++)
		isElf32 ? phdr32_arr[i].Show() : phdr64_arr[i].Show();
	ConLog.Write("----------------------------------------------------");
	ConLog.Write("Section info");
	ConLog.Write("----------------------------------------------------");
	for(unsigned int i = 0; i < secinfo_arr.GetCount(); i++)
		secinfo_arr[i].Show();
	ConLog.Write("----------------------------------------------------");
	ConLog.Write("SCE version info");
	ConLog.Write("----------------------------------------------------");
	scev_info.Show();
	ConLog.Write("----------------------------------------------------");
	ConLog.Write("Control info");
	ConLog.Write("----------------------------------------------------");
	for(unsigned int i = 0; i < ctrlinfo_arr.GetCount(); i++)
		ctrlinfo_arr[i].Show();
	ConLog.Write("----------------------------------------------------");
	ConLog.Write("ELF section headers");
	ConLog.Write("----------------------------------------------------");
	for(unsigned int i = 0; i < ((isElf32) ? shdr32_arr.GetCount() : shdr64_arr.GetCount()); i++)
		isElf32 ? shdr32_arr[i].Show() : shdr64_arr[i].Show();
	ConLog.Write("----------------------------------------------------");
}

bool SELFDecrypter::DecryptNPDRM(u8 *metadata, u32 metadata_size)
{
	aes_context aes;
	ControlInfo *ctrl = NULL;
	u8 npdrm_key[0x10];
	u8 npdrm_iv[0x10];

	// Parse the control info structures to find the NPDRM control info.
	for(unsigned int i = 0; i < ctrlinfo_arr.GetCount(); i++)
	{
		if (ctrlinfo_arr[i].type == 3)
		{
			ctrl = &ctrlinfo_arr[i];
			break;
		}
	}

	// Check if we have a valid NPDRM control info structure.
	// If not, the data has no NPDRM layer.
	if (!ctrl)
	{
		ConLog.Warning("SELF: No NPDRM control info found!");
		return true;
	}

	u8 klicensee_key[0x10];
	memcpy(klicensee_key, key_v.GetKlicenseeKey(), 0x10);

	// Use klicensee if available.
	if (klicensee_key != NULL)
		memcpy(npdrm_key, klicensee_key, 0x10);

	if (ctrl->npdrm.license == 1)  // Network license.
	{
		ConLog.Error("SELF: Can't decrypt network NPDRM!");
		return false;
	}
	else if (ctrl->npdrm.license == 2)  // Local license.
	{
		// Try to find a RAP file to get the key.
		if (!GetKeyFromRap(ctrl->npdrm.content_id, npdrm_key))
		{
			ConLog.Error("SELF: Can't find RAP file for NPDRM decryption!");
			return false;
		}
	}
	else if (ctrl->npdrm.license == 3)  // Free license.
	{
		// Use the NP_KLIC_FREE.
		memcpy(npdrm_key, NP_KLIC_FREE, 0x10);
	}
	else
	{
		ConLog.Error("SELF: Invalid NPDRM license type!");
		return false;
	}

	// Decrypt our key with NP_KLIC_KEY.
	aes_setkey_dec(&aes, NP_KLIC_KEY, 128);
	aes_crypt_ecb(&aes, AES_DECRYPT, npdrm_key, npdrm_key);

	// IV is empty.
	memset(npdrm_iv, 0, 0x10);

	// Use our final key to decrypt the NPDRM layer.
	aes_setkey_dec(&aes, npdrm_key, 128);
	aes_crypt_cbc(&aes, AES_DECRYPT, metadata_size, npdrm_iv, metadata, metadata);

	return true;
}

bool SELFDecrypter::LoadMetadata()
{
	aes_context aes;
	u32 metadata_info_size = sizeof(meta_info);
	u8 *metadata_info = (u8 *)malloc(metadata_info_size);
	u32 metadata_headers_size = sce_hdr.se_hsize - (sizeof(sce_hdr) + sce_hdr.se_meta + sizeof(meta_info));
	u8 *metadata_headers = (u8 *)malloc(metadata_headers_size);

	// Locate and read the encrypted metadata info.
	self_f.Seek(sce_hdr.se_meta + sizeof(sce_hdr));
	self_f.Read(metadata_info, metadata_info_size);

	// Locate and read the encrypted metadata header and section header.
	self_f.Seek(sce_hdr.se_meta + sizeof(sce_hdr) + metadata_info_size);
	self_f.Read(metadata_headers, metadata_headers_size);

	// Find the right keyset from the key vault.
	SELF_KEY keyset = key_v.FindSelfKey(app_info.self_type, sce_hdr.se_flags, app_info.version);

	// Copy the necessary parameters.
	u8 metadata_key[0x20];
	u8 metadata_iv[0x10];
	memcpy(metadata_key, keyset.erk, 0x20);
	memcpy(metadata_iv, keyset.riv, 0x10);

	// Check DEBUG flag.
	if ((sce_hdr.se_flags & 0x8000) != 0x8000)
	{
		// Decrypt the NPDRM layer.
		if (!DecryptNPDRM(metadata_info, metadata_info_size))
			return false;

		// Decrypt the metadata info.
		aes_setkey_dec(&aes, metadata_key, 256);  // AES-256
		aes_crypt_cbc(&aes, AES_DECRYPT, metadata_info_size, metadata_iv, metadata_info, metadata_info);
	}

	// Load the metadata info.
	meta_info.Load(metadata_info);

	// If the padding is not NULL for the key or iv fields, the metadata info
	// is not properly decrypted.
	if ((meta_info.key_pad[0] != 0x00) ||
		(meta_info.iv_pad[0] != 0x00))
	{
		ConLog.Error("SELF: Failed to decrypt metadata info!");
		return false;
	}

	// Perform AES-CTR encryption on the metadata headers.
	size_t ctr_nc_off = 0;
	u8 ctr_stream_block[0x10];
	aes_setkey_enc(&aes, meta_info.key, 128);
	aes_crypt_ctr(&aes, metadata_headers_size, &ctr_nc_off, meta_info.iv, ctr_stream_block, metadata_headers, metadata_headers);

	// Load the metadata header.
	meta_hdr.Load(metadata_headers);

	// Load the metadata section headers.
	meta_shdr.Clear();
	for (unsigned int i = 0; i < meta_hdr.section_count; i++)
	{
		MetadataSectionHeader* m_shdr = new MetadataSectionHeader();
		m_shdr->Load(metadata_headers + sizeof(meta_hdr) + sizeof(MetadataSectionHeader) * i);
		meta_shdr.Move(m_shdr);
	}

	// Copy the decrypted data keys.
	data_keys_length = meta_hdr.key_count * 0x10;
	data_keys = (u8 *) malloc (data_keys_length);
	memcpy(data_keys, metadata_headers + sizeof(meta_hdr) + meta_hdr.section_count * sizeof(MetadataSectionHeader), data_keys_length);

	return true;
}

bool SELFDecrypter::DecryptData()
{
	aes_context aes;

	// Calculate the total data size.
	for (unsigned int i = 0; i < meta_hdr.section_count; i++)
	{
		if (meta_shdr[i].encrypted == 3)
		{
			if ((meta_shdr[i].key_idx <= meta_hdr.key_count - 1) && (meta_shdr[i].iv_idx <= meta_hdr.key_count))
				data_buf_length += meta_shdr[i].data_size;
		}
	}

	// Allocate a buffer to store decrypted data.
	data_buf = (u8*)malloc(data_buf_length);

	// Set initial offset.
	u32 data_buf_offset = 0;

	// Parse the metadata section headers to find the offsets of encrypted data.
	for (unsigned int i = 0; i < meta_hdr.section_count; i++)
	{
		size_t ctr_nc_off = 0;
		u8 ctr_stream_block[0x10];
		u8 data_key[0x10];
		u8 data_iv[0x10];

		// Check if this is an encrypted section.
		if (meta_shdr[i].encrypted == 3)
		{
			// Make sure the key and iv are not out of boundaries.
			if((meta_shdr[i].key_idx <= meta_hdr.key_count - 1) && (meta_shdr[i].iv_idx <= meta_hdr.key_count))
			{
				// Get the key and iv from the previously stored key buffer.
				memcpy(data_key, data_keys + meta_shdr[i].key_idx * 0x10, 0x10);
				memcpy(data_iv, data_keys + meta_shdr[i].iv_idx * 0x10, 0x10);

				// Allocate a buffer to hold the data.
				u8 *buf = (u8 *)malloc(meta_shdr[i].data_size);

				// Seek to the section data offset and read the encrypted data.
				self_f.Seek(meta_shdr[i].data_offset);
				self_f.Read(buf, meta_shdr[i].data_size);

				// Zero out our ctr nonce.
				memset(ctr_stream_block, 0, sizeof(ctr_stream_block));

				// Perform AES-CTR encryption on the data blocks.
				aes_setkey_enc(&aes, data_key, 128);
				aes_crypt_ctr(&aes, meta_shdr[i].data_size, &ctr_nc_off, data_iv, ctr_stream_block, buf, buf);

				// Copy the decrypted data.
				memcpy(data_buf + data_buf_offset, buf, meta_shdr[i].data_size);

				// Advance the buffer's offset.
				data_buf_offset += meta_shdr[i].data_size;

				// Release the temporary buffer.
				free(buf);
			}
		}
	}

	return true;
}

bool SELFDecrypter::MakeElf(const std::string& elf, bool isElf32)
{
	// Create a new ELF file.
	wxFile e(elf.c_str(), wxFile::write);
	if(!e.IsOpened())
	{
		ConLog.Error("Could not create ELF file! (%s)", elf.c_str());
		return false;
	}

	// Set initial offset.
	u32 data_buf_offset = 0;

	if (isElf32)
	{
		// Write ELF header.
		WriteEhdr(e, elf32_hdr);

		// Write program headers.
		for(u32 i = 0; i < elf32_hdr.e_phnum; ++i)
			WritePhdr(e, phdr32_arr[i]);

		for (unsigned int i = 0; i < meta_hdr.section_count; i++)
		{
			// PHDR type.
			if (meta_shdr[i].type == 2)
			{
				// Seek to the program header data offset and write the data.
				e.Seek(phdr32_arr[meta_shdr[i].program_idx].p_offset);
				e.Write(data_buf + data_buf_offset, meta_shdr[i].data_size);

				// Advance the data buffer offset by data size.
				data_buf_offset += meta_shdr[i].data_size;
			}
		}

		// Write section headers.
		if(self_hdr.se_shdroff != NULL)
		{
			e.Seek(elf32_hdr.e_shoff);

			for(u32 i = 0; i < elf32_hdr.e_shnum; ++i)
				WriteShdr(e, shdr32_arr[i]);
		}
	}
	else
	{
		// Write ELF header.
		WriteEhdr(e, elf64_hdr);

		// Write program headers.
		for(u32 i = 0; i < elf64_hdr.e_phnum; ++i)
			WritePhdr(e, phdr64_arr[i]);

		// Write data.
		for (unsigned int i = 0; i < meta_hdr.section_count; i++)
		{
			// PHDR type.
			if (meta_shdr[i].type == 2)
			{
				// Decompress if necessary.
				if (meta_shdr[i].compressed == 2)
				{
					// Allocate a buffer for decompression.
					u8 *decomp_buf = (u8 *)malloc(phdr64_arr[meta_shdr[i].program_idx].p_filesz);

					// Set up memory streams for input/output.
					wxMemoryInputStream decomp_stream_in(data_buf + data_buf_offset, meta_shdr[i].data_size);
					wxMemoryOutputStream decomp_stream_out;

					// Create a Zlib stream, read the data and flush the stream.
					wxZlibInputStream* z_stream = new wxZlibInputStream(decomp_stream_in);
					z_stream->Read(decomp_stream_out);
					delete z_stream;

					// Copy the decompressed result from the stream.
					decomp_stream_out.CopyTo(decomp_buf, phdr64_arr[meta_shdr[i].program_idx].p_filesz);

					// Seek to the program header data offset and write the data.
					e.Seek(phdr64_arr[meta_shdr[i].program_idx].p_offset);
					e.Write(decomp_buf, phdr64_arr[meta_shdr[i].program_idx].p_filesz);

					// Release the decompression buffer.
					free(decomp_buf);
				}
				else
				{
					// Seek to the program header data offset and write the data.
					e.Seek(phdr64_arr[meta_shdr[i].program_idx].p_offset);
					e.Write(data_buf + data_buf_offset, meta_shdr[i].data_size);
				}

				// Advance the data buffer offset by data size.
				data_buf_offset += meta_shdr[i].data_size;
			}
		}

		// Write section headers.
		if(self_hdr.se_shdroff != NULL)
		{
			e.Seek(elf64_hdr.e_shoff);

			for(u32 i = 0; i < elf64_hdr.e_shnum; ++i)
				WriteShdr(e, shdr64_arr[i]);
		}
	}

	e.Close();
	return true;
}

bool SELFDecrypter::GetKeyFromRap(u8 *content_id, u8 *npdrm_key)
{
	// Set empty RAP key.
	u8 rap_key[0x10];
	memset(rap_key, 0, 0x10);

	// Try to find a matching RAP file under dev_usb000.
	std::string ci_str((const char *)content_id);
	std::string rap_path(fmt::ToUTF8(wxGetCwd()) + "/dev_usb000/" + ci_str + ".rap");

	// Check if we have a valid RAP file.
	if (!wxFile::Exists(fmt::FromUTF8(rap_path)))
	{
		ConLog.Error("This application requires a valid RAP file for decryption!");
		return false;
	}

	// Open the RAP file and read the key.
	wxFile rap_file(fmt::FromUTF8(rap_path), wxFile::read);

	if (!rap_file.IsOpened())
	{
		ConLog.Error("Failed to load RAP file!");
		return false;
	}

	ConLog.Write("Loading RAP file %s", (ci_str + ".rap").c_str());
	rap_file.Read(rap_key, 0x10);
	rap_file.Close();

	// Convert the RAP key.
	rap_to_rif(rap_key, npdrm_key);

	return true;
}

bool IsSelf(const std::string& path)
{
	vfsLocalFile f(nullptr);

	if(!f.Open(path))
		return false;

	SceHeader hdr;
	hdr.Load(f);

	return hdr.CheckMagic();
}

bool IsSelfElf32(const std::string& path)
{
	vfsLocalFile f(nullptr);

	if(!f.Open(path))
		return false;

	SceHeader hdr;
	SelfHeader sh;
	hdr.Load(f);
	sh.Load(f);
	
	// Locate the class byte and check it.
	u8 elf_class[0x8];
	f.Seek(sh.se_elfoff);
	f.Read(elf_class, 0x8);

	return (elf_class[4] == 1);
}

bool CheckDebugSelf(const std::string& self, const std::string& elf)
{
	// Open the SELF file.
	wxFile s(fmt::FromUTF8(self));

	if(!s.IsOpened())
	{
		ConLog.Error("Could not open SELF file! (%s)", self.c_str());
		return false;
	}

	// Get the key version.
	s.Seek(0x08);
	u16 key_version;
	s.Read(&key_version, sizeof(key_version));

	// Check for DEBUG version.
	if(swap16(key_version) == 0x8000)
	{
		ConLog.Warning("Debug SELF detected! Removing fake header...");

		// Get the real elf offset.
		s.Seek(0x10);
		u64 elf_offset;
		s.Read(&elf_offset, sizeof(elf_offset));

		// Start at the real elf offset.
		elf_offset = swap64(elf_offset);
		s.Seek(elf_offset);

		// Write the real ELF file back.
		wxFile e(fmt::FromUTF8(elf), wxFile::write);
		if(!e.IsOpened())
		{
			ConLog.Error("Could not create ELF file! (%s)", elf.c_str());
			return false;
		}

		// Copy the data.
		char buf[2048];
		while (ssize_t size = s.Read(buf, 2048))
			e.Write(buf, size);

		e.Close();
		return true;
	}
	else
	{
		// Leave the file untouched.
		s.Seek(0);
		return false;
	}

	s.Close();
}

bool DecryptSelf(const std::string& elf, const std::string& self)
{
	// Check for a debug SELF first.
	if (!CheckDebugSelf(self, elf))
	{
		// Set a virtual pointer to the SELF file.
		vfsLocalFile self_vf(nullptr);

		if (!self_vf.Open(self))
			return false;

		// Check the ELF file class (32 or 64 bit).
		bool isElf32 = IsSelfElf32(self);

		// Start the decrypter on this SELF file.
		SELFDecrypter self_dec(self_vf);

		// Load the SELF file headers.
		if (!self_dec.LoadHeaders(isElf32))
		{
			ConLog.Error("SELF: Failed to load SELF file headers!");
			return false;
		}
		
		// Load and decrypt the SELF file metadata.
		if (!self_dec.LoadMetadata())
		{
			ConLog.Error("SELF: Failed to load SELF file metadata!");
			return false;
		}
		
		// Decrypt the SELF file data.
		if (!self_dec.DecryptData())
		{
			ConLog.Error("SELF: Failed to decrypt SELF file data!");
			return false;
		}
		
		// Make a new ELF file from this SELF.
		if (!self_dec.MakeElf(elf, isElf32))
		{
			ConLog.Error("SELF: Failed to make ELF file from SELF!");
			return false;
		}
	}

	return true;
}
