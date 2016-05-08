#include "ps3emu_api.h"

ps3emu_api::ps3emu_api(const std::string &path)
{
	load(path);
}

bool ps3emu_api::load(const std::string &path)
{
	if (!m_library.load(path))
	{
		return false;
	}

	bool is_no_errors = true;

	if (!m_library.get(get_api_version, "ps3emu_api_get_api_version") || get_api_version() != ps3emu_api_version)
	{
		is_no_errors = false;
	}

	is_no_errors = is_no_errors && m_library.get(initialize, "ps3emu_api_initialize");
	is_no_errors = is_no_errors && m_library.get(destroy, "ps3emu_api_destroy");

	is_no_errors = is_no_errors && m_library.get(get_version_string, "ps3emu_api_get_version_string");
	is_no_errors = is_no_errors && m_library.get(get_version_number, "ps3emu_api_get_version_number");
	is_no_errors = is_no_errors && m_library.get(get_name_string, "ps3emu_api_get_name_string");

	is_no_errors = is_no_errors && m_library.get(load_elf, "ps3emu_api_load_elf");

	is_no_errors = is_no_errors && m_library.get(set_state, "ps3emu_api_set_state");
	is_no_errors = is_no_errors && m_library.get(get_state, "ps3emu_api_get_state");

	if (!is_no_errors)
	{
		close();
		return false;
	}

	return true;
}

bool ps3emu_api::loaded() const
{
	return m_library.loaded();
}

void ps3emu_api::close()
{
	initialize = nullptr;
	destroy = nullptr;
	get_version_string = nullptr;
	get_version_number = nullptr;
	get_name_string = nullptr;
	load_elf = nullptr;
	set_state = nullptr;
	get_state = nullptr;

	m_library.close();
}

ps3emu_api::operator bool() const
{
	return loaded();
}
