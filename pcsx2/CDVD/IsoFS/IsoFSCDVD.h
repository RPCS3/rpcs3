#pragma once

#include <stdio.h>

#include "SectorSource.h"

class IsoFSCDVD: public SectorSource
{
public:
	IsoFSCDVD();

	virtual bool readSector(unsigned char* buffer, int lba);

	virtual int  getNumSectors();

	virtual ~IsoFSCDVD(void);
};
