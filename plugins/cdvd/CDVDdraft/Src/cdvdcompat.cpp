// cdvdcompat.cpp: implementation of the cdvdcompat class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cdvddraft.h"
#include "cdvdcompat.h"
#include "cdvdmisc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// test cdrom-from old interface ;p                                      //
// added for psemupro compatibility and testing only!                    //
// although, its seems more stable than my old one hehe..                //
///////////////////////////////////////////////////////////////////////////

static char *libraryName     = "PSEmuPro (C/DVD) Compatibility Driver";
const unsigned char version  = 1;
const unsigned char revision = 0;
const unsigned char build    = 22;    

#define	INT2BCD(n)		    ((n)/10)*16 + (n)%10
#define BCD2INT(n)		    ((n & 0x0F) + 10 * ( (n & 0xF0) >> 4))
#define MSF2SECT(m,s,f)		(((m)*60 + (s) - 2)*75 + (f) )
#define SECT2MSF(sect,m,s,f){ (f) =(UI08)(sect%75);(s) =(UI08)(((sect - (f))/75)%60);(m) = (UI08)((((sect - (f))/75)-(s))/60);	}	
#define NORMALIZE(m,s,f)    { while((f) >= 75){ (f) -= 75; (s) += 1; } while((s) >= 60){ (s) -= 60; (m) += 1;} }

/*************************************************************************/
/* psemupro library identifier functions                                 */
/*************************************************************************/
char * CALLBACK PSEgetLibName()
{
    return libraryName;
}

unsigned int CALLBACK PSEgetLibType()
{
    return 1; // PSE_LT_CDR
}

unsigned int CALLBACK PSEgetLibVersion()
{
    return version<<16|revision<<8|build;
}

/*************************************************************************/
/* psemupro config/test   functions                                      */
/*************************************************************************/

int CALLBACK CDRconfigure(void)
{
    CDVDconfigure();
    return CDVD_ERROR_SUCCESS;
}

void CALLBACK CDRabout(void)
{
    CDVDabout();
}

int CALLBACK CDRtest(void)
{
    return CDVDtest();
}

/*************************************************************************/
/* psemupro library init/shutdown functions                              */
/*************************************************************************/

int CALLBACK CDRinit()
{
	return CDVDinit();
}

int CALLBACK CDRshutdown()
{
    CDVDshutdown();
    return CDVD_ERROR_SUCCESS;
}

int CALLBACK CDRopen()
{
    return CDVDopen();
}

int CALLBACK CDRclose()
{
    CDVDclose();
    return CDVD_ERROR_SUCCESS;
}

/*************************************************************************/
/* psemupro library cdrom functions                                      */
/*************************************************************************/

int CALLBACK CDRgetTN(cdvdTN *buffer)
{
    return CDVDgetTN(buffer);            
}

int CALLBACK CDRgetTD(unsigned char track, unsigned char *buffer)
{
/*    cdvdTD td;

    int retval = CDVDgetTD(track, &td);

    if(retval == CDVD_ERROR_FAIL)
        return CDVD_ERROR_FAIL;

    //SECT2MSF(retval, m, s, f);
    
    NORMALIZE(td.minute, td.second, td.frame);

	buffer[0] = INT2BCD(td.minute);
    buffer[2] = INT2BCD(td.frame);

    if(track == 0)
    {
		buffer[1] = INT2BCD(td.second);
    }
    else
    {
        // (add 0:2:0 to convert to pysical addr)
		buffer[1] = INT2BCD(td.second+2);
    }*/

    return CDVD_ERROR_SUCCESS;
}

int CALLBACK CDRreadTrack(unsigned char *time)
{
    int lsn = MSF2SECT(BCD2INT(time[0]), BCD2INT(time[1]), BCD2INT(time[2]));
    
    return CDVDreadTrack(lsn, CDVD_MODE_2352);
}

unsigned char *CALLBACK CDRgetBuffer(void)
{
    return (CDVDgetBuffer() + 12);
}

// heck if i know if this works -_-
int CALLBACK CDRplay(unsigned char *sector)
{
    flush_all();
    int sect = MSF2SECT(sector[0], sector[1], sector[2]);
    cdvd_play(sect, 0xFFFFFF);

    return CDVD_ERROR_SUCCESS;
}

int CALLBACK CDRstop(void)
{
    flush_all();
    cdvd_stop();

    return CDVD_ERROR_SUCCESS;
}

/*************************************************************************/
/* psemupro library extended functions                                   */
/*************************************************************************/
typedef struct tagCDRSTAT
{
 unsigned long Type;
 unsigned long Status;
 unsigned char Time[3]; // current playing time

} CDRSTAT, *LPCDRSTAT;

int CALLBACK CDRgetStatus(LPCDRSTAT stat) 
{
    memset(stat, 0, sizeof(CDRSTAT));

    stat->Type = 0x01; //always data for now..

    if(cdvd_testready() != CDVD_ERROR_SUCCESS)
    {
        stat->Type = 0xff;
        stat->Status |= 0x10;
    }
    else
    {
        // new disc, flush cashe markers
        flush_all();
    }

    return CDVD_ERROR_SUCCESS;
}
