
#include "Loader.h"

class PKGLoader
{
	rFile& pkg_f;

public:
	PKGLoader(rFile& f);
	virtual bool Install(std::string dest);
	virtual bool Close();
};
