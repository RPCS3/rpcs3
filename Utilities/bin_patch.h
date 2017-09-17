#pragma once

#include "BEType.h"
#include <vector>
#include <string>
#include <unordered_map>

enum class patch_type
{
	load,
	byte,
	le16,
	le32,
	le64,
	lef32,
	lef64,
	be16,
	be32,
	be64,
	bef32,
	bef64,
};

class patch_engine
{
	struct patch
	{
		patch_type type;
		u32 offset;
		u64 value;

		template <typename T>
		T& value_as()
		{
			return *reinterpret_cast<T*>(reinterpret_cast<char*>(&value));
		}
	};

	// Database
	std::unordered_map<std::string, std::vector<patch>> m_map;

public:
	// Load from file
	void append(const std::string& path);

	// Apply patch (returns the number of entries applied)
	std::size_t apply(const std::string& name, u8* dst) const;
};
