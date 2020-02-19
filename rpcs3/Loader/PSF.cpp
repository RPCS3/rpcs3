#include "stdafx.h"
#include "PSF.h"

LOG_CHANNEL(psf_log, "PSF");

template<>
void fmt_class_string<psf::format>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto fmt)
	{
		switch (fmt)
		{
		STR_CASE(psf::format::array);
		STR_CASE(psf::format::string);
		STR_CASE(psf::format::integer);
		}

		return unknown;
	});
}

namespace psf
{
	struct header_t
	{
		le_t<u32> magic;
		le_t<u32> version;
		le_t<u32> off_key_table;
		le_t<u32> off_data_table;
		le_t<u32> entries_num;
	};

	struct def_table_t
	{
		le_t<u16> key_off;
		le_t<format> param_fmt;
		le_t<u32> param_len;
		le_t<u32> param_max;
		le_t<u32> data_off;
	};


	entry::entry(format type, u32 max_size, const std::string& value)
		: m_type(type)
		, m_max_size(max_size)
		, m_value_string(value)
	{
		verify(HERE), type == format::string || type == format::array, max_size;
	}

	entry::entry(u32 value)
		: m_type(format::integer)
		, m_max_size(sizeof(u32))
		, m_value_integer(value)
	{
	}

	entry::~entry()
	{
	}

	const std::string& entry::as_string() const
	{
		verify(HERE), m_type == format::string || m_type == format::array;
		return m_value_string;
	}

	u32 entry::as_integer() const
	{
		verify(HERE), m_type == format::integer;
		return m_value_integer;
	}

	entry& entry::operator =(const std::string& value)
	{
		verify(HERE), m_type == format::string || m_type == format::array;
		m_value_string = value;
		return *this;
	}

	entry& entry::operator =(u32 value)
	{
		verify(HERE), m_type == format::integer;
		m_value_integer = value;
		return *this;
	}

	u32 entry::size() const
	{
		switch (m_type)
		{
		case format::string:
		case format::array:
			return std::min(m_max_size, ::narrow<u32>(m_value_string.size() + (m_type == format::string)));

		case format::integer:
			return sizeof(u32);
		}

		fmt::throw_exception("Invalid format (0x%x)" HERE, m_type);
	}

	registry load_object(const fs::file& stream)
	{
		registry result;

		// Hack for empty input (TODO)
		if (!stream || !stream.size())
		{
			return result;
		}

		// Get header
		header_t header;
		verify(HERE), stream.read(header);

		// Check magic and version
		verify(HERE),
			header.magic == "\0PSF"_u32,
			header.version == 0x101u,
			sizeof(header_t) + header.entries_num * sizeof(def_table_t) <= header.off_key_table,
			header.off_key_table <= header.off_data_table,
			header.off_data_table <= stream.size();

		// Get indices
		std::vector<def_table_t> indices;
		verify(HERE), stream.read(indices, header.entries_num);

		// Get keys
		std::string keys;
		verify(HERE), stream.seek(header.off_key_table) == header.off_key_table;
		verify(HERE), stream.read(keys, header.off_data_table - header.off_key_table);

		// Load entries
		for (u32 i = 0; i < header.entries_num; ++i)
		{
			verify(HERE), indices[i].key_off < header.off_data_table - header.off_key_table;

			// Get key name (null-terminated string)
			std::string key(keys.data() + indices[i].key_off);

			// Check entry
			verify(HERE),
				result.count(key) == 0,
				indices[i].param_len <= indices[i].param_max,
				indices[i].data_off < stream.size() - header.off_data_table,
				indices[i].param_max < stream.size() - indices[i].data_off;

			// Seek data pointer
			stream.seek(header.off_data_table + indices[i].data_off);

			if (indices[i].param_fmt == format::integer && indices[i].param_max == sizeof(u32) && indices[i].param_len == sizeof(u32))
			{
				// Integer data
				le_t<u32> value;
				verify(HERE), stream.read(value);

				result.emplace(std::piecewise_construct,
					std::forward_as_tuple(std::move(key)),
					std::forward_as_tuple(value));
			}
			else if (indices[i].param_fmt == format::string || indices[i].param_fmt == format::array)
			{
				// String/array data
				std::string value;
				verify(HERE), stream.read(value, indices[i].param_len);

				if (indices[i].param_fmt == format::string)
				{
					// Find null terminator
					value.resize(std::strlen(value.c_str()));
				}

				result.emplace(std::piecewise_construct,
					std::forward_as_tuple(std::move(key)),
					std::forward_as_tuple(indices[i].param_fmt, indices[i].param_max, std::move(value)));
			}
			else
			{
				// Possibly unsupported format, entry ignored
				psf_log.error("Unknown entry format (key='%s', fmt=0x%x, len=0x%x, max=0x%x)", key, indices[i].param_fmt, indices[i].param_len, indices[i].param_max);
			}
		}

		return result;
	}

	void save_object(const fs::file& stream, const psf::registry& psf)
	{
		std::vector<def_table_t> indices; indices.reserve(psf.size());

		// Generate indices and calculate key table length
		std::size_t key_offset = 0, data_offset = 0;

		for (const auto& entry : psf)
		{
			def_table_t index;
			index.key_off = ::narrow<u32>(key_offset);
			index.param_fmt = entry.second.type();
			index.param_len = entry.second.size();
			index.param_max = entry.second.max();
			index.data_off = ::narrow<u32>(data_offset);

			// Update offsets:
			key_offset += ::narrow<u32>(entry.first.size() + 1); // key size
			data_offset += index.param_max;

			indices.push_back(index);
		}

		// Align next section (data) offset
		key_offset = ::align(key_offset, 4);

		// Generate header
		header_t header;
		header.magic = "\0PSF"_u32;
		header.version = 0x101;
		header.off_key_table = ::narrow<u32>(sizeof(header_t) + sizeof(def_table_t) * psf.size());
		header.off_data_table = ::narrow<u32>(header.off_key_table + key_offset);
		header.entries_num = ::narrow<u32>(psf.size());

		// Save header and indices
		stream.write(header);
		stream.write(indices);

		// Save key table
		for (const auto& entry : psf)
		{
			stream.write(entry.first);
			stream.write('\0');
		}

		// Skip padding
		stream.trunc(stream.seek(header.off_data_table));

		// Save data
		for (const auto& entry : psf)
		{
			const auto fmt = entry.second.type();
			const u32 max = entry.second.max();

			if (fmt == format::integer && max == sizeof(u32))
			{
				const le_t<u32> value = entry.second.as_integer();
				stream.write(value);
			}
			else if (fmt == format::string || fmt == format::array)
			{
				const std::string& value = entry.second.as_string();
				const std::size_t size = std::min<std::size_t>(max, value.size());

				if (value.size() + (fmt == format::string) > max)
				{
					// TODO: check real limitations of PSF format
					psf_log.error("Entry value shrinkage (key='%s', value='%s', size=0x%zx, max=0x%x)", entry.first, value, size, max);
				}

				stream.write(value);
				stream.trunc(stream.seek(max - size, fs::seek_cur)); // Skip up to max_size
			}
			else
			{
				fmt::throw_exception("Invalid entry format (key='%s', fmt=0x%x)" HERE, entry.first, fmt);
			}
		}
	}

	std::string get_string(const registry& psf, const std::string& key, const std::string& def)
	{
		const auto found = psf.find(key);

		if (found == psf.end() || (found->second.type() != format::string && found->second.type() != format::array))
		{
			return def;
		}

		return found->second.as_string();
	}

	u32 get_integer(const registry& psf, const std::string& key, u32 def)
	{
		const auto found = psf.find(key);

		if (found == psf.end() || found->second.type() != format::integer)
		{
			return def;
		}

		return found->second.as_integer();
	}
}
