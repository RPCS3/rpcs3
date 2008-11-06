// cdvddraft.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "cdvddraft.h"
#include "ps2etypes.h"
#include "ps2edefs.h"
#include "cdvdmisc.h"
#include "config.h"
#include "resource.h"
#include "aboutbox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*************************************************************************/
/* ps2emu cdvddraft history                                              */
/*************************************************************************/
/*                                                                       */
/* v0.62    Initial release                                              */
/* v0.63    Changed CDVDgetTD for new specs                              */
/*                                                                       */
/*************************************************************************/

/*************************************************************************/
/* ps2emu library MFC application placeholder                            */
/*************************************************************************/
class CCdvdDraft : public CWinApp
{
public:
	CCdvdDraft();
};
CCdvdDraft::CCdvdDraft(){}
CCdvdDraft theApp;

/*************************************************************************/
/* ps2emu library versions                                               */
/*************************************************************************/

static char *libraryName     = "PCXS2 (C/DVD) Draft Driver";
const unsigned char version  = PS2E_CDVD_VERSION;
const unsigned char revision = 0;
const unsigned char build    = 64;    

/*************************************************************************/
/* global variables start here                                           */
/*************************************************************************/

static BOOL                 g_library_opened        = FALSE;             // open flag
static BOOL                 g_library_initialized   = FALSE;             // init flag
static int                  g_current_drvtype       = CDVD_DRIVETYPE_CD; // drive type (cd/dvd/cdvd)
static int                  g_current_medtype       = CDVD_MEDIATYPE_CD; // media type (cd/dvd)
static UI08                *g_current_buff          = NULL;              // current buffer pointer
static int                  g_current_buffnum       = -1;                // current buffer number
static TOCDATA              g_current_tocdata       = {0};               // current tocdata;
static DECODEDTRACKINFO     g_current_trkinfo       = {0};               // current trkdata;
static int                  g_request_offset        = 0;                 // offset from base of buffer

CDVD_BUFFER_MODE     g_current_buffmode      = CDVD_BUFFER_SYNC;  // current buffer mode (sync/async)
CDVD_INTERFACE_TYPE  g_current_intftype      = CDVD_INTF_ASPI;    // interface type (aspi/ioctl)
CDVD_READ_MODE       g_current_readmode      = CDVD_READ_MMC;     // current read mode (mmc/scsi10)
int                  g_current_drivenum      = -1;                // currently opened drive number
int                  g_status   [CDVD_NUM_BUFFERS];               // status of read to buffer-n, (srb status, for asynchronous)
HANDLE               g_handle   [CDVD_NUM_BUFFERS];               // handle used to read to buffer-n, (for asynchronous)
int                  g_sectstart[CDVD_NUM_BUFFERS];               // start of sector read to buffer-n 
BOOL                 g_filled   [CDVD_NUM_BUFFERS];               // check if buffer-n was filled

#define CHECKLIBRARYOPENED() if(g_library_opened == FALSE) return CDVD_ERROR_FAIL;
#define SECT2MSF(sect,m,s,f){ (f) =(UI08)(sect%75);(s) =(UI08)(((sect - (f))/75)%60);(m) = (UI08)((((sect - (f))/75)-(s))/60);	}	
  
/*************************************************************************/
/* ps2emu library identifier functions                                   */
/*************************************************************************/
unsigned int CALLBACK PS2EgetLibType() 
{
    return PS2E_LT_CDVD;
}

char *CALLBACK PS2EgetLibName() 
{
	return libraryName;
}

unsigned int CALLBACK PS2EgetLibVersion2(unsigned int type) 
{
	return (version<<16)|(revision<<8)|build;
}

/*************************************************************************/
/* ps2emu config/test   functions                                        */
/*************************************************************************/
//  empty for now
void CALLBACK CDVDconfigure()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CConfig config;
    config.DoModal();
    //::MessageBox(NULL, _T("No Configuration.\n\n"), _T("Config"), MB_OK);
}

// empty for now
void CALLBACK CDVDabout()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CAboutBox about;
    about.DoModal();
}

// return bogus for now
int  CALLBACK CDVDtest()
{
    return CDVD_ERROR_SUCCESS;
}

/*************************************************************************/
/* ps2emu library c/dvd functions                                        */
/*************************************************************************/
// init, normally called once, setup for aspi for now?
int CALLBACK CDVDinit()
{
    if(g_library_initialized)
        return CDVD_ERROR_SUCCESS;

    for(int i=0; i<CDVD_NUM_BUFFERS; i++)
        g_handle[i] = NULL;

    g_library_initialized = TRUE;

    return CDVD_ERROR_SUCCESS;
}

// shutdown, normally called once?
void CALLBACK CDVDshutdown()
{
    if(g_library_initialized)
    {
        cdvd_shutdown();
        g_library_initialized = FALSE;
    }
}

// open, called everytime emulation starts?
int  CALLBACK CDVDopen()
{
    //if(g_library_initialized == FALSE) 
    //    return CDVD_ERROR_FAIL;
    // instead, lets do the Init for lazy emus.
    if(g_library_initialized == FALSE) 
        if(CDVDinit() != CDVD_ERROR_SUCCESS)
            return CDVD_ERROR_FAIL;

    // already opened
    if(g_library_opened) 
        return CDVD_ERROR_SUCCESS;

    //-----------------------------------------
    // LOAD REGISTRY SETTINGS HERE!
    if(load_registrysettings() 
        == CDVD_ERROR_SUCCESS)
    {
        TRACE("CDVDopen: registry loaded successfully.\n");
        // override registry set values for now
        // ioctl doesnt exist yet and although
        // async seems to work fine, havent tested thoroughly
        //g_current_buffmode = CDVD_BUFFER_ASYNC;
        // set interface type
        TRACE("CDVDopen: registry read_mode: %ld\n", g_current_buffmode);
        g_current_intftype = CDVD_INTF_ASPI; 
    }
    else // set some bs values
    {
        TRACE("CDVDopen: unable to load registry\n");
        g_current_drivenum = 0;
        // set to sync
        g_current_buffmode = CDVD_BUFFER_SYNC;
        // set interface type
        g_current_intftype = CDVD_INTF_ASPI;
    }
    //-----------------------------------------

    if(cdvd_init(g_current_intftype) 
        != CDVD_ERROR_SUCCESS)
        return CDVD_ERROR_FAIL;

    int num_drives = cdvd_getnumdrives();   

    if(num_drives == 0) 
        return CDVD_ERROR_FAIL;

    if(cdvd_opendrive(g_current_drivenum) 
        != CDVD_ERROR_SUCCESS)
        return CDVD_ERROR_FAIL;

    if(g_current_readmode == CDVD_READ_UNKNOWN)
    {
        // wait for media ready
        cdvd_testready();
        TRACE("CDVDopen: autodetecting read-mode.\n");
        if((g_current_readmode = (CDVD_READ_MODE) cdvd_detectandsetreadmode()) 
            == CDVD_ERROR_FAIL)
        {
            // try setting to mmc again
            if(cdvd_setreadmode(CDVD_READ_MMC, CDVD_MMC_DATAMODE_RAW)
                != CDVD_ERROR_SUCCESS)
                return CDVD_ERROR_FAIL;
            else
                g_current_readmode = CDVD_READ_MMC;
        }
    }
    else
    {
        TRACE("CDVDopen: setting read-mode: %ld\n", g_current_readmode);
        int mediatype = cdvd_getmediatype();
        int mmc_mode = (mediatype == CDVD_MEDIATYPE_DVD) ? CDVD_MMC_DATAMODE_USER : CDVD_MMC_DATAMODE_RAW;
        if(cdvd_setreadmode(g_current_readmode, mmc_mode)
            == CDVD_ERROR_FAIL)
        {
            return CDVD_ERROR_FAIL;
        }
    }

    g_current_drvtype = cdvd_getdrivetype();
    g_current_medtype = cdvd_getmediatype();

    init_buffersandflags();

    // set to first buffer
    g_current_buffnum = 0;
    cdvd_setcurrentbuffer(0);

    g_library_opened = TRUE;

    return CDVD_ERROR_SUCCESS;
}

// close, called everytime emulation stops?
void CALLBACK CDVDclose()
{
    cdvd_closedrive();
    shut_buffersandflags();
    g_library_opened = FALSE;
}


// readtrack, called during emulation
// it's a mess right now.
int CALLBACK CDVDreadTrack(unsigned int lsn, int mode)
{
    CHECKLIBRARYOPENED();

    int  max_loop    = (g_current_buffmode == CDVD_BUFFER_ASYNC) ? CDVD_NUM_BUFFERS : 1;
    int  rq_sector   = lsn;
    BOOL rq_isinbuff = FALSE;

    //------------------------------------------
    // return a clean buffer when no media present in tray
    // for psemupro compatibility only, since i dont
    // use the pemupro return code definitions :p
    if(g_current_medtype == CDVD_MEDIATYPE_UNKNOWN)
    {
        // lets see if user inserted/initialized a disc afterwards
        if((g_current_medtype = cdvd_getmediatype()) 
            == CDVD_MEDIATYPE_UNKNOWN)
        {
            g_current_buff = cdvd_getbufferaddress(0);;
            g_request_offset = 0;
            memset(g_current_buff, 0, CDVD_SECTOR_SIZE_CD);
            return CDVD_ERROR_SUCCESS;
        }
        else
        {
            cdvd_testready();
            if((g_current_readmode = (CDVD_READ_MODE) cdvd_detectandsetreadmode()) 
                == CDVD_ERROR_FAIL)
                return CDVD_ERROR_FAIL;
        }
    }
    //------------------------------------------

    for(;;)
	{
		rq_isinbuff = FALSE;
		g_current_buffnum = -1;
		for(int i=0; i < max_loop; i++)
		{
			if(rq_sector >= g_sectstart[i] && 
               rq_sector < (g_sectstart[i] + CDVD_NUM_SECTORS_PER_BUFF) 
               && g_sectstart[i] != -1)
			{
				rq_isinbuff = TRUE;
				g_current_buffnum = i;
				break;
			}
		}

		if(rq_isinbuff == TRUE)
		{
            // bufffilled? else 
			if(g_filled[g_current_buffnum] == FALSE) 
			{
				int ret = WaitForSingleObject(g_handle[g_current_buffnum], INFINITE); // ->> check error here!
                if(g_status[g_current_buffnum] == CDVD_ERROR_FAIL ||
                   cdvd_getsrbstatus(g_current_buffnum) != CDVD_SRB_COMPLETED) // there was an error
                {
                    TRACE("CDVDreadTrack: error srb not completed1 stat: %ld, srb: %ld\n", g_status[g_current_buffnum],
                        cdvd_getsrbstatus(g_current_buffnum));
                    TRACE("CDVDreadTrack: waitret: %ld, extended: %ld\n", ret, GetLastError());
                    //-----------------------------------------
                    // one last try, we might get lucky :p
                    cdvd_setcurrentbuffer(g_current_buffnum);
				    g_status[g_current_buffnum] = cdvd_readsector(rq_sector, NULL);
                    if(g_status[g_current_buffnum] != CDVD_ERROR_SUCCESS)
                        return CDVD_ERROR_FAIL;
                    else 
                        return CDVD_ERROR_SUCCESS;
                    //-----------------------------------------
                    //return CDVD_ERROR_FAIL;
                }

    			g_filled[g_current_buffnum] = TRUE;
			}
			
			int sect_in_buff = rq_sector - g_sectstart[g_current_buffnum];

            // okay, just send another async read (prefetch)
            // ONLY supports 2 buffers
            if(g_current_buffmode == CDVD_BUFFER_ASYNC)
            {
                int altbuff = !g_current_buffnum;
			    if(g_sectstart[g_current_buffnum] >  g_sectstart[altbuff])
			    {
                    TRACE("CDVDreadTrack: sending prefetch.\n");
				    WaitForSingleObject(g_handle[altbuff], INFINITE);
				    g_sectstart[altbuff] = g_sectstart[g_current_buffnum] + CDVD_NUM_SECTORS_PER_BUFF;
                    cdvd_setcurrentbuffer(altbuff);
                    _ASSERTE(g_handle[altbuff] != NULL);
				    g_status[altbuff] = cdvd_readsector(g_sectstart[altbuff], &g_handle[altbuff]);
				    g_filled[altbuff] = FALSE;
			    }
            }

            g_current_buff   = cdvd_getbufferaddress(g_current_buffnum);
            static const UI32 mode_pad[] = {0, 12, 24, 24};

            if(g_current_medtype == CDVD_MEDIATYPE_DVD)
            {
			    g_request_offset = sect_in_buff * CDVD_SECTOR_SIZE_DVD; 
            }
            else 
            {
                g_request_offset = (sect_in_buff * CDVD_SECTOR_SIZE_CD) + mode_pad[mode];
            }

			return CDVD_ERROR_SUCCESS;
		}
		else 
		{
			for(int i=0; i < max_loop; i++)
			{
                // wait for any previous srb's pending
                if(g_filled[i] == FALSE && 
                   cdvd_getsrbstatus(i) == CDVD_SRB_PENDING) 
                   WaitForSingleObject(g_handle[i], INFINITE);

                g_sectstart[i] = rq_sector + (CDVD_NUM_SECTORS_PER_BUFF * i);
                cdvd_setcurrentbuffer(i);
                _ASSERTE(g_handle[i] != NULL);
                g_status[i] = cdvd_readsector(g_sectstart[i], &g_handle[i]);
                
                if(g_current_buffmode == CDVD_BUFFER_SYNC)
                {
                    WaitForSingleObject(g_handle[i], INFINITE);
                    if(g_status[g_current_buffnum] == CDVD_ERROR_FAIL ||
                       cdvd_getsrbstatus(i) != CDVD_SRB_COMPLETED)
                    {
                        TRACE("CDVDreadTrack: error srb not completed1 stat: %ld, srb: %ld\n", g_status[i],
                            cdvd_getsrbstatus(i));
                        return CDVD_ERROR_FAIL;
                    }
                    g_filled[i] = TRUE;
                }
                else
                {
                    g_filled[i] = FALSE;
                }
			}
		}
	}

	return CDVD_ERROR_SUCCESS;
}

// getbuffer, retrieve previously read sector buffer
unsigned char *CALLBACK CDVDgetBuffer()
{
    return (g_current_buff + g_request_offset);
}

s32  CALLBACK CDVDreadSubQ(u32 lsn, cdvdSubQ* subq) {
	// fake it
	u8 min, sec, frm;
	subq->ctrl		= 4;
	subq->mode		= 1;
	subq->trackNum	= itob(1);
	subq->trackIndex= itob(1);
	
	lba_to_msf(lsn, &min, &sec, &frm);
	subq->trackM	= itob(min);
	subq->trackS	= itob(sec);
	subq->trackF	= itob(frm);
	
	subq->pad		= 0;
	
	lba_to_msf(lsn + (2*75), &min, &sec, &frm);
	subq->discM		= itob(min);
	subq->discS		= itob(sec);
	subq->discF		= itob(frm);
	return 0;
}

// retrieve track number
int  CALLBACK CDVDgetTN(cdvdTN *buffer)
{
    CHECKLIBRARYOPENED();

    if(cdvd_gettoc() != CDVD_ERROR_SUCCESS)
        return CDVD_ERROR_FAIL;

    g_current_tocdata = cdvd_gettocdata();

    if(g_current_tocdata.first_track_num == 0 ||
       g_current_tocdata.last_track_num  == 0)
    {
        //fail? then lie ;P
        buffer->strack = 1;
        buffer->etrack = 1;
        return CDVD_ERROR_SUCCESS; // TODO: or fail???
    }
    
    buffer->strack = g_current_tocdata.first_track_num;
    buffer->etrack = g_current_tocdata.last_track_num;

    //printf("s: %ld, e: %ld\n", buffer->strack, buffer->etrack);
    return CDVD_ERROR_SUCCESS;
}

// retrives total sects OR offset of track
int  CALLBACK CDVDgetTD(unsigned char track, cdvdTD *buffer)
{
    CHECKLIBRARYOPENED();

    if(track == 0)
    {
		buffer->lsn = cdvd_getnumsectors();
        buffer->type = 0x00; // well, dunno if its even needed here.

        return CDVD_ERROR_SUCCESS;
    }

    if(track > 0xAA) // max tracks per session 
        return CDVD_ERROR_FAIL;
    if(cdvd_gettoc() != CDVD_ERROR_SUCCESS)
        return CDVD_ERROR_FAIL;
        
    g_current_trkinfo = cdvd_gettrackdetail();
    
    // okay, not sure if you need to add a 0:2:0 lead-in for cds?
    buffer->lsn = g_current_trkinfo.track_offset[track-1];

    switch(g_current_medtype)
    {
    case CDVD_MEDIATYPE_DVD: 
        buffer->type = CDVD_MODE1_TRACK; // always data track
        break;  
    case CDVD_MEDIATYPE_CD:  
        switch(g_current_trkinfo.track_type[track-1])
        {
            case 0x01: buffer->type = CDVD_MODE1_TRACK; break;
            case 0x02: buffer->type = CDVD_MODE2_TRACK; break;
            default: buffer->type = CDVD_AUDIO_TRACK; break; // for 0x00 and everything else
        }
        break;
    default: 
        buffer->type = CDVD_AUDIO_TRACK; // lets lie.
        break;
    }

    return CDVD_ERROR_SUCCESS; //g_current_trkinfo.track_offset[track-1];
}

s32  CALLBACK CDVDgetTOC(void* toc) {
	u8 type = CDVDgetDiskType();
	u8* tocBuff = (u8*)toc;
	
	if(	type == CDVD_TYPE_DVDV ||
		type == CDVD_TYPE_PS2DVD)
	{
		// get dvd structure format
		// scsi command 0x43
		memset(tocBuff, 0, 2048);
		// fake it
		tocBuff[ 0] = 0x04;
		tocBuff[ 1] = 0x02;
		tocBuff[ 2] = 0xF2;
		tocBuff[ 3] = 0x00;
		tocBuff[ 4] = 0x86;
		tocBuff[ 5] = 0x72;

		tocBuff[16] = 0x00;
		tocBuff[17] = 0x03;
		tocBuff[18] = 0x00;
		tocBuff[19] = 0x00;
	}
	else if(type == CDVD_TYPE_CDDA ||
			type == CDVD_TYPE_PS2CDDA ||
			type == CDVD_TYPE_PS2CD ||
			type == CDVD_TYPE_PSCDDA ||
			type == CDVD_TYPE_PSCD)
	{
		// cd toc
		// (could be replaced by 1 command that reads the full toc)
		u8 min, sec, frm;
		s32 i, err;
		cdvdTN diskInfo;
		cdvdTD trackInfo;
		memset(tocBuff, 0, 1024);
		if (CDVDgetTN(&diskInfo) == -1)	{ diskInfo.etrack = 0;diskInfo.strack = 1; }
		if (CDVDgetTD(0, &trackInfo) == -1) trackInfo.lsn = 0;
		
		tocBuff[0] = 0x41;
		tocBuff[1] = 0x00;
		
		//Number of FirstTrack
		tocBuff[2] = 0xA0;
		tocBuff[7] = itob(diskInfo.strack);
		
		//Number of LastTrack
		tocBuff[12] = 0xA1;
		tocBuff[17] = itob(diskInfo.etrack);

		//DiskLength
		lba_to_msf(trackInfo.lsn, &min, &sec, &frm);
		tocBuff[22] = 0xA2;
		tocBuff[27] = itob(min);
		tocBuff[28] = itob(sec);
		
		for (i=diskInfo.strack; i<=diskInfo.etrack; i++)
		{
			err = CDVDgetTD(i, &trackInfo);
			lba_to_msf(trackInfo.lsn, &min, &sec, &frm);
			tocBuff[i*10+30] = trackInfo.type;
			tocBuff[i*10+32] = err == -1 ? 0 : itob(i);	  //number
			tocBuff[i*10+37] = itob(min);
			tocBuff[i*10+38] = itob(sec);
			tocBuff[i*10+39] = itob(frm);
		}
	}
	else
		return -1;
	
	return 0;
}

// return bogus data for now
int  CALLBACK CDVDgetDiskType()
{
    CHECKLIBRARYOPENED();

    int mediatype = cdvd_getmediatype();

    // TODO: need to retrieve dvd subtype here
    if(mediatype == CDVD_MEDIATYPE_DVD)
        return CDVD_TYPE_PS2DVD;

    // TODO: need to retrieve cd subtype here
    if(mediatype == CDVD_MEDIATYPE_CD)
        return CDVD_TYPE_PS2CD;

    return CDVD_TYPE_UNKNOWN;
}

// return bogus data for now
int  CALLBACK CDVDgetTrayStatus()
{
    CHECKLIBRARYOPENED();

    return CDVD_TRAY_CLOSE;
}

s32  CALLBACK CDVDctrlTrayOpen() {
	return 0;
}
s32  CALLBACK CDVDctrlTrayClose() {
	return 0;
}

