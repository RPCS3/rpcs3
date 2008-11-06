/******************************************************************************
**
**  Module Name:    wnaspi32.h
**
**  Description:    Header file for ASPI for Win32.  This header includes
**                  macro and type declarations, and can be included without
**                  modification when using Borland C++ or Microsoft Visual
**                  C++ with 32-bit compilation.  If you are using a different
**                  compiler then you MUST ensure that structures are packed
**                  onto byte alignments, and that C++ name mangling is turned
**                  off.
**
**  Notes:          This file created using 4 spaces per tab.
**
******************************************************************************/

#ifndef __WNASPI32_H__
#define __WNASPI32_H__

/*
** Make sure structures are packed and undecorated.
*/

#ifdef __BORLANDC__
#pragma option -a1
#endif //__BORLANDC__

#ifdef _MSC_VER
#pragma pack(1)
#endif //__MSC_VER

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//*****************************************************************************
//      %%% SCSI MISCELLANEOUS EQUATES %%%
//*****************************************************************************

#define SENSE_LEN                   14          // Default sense buffer length
#define SRB_DIR_SCSI                0x00        // Direction determined by SCSI
#define SRB_POSTING                 0x01        // Enable ASPI posting
#define SRB_ENABLE_RESIDUAL_COUNT   0x04        // Enable residual byte count reporting
#define SRB_DIR_IN                  0x08        // Transfer from SCSI target to host
#define SRB_DIR_OUT                 0x10        // Transfer from host to SCSI target
#define SRB_EVENT_NOTIFY            0x40        // Enable ASPI event notification

#define RESIDUAL_COUNT_SUPPORTED    0x02        // Extended buffer flag
#define MAX_SRB_TIMEOUT             108000lu    // 30 hour maximum timeout in s
#define DEFAULT_SRB_TIMEOUT         108000lu    // Max timeout by default


//*****************************************************************************
//      %%% ASPI Command Definitions %%%
//*****************************************************************************

#define SC_HA_INQUIRY               0x00        // Host adapter inquiry
#define SC_GET_DEV_TYPE             0x01        // Get device type
#define SC_EXEC_SCSI_CMD            0x02        // Execute SCSI command
#define SC_ABORT_SRB                0x03        // Abort an SRB
#define SC_RESET_DEV                0x04        // SCSI bus device reset
#define SC_SET_HA_PARMS             0x05        // Set HA parameters
#define SC_GET_DISK_INFO            0x06        // Get Disk information
#define SC_RESCAN_SCSI_BUS          0x07        // ReBuild SCSI device map
#define SC_GETSET_TIMEOUTS          0x08        // Get/Set target timeouts

//*****************************************************************************
//      %%% SRB Status %%%
//*****************************************************************************

#define SS_PENDING                  0x00        // SRB being processed
#define SS_COMP                     0x01        // SRB completed without error
#define SS_ABORTED                  0x02        // SRB aborted
#define SS_ABORT_FAIL               0x03        // Unable to abort SRB
#define SS_ERR                      0x04        // SRB completed with error

#define SS_INVALID_CMD              0x80        // Invalid ASPI command
#define SS_INVALID_HA               0x81        // Invalid host adapter number
#define SS_NO_DEVICE                0x82        // SCSI device not installed

#define SS_INVALID_SRB              0xE0        // Invalid parameter set in SRB
#define SS_OLD_MANAGER              0xE1        // ASPI manager doesn't support Windows
#define SS_BUFFER_ALIGN             0xE1        // Buffer not aligned (replaces OLD_MANAGER in Win32)
#define SS_ILLEGAL_MODE             0xE2        // Unsupported Windows mode
#define SS_NO_ASPI                  0xE3        // No ASPI managers resident
#define SS_FAILED_INIT              0xE4        // ASPI for windows failed init
#define SS_ASPI_IS_BUSY             0xE5        // No resources available to execute cmd
#define SS_BUFFER_TO_BIG            0xE6        // Buffer size to big to handle!
#define SS_MISMATCHED_COMPONENTS    0xE7        // The DLLs/EXEs of ASPI don't version check
#define SS_NO_ADAPTERS              0xE8        // No host adapters to manage
#define SS_INSUFFICIENT_RESOURCES   0xE9        // Couldn't allocate resources needed to init
#define SS_ASPI_IS_SHUTDOWN         0xEA        // Call came to ASPI after PROCESS_DETACH
#define SS_BAD_INSTALL              0xEB        // The DLL or other components are installed wrong

//*****************************************************************************
//      %%% Host Adapter Status %%%
//*****************************************************************************

#define HASTAT_OK                   0x00        // Host adapter did not detect an                                                                                                                       // error
#define HASTAT_SEL_TO               0x11        // Selection Timeout
#define HASTAT_DO_DU                0x12        // Data overrun data underrun
#define HASTAT_BUS_FREE             0x13        // Unexpected bus free
#define HASTAT_PHASE_ERR            0x14        // Target bus phase sequence                                                                                                                            // failure
#define HASTAT_TIMEOUT              0x09        // Timed out while SRB was                                                                                                                                      waiting to beprocessed.
#define HASTAT_COMMAND_TIMEOUT      0x0B        // Adapter timed out processing SRB.
#define HASTAT_MESSAGE_REJECT       0x0D        // While processing SRB, the                                                                                                                            // adapter received a MESSAGE
#define HASTAT_BUS_RESET            0x0E        // A bus reset was detected.
#define HASTAT_PARITY_ERROR         0x0F        // A parity error was detected.
#define HASTAT_REQUEST_SENSE_FAILED 0x10        // The adapter failed in issuing

//*****************************************************************************
//          %%% SRB - HOST ADAPTER INQUIRY - SC_HA_INQUIRY (0) %%%
//*****************************************************************************

typedef struct                                  // Offset
{                                               // HX/DEC
    BYTE        SRB_Cmd;                        // 00/000 ASPI command code = SC_HA_INQUIRY
    BYTE        SRB_Status;                     // 01/001 ASPI command status byte
    BYTE        SRB_HaId;                       // 02/002 ASPI host adapter number
    BYTE        SRB_Flags;                      // 03/003 ASPI request flags
    DWORD       SRB_Hdr_Rsvd;                   // 04/004 Reserved, MUST = 0
    BYTE        HA_Count;                       // 08/008 Number of host adapters present
    BYTE        HA_SCSI_ID;                     // 09/009 SCSI ID of host adapter
    BYTE        HA_ManagerId[16];               // 0A/010 String describing the manager
    BYTE        HA_Identifier[16];              // 1A/026 String describing the host adapter
    BYTE        HA_Unique[16];                  // 2A/042 Host Adapter Unique parameters
    WORD        HA_Rsvd1;                       // 3A/058 Reserved, MUST = 0
}
SRB_HAInquiry, *PSRB_HAInquiry, FAR *LPSRB_HAInquiry;

//*****************************************************************************
//          %%% SRB - GET DEVICE TYPE - SC_GET_DEV_TYPE (1) %%%
//*****************************************************************************

typedef struct                                  // Offset
{                                               // HX/DEC
    BYTE        SRB_Cmd;                        // 00/000 ASPI command code = SC_GET_DEV_TYPE
    BYTE        SRB_Status;                     // 01/001 ASPI command status byte
    BYTE        SRB_HaId;                       // 02/002 ASPI host adapter number
    BYTE        SRB_Flags;                      // 03/003 Reserved, MUST = 0
    DWORD       SRB_Hdr_Rsvd;                   // 04/004 Reserved, MUST = 0
    BYTE        SRB_Target;                     // 08/008 Target's SCSI ID
    BYTE        SRB_Lun;                        // 09/009 Target's LUN number
    BYTE        SRB_DeviceType;                 // 0A/010 Target's peripheral device type
    BYTE        SRB_Rsvd1;                      // 0B/011 Reserved, MUST = 0
}
SRB_GDEVBlock, *PSRB_GDEVBlock, FAR *LPSRB_GDEVBlock;

//*****************************************************************************
//          %%% SRB - EXECUTE SCSI COMMAND - SC_EXEC_SCSI_CMD (2) %%%
//*****************************************************************************

typedef struct                                  // Offset
{                                               // HX/DEC
    BYTE        SRB_Cmd;                        // 00/000 ASPI command code = SC_EXEC_SCSI_CMD
    BYTE        SRB_Status;                     // 01/001 ASPI command status byte
    BYTE        SRB_HaId;                       // 02/002 ASPI host adapter number
    BYTE        SRB_Flags;                      // 03/003 ASPI request flags
    DWORD       SRB_Hdr_Rsvd;                   // 04/004 Reserved
    BYTE        SRB_Target;                     // 08/008 Target's SCSI ID
    BYTE        SRB_Lun;                        // 09/009 Target's LUN number
    WORD        SRB_Rsvd1;                      // 0A/010 Reserved for Alignment
    DWORD       SRB_BufLen;                     // 0C/012 Data Allocation Length
    BYTE        FAR *SRB_BufPointer;            // 10/016 Data Buffer Pointer
    BYTE        SRB_SenseLen;                   // 14/020 Sense Allocation Length
    BYTE        SRB_CDBLen;                     // 15/021 CDB Length
    BYTE        SRB_HaStat;                     // 16/022 Host Adapter Status
    BYTE        SRB_TargStat;                   // 17/023 Target Status
    VOID        FAR *SRB_PostProc;              // 18/024 Post routine
    BYTE        SRB_Rsvd2[20];                  // 1C/028 Reserved, MUST = 0
    BYTE        CDBByte[16];                    // 30/048 SCSI CDB
    BYTE        SenseArea[SENSE_LEN+2];         // 50/064 Request Sense buffer
}
SRB_ExecSCSICmd, *PSRB_ExecSCSICmd, FAR *LPSRB_ExecSCSICmd;

//*****************************************************************************
//          %%% SRB - ABORT AN SRB - SC_ABORT_SRB (3) %%%
//*****************************************************************************

typedef struct                                  // Offset
{                                               // HX/DEC
    BYTE        SRB_Cmd;                        // 00/000 ASPI command code = SC_ABORT_SRB
    BYTE        SRB_Status;                     // 01/001 ASPI command status byte
    BYTE        SRB_HaId;                       // 02/002 ASPI host adapter number
    BYTE        SRB_Flags;                      // 03/003 Reserved
    DWORD       SRB_Hdr_Rsvd;                   // 04/004 Reserved
    VOID        FAR *SRB_ToAbort;               // 08/008 Pointer to SRB to abort
}
SRB_Abort, *PSRB_Abort, FAR *LPSRB_Abort;

//*****************************************************************************
//          %%% SRB - BUS DEVICE RESET - SC_RESET_DEV (4) %%%
//*****************************************************************************

typedef struct                                  // Offset
{                                               // HX/DEC
    BYTE        SRB_Cmd;                        // 00/000 ASPI command code = SC_RESET_DEV
    BYTE        SRB_Status;                     // 01/001 ASPI command status byte
    BYTE        SRB_HaId;                       // 02/002 ASPI host adapter number
    BYTE        SRB_Flags;                      // 03/003 ASPI request flags
    DWORD       SRB_Hdr_Rsvd;                   // 04/004 Reserved
    BYTE        SRB_Target;                     // 08/008 Target's SCSI ID
    BYTE        SRB_Lun;                        // 09/009 Target's LUN number
    BYTE        SRB_Rsvd1[12];                  // 0A/010 Reserved for Alignment
    BYTE        SRB_HaStat;                     // 16/022 Host Adapter Status
    BYTE        SRB_TargStat;                   // 17/023 Target Status
    VOID        FAR *SRB_PostProc;              // 18/024 Post routine
    BYTE        SRB_Rsvd2[36];                  // 1C/028 Reserved, MUST = 0
}
SRB_BusDeviceReset, *PSRB_BusDeviceReset, FAR *LPSRB_BusDeviceReset;

//*****************************************************************************
//          %%% SRB - GET DISK INFORMATION - SC_GET_DISK_INFO %%%
//*****************************************************************************

typedef struct                                  // Offset
{                                               // HX/DEC
    BYTE        SRB_Cmd;                        // 00/000 ASPI command code = SC_GET_DISK_INFO
    BYTE        SRB_Status;                     // 01/001 ASPI command status byte
    BYTE        SRB_HaId;                       // 02/002 ASPI host adapter number
    BYTE        SRB_Flags;                      // 03/003 Reserved, MUST = 0
    DWORD       SRB_Hdr_Rsvd;                   // 04/004 Reserved, MUST = 0
    BYTE        SRB_Target;                     // 08/008 Target's SCSI ID
    BYTE        SRB_Lun;                        // 09/009 Target's LUN number
    BYTE        SRB_DriveFlags;                 // 0A/010 Driver flags
    BYTE        SRB_Int13HDriveInfo;            // 0B/011 Host Adapter Status
    BYTE        SRB_Heads;                      // 0C/012 Preferred number of heads translation
    BYTE        SRB_Sectors;                    // 0D/013 Preferred number of sectors translation
    BYTE        SRB_Rsvd1[10];                  // 0E/014 Reserved, MUST = 0
}
SRB_GetDiskInfo, *PSRB_GetDiskInfo, FAR *LPSRB_GetDiskInfo;

//*****************************************************************************
//          %%%  SRB - RESCAN SCSI BUS(ES) ON SCSIPORT %%%
//*****************************************************************************

typedef struct                                  // Offset
{                                               // HX/DEC
    BYTE        SRB_Cmd;                        // 00/000 ASPI command code = SC_RESCAN_SCSI_BUS
    BYTE        SRB_Status;                     // 01/001 ASPI command status byte
    BYTE        SRB_HaId;                       // 02/002 ASPI host adapter number
    BYTE        SRB_Flags;                      // 03/003 Reserved, MUST = 0
    DWORD       SRB_Hdr_Rsvd;                   // 04/004 Reserved, MUST = 0
}
SRB_RescanPort, *PSRB_RescanPort, FAR *LPSRB_RescanPort;

//*****************************************************************************
//          %%% SRB - GET/SET TARGET TIMEOUTS %%%
//*****************************************************************************

typedef struct                                  // Offset
{                                               // HX/DEC
    BYTE        SRB_Cmd;                        // 00/000 ASPI command code = SC_GETSET_TIMEOUTS
    BYTE        SRB_Status;                     // 01/001 ASPI command status byte
    BYTE        SRB_HaId;                       // 02/002 ASPI host adapter number
    BYTE        SRB_Flags;                      // 03/003 ASPI request flags
    DWORD       SRB_Hdr_Rsvd;                   // 04/004 Reserved, MUST = 0
    BYTE        SRB_Target;                     // 08/008 Target's SCSI ID
    BYTE        SRB_Lun;                        // 09/009 Target's LUN number
    DWORD       SRB_Timeout;                    // 0A/010 Timeout in half seconds
}
SRB_GetSetTimeouts, *PSRB_GetSetTimeouts, FAR *LPSRB_GetSetTimeouts;

//*****************************************************************************
//          %%% ASPIBUFF - Structure For Controllng I/O Buffers %%%
//*****************************************************************************

typedef struct tag_ASPI32BUFF                   // Offset
{                                               // HX/DEC
    PBYTE                   AB_BufPointer;      // 00/000 Pointer to the ASPI allocated buffer
    DWORD                   AB_BufLen;          // 04/004 Length in bytes of the buffer
    DWORD                   AB_ZeroFill;        // 08/008 Flag set to 1 if buffer should be zeroed
    DWORD                   AB_Reserved;        // 0C/012 Reserved
}
ASPI32BUFF, *PASPI32BUFF, FAR *LPASPI32BUFF;

//*****************************************************************************
//          %%% PROTOTYPES - User Callable ASPI for Win32 Functions %%%
//*****************************************************************************

typedef void *LPSRB;

#if defined(__BORLANDC__)

DWORD _import GetASPI32SupportInfo( void );
DWORD _import SendASPI32Command( LPSRB );
BOOL _import GetASPI32Buffer( PASPI32BUFF );
BOOL _import FreeASPI32Buffer( PASPI32BUFF );
BOOL _import TranslateASPI32Address( PDWORD, PDWORD );

#elif defined(_MSC_VER)

__declspec(dllimport) DWORD GetASPI32SupportInfo( void );
__declspec(dllimport) DWORD SendASPI32Command( LPSRB );
__declspec(dllimport) BOOL GetASPI32Buffer( PASPI32BUFF );
__declspec(dllimport) BOOL FreeASPI32Buffer( PASPI32BUFF );
__declspec(dllimport) BOOL TranslateASPI32Address( PDWORD, PDWORD );

#else

extern DWORD GetASPI32SupportInfo( void );
extern DWORD GetASPI32Command( LPSRB );
extern BOOL GetASPI32Buffer( PASPI32BUFF );
extern BOOL FreeASPI32Buffer( PASPI32BUFF );
extern BOOL TranslateASPI32Address( PDWORD, PDWORD );

#endif

/*
** Restore compiler default packing and close off the C declarations.
*/

#ifdef __BORLANDC__
#pragma option -a.
#endif //__BORLANDC__

#ifdef _MSC_VER
#pragma pack()
#endif //_MSC_VER

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__WNASPI32_H__
