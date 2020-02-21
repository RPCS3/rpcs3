#pragma once

#include <map>

namespace psf
{
	enum class format : u16
	{
		array   = 0x0004, // claimed to be a non-NTS string (char array)
		string  = 0x0204,
		integer = 0x0404,
	};

	class entry final
	{
		format m_type;
		u32 m_max_size; // Entry max size (supplementary info, stored in PSF format)
		u32 m_value_integer; // TODO: is it really unsigned?
		std::string m_value_string;

	public:
		// Construct string entry, assign the value
		entry(format type, u32 max_size, const std::string& value = {});

		// Construct integer entry, assign the value
		entry(u32 value);

		~entry();

		const std::string& as_string() const;
		u32 as_integer() const;

		entry& operator =(const std::string& value);
		entry& operator =(u32 value);

		format type() const { return m_type; }
		u32 max() const { return m_max_size; }
		u32 size() const;
	};

	// Define PSF registry as a sorted map of entries:
	using registry = std::map<std::string, entry>;

	// Load PSF registry from SFO binary format
	registry load_object(const fs::file&);

	// Convert PSF registry to SFO binary format
	void save_object(const fs::file&, const registry&);

	// Get string value or default value
	std::string get_string(const registry& psf, const std::string& key, const std::string& def = {});

	// Get integer value or default value
	u32 get_integer(const registry& psf, const std::string& key, u32 def = 0);

	// Assign new entry
	inline void assign(registry& psf, const std::string& key, entry&& _entry)
	{
		const auto found = psf.find(key);

		if (found == psf.end())
		{
			psf.emplace(key, std::move(_entry));
			return;
		}

		found->second = std::move(_entry);
		return;
	}

	// Make string entry
	inline entry string(u32 max_size, const std::string& value)
	{
		return {format::string, max_size, value};
	}

	// Make array entry
	inline entry array(u32 max_size, const std::string& value)
	{
		return {format::array, max_size, value};
	}
}
