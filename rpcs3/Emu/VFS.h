#pragma once

namespace vfs
{
	// VFS type
	enum class type
	{
		ps3,
		psv,
	};

	// Mount VFS device
	bool mount(const std::string& dev_name, const std::string& path);

	// Convert VFS path to fs path
	std::string get(const std::string& vpath, type _type = type::ps3);
}
