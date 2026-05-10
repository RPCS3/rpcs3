#include "stdafx.h"

#include "Crypto/sha1.h"
#include "Crypto/key_vault.h"

#include "PUP.h"

pup_object::pup_object(fs::file&& file) : m_file(std::move(file))
{
	if (!m_file)
	{
		m_error = pup_error::stream;
		return;
	}

	m_file.seek(0);
	PUPHeader m_header{};

	const usz file_size = m_file.size();

	if (!m_file.read(m_header))
	{
		// File is not large enough to contain header or magic is invalid
		m_error = pup_error::header_read;
		m_formatted_error = fmt::format("Too small PUP file to contain header: 0x%x", file_size);
		return;
	}

	if (m_header.magic != "SCEUF\0\0\0"_u64)
	{
		m_error = pup_error::header_magic;
		return;
	}

	// Check if file size is the expected size, use subtraction to avoid overflows
	if (file_size < m_header.header_length || file_size - m_header.header_length < m_header.data_length)
	{
		m_formatted_error = fmt::format("Firmware size mismatch, expected: 0x%x + 0x%x, actual: 0x%x", m_header.header_length, m_header.data_length, file_size);
		m_error = pup_error::expected_size;
		return;
	}

	if (!m_header.file_count)
	{
		m_error = pup_error::header_file_count;
		return;
	}

	if (!m_file.read(m_file_tbl, m_header.file_count) || !m_file.read(m_hash_tbl, m_header.file_count))
	{
		m_error = pup_error::header_file_count;
		return;
	}

	if (pup_error err = validate_hashes(); err != pup_error::ok)
	{
		m_error = err;
		return;
	}

	m_error = pup_error::ok;
}

fs::file pup_object::get_file(u64 entry_id) const
{
	if (m_error != pup_error::ok) return {};

	for (const PUPFileEntry& file_entry : m_file_tbl)
	{
		if (file_entry.entry_id == entry_id)
		{
			std::vector<u8> file_buf(file_entry.data_length);
			m_file.seek(file_entry.data_offset);
			m_file.read(file_buf, file_entry.data_length);
			return fs::make_stream(std::move(file_buf));
		}
	}

	return {};
}

pup_error pup_object::validate_hashes()
{
	AUDIT(m_error == pup_error::ok);

	std::vector<u8> buffer;

	const usz size = m_file.size();

	for (const PUPFileEntry& file : m_file_tbl)
	{
		// Sanity check for offset and length, use subtraction to avoid overflows
		if (size < file.data_offset || size - file.data_offset < file.data_length)
		{
			m_formatted_error = fmt::format("File database entry is invalid. (offset=0x%x, length=0x%x, PUP.size=0x%x)", file.data_offset, file.data_length, size);
			return pup_error::file_entries;
		}

		// Reuse buffer
		buffer.resize(file.data_length);
		m_file.seek(file.data_offset);
		m_file.read(buffer.data(), file.data_length);

		u8 output[20] = {};
		sha1_hmac(PUP_KEY, sizeof(PUP_KEY), buffer.data(), buffer.size(), output);

		// Compare to hash entry
		if (std::memcmp(output, m_hash_tbl[&file - m_file_tbl.data()].hash, 20) != 0)
		{
			return pup_error::hash_mismatch;
		}
	}

	return pup_error::ok;
}
