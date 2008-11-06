// typedefs.h: interface for the typedefs class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TYPEDEFS_H__A0D50EB0_BD80_11D7_8E2C_0050DA15DE89__INCLUDED_)
#define AFX_TYPEDEFS_H__A0D50EB0_BD80_11D7_8E2C_0050DA15DE89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if !defined(UI32)
typedef unsigned long UI32;
#endif

#if !defined(SI32)
typedef long SI32;
#endif

#if !defined(UI16)
typedef unsigned short UI16;
#endif

#if !defined(SI16)
typedef short SI16;
#endif

#if !defined(UI08)
typedef unsigned char UI08;
#endif

#if !defined(SI08)
typedef char SI08;
#endif

#if !defined(MAX_PATH)
const int MAX_PATH = 256;
#endif

#if !defined(EXTERN)
#define EXTERN extern
#endif

class CCdvd;

typedef int (CCdvd::*CDVD_INTF_FUNC_T1)();
typedef int (CCdvd::*CDVD_INTF_FUNC_T2)(int a);
typedef int (CCdvd::*CDVD_INTF_FUNC_T3)(int a, int b);
typedef int (CCdvd::*CDVD_INTF_FUNC_T4)(int a, HANDLE *b);
typedef DWORD (*CDROMSendASPI32Command)(LPSRB);
typedef DWORD (*CDROMGetASPI32SupportInfo)(void);

// adapater info
typedef struct tagADAPTERINFO
{
    UI08 ha;
    UI08 id;
    UI08 lun;
    char hostname[MAX_PATH];
    char name[MAX_PATH];

} ADAPTERINFO, *LPADAPTERINFO;

// SPTI command buffer
typedef struct tagSPTD_WITH_SENSE_BUFF
{
	SCSI_PASS_THROUGH_DIRECT sptd;
    UI32 filler;             
	UI08 sensebuff[32];

} SPTD_WITH_SENSE_BUFF, *PSPTD_WITH_SENSE_BUFF;

// scsi mode/block selection header
typedef struct tagMODESELHEADER
{
	UI08 reserved1;
	UI08 medium;
	UI08 reserved2;
	UI08 block_desc_length;
	UI08 density;
	UI08 number_of_blocks_hi;
	UI08 number_of_blocks_med;
	UI08 number_of_blocks_lo;
	UI08 reserved3;
	UI08 block_length_hi;
	UI08 block_length_med;
	UI08 block_length_lo;

} MODESELHEADER, *PMODESELHEADER;

// toc track descriptor definition
typedef struct tagTOCTRACKDESCRIPTOR
{
	UI08 reserved1;
	UI08 adr_control;
	UI08 track_num;  // based 1
	UI08 reserved2;
	UI08 lba_byte3;
	UI08 lba_byte2;
	UI08 lba_byte1;
	UI08 lba_byte0;

} TOCTRACK_DESCRIPTOR, *LPTOCTRACKDESCRIPTOR;

// toc data definition
typedef struct tagTOCDATA
{
	UI08 datalen_byte1; 
	UI08 datalen_byte0;
	UI08 first_track_num;    //based 1
	UI08 last_track_num;     //based 1

} TOCDATA, *LPTOCDATA;

// information recovered per track (start offset & track_type only)
typedef struct tagDECODEDTRACKINFO
{
	UI32 track_offset[256];
	UI08 track_type[256];   
    UI32 total_tracks;

} DECODEDTRACKINFO, *LPDECODEDTRACKINFO;


#endif // !defined(AFX_TYPEDEFS_H__A0D50EB0_BD80_11D7_8E2C_0050DA15DE89__INCLUDED_)
