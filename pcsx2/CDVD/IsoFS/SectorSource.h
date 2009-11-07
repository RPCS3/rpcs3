#pragma once

class SectorSource
{
public:
	virtual int  getNumSectors()=0; 
	virtual bool readSector(unsigned char* buffer, int lba)=0;
	virtual ~SectorSource(void) {};
};
