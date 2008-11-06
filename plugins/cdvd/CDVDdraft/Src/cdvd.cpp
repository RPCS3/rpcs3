// Cdvd.cpp: implementation of the CCdvd class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Cdvd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#if defined(CDVD_SIMPLE_INTERFACE)
// our fuckup method of interfacing ;p
CCdvd             OBJECT_HOLDER_CDVD;
CDVD_INTF_FUNC_T1 FUNCTION_GETNUMSECTORS;
CDVD_INTF_FUNC_T1 FUNCTION_GETTOC;
CDVD_INTF_FUNC_T1 FUNCTION_TESTREADY;
CDVD_INTF_FUNC_T2 FUNCTION_SETSPEED;
CDVD_INTF_FUNC_T1 FUNCTION_STOP;
CDVD_INTF_FUNC_T2 FUNCTION_SETSECTORSIZE;
CDVD_INTF_FUNC_T3 FUNCTION_PLAY;
CDVD_INTF_FUNC_T4 FUNCTION_READSECTOR;

#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCdvd::CCdvd()
{
    m_hIoctlDrv         = NULL;
    m_hAspi             = NULL;
    m_bAspiInitialized  = FALSE;
    m_bIoctlInitialized = FALSE;
    m_bDriveOpened      = FALSE;  
    m_nCurrentDrive     = -1;
    m_nCurrentBuffer    = 0;
    m_nDrives           = 0;
    m_IntfType          = CDVD_INTF_UNKNOWN;
    m_nMmcDataMode      = CDVD_MMC_DATAMODE_RAW;
    m_nCurrentReadMode  = CDVD_READ_MMC;
    
    memset(m_drvDetails,    0, CDVD_MAX_SUPPORTED_DRIVES * sizeof(ADAPTERINFO));
    memset(m_srbReadCd,     0, CDVD_NUM_BUFFERS * sizeof(SRB_ExecSCSICmd));
    memset(m_srbRead10,     0, CDVD_NUM_BUFFERS * sizeof(SRB_ExecSCSICmd));     
    memset(m_srbReadMat,    0, CDVD_NUM_BUFFERS * sizeof(SRB_ExecSCSICmd));     
    memset(m_srbReadSony,   0, CDVD_NUM_BUFFERS * sizeof(SRB_ExecSCSICmd));     
    memset(m_srbReadNec,    0, CDVD_NUM_BUFFERS * sizeof(SRB_ExecSCSICmd));  
    memset(m_sptwbReadCd,   0, CDVD_NUM_BUFFERS * sizeof(SPTD_WITH_SENSE_BUFF));
    memset(m_sptwbRead10,   0, CDVD_NUM_BUFFERS * sizeof(SPTD_WITH_SENSE_BUFF));
    memset(m_sptwbReadMat,  0, CDVD_NUM_BUFFERS * sizeof(SPTD_WITH_SENSE_BUFF));
    memset(m_sptwbReadSony, 0, CDVD_NUM_BUFFERS * sizeof(SPTD_WITH_SENSE_BUFF));
    memset(m_sptwbReadNec,  0, CDVD_NUM_BUFFERS * sizeof(SPTD_WITH_SENSE_BUFF));
    
    memset(&m_tocDetails, 0, sizeof(TOCDATA));

    GetNumSectors = Dummy_T1;
    GetToc        = Dummy_T1;
    Stop          = Dummy_T1;
    Play          = Dummy_T3;
    TestReady     = Dummy_T1;
//  GetHeader     = Dummy_T1;
    SetSpeed      = Dummy_T2;
    SetSectorSize = Dummy_T2;
    ReadSector    = Dummy_T4;

    UpdateInterfaceObject();
}

CCdvd::~CCdvd()
{
    CloseDrive();
    Shutdown();
}

// based on msdn example
WIN32OSTYPE CCdvd::GetWin32OSType()
{
	WIN32OSTYPE wintype;
	UI32 version;
	OSVERSIONINFO *osvi;
	
	version = GetVersion();
	if(version < 0x80000000)
    {
		osvi = (OSVERSIONINFO *) malloc(sizeof(OSVERSIONINFO));
        if (osvi)
        {
			memset(osvi, 0, sizeof(OSVERSIONINFO));
			osvi->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionEx(osvi);
			if (osvi->dwMajorVersion >= 4L)
				wintype = WINNTNEW;
            else
                wintype = WINNTOLD;

			free(osvi);
		}
	}
	else if  (LOBYTE(LOWORD(version)) < 4L)
		wintype = WIN32S; 	
    else
	    wintype = WIN95;

    return wintype;
}

// Initializes an interface, only one should be initialized. :)
int CCdvd::Init(CDVD_INTERFACE_TYPE intf_type)
{
    int retval = CDVD_ERROR_SUCCESS;

    switch(intf_type)
    {
    case CDVD_INTF_ASPI:
         m_IntfType = CDVD_INTF_ASPI;
         Aspi_Init();
         break;
    //case CDVD_INTF_IOCTL:
    //     m_IntfType = CDVD_INTF_IOCTL;
    //     Ioctl_Init();
    //     break;
    case CDVD_INTF_UNKNOWN:
         if(m_bAspiInitialized) 
             retval = CDVD_INTF_ASPI;
         else 
             if(m_bIoctlInitialized) retval = CDVD_INTF_IOCTL;
         else 
             retval = CDVD_INTF_UNKNOWN;
         break;

    default:
         retval = CDVD_ERROR_FAIL;
         break;
    }

    return retval;
}

// shutsdown previously opened interface
int CCdvd::Shutdown()
{
    int retval = CDVD_ERROR_SUCCESS;
    switch(m_IntfType)
    {
    case CDVD_INTF_ASPI:
         Aspi_Shutdown();
         break;
    //case CDVD_INTF_IOCTL:
    //     Ioctl_Shutdown();
    //     break;
    default:  break;
    }

    m_IntfType = CDVD_INTF_UNKNOWN;

    return retval;
}

// retrieve number of c/dvd drives
int CCdvd::GetNumDrives() const
{
    return m_nDrives;
}

// open a drive for reading
int CCdvd::OpenDrive(int drv_num)
{
    int retval = CDVD_ERROR_FAIL;
    _ASSERT(drv_num < CDVD_MAX_SUPPORTED_DRIVES);
    
    // run-time check, for now.
    if(drv_num > (int) m_nDrives) 
        return CDVD_ERROR_FAIL;

    m_bDriveOpened = FALSE;
    switch(m_IntfType)
    {
    case CDVD_INTF_ASPI:
         if(Aspi_OpenDrive(drv_num) == CDVD_ERROR_SUCCESS)
         {
             m_bDriveOpened = TRUE;
             retval = CDVD_ERROR_SUCCESS;
         }
         break;
    //case CDVD_INTF_IOCTL:
    //    if(Ioctl_OpenDrive(drv_num) == CDVD_ERROR_SUCCESS)
    //    {
    //        m_bDriveOpened = TRUE;
    //        retval = CDVD_ERROR_SUCCESS;
    //    }
    default: break;
    }

    return retval;
}

// close a previously opened drive
int CCdvd::CloseDrive()
{
    int retval = CDVD_ERROR_SUCCESS;
    switch(m_IntfType)
    {
    case CDVD_INTF_ASPI:  Aspi_CloseDrive(); break;
    //case CDVD_INTF_IOCTL: Ioctl_CloseDrive(); break;
    default: break;
    }

    return retval;
}

// allocate and initialize buffers for BOTH aspi and ioctl
int CCdvd::InitializeBuffers()
{
    int retval = CDVD_ERROR_SUCCESS;
    UI32 result = 0xFFFFFFFF;    
    for(int i=0; i < CDVD_NUM_BUFFERS; i++)
    {
        if(m_ReadBuffer[i].AB_BufPointer != NULL) continue;
        m_ReadBuffer[i].AB_BufLen = CDVD_NUM_SECTORS_PER_BUFF * CDVD_SECTOR_SIZE_CD;
        m_ReadBuffer[i].AB_BufPointer = (PBYTE) VirtualAlloc
        (
            NULL,
            m_ReadBuffer[i].AB_BufLen,
            MEM_COMMIT,
            PAGE_READWRITE
        );

        result &= (UI32) m_ReadBuffer[i].AB_BufPointer;
        if(m_ReadBuffer[i].AB_BufPointer != NULL)
            memset(m_ReadBuffer[i].AB_BufPointer, 0, m_ReadBuffer[i].AB_BufLen);
    }

    if(result == 0)
    {
        ShutdownBuffers();
        retval = CDVD_ERROR_FAIL;
        TRACE("unable to allocate all buffers\n");
    }

    return retval;
}

// free-up previously allocated buffers
int CCdvd::ShutdownBuffers()
{
    int retval = CDVD_ERROR_SUCCESS;
    
    for(int i=0; i < CDVD_NUM_BUFFERS; i++)
    {
        if(m_ReadBuffer[i].AB_BufPointer != NULL)
            VirtualFree( m_ReadBuffer[i].AB_BufPointer, 0, MEM_RELEASE );
        m_ReadBuffer[i].AB_BufPointer = NULL;
        m_ReadBuffer[i].AB_BufLen = 0;
    }

    return retval;
}

// set the current buffer to be used
// only 2! at the moment
int CCdvd::SetCurrentBuffer(int buf_num)
{
    int retval = CDVD_ERROR_SUCCESS;
    if(buf_num >= CDVD_NUM_BUFFERS) 
        return CDVD_ERROR_FAIL;
    
    m_nCurrentBuffer = buf_num;

    return retval;
}

// this is terribly WRONG :P, but i dont care, works most of the time :p
int CCdvd::ExtractTocData(UI08 *buffer)
{
    int retval = CDVD_ERROR_SUCCESS;
    LPTOCDATA pdata = (LPTOCDATA) buffer;
    
    memcpy(&m_tocDetails, buffer, sizeof(TOCDATA));

    int total_tracks = 0;
    int mediatype = GetCdvdMediaType();

    UI32 numRead = ((UI32) pdata->datalen_byte1) << 8 | (UI32) (pdata->datalen_byte0);
    SetCurrentBuffer(0);
    
    memset(&m_trkDetails, 0, sizeof(DECODEDTRACKINFO));

    int offset = 4;
    for(int i = 0; i < pdata->last_track_num; i++)
    {
        offset = i*8+4;
        LPTOCTRACKDESCRIPTOR pdesc = (LPTOCTRACKDESCRIPTOR) (buffer + offset);   
 
        UI32 start_track = ((UI32) pdesc->lba_byte3) << 24 |
                           ((UI32) pdesc->lba_byte2) << 16 |
				           ((UI32) pdesc->lba_byte1) << 8  |
				            (UI32) pdesc->lba_byte0;

        // track types, 0x00 - cdda, 0x01 - data, 0x02 - cdxa
        UI08 tracktype = 0;
        if(mediatype == CDVD_MEDIATYPE_DVD)
        {
            tracktype = 0x01; // data sector
        }
        else if(((this)->*(ReadSector))(start_track, NULL) == CDVD_ERROR_SUCCESS)
        { 
             tracktype = (UI08) * (m_ReadBuffer[m_nCurrentBuffer].AB_BufPointer+15);
        }
        else
        {
            // skip setting info for this track, and try to proceed with others
            ++total_tracks;
            continue;
        }
        // okay, something is fucked.. just lie and say its a cdda :p
        if(tracktype > 2) tracktype = 0; 
       
        m_trkDetails.track_offset[i] = start_track;
        m_trkDetails.track_type[i]   = tracktype;
        
        TRACE("track %ld -> start_at: %ld with type: %ld\n", i, start_track, tracktype);
        ++total_tracks;
    }
    
    if(total_tracks) m_trkDetails.total_tracks = total_tracks;

    return retval;
}

// set read mode (i.e. mmc, scsi10 etc).
// Note: only mmc and scsi10 is enabled
//       i've disabled all goddamn proprietary read modes 
//       on this version (sony, nec, matsushita)
int CCdvd::SetReadMode(CDVD_READ_MODE read_mode, int data_mode)
{
    _ASSERT(read_mode >= CDVD_READ_MMC && read_mode <= CDVD_READ_SCSI10);

    m_nCurrentReadMode = read_mode;
    m_nMmcDataMode     = data_mode;

    int retval = CDVD_ERROR_SUCCESS;
    switch(m_IntfType)
    {
    case CDVD_INTF_ASPI:
         retval = Aspi_SetReadMode(read_mode);
         break;
    //case CDVD_INTF_IOCTL:
    //   retval = Ioctl_SetReadMode();
    //   break;
    default: 
        retval = CDVD_ERROR_FAIL;
        break;
    }

    return CDVD_ERROR_SUCCESS;
}

// returns CDVD_ERROR_PENDING if SS_PENDING, CDVD_ERROR_SUCCESS if SS_COMP
// and CDVD_ERROR_FAIL if you are a dumbass and you haven't set any read_mode
int CCdvd::GetSrbStatus(int srb_num)
{
    int retval = CDVD_ERROR_SUCCESS;
    switch(m_IntfType)
    {
    case CDVD_INTF_ASPI:
         retval = Aspi_GetSrbStatus(srb_num);
         break;
    //case CDVD_INTF_IOCTL:
    //   retval = Ioctl_GetSrbStatus(srb_num);
    //   break;
    default: 
        retval = CDVD_ERROR_FAIL;
        break;
    }

    return retval;
}

int CCdvd::GetCdvdConfiguration(UI16 feature, UI08 *dest, UI32 dest_size)
{
    int retval = CDVD_ERROR_SUCCESS;
    switch(m_IntfType)
    {
    case CDVD_INTF_ASPI:
         retval = Aspi_GetConfiguration(feature, dest, dest_size);
         break;
    //case CDVD_INTF_IOCTL:
    //   retval = Ioctl_GetConfiguration(feature, dest, dest_size);
    //   break;
    default: 
        retval = CDVD_ERROR_FAIL;
        break;
    }

    return retval;
}

const UI16 CDVD_FEATURE_READCD  = 0x001E;
const UI16 CDVD_FEATURE_READDVD = 0x001F;

// as the name says
int CCdvd::GetCdvdDriveType() 
{   
    int drivetype = CDVD_DRIVETYPE_UNKNOWN;
    UI08 buffer[1024];
    UI16 feature_available = 0;
    BOOL is_cdromdrive = FALSE;
    BOOL is_dvddrive   = FALSE;

    if(GetCdvdConfiguration(CDVD_FEATURE_READCD, buffer, sizeof(buffer)) 
        == CDVD_ERROR_SUCCESS)
    {
        feature_available = (UI16) buffer[8] << 8 | (UI16) buffer[9];
        if(feature_available == CDVD_FEATURE_READCD)
        {
            is_cdromdrive = TRUE;
        }
    }

    if(GetCdvdConfiguration(CDVD_FEATURE_READDVD, buffer, sizeof(buffer)) 
        == CDVD_ERROR_SUCCESS)
    {
        feature_available = (UI16) buffer[8] << 8 | (UI16) buffer[9];
        if(feature_available == CDVD_FEATURE_READDVD)
        {
            is_dvddrive = TRUE;
        }
    }

    if(is_dvddrive && is_cdromdrive)  drivetype = CDVD_DRIVETYPE_CDVD;
    if(is_dvddrive && !is_cdromdrive) drivetype = CDVD_DRIVETYPE_DVD;
    if(!is_dvddrive && is_cdromdrive) drivetype = CDVD_DRIVETYPE_CD;

    return drivetype;
}

// as the naem says
int CCdvd::GetCdvdMediaType()
{
    int mediatype = CDVD_MEDIATYPE_UNKNOWN;
    UI08 buffer[1024];
    UI08 media;

    if(GetCdvdConfiguration(CDVD_FEATURE_READCD, buffer, sizeof(buffer)) 
        == CDVD_ERROR_SUCCESS)
    {
        media = buffer[10] & 0x01; // current loaded media is a cdrom if "Current" bit is on
        if(media)
        {
            return CDVD_MEDIATYPE_CD;
        }
    }

    if(GetCdvdConfiguration(CDVD_FEATURE_READDVD, buffer, sizeof(buffer)) 
        == CDVD_ERROR_SUCCESS)
    {
        media = buffer[10] & 0x01; // current loaded media is a dvd if "Current" bit is on
        if(media)
        {
            return CDVD_MEDIATYPE_DVD;
        }
    }

    return mediatype;
}

// automatically detect read_mode as well as set it.
int CCdvd::DetectAndSetReadMode()
{
#define CHECKMODE(m) memset(m_ReadBuffer[m_nCurrentBuffer].AB_BufPointer, 0xCC, m_ReadBuffer[m_nCurrentBuffer].AB_BufLen); \
        SetReadMode((m), m_nMmcDataMode);\
        if(((this)->*(ReadSector))(0, NULL) == CDVD_ERROR_SUCCESS){\
        if(*((UI32*) m_ReadBuffer[m_nCurrentBuffer].AB_BufPointer) != 0xCCCCCCCC){\
            TRACE("data: %08Xh, detected read mode: %ld\n", *((UI32*) m_ReadBuffer[m_nCurrentBuffer].AB_BufPointer), (m));\
                return (m);\
            }\
        }

    int retval = CDVD_ERROR_FAIL;
    if(GetCdvdDriveType() != CDVD_DRIVETYPE_UNKNOWN)
    {
        int mediatype = GetCdvdMediaType();
        if(mediatype != CDVD_MEDIATYPE_UNKNOWN) // no freaking media present dipshit
        {
            if(mediatype == CDVD_MEDIATYPE_DVD)
            {
                m_nMmcDataMode = CDVD_MMC_DATAMODE_USER;
            }
            else if(mediatype == CDVD_MEDIATYPE_CD)
            {
                m_nMmcDataMode = CDVD_MMC_DATAMODE_RAW;
            }

            // start checking which one will work;

            SetCurrentBuffer(0); 

            // lets try scsi10 first, since scsi10/scsi12 is mandatory for dvd's
            if(((this)->*(SetSectorSize))((m_nMmcDataMode == CDVD_MMC_DATAMODE_RAW) ? 
                CDVD_SECTOR_SIZE_CD : CDVD_SECTOR_SIZE_DVD) == CDVD_ERROR_SUCCESS)
            {
                CHECKMODE(CDVD_READ_SCSI10);
            }
            
            // then check for mmc
            CHECKMODE(CDVD_READ_MMC);
            // mmc doesnt work? lets try the others
            
            TRACE("cdvdlib: failed to retrieve mode. \n");
            // disabled all other read_modes for now.
            //CHECKMODE(CDVD_READ_D8);
            //CHECKMODE(CDVD_READ_D410);
            //CHECKMODE(CDVD_READ_D412);
        }
    }

    return retval;
}

// update the global interface for some magic
void CCdvd::UpdateInterfaceObject()
{
#if defined(CDVD_SIMPLE_INTERFACE)

    FUNCTION_GETNUMSECTORS = GetNumSectors;
    FUNCTION_GETTOC        = GetToc;
    FUNCTION_TESTREADY     = TestReady;
    FUNCTION_SETSPEED      = SetSpeed;
    FUNCTION_STOP          = Stop;
    FUNCTION_SETSECTORSIZE = SetSectorSize;
    FUNCTION_PLAY          = Play;
    FUNCTION_READSECTOR    = ReadSector;

#endif

}

TOCDATA CCdvd::GetTocData()
{   
    TOCDATA dummy;

    if(m_nCurrentDrive == -1)
    {
        memset(&dummy, 0, sizeof(TOCDATA));
        return dummy;
    }

    return m_tocDetails;
}

DECODEDTRACKINFO CCdvd::GetTrackDetail()
{   
    DECODEDTRACKINFO dummy;

    if(m_nCurrentDrive == -1)
    {
        memset(&dummy, 0, sizeof(DECODEDTRACKINFO));
        return dummy;
    }

    return m_trkDetails;
}

UI08 *CCdvd::GetBufferAddress(int buff_num) const
{
    _ASSERT(buff_num < CDVD_NUM_BUFFERS);

    return m_ReadBuffer[buff_num].AB_BufPointer;
}

ADAPTERINFO CCdvd::GetAdapterDetail(int drv_num)
{
    _ASSERT(drv_num < CDVD_MAX_SUPPORTED_DRIVES);
    ADAPTERINFO dummy;
  
    if(m_nDrives == 0) 
    {
        memset(&dummy, 0, sizeof(ADAPTERINFO));
        return dummy;
    }

    return m_drvDetails[drv_num];
}


/***********************************************************/
/*              DUMMY C/DVD Methods                        */
/***********************************************************/
int CCdvd::Dummy_T1()
{ 
    TRACE("cdvdlib: dummy_t1\n");
    return CDVD_ERROR_UNINITIALIZED; 
}

int CCdvd::Dummy_T2(int a)
{ 
    TRACE("cdvdlib: dummy_t2\n");
    return CDVD_ERROR_UNINITIALIZED; 
}

int CCdvd::Dummy_T3(int a, int b)
{ 
    TRACE("cdvdlib: dummy_t3\n");
    return CDVD_ERROR_UNINITIALIZED; 
}

int CCdvd::Dummy_T4(int a, HANDLE *b)
{ 
    TRACE("cdvdlib: dummy_t4\n");
    return CDVD_ERROR_UNINITIALIZED; 
}

