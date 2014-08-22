#pragma once
#include "Loader.h"

class PKGLoader
{
	vfsFile& pkg_f;

public:
	PKGLoader(vfsFile& f);
	virtual bool Install(std::string dest);
	virtual bool Close();
};
