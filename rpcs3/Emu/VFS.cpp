#include "stdafx.h"
#include "IdManager.h"
#include "VFS.h"

#include <regex>

struct vfs_manager
{
	shared_mutex mutex;

	// Device name -> Real path
	std::unordered_map<std::string, std::string> mounted;
};

const std::regex s_regex_ps3("^/+(.*?)(?:$|/)(.*)", std::regex::optimize);
const std::regex s_regex_psv("^(.*?):(.*)", std::regex::optimize);

bool vfs::mount(const std::string& dev_name, const std::string& path)
{
	const auto table = fxm::get_always<vfs_manager>();

	writer_lock lock(table->mutex);

	return table->mounted.emplace(dev_name, path).second;
}

std::string vfs::get(const std::string& vpath, vfs::type _type)
{
	std::smatch match;

	if (!std::regex_match(vpath, match, _type == type::ps3 ? s_regex_ps3 : s_regex_psv))
	{
		LOG_WARNING(GENERAL, "vfs::get(): invalid input: %s", vpath);
		return{};
	}

	const auto table = fxm::get_always<vfs_manager>();

	reader_lock lock(table->mutex);

	const auto found = table->mounted.find(match.str(1));

	if (found == table->mounted.end())
	{
		LOG_WARNING(GENERAL, "vfs::get(): device not found: %s", vpath);
		return{};
	}

	// Concatenate
	return found->second + match.str(2);
}
