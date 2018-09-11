#pragma once

#include <vector>
#include <string>
#include <string_view>

namespace vfs
{
	// Mount VFS device
	bool mount(std::string_view vpath, std::string_view path);

	// Convert VFS path to fs path, optionally listing directories mounted in it
	std::string get(std::string_view vpath, std::vector<std::string>* out_dir = nullptr);

	// Escape VFS path by replacing non-portable characters with surrogates
	std::string escape(std::string_view path);

	// Invert escape operation
	std::string unescape(std::string_view path);
}
