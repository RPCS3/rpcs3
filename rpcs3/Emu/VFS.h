#pragma once

#include <string>

namespace vfs
{
	// Mount VFS device
	bool mount(const std::string& dev_name, const std::string& path);

	// Convert VFS path to fs path
	std::string get(const std::string& vpath, const std::string* = nullptr, std::size_t = 0);

	// Escape VFS path by replacing non-portable characters with surrogates
	std::string escape(const std::string& path);

	// Invert escape operation
	std::string unescape(const std::string& path);
}
