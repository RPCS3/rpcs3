#pragma once

namespace vfs
{
	// Print mounted directories
	void dump();

	// Convert PS3/PSV path to fs-compatible path
	std::string get(const std::string& vpath);
}
