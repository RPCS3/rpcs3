#include "PrecompiledHeader.h"
#include "IsoFSCDVD.h"
#include "../CDVDaccess.h"

IsoFSCDVD::IsoFSCDVD()
{
}

bool IsoFSCDVD::readSector(unsigned char* buffer, int lba)
{
	return DoCDVDreadSector(buffer,lba,CDVD_MODE_2048)>=0;
}

int IsoFSCDVD::getNumSectors()
{
	cdvdTD td;
	CDVD->getTD(0,&td);

	return td.lsn;
}

IsoFSCDVD::~IsoFSCDVD(void) throw()
{
}
