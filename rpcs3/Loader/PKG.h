#pragma once
#include "Loader.h"

class PKGLoader
{
	wxFile& pkg_f;

public:
	PKGLoader(wxFile& f);
	virtual bool Install(std::string dest);
	virtual bool Close();
};
