#include "stdafx.h"
#include "config.h"
#include <fstream>

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
		if (!m_path.empty())
			deserialize(std::ifstream{ m_path });
	}

	void config_t::save() const
	{
		if (!m_path.empty())
			serialize(std::ofstream{ m_path });
	}

	config_t config{ "rpcs3.new.ini" };
}