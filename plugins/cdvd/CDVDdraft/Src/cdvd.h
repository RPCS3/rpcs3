// Cdvd.h: interface for the CCdvd class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CDVD_H__1189B8A5_BC21_11D7_8E2C_0050DA15DE89__INCLUDED_)
#define AFX_CDVD_H__1189B8A5_BC21_11D7_8E2C_0050DA15DE89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <devioctl.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <ntddcdrm.h>
#include "winaspi/wnaspi32.h"
#include "winaspi/scsidefs.h"
#include "constants.h"
#include "typedefs.h"

#define CDVD_SIMPLE_INTERFACE

// [normal sequence of operation]
// 1. [open]
// Init -> GetNumDrives -> OpenDrive -> SetReadMode -> (DO WHATEVER, read, toc, etc)
// 2. [close]
// CloseDrive -> Shutdown;
// 3. [synchrounous read]
// ReadSector -> (verify if successful)
// 4. [asynchronous read]
// ReadSector -> GetSrbStatus -> (verify if successful)

// CDVD class, don't complain.
class CCdvd  
{
    // public interfaces only
public:

    int Init(CDVD_INTERFACE_TYPE intf_type); // initialize an interface, closes previously opened one.
    int Shutdown();                          // shutsdown an interface
    int GetNumDrives() const;                // get num of c/dvd drives available
    int OpenDrive(int drv_num);              // open a drive
    int CloseDrive();                        // close previously opened drive
    int SetCurrentBuffer(int buf_num);       // currently there are ONLY 2!

    // call, when doing asynchronous requests to retrieve an srb status (i.e. buffer0 = srb0)
    // the return value is only valid when an asynchronous read was previously issued
    int GetSrbStatus(int srb_num);
    
    // manually set read_mode, if mmc mode is specified, data_filter is also set.
    // data_mode can be CDVD_MMC_DATAMODE_RAW / CDVD_MMC_DATAMODE_USER 
    // note that CDVD_MMC_DATAMODE_USER should always be used for DVD medias,
    // and the only read_mode's enabled are CDVD_READ_MMC and CDVD_READ_SCSI10
    // i've disabled all the other modes in this version (unlike the previous one.)
    int SetReadMode(CDVD_READ_MODE read_mode, int data_mode = CDVD_MMC_DATAMODE_RAW);
    
    // determine drive type
    int GetCdvdDriveType();

    // determine media type
    int GetCdvdMediaType();

    // automatically detect media type, read_type and set it.
    // returns detected read mode, else CDVD_ERROR_FAIL if error
    int DetectAndSetReadMode();

    UI08 *GetBufferAddress(int buff_num) const;

    CDVD_INTF_FUNC_T1 GetNumSectors;
    CDVD_INTF_FUNC_T1 GetToc;
    CDVD_INTF_FUNC_T1 TestReady;
    CDVD_INTF_FUNC_T2 SetSpeed;
    CDVD_INTF_FUNC_T1 Stop;
    CDVD_INTF_FUNC_T2 SetSectorSize;
    CDVD_INTF_FUNC_T3 Play;
    CDVD_INTF_FUNC_T4 ReadSector;

    ADAPTERINFO      GetAdapterDetail(int drv_num);

    // for both gettrackdetail and gettocdata
    // only valid if there was  previous call to GetToc;
    DECODEDTRACKINFO GetTrackDetail();
    TOCDATA          GetTocData(); 

    // auxilliary function
    static WIN32OSTYPE GetWin32OSType();
    static int Aspi_CheckValidInstallation();

protected:

    int Aspi_Init();
    int Aspi_Shutdown();
    int Aspi_OpenDrive    (int drv_num);
    int Aspi_CloseDrive   ();
    int Aspi_GetSrbStatus (int srb_num);
    int Aspi_GetNumSectors();
    int Aspi_TestReady    ();   // legacy
    int Aspi_GetToc       ();   // legacy
  //int Aspi_GetHeader    ();
    int Aspi_SetSpeed     (int speed);
    int Aspi_Stop         ();
    int Aspi_SetSectorSize(int sect_size);
    int Aspi_Play         (int sect_start, int sect_stop);
    int Aspi_SetReadMode  (CDVD_READ_MODE read_mode);
    
    // if handle is not NULL, then the read is asynchronous
    int Aspi_ReadSector_mmc    (int sect, HANDLE *handle = NULL);
    int Aspi_ReadSector_scsi10 (int sect, HANDLE *handle = NULL);
    int Aspi_ReadSector_matsu  (int sect, HANDLE *handle = NULL);
    int Aspi_ReadSector_sony   (int sect, HANDLE *handle = NULL);
    int Aspi_ReadSector_nec    (int sect, HANDLE *handle = NULL);

    // retrieve a configuration from the cdvd drive
    int Aspi_GetConfiguration(UI16 feature, UI08 *dest, UI32 dest_size);

    int Ioctl_Init();
    int Ioctl_Shutdown();
    int Ioctl_OpenDrive    (int drv_num);
    int Ioctl_CloseDrive   ();
    int Ioctl_GetSrbStatus (int srb_num);
    int Ioctl_GetNumSectors();
    int Ioctl_TestReady    ();  // legacy
    int Ioctl_GetToc       ();  // legacy
  //int Ioctl_GetHeader    ();
    int Ioctl_SetSpeed     (int speed);
    int Ioctl_Stop         ();
    int Ioctl_SetSectorSize(int sect_size);
    int Ioctl_Play         (int sect_start, int sect_stop);
    int Ioctl_SetReadMode  (CDVD_READ_MODE read_mode);

    // if handle is not NULL, read is still NOT ASYNCHRONOUS, 
    // stupid IOCTL BLOCKS OVERLAPPED OPERATIONS -_-
    int Ioctl_ReadSector_mmc    (int sect, HANDLE *handle = NULL);
    int Ioctl_ReadSector_scsi10 (int sect, HANDLE *handle = NULL);
    int Ioctl_ReadSector_matsu  (int sect, HANDLE *handle = NULL);
    int Ioctl_ReadSector_sony   (int sect, HANDLE *handle = NULL);
    int Ioctl_ReadSector_nec    (int sect, HANDLE *handle = NULL);

    // retrieve a configuration from the cdvd drive
    int Ioctl_GetConfiguration(UI16 feature, UI08 *dest, UI32 dest_size);

    // dummy functions, used for init only so that stupid mofu's 
    // calling no-initialized interfaces won't crash
    int Dummy_T1();
    int Dummy_T2(int a);
    int Dummy_T3(int a, int b);
    int Dummy_T4(int a, HANDLE *b = NULL); 

protected:
    int Aspi_ScsiBusScan();
    int Aspi_ScsiInquiry(UI08 ha, UI08 id, UI08 lun, char *destination);
    int Aspi_ExecuteSrb(SRB_ExecSCSICmd &srbExec, HANDLE *handle, int nretry);
    
    int Ioctl_ScsiBusScan();
    int Ioctl_AddAdapter(int adptr_num, UI08 *buffer);

    int InitializeBuffers();
    int ShutdownBuffers();
    int ExtractTocData(UI08 *buffer);
    int GetCdvdConfiguration(UI16 feature, UI08 *dest, UI32 dest_size);

    void UpdateInterfaceObject();

protected:

    ADAPTERINFO      m_drvDetails[CDVD_MAX_SUPPORTED_DRIVES];
    TOCDATA          m_tocDetails ;
    DECODEDTRACKINFO m_trkDetails;
    
    SI32        m_nCurrentDrive;
    SI32        m_nCurrentBuffer;

    HANDLE      m_hIoctlDrv;
    HINSTANCE   m_hAspi;
    BOOL        m_bAspiInitialized;
    BOOL        m_bIoctlInitialized;
    BOOL        m_bDriveOpened;
    UI32        m_nDrives;
    SI32        m_nMmcDataMode;

    CDVD_INTERFACE_TYPE m_IntfType;
    CDVD_READ_MODE      m_nCurrentReadMode;

protected:

	CDROMSendASPI32Command	  SendASPI32Command;
	CDROMGetASPI32SupportInfo GetASPI32SupportInfo;
	ASPI32BUFF m_ReadBuffer[CDVD_NUM_BUFFERS];  // our buffers

    // SRB's for ASPI
	SRB_ExecSCSICmd      m_srbReadCd  [CDVD_NUM_BUFFERS];     
	SRB_ExecSCSICmd      m_srbRead10  [CDVD_NUM_BUFFERS];     
	SRB_ExecSCSICmd      m_srbReadMat [CDVD_NUM_BUFFERS];     
	SRB_ExecSCSICmd      m_srbReadSony[CDVD_NUM_BUFFERS];
	SRB_ExecSCSICmd      m_srbReadNec [CDVD_NUM_BUFFERS];
	
    // SRB's for IOCTL
    SPTD_WITH_SENSE_BUFF m_sptwbReadCd  [CDVD_NUM_BUFFERS];
	SPTD_WITH_SENSE_BUFF m_sptwbRead10  [CDVD_NUM_BUFFERS];
	SPTD_WITH_SENSE_BUFF m_sptwbReadMat [CDVD_NUM_BUFFERS];
	SPTD_WITH_SENSE_BUFF m_sptwbReadSony[CDVD_NUM_BUFFERS];
	SPTD_WITH_SENSE_BUFF m_sptwbReadNec [CDVD_NUM_BUFFERS];

public:
	CCdvd();
	virtual ~CCdvd();

};

#if defined(CDVD_SIMPLE_INTERFACE)
// declare our global interface functions ;P
EXTERN CCdvd                OBJECT_HOLDER_CDVD;
EXTERN CDVD_INTF_FUNC_T1    FUNCTION_GETNUMSECTORS;
EXTERN CDVD_INTF_FUNC_T1    FUNCTION_GETTOC;
EXTERN CDVD_INTF_FUNC_T1    FUNCTION_TESTREADY;
EXTERN CDVD_INTF_FUNC_T2    FUNCTION_SETSPEED;
EXTERN CDVD_INTF_FUNC_T1    FUNCTION_STOP;
EXTERN CDVD_INTF_FUNC_T2    FUNCTION_SETSECTORSIZE;
EXTERN CDVD_INTF_FUNC_T3    FUNCTION_PLAY;
EXTERN CDVD_INTF_FUNC_T4    FUNCTION_READSECTOR;

// this is where the weird shit starts
// wrap C++ to C, who said i didn't obfuscate :P

#define cdvd_getnumsectors      (OBJECT_HOLDER_CDVD.*FUNCTION_GETNUMSECTORS)
#define cdvd_gettoc             (OBJECT_HOLDER_CDVD.*FUNCTION_GETTOC)
#define cdvd_testready          (OBJECT_HOLDER_CDVD.*FUNCTION_TESTREADY)
#define cdvd_setspeed           (OBJECT_HOLDER_CDVD.*FUNCTION_SETSPEED)
#define cdvd_stop               (OBJECT_HOLDER_CDVD.*FUNCTION_STOP)
#define cdvd_setsectorsize      (OBJECT_HOLDER_CDVD.*FUNCTION_SETSECTORSIZE)
#define cdvd_play               (OBJECT_HOLDER_CDVD.*FUNCTION_PLAY)
#define cdvd_readsector         (OBJECT_HOLDER_CDVD.*FUNCTION_READSECTOR)

#define cdvd_init               (OBJECT_HOLDER_CDVD.Init)
#define cdvd_shutdown           (OBJECT_HOLDER_CDVD.Shutdown)
#define cdvd_getnumdrives       (OBJECT_HOLDER_CDVD.GetNumDrives)
#define cdvd_opendrive          (OBJECT_HOLDER_CDVD.OpenDrive)
#define cdvd_closedrive         (OBJECT_HOLDER_CDVD.CloseDrive)
#define cdvd_setcurrentbuffer   (OBJECT_HOLDER_CDVD.SetCurrentBuffer)
#define cdvd_getsrbstatus       (OBJECT_HOLDER_CDVD.GetSrbStatus)
#define cdvd_setreadmode        (OBJECT_HOLDER_CDVD.SetReadMode)
#define cdvd_getmediatype       (OBJECT_HOLDER_CDVD.GetCdvdMediaType)
#define cdvd_getdrivetype       (OBJECT_HOLDER_CDVD.GetCdvdDriveType)
#define cdvd_getadapterdetail   (OBJECT_HOLDER_CDVD.GetAdapterDetail)
#define cdvd_gettocdata         (OBJECT_HOLDER_CDVD.GetTocData)
#define cdvd_gettrackdetail     (OBJECT_HOLDER_CDVD.GetTrackDetail)
#define cdvd_detectandsetreadmode (OBJECT_HOLDER_CDVD.DetectAndSetReadMode)
#define cdvd_getbufferaddress    (OBJECT_HOLDER_CDVD.GetBufferAddress)

#endif

#endif // !defined(AFX_CDVD_H__1189B8A5_BC21_11D7_8E2C_0050DA15DE89__INCLUDED_)
