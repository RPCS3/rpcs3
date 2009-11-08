#pragma once

#include <stdio.h>

#include "SectorSource.h"

class IsoFSCDVD: public SectorSource
{
public:
	IsoFSCDVD();
	virtual ~IsoFSCDVD() throw();

	virtual bool readSector(unsigned char* buffer, int lba);

	virtual int  getNumSectors();
};
