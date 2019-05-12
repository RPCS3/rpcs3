#include "stdafx.h"

#include "PUP.h"

pup_object::pup_object(const fs::file& file): m_file(file)
{
	if (!file)
	{
		isValid = false;
		return;
	}

	PUPHeader m_header;
	m_file.read(m_header);
	if (m_header.magic != "SCEUF\0\0\0"_u64)
	{
		isValid = false;
		return;
	}

	m_file_tbl.resize(m_header.file_count);
	m_file.read(m_file_tbl);
	m_hash_tbl.resize(m_header.file_count);
	m_file.read(m_hash_tbl);
}

fs::file pup_object::get_file(u64 entry_id)
{
	if (!isValid) return fs::file();

	for (PUPFileEntry file_entry : m_file_tbl)
	{
		if (file_entry.entry_id == entry_id)
		{
			std::vector<u8> file_buf(file_entry.data_length);
			m_file.seek(file_entry.data_offset);
			m_file.read(file_buf, file_entry.data_length);
			return fs::make_stream(std::move(file_buf));
		}
	}
	return fs::file();
}
