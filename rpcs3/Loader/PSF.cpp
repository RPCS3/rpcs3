#include "stdafx.h"
#include "Emu/FS/vfsStream.h"
#include "PSF.h"

namespace psf
{
	u32 entry::max_size() const
	{
		return m_max_size;
	}

	entry& entry::max_size(u32 value)
	{
		m_max_size = value;
		return *this;
	}

	entry_format entry::format() const
	{
		return m_format;
	}

	entry& entry::format(entry_format value)
	{
		m_format = value;
		return *this;
	}

	std::string entry::as_string() const
	{
		if (m_format != entry_format::string && m_format != entry_format::string_not_null_term)
		{
			throw std::logic_error("psf entry as_string() error: bad format");
		}

		return m_value_string;
	}

	u32 entry::as_integer() const
	{
		if (m_format != entry_format::integer)
		{
			throw std::logic_error("psf entry as_integer() error: bad format");
		}

		return m_value_integer;
	}

	std::string entry::to_string() const
	{
		switch (m_format)
		{
		case entry_format::string:
		case entry_format::string_not_null_term:
			return m_value_string;

		case entry_format::integer:
			return std::to_string(m_value_integer);
		}

		throw std::logic_error("psf entry to_string() error: bad format");
	}

	u32 entry::to_integer() const
	{
		switch (m_format)
		{
		case entry_format::string:
		case entry_format::string_not_null_term:
			return std::stoul(m_value_string);

		case entry_format::integer:
			return m_value_integer;
		}

		throw std::logic_error("psf entry to_integer() error: bad format");
	}

	entry& entry::value(const std::string &value_)
	{
		if (m_format != entry_format::string_not_null_term)
		{
			m_format = entry_format::string;
		}

		m_value_string = value_;

		if (m_max_size && m_value_string.size() > m_max_size)
		{
			if (m_format != entry_format::string_not_null_term)
			{
				m_value_string.erase(m_max_size);
			}
			else
			{
				m_value_string.erase(m_max_size - 1);
			}
		}

		return *this;
	}

	entry& entry::value(u32 value_)
	{
		m_format = entry_format::integer;
		m_value_integer = value_;

		if (m_max_size && m_max_size != 4)
		{
			throw std::logic_error("entry::value() error: bad integer max length");
		}

		return *this;
	}

	entry& entry::operator = (const std::string &value_)
	{
		return value(value_);
	}

	entry& entry::operator = (u32 value_)
	{
		return value(value_);
	}

	std::size_t entry::size() const
	{
		switch (m_format)
		{
		case entry_format::string:
			return m_value_string.size() + 1;

		case entry_format::string_not_null_term:
			return m_value_string.size();

		case entry_format::integer:
			return sizeof(u32);
		}

		throw std::logic_error("entry::size(): bad format");
	}

	bool object::load(vfsStream& stream)
	{
		clear();

		header header_;

		// load header
		if (!stream.SRead(header_))
		{
			return false;
		}

		// check magic
		if (header_.magic != *(u32*)"\0PSF")
		{
			LOG_ERROR(LOADER, "psf::load() failed: unknown magic (0x%x)", header_.magic);
			return false;
		}

		// check version
		if (header_.version != 0x101)
		{
			LOG_ERROR(LOADER, "psf::load() failed: unknown version (0x%x)", header_.version);
			return false;
		}

		// load indices
		std::vector<def_table> indices;

		indices.resize(header_.entries_num);

		if (!stream.SRead(indices[0], sizeof(def_table) * header_.entries_num))
		{
			return false;
		}

		// load key table
		if (header_.off_key_table > header_.off_data_table)
		{
			LOG_ERROR(LOADER, "psf::load() failed: off_key_table=0x%x, off_data_table=0x%x", header_.off_key_table, header_.off_data_table);
			return false;
		}

		const u32 key_table_size = header_.off_data_table - header_.off_key_table;

		std::unique_ptr<char[]> keys(new char[key_table_size + 1]);

		stream.Seek(header_.off_key_table);

		if (stream.Read(keys.get(), key_table_size) != key_table_size)
		{
			return false;
		}

		keys.get()[key_table_size] = 0;

		// load entries
		for (u32 i = 0; i < header_.entries_num; ++i)
		{
			if (indices[i].key_off >= key_table_size)
			{
				return false;
			}

			std::string key = keys.get() + indices[i].key_off;

			entry &entry_ = (*this)[key];

			entry_.format(indices[i].param_fmt);
			entry_.max_size(indices[i].param_max);

			// load data
			stream.Seek(header_.off_data_table + indices[i].data_off);

			if (indices[i].param_fmt == entry_format::integer && indices[i].param_len == 4 && indices[i].param_max >= indices[i].param_len)
			{
				// load int data

				u32 value;
				if (!stream.SRead(value))
				{
					return false;
				}

				entry_.value(value);
			}
			else if ((indices[i].param_fmt == entry_format::string || indices[i].param_fmt == entry_format::string_not_null_term)
				&& indices[i].param_max >= indices[i].param_len)
			{
				// load str data

				const u32 size = indices[i].param_len;

				if (size > 0)
				{
					std::unique_ptr<char[]> str(new char[size]);

					if (stream.Read(str.get(), size) != size)
					{
						return false;
					}

					if (indices[i].param_fmt == entry_format::string)
					{
						str.get()[size - 1] = '\0';
						entry_.value(str.get());
					}
					else
					{
						entry_.value(std::string{ str.get(), size });
					}
				}
			}
			else
			{
				LOG_ERROR(LOADER, "psf::load() failed: (i=%d) fmt=0x%x, len=0x%x, max=0x%x", i, indices[i].param_fmt, indices[i].param_len, indices[i].param_max);
				return false;
			}
		}

		return true;
	}

	bool object::save(vfsStream& stream) const
	{
		std::vector<def_table> indices;

		indices.resize(m_entries.size());

		// generate header
		header header_;
		header_.magic = *(u32*)"\0PSF";
		header_.version = 0x101;
		header_.entries_num = static_cast<u32>(m_entries.size());
		header_.off_key_table = sizeof(header) + sizeof(def_table) * header_.entries_num;

		{
			// calculate key table length and generate indices

			u32& key_offset = header_.off_data_table = 0;
			u32 data_offset = 0;
			std::size_t index = 0;
			for (auto &entry : m_entries)
			{
				indices[index].key_off = key_offset;
				indices[index].data_off = data_offset;
				indices[index].param_fmt = entry.second.format();

				key_offset += static_cast<u32>(entry.first.size()) + 1; // key size

				u32 max_size = entry.second.max_size();
				if (max_size == 0)
				{
					max_size = entry.second.size();
				}

				data_offset += max_size;
			}
		}

		header_.off_data_table += header_.off_key_table;

		// save header
		if (!stream.SWrite(header_))
		{
			return false;
		}

		// save indices
		if (!stream.SWrite(indices[0], sizeof(def_table) * m_entries.size()))
		{
			return false;
		}

		// save key table
		for (const auto& entry : m_entries)
		{
			if (!stream.SWrite(entry.first, entry.first.size() + 1))
			{
				return false;
			}
		}

		// save data
		for (const auto& entry : m_entries)
		{
			switch (entry.second.format())
			{
			case entry_format::string:
			{
				std::string value = entry.second.as_string();
				if (!stream.SWrite(value.data(), value.size() + 1))
				{
					return false;
				}
				break;
			}
			case entry_format::string_not_null_term:
			{
				std::string value = entry.second.as_string();
				if (!stream.SWrite(value.data(), value.size()))
				{
					return false;
				}
				break;
			}
			case entry_format::integer:
			{
				if (!stream.SWrite(entry.second.as_integer()))
				{
					return false;
				}
				break;
			}
			}
		}

		return true;
	}

	void object::clear()
	{
		m_entries.clear();
	}

	entry& object::operator[](const std::string &key)
	{
		return m_entries[key];
	}

	const entry& object::operator[](const std::string &key) const
	{
		return m_entries.at(key);
	}

	const entry* object::get(const std::string &key) const
	{
		auto found = m_entries.find(key);

		if (found == m_entries.end())
		{
			return nullptr;
		}

		return &found->second;
	}

	std::string object::get_string_or(const std::string &key, const std::string &default_value) const
	{
		if (const psf::entry *found = get(key))
		{
			return found->as_string();
		}

		return default_value;
	}

	u32 object::get_integer_or(const std::string &key, u32 default_value) const
	{
		if (const psf::entry *found = get(key))
		{
			return found->as_integer();
		}

		return default_value;
	}
}
