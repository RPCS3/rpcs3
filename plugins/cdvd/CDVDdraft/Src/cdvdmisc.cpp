// cdvdmisc.cpp: implementation of the cdvdmisc class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cdvddraft.h"
#include "cdvdmisc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

extern int        g_status   [CDVD_NUM_BUFFERS];    // status of read to buffer-n
extern HANDLE     g_handle   [CDVD_NUM_BUFFERS];    // handle used to read to buffer-n
extern int        g_sectstart[CDVD_NUM_BUFFERS];    // start of sector read to buffer-n
extern BOOL       g_filled   [CDVD_NUM_BUFFERS];    // check if buffer-n was filled
extern CDVD_BUFFER_MODE     g_current_buffmode;
extern CDVD_INTERFACE_TYPE  g_current_intftype;
extern int                  g_current_drivenum;
extern CDVD_READ_MODE       g_current_readmode;

int init_buffersandflags()
{
    for(int i=0; i<CDVD_NUM_BUFFERS; i++)
    {
        g_status[i]    = -1;
        g_sectstart[i] = -1;
        g_filled[i]    = FALSE;
        if(g_handle != NULL) CloseHandle(g_handle[i]);
            g_handle[i]    = CreateEvent(NULL, TRUE, TRUE, NULL);
    }

    return CDVD_ERROR_SUCCESS;
}
        
int shut_buffersandflags()
{
    for(int i=0; i<CDVD_NUM_BUFFERS; i++)
    {
        g_status[i]    = -1;
        g_sectstart[i] = -1;
        g_filled[i]    = FALSE;
        if(g_handle[i]) CloseHandle(g_handle[i]);
    }

    return CDVD_ERROR_SUCCESS;
}

int flush_all()
{
    for(int i=0; i<CDVD_NUM_BUFFERS; i++)
    {
        g_status[i] = -1;
        g_sectstart[i] = -1;
        g_filled[i] = FALSE;
        if(g_handle[i])
            WaitForSingleObject(g_handle[i], INFINITE);
    }

    return CDVD_ERROR_SUCCESS;
}

#include <atlbase.h>
int load_registrysettings()
{
	CRegKey rKey;
	
	if(rKey.Create(HKEY_CURRENT_USER, PLUGIN_REG_PATH) == ERROR_SUCCESS)
	{
        DWORD val;

        g_current_readmode = CDVD_READ_UNKNOWN;
        if(rKey.QueryValue(val, _T("DriveNum")) == ERROR_SUCCESS)
            g_current_drivenum = val;
        if(rKey.QueryValue(val, _T("BuffMode")) == ERROR_SUCCESS)
            g_current_buffmode = (CDVD_BUFFER_MODE) val;
        TRACE("---- buffmode: %ld -----\n", g_current_buffmode);
        if(rKey.QueryValue(val, _T("Interface")) == ERROR_SUCCESS)
            g_current_intftype = (CDVD_INTERFACE_TYPE) val;
        if(rKey.QueryValue(val, _T("ReadMode")) == ERROR_SUCCESS)
        {
            if(val == 0) g_current_readmode = CDVD_READ_UNKNOWN;
            else
                g_current_readmode = (CDVD_READ_MODE) (val - 1);
        }
        rKey.Close();
	}
    else return CDVD_ERROR_FAIL;

    return CDVD_ERROR_SUCCESS;
}