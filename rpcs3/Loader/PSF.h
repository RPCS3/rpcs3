#pragma once

struct vfsStream;

namespace psf
{
	enum class entry_format : u16
	{
		unknown = 0,
		string_not_null_term = 0x0004,
		string = 0x0204,
		integer = 0x0404,
	};

	struct header
	{
		u32 magic;
		u32 version;
		u32 off_key_table;
		u32 off_data_table;
		u32 entries_num;
	};

	struct def_table
	{
		u16 key_off;
		entry_format param_fmt;
		u32 param_len;
		u32 param_max;
		u32 data_off;
	};

	class entry
	{
		entry_format m_format = entry_format::unknown;
		u32 m_max_size = 0;

		u32 m_value_integer;
		std::string m_value_string;

	public:
		u32 max_size() const;
		entry& max_size(u32 value);
		entry_format format() const;
		entry& format(entry_format value);
		std::string as_string() const;
		u32 as_integer() const;
		std::string to_string() const;
		u32 to_integer() const;
		entry& value(const std::string &value_);
		entry& value(u32 value_);
		entry& operator = (const std::string &value_);
		entry& operator = (u32 value_);

		std::size_t size() const;
	};

	class object
	{
		std::unordered_map<std::string, entry> m_entries;

	public:
		object() = default;

		object(vfsStream& stream)
		{
			load(stream);
		}

		virtual ~object() = default;

		bool load(vfsStream& stream);
		bool save(vfsStream& stream) const;
		void clear();

		explicit operator bool() const
		{
			return !m_entries.empty();
		}

		entry& operator[](const std::string &key);
		const entry& operator[](const std::string &key) const;

		//returns pointer to entry or null, if not exists
		const entry *get(const std::string &key) const;

		bool exists(const std::string &key) const
		{
			return m_entries.find(key) != m_entries.end();
		}

		std::unordered_map<std::string, entry>& entries()
		{
			return m_entries;
		}

		const std::unordered_map<std::string, entry>& entries() const
		{
			return m_entries;
		}

		std::unordered_map<std::string, entry>::iterator begin()
		{
			return m_entries.begin();
		}

		std::unordered_map<std::string, entry>::iterator end()
		{
			return m_entries.end();
		}

		std::unordered_map<std::string, entry>::const_iterator begin() const
		{
			return m_entries.begin();
		}

		std::unordered_map<std::string, entry>::const_iterator end() const
		{
			return m_entries.end();
		}
	};
}
