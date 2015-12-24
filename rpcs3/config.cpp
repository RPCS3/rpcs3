#include "stdafx.h"
#include "config.h"

namespace rpcs3
{
	config_t::config_t(const std::string &path_)
	{
		path(path_);
	}
	config_t::config_t(const config_t& rhs)
	{
		assign(rhs);
	}

	config_t& config_t::operator =(const config_t& rhs)
	{
		assign(rhs);
		path(rhs.path());
		return *this;
	}

	void config_t::path(const std::string &new_path)
	{
		m_path = new_path;
	}

	std::string config_t::path() const
	{
		return m_path;
	}

	void config_t::load()
	{
		fs::file file(m_path, fom::create | fom::read);

		if (file)
			from_string((const std::string)file);
	}

	void config_t::save() const
	{
		fs::file(m_path, fom::rewrite) << to_string();
	}

	config_t config{ fs::get_config_dir() + "rpcs3.new.ini" };
}
