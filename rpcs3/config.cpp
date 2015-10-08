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
		return *this;
	}

	config_t config{ "rpcs3.new.ini" };
}