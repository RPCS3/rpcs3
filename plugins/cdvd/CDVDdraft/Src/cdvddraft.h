// cdvddraft.h : main header file for the CDVDDRAFT DLL
//

#if !defined(AFX_CDVDDRAFT_H__A0D50EA8_BD80_11D7_8E2C_0050DA15DE89__INCLUDED_)
#define AFX_CDVDDRAFT_H__A0D50EA8_BD80_11D7_8E2C_0050DA15DE89__INCLUDED_

#if _MSC_VER > 1000
#pragma once 
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#define  __WIN32__

#include "resource.h"		// main symbols
#include "ps2etypes.h"
#include "ps2edefs.h"

#include "cdvd.h"

#define PLUGIN_REG_PATH _T("Software\\PS2EPlugin\\CDVD\\CdvdXeven")
   
/*************************************************************************/
/* ps2emu library identifier functions                                   */
/*************************************************************************/

unsigned int CALLBACK PS2EgetLibType();
char        *CALLBACK PS2EgetLibName();
unsigned int CALLBACK PS2EgetLibVersion2(unsigned int type); 

/*************************************************************************/
/* ps2emu config/test   functions                                        */
/*************************************************************************/

void CALLBACK CDVDconfigure();
void CALLBACK CDVDabout();
int  CALLBACK CDVDtest();

/*************************************************************************/
/* ps2emu library c/dvd functions                                        */
/*************************************************************************/

int  CALLBACK CDVDinit();
void CALLBACK CDVDshutdown();
int  CALLBACK CDVDopen();
void CALLBACK CDVDclose();
int  CALLBACK CDVDreadTrack(unsigned int lsn, int mode);
unsigned char *CALLBACK CDVDgetBuffer();
int  CALLBACK CDVDgetTN(cdvdTN *buffer);
int  CALLBACK CDVDgetTD(unsigned char track, cdvdTD *buffer);
int  CALLBACK CDVDgetType();
int  CALLBACK CDVDgetTrayStatus();

#endif // !defined(AFX_CDVDDRAFT_H__A0D50EA8_BD80_11D7_8E2C_0050DA15DE89__INCLUDED_)
