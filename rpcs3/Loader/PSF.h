#pragma once

#include "util/types.hpp"
#include <map>
#include <string>
#include <string_view>

namespace fs
{
	class file;
}

namespace psf
{
	enum sound_format_flag : s32
	{
		lpcm_2   = 1 << 0, // Linear PCM 2 Ch.
		lpcm_5_1 = 1 << 2, // Linear PCM 5.1 Ch.
		lpcm_7_1 = 1 << 4, // Linear PCM 7.1 Ch.
		ac3      = 1 << 8, // Dolby Digital 5.1 Ch.
		dts      = 1 << 9, // DTS 5.1 Ch.
	};

	enum resolution_flag : s32
	{
		_480p      = 1 << 0,
		_576p      = 1 << 1,
		_720p      = 1 << 2,
		_1080p     = 1 << 3,
		_480p_16_9 = 1 << 4,
		_576p_16_9 = 1 << 5,
	};

	enum class format : u16
	{
		array   = 0x0004, // claimed to be a non-NTS string (char array)
		string  = 0x0204,
		integer = 0x0404,
	};

	enum class error
	{
		ok,
		stream,
		not_psf,
		corrupt,
	};

	class entry final
	{
		format m_type{};
		u32 m_max_size{}; // Entry max size (supplementary info, stored in PSF format)
		u32 m_value_integer{}; // TODO: is it really unsigned?
		std::string m_value_string{};

	public:
		// Construct string entry, assign the value
		entry(format type, u32 max_size, std::string_view value);

		// Construct integer entry, assign the value
		entry(u32 value);

		~entry() = default;

		const std::string& as_string() const;
		u32 as_integer() const;

		entry& operator =(std::string_view value);
		entry& operator =(u32 value);

		format type() const { return m_type; }
		u32 max() const { return m_max_size; }
		u32 size() const;
	};

	// Define PSF registry as a sorted map of entries:
	using registry = std::map<std::string, entry, std::less<>>;

	struct load_result_t
	{
		registry sfo;
		error errc;

		explicit operator bool() const
		{
			return !sfo.empty();
		}
	};

	// Load PSF registry from SFO binary format
	load_result_t load(const fs::file&);
	load_result_t load(const std::string& filename);
	inline registry load_object(const fs::file& f) { return load(f).sfo; }

	// Convert PSF registry to SFO binary format
	std::vector<u8> save_object(const registry&, std::vector<u8>&& init = std::vector<u8>{});

	// Get string value or default value
	std::string_view get_string(const registry& psf, std::string_view key, std::string_view def = ""sv);

	// Get integer value or default value
	u32 get_integer(const registry& psf, std::string_view key, u32 def = 0);

	// Assign new entry
	inline void assign(registry& psf, std::string_view key, entry&& _entry)
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
	inline entry string(u32 max_size, std::string_view value)
	{
		return {format::string, max_size, value};
	}

	// Make array entry
	inline entry array(u32 max_size, std::string_view value)
	{
		return {format::array, max_size, value};
	}

	// Checks if of HDD catgeory (assumes a valid category is being passed)
	constexpr bool is_cat_hdd(std::string_view cat)
	{
		return cat.size() == 2u && cat[1] != 'D' && cat != "DG"sv && cat != "MS"sv;
	}
}
