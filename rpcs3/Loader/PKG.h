#pragma once

struct fs::file;

struct PKGLoader
{
	static bool Install(const fs::file& pkg_f, std::string dest);
};
