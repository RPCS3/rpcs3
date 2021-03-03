#include "stdafx.h"

#include "Crypto/sha1.h"
#include "Crypto/key_vault.h"

#include "PUP.h"

pup_object::pup_object(const fs::file& file): m_file(file)
{
	if (!file)
	{
		return;
	}

	m_file.seek(0);
	PUPHeader m_header{};

	if (!m_file.read(m_header) || m_header.magic != "SCEUF\0\0\0"_u64)
	{
		// Either file is not large enough to contain header or magic is invalid
		return;
	}

	constexpr usz entry_size = sizeof(PUPFileEntry) + sizeof(PUPHashEntry);

	if (!m_header.file_count || (m_file.size() - sizeof(PUPHeader)) / entry_size < m_header.file_count)
	{
		// These checks before read() are to avoid some std::bad_alloc exceptions when file_count is too large
		// So we cannot rely on read() for error checking in such cases
		return;
	}

	m_file_tbl.resize(m_header.file_count);
	m_hash_tbl.resize(m_header.file_count);

	if (!m_file.read(m_file_tbl) || !m_file.read(m_hash_tbl))
	{
		// If these fail it is an unexpected filesystem error, because file size must suffice as we checked in previous checks
		return;
	}

	isValid = true;
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

bool pup_object::validate_hashes()
{
	if (!isValid) return false;

	for (usz i = 0; i < m_file_tbl.size(); i++)
	{
		u8 *hash = m_hash_tbl[i].hash;
		PUPFileEntry file = m_file_tbl[i];

		// Sanity check for offset and length, use substraction to avoid overflows
		if (usz size = m_file.size(); size < file.data_offset || size - file.data_offset < file.data_length)
		{
			return false;
		}

		std::vector<u8> buffer(file.data_length);
		m_file.seek(file.data_offset);
		m_file.read(buffer.data(), file.data_length);

		u8 output[20] = {};
		sha1_hmac(PUP_KEY, sizeof(PUP_KEY), buffer.data(), buffer.size(), output);
		if (memcmp(output, hash, 20) != 0)
		{
			return false;
		}
	}
	return true;
}
