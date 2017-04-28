#pragma once

#include "BEType.h"
#include <vector>
#include <string>
#include <unordered_map>

enum class patch_type
{
	byte,
	le16,
	le32,
	le64,
	be16,
	be32,
	be64,
};

class patch_engine
{
	struct patch
	{
		patch_type type;
		u32 offset;
		u64 value;
	};

	// Database
	std::unordered_map<std::string, std::vector<patch>> m_map;

public:
	// Load from file
	void append(const std::string& path);

	// Apply patch
	void apply(const std::string& name, u8* dst) const;
};
