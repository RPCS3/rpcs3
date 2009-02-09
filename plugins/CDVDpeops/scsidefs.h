//***************************************************************************
//
// Name:            SCSIDEFS.H
//
// Description: SCSI definitions ('C' Language)
//
//***************************************************************************

//***************************************************************************
//                          %%% TARGET STATUS VALUES %%%
//***************************************************************************
#define STATUS_GOOD     0x00    // Status Good
#define STATUS_CHKCOND  0x02    // Check Condition
#define STATUS_CONDMET  0x04    // Condition Met
#define STATUS_BUSY     0x08    // Busy
#define STATUS_INTERM   0x10    // Intermediate
#define STATUS_INTCDMET 0x14    // Intermediate-condition met
#define STATUS_RESCONF  0x18    // Reservation conflict
#define STATUS_COMTERM  0x22    // Command Terminated
#define STATUS_QFULL    0x28    // Queue full

//***************************************************************************
//                      %%% SCSI MISCELLANEOUS EQUATES %%%
//***************************************************************************
#define MAXLUN          7       // Maximum Logical Unit Id
#define MAXTARG         7       // Maximum Target Id
#define MAX_SCSI_LUNS   64      // Maximum Number of SCSI LUNs
#define MAX_NUM_HA      8       // Maximum Number of SCSI HA's

//\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
//
//                          %%% SCSI COMMAND OPCODES %%%
//
///\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/

//***************************************************************************
//               %%% Commands for all Device Types %%%
//***************************************************************************
#define SCSI_CHANGE_DEF 0x40    // Change Definition (Optional)
#define SCSI_COMPARE    0x39    // Compare (O)
#define SCSI_COPY       0x18    // Copy (O)
#define SCSI_COP_VERIFY 0x3A    // Copy and Verify (O)
#define SCSI_INQUIRY    0x12    // Inquiry (MANDATORY)
#define SCSI_LOG_SELECT 0x4C    // Log Select (O)
#define SCSI_LOG_SENSE  0x4D    // Log Sense (O)
#define SCSI_MODE_SEL6  0x15    // Mode Select 6-byte (Device Specific)
#define SCSI_MODE_SEL10 0x55    // Mode Select 10-byte (Device Specific)
#define SCSI_MODE_SEN6  0x1A    // Mode Sense 6-byte (Device Specific)
#define SCSI_MODE_SEN10 0x5A    // Mode Sense 10-byte (Device Specific)
#define SCSI_READ_BUFF  0x3C    // Read Buffer (O)
#define SCSI_REQ_SENSE  0x03    // Request Sense (MANDATORY)
#define SCSI_SEND_DIAG  0x1D    // Send Diagnostic (O)
#define SCSI_TST_U_RDY  0x00    // Test Unit Ready (MANDATORY)
#define SCSI_WRITE_BUFF 0x3B    // Write Buffer (O)

//***************************************************************************
//            %%% Commands Unique to Direct Access Devices %%%
//***************************************************************************
#define SCSI_COMPARE    0x39    // Compare (O)
#define SCSI_FORMAT     0x04    // Format Unit (MANDATORY)
#define SCSI_LCK_UN_CAC 0x36    // Lock Unlock Cache (O)
#define SCSI_PREFETCH   0x34    // Prefetch (O)
#define SCSI_MED_REMOVL 0x1E    // Prevent/Allow medium Removal (O)
#define SCSI_READ6      0x08    // Read 6-byte (MANDATORY)
#define SCSI_READ10     0x28    // Read 10-byte (MANDATORY)
#define SCSI_RD_CAPAC   0x25    // Read Capacity (MANDATORY)
#define SCSI_RD_DEFECT  0x37    // Read Defect Data (O)
#define SCSI_READ_LONG  0x3E    // Read Long (O)
#define SCSI_REASS_BLK  0x07    // Reassign Blocks (O)
#define SCSI_RCV_DIAG   0x1C    // Receive Diagnostic Results (O)
#define SCSI_RELEASE    0x17    // Release Unit (MANDATORY)
#define SCSI_REZERO     0x01    // Rezero Unit (O)
#define SCSI_SRCH_DAT_E 0x31    // Search Data Equal (O)
#define SCSI_SRCH_DAT_H 0x30    // Search Data High (O)
#define SCSI_SRCH_DAT_L 0x32    // Search Data Low (O)
#define SCSI_SEEK6      0x0B    // Seek 6-Byte (O)
#define SCSI_SEEK10     0x2B    // Seek 10-Byte (O)
#define SCSI_SEND_DIAG  0x1D    // Send Diagnostics (MANDATORY)
#define SCSI_SET_LIMIT  0x33    // Set Limits (O)
#define SCSI_START_STP  0x1B    // Start/Stop Unit (O)
#define SCSI_SYNC_CACHE 0x35    // Synchronize Cache (O)
#define SCSI_VERIFY     0x2F    // Verify (O)
#define SCSI_WRITE6     0x0A    // Write 6-Byte (MANDATORY)
#define SCSI_WRITE10    0x2A    // Write 10-Byte (MANDATORY)
#define SCSI_WRT_VERIFY 0x2E    // Write and Verify (O)
#define SCSI_WRITE_LONG 0x3F    // Write Long (O)
#define SCSI_WRITE_SAME 0x41    // Write Same (O)

//***************************************************************************
//          %%% Commands Unique to Sequential Access Devices %%%
//***************************************************************************
#define SCSI_ERASE      0x19    // Erase (MANDATORY)
#define SCSI_LOAD_UN    0x1B    // Load/Unload (O)
#define SCSI_LOCATE     0x2B    // Locate (O)
#define SCSI_RD_BLK_LIM 0x05    // Read Block Limits (MANDATORY)
#define SCSI_READ_POS   0x34    // Read Position (O)
#define SCSI_READ_REV   0x0F    // Read Reverse (O)
#define SCSI_REC_BF_DAT 0x14    // Recover Buffer Data (O)
#define SCSI_RESERVE    0x16    // Reserve Unit (MANDATORY)
#define SCSI_REWIND     0x01    // Rewind (MANDATORY)
#define SCSI_SPACE      0x11    // Space (MANDATORY)
#define SCSI_VERIFY_T   0x13    // Verify (Tape) (O)
#define SCSI_WRT_FILE   0x10    // Write Filemarks (MANDATORY)

//***************************************************************************
//                %%% Commands Unique to Printer Devices %%%
//***************************************************************************
#define SCSI_PRINT      0x0A    // Print (MANDATORY)
#define SCSI_SLEW_PNT   0x0B    // Slew and Print (O)
#define SCSI_STOP_PNT   0x1B    // Stop Print (O)
#define SCSI_SYNC_BUFF  0x10    // Synchronize Buffer (O)

//***************************************************************************
//               %%% Commands Unique to Processor Devices %%%
//***************************************************************************
#define SCSI_RECEIVE    0x08        // Receive (O)
#define SCSI_SEND       0x0A        // Send (O)

//***************************************************************************
//              %%% Commands Unique to Write-Once Devices %%%
//***************************************************************************
#define SCSI_MEDIUM_SCN 0x38    // Medium Scan (O)
#define SCSI_SRCHDATE10 0x31    // Search Data Equal 10-Byte (O)
#define SCSI_SRCHDATE12 0xB1    // Search Data Equal 12-Byte (O)
#define SCSI_SRCHDATH10 0x30    // Search Data High 10-Byte (O)
#define SCSI_SRCHDATH12 0xB0    // Search Data High 12-Byte (O)
#define SCSI_SRCHDATL10 0x32    // Search Data Low 10-Byte (O)
#define SCSI_SRCHDATL12 0xB2    // Search Data Low 12-Byte (O)
#define SCSI_SET_LIM_10 0x33    // Set Limits 10-Byte (O)
#define SCSI_SET_LIM_12 0xB3    // Set Limits 10-Byte (O)
#define SCSI_VERIFY10   0x2F    // Verify 10-Byte (O)
#define SCSI_VERIFY12   0xAF    // Verify 12-Byte (O)
#define SCSI_WRITE12    0xAA    // Write 12-Byte (O)
#define SCSI_WRT_VER10  0x2E    // Write and Verify 10-Byte (O)
#define SCSI_WRT_VER12  0xAE    // Write and Verify 12-Byte (O)

//***************************************************************************
//                %%% Commands Unique to CD-ROM Devices %%%
//***************************************************************************
#define SCSI_PLAYAUD_10 0x45    // Play Audio 10-Byte (O)
#define SCSI_PLAYAUD_12 0xA5    // Play Audio 12-Byte 12-Byte (O)
#define SCSI_PLAYAUDMSF 0x47    // Play Audio MSF (O)
#define SCSI_PLAYA_TKIN 0x48    // Play Audio Track/Index (O)
#define SCSI_PLYTKREL10 0x49    // Play Track Relative 10-Byte (O)
#define SCSI_PLYTKREL12 0xA9    // Play Track Relative 12-Byte (O)
#define SCSI_READCDCAP  0x25    // Read CD-ROM Capacity (MANDATORY)
#define SCSI_READHEADER 0x44    // Read Header (O)
#define SCSI_SUBCHANNEL 0x42    // Read Subchannel (O)
#define SCSI_READ_TOC   0x43    // Read TOC (O)

//***************************************************************************
//                %%% Commands Unique to Scanner Devices %%%
//***************************************************************************
#define SCSI_GETDBSTAT  0x34    // Get Data Buffer Status (O)
#define SCSI_GETWINDOW  0x25    // Get Window (O)
#define SCSI_OBJECTPOS  0x31    // Object Postion (O)
#define SCSI_SCAN       0x1B    // Scan (O)
#define SCSI_SETWINDOW  0x24    // Set Window (MANDATORY)

//***************************************************************************
//           %%% Commands Unique to Optical Memory Devices %%%
//***************************************************************************
#define SCSI_UpdateBlk  0x3D    // Update Block (O)

//***************************************************************************
//           %%% Commands Unique to Medium Changer Devices %%%
//***************************************************************************
#define SCSI_EXCHMEDIUM 0xA6    // Exchange Medium (O)
#define SCSI_INITELSTAT 0x07    // Initialize Element Status (O)
#define SCSI_POSTOELEM  0x2B    // Position to Element (O)
#define SCSI_REQ_VE_ADD 0xB5    // Request Volume Element Address (O)
#define SCSI_SENDVOLTAG 0xB6    // Send Volume Tag (O)

//***************************************************************************
//            %%% Commands Unique to Communication Devices %%%
//***************************************************************************
#define SCSI_GET_MSG_6  0x08    // Get Message 6-Byte (MANDATORY)
#define SCSI_GET_MSG_10 0x28    // Get Message 10-Byte (O)
#define SCSI_GET_MSG_12 0xA8    // Get Message 12-Byte (O)
#define SCSI_SND_MSG_6  0x0A    // Send Message 6-Byte (MANDATORY)
#define SCSI_SND_MSG_10 0x2A    // Send Message 10-Byte (O)
#define SCSI_SND_MSG_12 0xAA    // Send Message 12-Byte (O)

//\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
//
//                    %%% END OF SCSI COMMAND OPCODES %%%
//
///\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/

//***************************************************************************
//                      %%% Request Sense Data Format %%%
//***************************************************************************
typedef struct {

    BYTE    ErrorCode;          // Error Code (70H or 71H)
    BYTE    SegmentNum;         // Number of current segment descriptor
    BYTE    SenseKey;           // Sense Key(See bit definitions too)
    BYTE    InfoByte0;          // Information MSB
    BYTE    InfoByte1;          // Information MID
    BYTE    InfoByte2;          // Information MID
    BYTE    InfoByte3;          // Information LSB
    BYTE    AddSenLen;          // Additional Sense Length
    BYTE    ComSpecInf0;        // Command Specific Information MSB
    BYTE    ComSpecInf1;        // Command Specific Information MID
    BYTE    ComSpecInf2;        // Command Specific Information MID
    BYTE    ComSpecInf3;        // Command Specific Information LSB
    BYTE    AddSenseCode;       // Additional Sense Code
    BYTE    AddSenQual;         // Additional Sense Code Qualifier
    BYTE    FieldRepUCode;      // Field Replaceable Unit Code
    BYTE    SenKeySpec15;       // Sense Key Specific 15th byte
    BYTE    SenKeySpec16;       // Sense Key Specific 16th byte
    BYTE    SenKeySpec17;       // Sense Key Specific 17th byte
    BYTE    AddSenseBytes;      // Additional Sense Bytes

} SENSE_DATA_FMT;

//***************************************************************************
//                       %%% REQUEST SENSE ERROR CODE %%%
//***************************************************************************
#define SERROR_CURRENT  0x70    // Current Errors
#define SERROR_DEFERED  0x71    // Deferred Errors

//***************************************************************************
//                   %%% REQUEST SENSE BIT DEFINITIONS %%%
//***************************************************************************
#define SENSE_VALID     0x80    // Byte 0 Bit 7
#define SENSE_FILEMRK   0x80    // Byte 2 Bit 7
#define SENSE_EOM       0x40    // Byte 2 Bit 6
#define SENSE_ILI       0x20    // Byte 2 Bit 5

//***************************************************************************
//               %%% REQUEST SENSE SENSE KEY DEFINITIONS %%%
//***************************************************************************
#define KEY_NOSENSE     0x00    // No Sense
#define KEY_RECERROR    0x01    // Recovered Error
#define KEY_NOTREADY    0x02    // Not Ready
#define KEY_MEDIUMERR   0x03    // Medium Error
#define KEY_HARDERROR   0x04    // Hardware Error
#define KEY_ILLGLREQ    0x05    // Illegal Request
#define KEY_UNITATT     0x06    // Unit Attention
#define KEY_DATAPROT    0x07    // Data Protect
#define KEY_BLANKCHK    0x08    // Blank Check
#define KEY_VENDSPEC    0x09    // Vendor Specific
#define KEY_COPYABORT   0x0A    // Copy Abort
#define KEY_EQUAL       0x0C    // Equal (Search)
#define KEY_VOLOVRFLW   0x0D    // Volume Overflow
#define KEY_MISCOMP     0x0E    // Miscompare (Search)
#define KEY_RESERVED    0x0F    // Reserved

//***************************************************************************
//                %%% PERIPHERAL DEVICE TYPE DEFINITIONS %%%
//***************************************************************************
#define DTYPE_DASD      0x00    // Disk Device
#define DTYPE_SEQD      0x01    // Tape Device
#define DTYPE_PRNT      0x02    // Printer
#define DTYPE_PROC      0x03    // Processor
#define DTYPE_WORM      0x04    // Write-once read-multiple
#define DTYPE_CROM      0x05    // CD-ROM device
#define DTYPE_CDROM     0x05    // CD-ROM device
#define DTYPE_SCAN      0x06    // Scanner device
#define DTYPE_OPTI      0x07    // Optical memory device
#define DTYPE_JUKE      0x08    // Medium Changer device
#define DTYPE_COMM      0x09    // Communications device
#define DTYPE_RESL      0x0A    // Reserved (low)
#define DTYPE_RESH      0x1E    // Reserved (high)
#define DTYPE_UNKNOWN   0x1F    // Unknown or no device type

//***************************************************************************
//                %%% ANSI APPROVED VERSION DEFINITIONS %%%
//***************************************************************************
#define ANSI_MAYBE      0x0     // Device may or may not be ANSI approved stand
#define ANSI_SCSI1      0x1     // Device complies to ANSI X3.131-1986 (SCSI-1)
#define ANSI_SCSI2      0x2     // Device complies to SCSI-2
#define ANSI_RESLO      0x3     // Reserved (low)
#define ANSI_RESHI      0x7     // Reserved (high)


////////////////////////////////////////////////////////////////

typedef struct {
  USHORT Length;
  UCHAR  ScsiStatus;
  UCHAR  PathId;
  UCHAR  TargetId;
  UCHAR  Lun;
  UCHAR  CdbLength;
  UCHAR  SenseInfoLength;
  UCHAR  DataIn;
  ULONG  DataTransferLength;
  ULONG  TimeOutValue;
  ULONG  DataBufferOffset;
  ULONG  SenseInfoOffset;
  UCHAR  Cdb[16];
} SCSI_PASS_THROUGH, *PSCSI_PASS_THROUGH;


typedef struct {
  USHORT Length;
  UCHAR  ScsiStatus;
  UCHAR  PathId;
  UCHAR  TargetId;
  UCHAR  Lun;
  UCHAR  CdbLength;
  UCHAR  SenseInfoLength;
  UCHAR  DataIn;
  ULONG  DataTransferLength;
  ULONG  TimeOutValue;
  PVOID  DataBuffer;
  ULONG  SenseInfoOffset;
  UCHAR  Cdb[16];
} SCSI_PASS_THROUGH_DIRECT, *PSCSI_PASS_THROUGH_DIRECT;


typedef struct {
  SCSI_PASS_THROUGH spt;
  ULONG Filler;
  UCHAR ucSenseBuf[32];
  UCHAR ucDataBuf[512];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;


typedef struct {
  SCSI_PASS_THROUGH_DIRECT spt;
  ULONG Filler;
  UCHAR ucSenseBuf[32];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;



typedef struct {
  UCHAR NumberOfLogicalUnits;
  UCHAR InitiatorBusId;
  ULONG InquiryDataOffset;
} SCSI_BUS_DATA, *PSCSI_BUS_DATA;


typedef struct {
  UCHAR NumberOfBusses;
  SCSI_BUS_DATA BusData[1];
} SCSI_ADAPTER_BUS_INFO, *PSCSI_ADAPTER_BUS_INFO;


typedef struct {
  UCHAR PathId;
  UCHAR TargetId;
  UCHAR Lun;
  BOOLEAN DeviceClaimed;
  ULONG InquiryDataLength;
  ULONG NextInquiryDataOffset;
  UCHAR InquiryData[1];
} SCSI_INQUIRY_DATA, *PSCSI_INQUIRY_DATA;


typedef struct {
  ULONG Length;
  UCHAR PortNumber;
  UCHAR PathId;
  UCHAR TargetId;
  UCHAR Lun;
} SCSI_ADDRESS, *PSCSI_ADDRESS;


/*
 * method codes
 */
#define  METHOD_BUFFERED     0
#define  METHOD_IN_DIRECT    1
#define  METHOD_OUT_DIRECT   2
#define  METHOD_NEITHER      3

/*
 * file access values
 */
#define  FILE_ANY_ACCESS      0
#define  FILE_READ_ACCESS     (0x0001)
#define  FILE_WRITE_ACCESS    (0x0002)


#define IOCTL_SCSI_BASE    0x00000004

/*
 * constants for DataIn member of SCSI_PASS_THROUGH* structures
 */
#define  SCSI_IOCTL_DATA_OUT          0
#define  SCSI_IOCTL_DATA_IN           1
#define  SCSI_IOCTL_DATA_UNSPECIFIED  2

/*
 * Standard IOCTL define
 */
#define CTL_CODE( DevType, Function, Method, Access ) (                 \
    ((DevType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define IOCTL_SCSI_PASS_THROUGH         CTL_CODE( IOCTL_SCSI_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )
#define IOCTL_SCSI_MINIPORT             CTL_CODE( IOCTL_SCSI_BASE, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )
#define IOCTL_SCSI_GET_INQUIRY_DATA     CTL_CODE( IOCTL_SCSI_BASE, 0x0403, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SCSI_GET_CAPABILITIES     CTL_CODE( IOCTL_SCSI_BASE, 0x0404, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SCSI_PASS_THROUGH_DIRECT  CTL_CODE( IOCTL_SCSI_BASE, 0x0405, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )
#define IOCTL_SCSI_GET_ADDRESS          CTL_CODE( IOCTL_SCSI_BASE, 0x0406, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define FILE_DEVICE_MASS_STORAGE        0x0000002d
#define IOCTL_STORAGE_BASE FILE_DEVICE_MASS_STORAGE

#define IOCTL_STORAGE_CHECK_VERIFY     CTL_CODE(IOCTL_STORAGE_BASE, 0x0200, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_CHECK_VERIFY2    CTL_CODE(IOCTL_STORAGE_BASE, 0x0200, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_MEDIA_REMOVAL    CTL_CODE(IOCTL_STORAGE_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_EJECT_MEDIA      CTL_CODE(IOCTL_STORAGE_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_LOAD_MEDIA       CTL_CODE(IOCTL_STORAGE_BASE, 0x0203, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_LOAD_MEDIA2      CTL_CODE(IOCTL_STORAGE_BASE, 0x0203, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_RESERVE          CTL_CODE(IOCTL_STORAGE_BASE, 0x0204, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_RELEASE          CTL_CODE(IOCTL_STORAGE_BASE, 0x0205, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_FIND_NEW_DEVICES CTL_CODE(IOCTL_STORAGE_BASE, 0x0206, METHOD_BUFFERED, FILE_READ_ACCESS)

#define FILE_DEVICE_CD_ROM              0x00000002
#define IOCTL_CDROM_BASE                FILE_DEVICE_CD_ROM
#define IOCTL_CDROM_RAW_READ            CTL_CODE(IOCTL_CDROM_BASE, 0x000F, METHOD_OUT_DIRECT,  FILE_READ_ACCESS)
#define IOCTL_CDROM_READ_Q_CHANNEL      CTL_CODE(IOCTL_CDROM_BASE, 0x000B, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_SEEK_AUDIO_MSF      CTL_CODE(IOCTL_CDROM_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct _CDROM_SEEK_AUDIO_MSF {
    UCHAR M;
    UCHAR S;
    UCHAR F;
} CDROM_SEEK_AUDIO_MSF, *PCDROM_SEEK_AUDIO_MSF;

//
// CD ROM Sub-Q Channel Data Format
//

#define IOCTL_CDROM_SUB_Q_CHANNEL    0x00
#define IOCTL_CDROM_CURRENT_POSITION 0x01
#define IOCTL_CDROM_MEDIA_CATALOG    0x02
#define IOCTL_CDROM_TRACK_ISRC       0x03

typedef struct _CDROM_SUB_Q_DATA_FORMAT {
    UCHAR Format;
    UCHAR Track;
} CDROM_SUB_Q_DATA_FORMAT, *PCDROM_SUB_Q_DATA_FORMAT;

typedef struct _SUB_Q_HEADER {
    UCHAR Reserved;
    UCHAR AudioStatus;
    UCHAR DataLength[2];
} SUB_Q_HEADER, *PSUB_Q_HEADER;

typedef struct _SUB_Q_CURRENT_POSITION {
    SUB_Q_HEADER Header;
    UCHAR FormatCode;
    UCHAR Control : 4;
    UCHAR ADR : 4;
    UCHAR TrackNumber;
    UCHAR IndexNumber;
    UCHAR AbsoluteAddress[4];
    UCHAR TrackRelativeAddress[4];
} SUB_Q_CURRENT_POSITION, *PSUB_Q_CURRENT_POSITION;

typedef struct _SUB_Q_MEDIA_CATALOG_NUMBER {
    SUB_Q_HEADER Header;
    UCHAR FormatCode;
    UCHAR Reserved[3];
    UCHAR Reserved1 : 7;
    UCHAR Mcval : 1;
    UCHAR MediaCatalog[15];
} SUB_Q_MEDIA_CATALOG_NUMBER, *PSUB_Q_MEDIA_CATALOG_NUMBER;

typedef struct _SUB_Q_TRACK_ISRC {
    SUB_Q_HEADER Header;
    UCHAR FormatCode;
    UCHAR Reserved0;
    UCHAR Track;
    UCHAR Reserved1;
    UCHAR Reserved2 : 7;
    UCHAR Tcval : 1;
    UCHAR TrackIsrc[15];
} SUB_Q_TRACK_ISRC, *PSUB_Q_TRACK_ISRC;

typedef union _SUB_Q_CHANNEL_DATA {
    SUB_Q_CURRENT_POSITION CurrentPosition;
    SUB_Q_MEDIA_CATALOG_NUMBER MediaCatalog;
    SUB_Q_TRACK_ISRC TrackIsrc;
} SUB_Q_CHANNEL_DATA, *PSUB_Q_CHANNEL_DATA;


// IOCTL_DISK_SET_CACHE allows the caller to get or set the state of the disk
// read/write caches.
//
// If the structure is provided as the input buffer for the ioctl the read &
// write caches will be enabled or disabled depending on the parameters
// provided.
//
// If the structure is provided as an output buffer for the ioctl the state
// of the read & write caches will be returned. If both input and outut buffers
// are provided the output buffer will contain the cache state BEFORE any
// changes are made


typedef enum {
    EqualPriority,
    KeepPrefetchedData,
    KeepReadData
} DISK_CACHE_RETENTION_PRIORITY;

#define FILE_DEVICE_DISK                0x00000007
#define IOCTL_DISK_BASE                 FILE_DEVICE_DISK
#define IOCTL_DISK_GET_CACHE_INFORMATION CTL_CODE(IOCTL_DISK_BASE, 0x0035, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_SET_CACHE_INFORMATION CTL_CODE(IOCTL_DISK_BASE, 0x0036, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

typedef struct _DISK_CACHE_INFORMATION {

    //
    // on return indicates that the device is capable of saving any parameters
    // in non-volatile storage.  On send indicates that the device should
    // save the state in non-volatile storage.
    //

    BOOLEAN ParametersSavable;

    //
    // Indicates whether the write and read caches are enabled.
    //

    BOOLEAN ReadCacheEnabled;
    BOOLEAN WriteCacheEnabled;

    //
    // Controls the likelyhood of data remaining in the cache depending on how
    // it got there.  Data cached from a READ or WRITE operation may be given
    // higher, lower or equal priority to data entered into the cache for other
    // means (like prefetch)
    //

    DISK_CACHE_RETENTION_PRIORITY ReadRetentionPriority;
    DISK_CACHE_RETENTION_PRIORITY WriteRetentionPriority;

    //
    // Requests for a larger number of blocks than this may have prefetching
    // disabled.  If this value is set to 0 prefetch will be disabled.
    //

    USHORT DisablePrefetchTransferLength;

    //
    // If TRUE then ScalarPrefetch (below) will be valid.  If FALSE then
    // the minimum and maximum values should be treated as a block count
    // (BlockPrefetch)
    //

    BOOLEAN PrefetchScalar;

    //
    // Contains the minimum and maximum amount of data which will be
    // will be prefetched into the cache on a disk operation.  This value
    // may either be a scalar multiplier of the transfer length of the request,
    // or an abolute number of disk blocks.  PrefetchScalar (above) indicates
    // which interpretation is used.
    //

    union {
        struct {
            USHORT Minimum;
            USHORT Maximum;

            //
            // The maximum number of blocks which will be prefetched - useful
            // with the scalar limits to set definite upper limits.
            //

            USHORT MaximumBlocks;
        } ScalarPrefetch;

        struct {
            USHORT Minimum;
            USHORT Maximum;
        } BlockPrefetch;
    };

} DISK_CACHE_INFORMATION, *PDISK_CACHE_INFORMATION;

