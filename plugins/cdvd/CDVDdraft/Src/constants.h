// constants.h: interface for the constants class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONSTANTS_H__A0D50EAF_BD80_11D7_8E2C_0050DA15DE89__INCLUDED_)
#define AFX_CONSTANTS_H__A0D50EAF_BD80_11D7_8E2C_0050DA15DE89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// win32 o/s types
typedef enum WIN32OSTYPE
{
	WIN32S,
	WIN95,
	WINNTOLD,
	WINNTNEW
};

// available read modes supported (only mmc & scsi10 are enabled atm)
typedef enum CDVD_READ_MODE
{
	CDVD_READ_MMC     = 0, 
    CDVD_READ_SCSI10  = 1, 
    CDVD_READ_D8      = 2,   // disabled 
    CDVD_READ_D410    = 3,   // disabled
    CDVD_READ_D412    = 4,   // disabled
    CDVD_READ_UNKNOWN = 5, 
    CDVD_READ_RESERVE = 6,
};

// available buffer modes supported (only sync & async are enabled atm)
typedef enum CDVD_BUFFER_MODE
{
	CDVD_BUFFER_SYNC    = 0, 
    CDVD_BUFFER_ASYNC   = 1, 
    CDVD_BUFFER_SINGLE  = 2,  // unused
    CDVD_BUFFER_UNKNOWN = 3,  // unused
    CDVD_BUFFER_RESERVE = 4
};

// available interfaces (only aspi & ioctl are enabled atm)
typedef enum CDVD_INTERFACE_TYPE
{
	CDVD_INTF_ASPI      = 0, 
    CDVD_INTF_IOCTL     = 1, 
    CDVD_INTF_IOCTL_RAW = 2, // unused
    CDVD_INTF_UNKNOWN   = 3, // unused
    CDVD_INTF_RESERVE   = 4
};

// return codes
const int CDVD_ERROR_SUCCESS        = 0;
const int CDVD_ERROR_FAIL           = -1;
const int CDVD_ERROR_UNINITIALIZED  = -2;
const int CDVD_ERROR_PENDING        = 1;

// srb codes
const int CDVD_SRB_ERROR            = -1;
const int CDVD_SRB_COMPLETED        =  0;
const int CDVD_SRB_PENDING          =  1;

// max number of c/dvd drives to be supported
const int CDVD_MAX_SUPPORTED_DRIVES = 26;

// max number of sectors returned per read
const int CDVD_NUM_SECTORS_PER_BUFF = 26;

// num buffers selectable (used by c/dvd reads)
const int CDVD_NUM_BUFFERS          = 2;

// supported sector sizes
const int CDVD_SECTOR_SIZE_CD       = 2352;
const int CDVD_SECTOR_SIZE_DVD      = 2048;

// retry for srb
const int CDVD_MAX_RETRY            = 2;

// mmc datamodes (indicates return fields)
const int CDVD_MMC_DATAMODE_RAW     = 0xF8;
const int CDVD_MMC_DATAMODE_USER    = 0x10;

// c/dvd drive types
const int CDVD_DRIVETYPE_UNKNOWN    = -1; // everything is fucked, dunno what type of drive it is
const int CDVD_DRIVETYPE_CD         = 1;  // it's a cd ONLY drive
const int CDVD_DRIVETYPE_DVD        = 2;  // it's a dvd ONLY drive
const int CDVD_DRIVETYPE_CDVD       = 3;  // it's a c/dvd drive (read's both cdrom & dvd's)

// c/dvd media types
const int CDVD_MEDIATYPE_UNKNOWN    = -1; // everything is fucked, unknown dumbfuck media
const int CDVD_MEDIATYPE_CD         = 1;  // its a cd, yay (yes i know there's various types of cd's)
const int CDVD_MEDIATYPE_DVD        = 2;  // it's a dvd, (okay various types of dvd and all that shit, but who cares)

// prelim, unused
const int CDVD_CDTYPE_UNKNOWN       = -1; 
const int CDVD_CDTYPE_CDDA          = 0;
const int CDVD_CDTYPE_DATA          = 1;
const int CDVD_CDTYPE_CDXA          = 2;

// prelim, unused
const int CDVD_DVDTYPE_UNKNOWN      = -1;
const int CDVD_DVDTYPE_DVDROM       = 0;
const int CDVD_DVDTYPE_DVDMINRW     = 1;
const int CDVD_DVDTYPE_DVDPLSRW     = 2;
const int CDVD_DVDTYPE_DVDRAM       = 3;


#endif // !defined(AFX_CONSTANTS_H__A0D50EAF_BD80_11D7_8E2C_0050DA15DE89__INCLUDED_)
