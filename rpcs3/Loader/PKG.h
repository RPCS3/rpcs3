#pragma once

struct rfile_t;

struct PKGLoader
{
	static bool Install(const rfile_t& pkg_f, std::string dest);
};
