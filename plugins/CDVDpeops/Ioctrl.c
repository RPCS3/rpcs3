/***************************************************************************
                          ioctrl.c  -  description
                             -------------------
    begin                : Sun Nov 16 2003
    copyright            : (C) 2003 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2003/11/16 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

/////////////////////////////////////////////////////////

#include "stdafx.h"
#define _IN_IOCTL
#include "externals.h"

/////////////////////////////////////////////////////////

HANDLE     hIOCTL=NULL;                                // global drive file handle
DWORD      dwIOCTLAttr=0;                              // open attribute
OVERLAPPED ovcIOCTL;                                   // global overlapped struct
SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptIOCTL;         // global read bufs
RAW_READ_INFO                        rawIOCTL;

/////////////////////////////////////////////////////////
// open drive

void OpenIOCTLHandle(int iA,int iT,int iL)
{
 char cLetter;

 if(hIOCTL) return;

 cLetter=MapIOCTLDriveLetter(iA,iT,iL);                // get drive

 if(!cLetter) return;

 hIOCTL=OpenIOCTLFile(cLetter,                         // open drive
                      (iUseCaching==2)?TRUE:FALSE);    // (caching:2 -> overlapped)
}

/////////////////////////////////////////////////////////
// close drive

void CloseIOCTLHandle(void)
{
 if(hIOCTL) CloseHandle(hIOCTL);
 hIOCTL=NULL;
}

/////////////////////////////////////////////////////////
// get drive letter by a,t,l

char MapIOCTLDriveLetter(int iA,int iT,int iL)
{
 char cLetter[4];int iDA,iDT,iDL;HANDLE hF;

 strcpy(cLetter,"C:\\");

 for(cLetter[0]='C';cLetter[0]<='Z';cLetter[0]++)
  {
   if(GetDriveType(cLetter)==DRIVE_CDROM)
    {
     hF=OpenIOCTLFile(cLetter[0],FALSE);
     GetIOCTLAdapter(hF,&iDA,&iDT,&iDL);
     CloseHandle(hF);
     if(iA==iDA && iT==iDT && iL==iDL)
      return cLetter[0];
    }
  }
 return 0;
}

/////////////////////////////////////////////////////////
// get cd drive list, using ioctl, not aspi

int GetIOCTLCDDrives(char * pDList)
{
 char cLetter[4];int iDA,iDT,iDL;HANDLE hF;
 int iCnt=0;char * p=pDList;

 strcpy(cLetter,"C:\\");

 for(cLetter[0]='C';cLetter[0]<='Z';cLetter[0]++)
  {
   if(GetDriveType(cLetter)==DRIVE_CDROM)
    {
     hF=OpenIOCTLFile(cLetter[0],FALSE);
     GetIOCTLAdapter(hF,&iDA,&iDT,&iDL);
     CloseHandle(hF);
     if(iDA!=-1 && iDT!=-1 && iDL!=-1)
      {
       wsprintf(p,"[%d:%d:%d] Drive %c:",
                iDA,iDT,iDL,cLetter[0]);
       p+=strlen(p)+1;
       iCnt++;
      }
    }
  }

 return iCnt;
}

/////////////////////////////////////////////////////////
// open drive in sync/async mode

HANDLE OpenIOCTLFile(char cLetter,BOOL bAsync)
{
 HANDLE hF;char szFName[16];
 OSVERSIONINFO ov;DWORD dwFlags;

 if(bAsync) dwIOCTLAttr=FILE_FLAG_OVERLAPPED;
 else       dwIOCTLAttr=0;

 memset(&ov,0,sizeof(OSVERSIONINFO));
 ov.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
 GetVersionEx(&ov);

 if((ov.dwPlatformId==VER_PLATFORM_WIN32_NT) &&
    (ov.dwMajorVersion>4))
      dwFlags = GENERIC_READ|GENERIC_WRITE;            // add gen write on W2k/XP
 else dwFlags = GENERIC_READ;

 wsprintf(szFName, "\\\\.\\%c:",cLetter);

 hF=CreateFile(szFName,dwFlags,FILE_SHARE_READ,        // open drive
               NULL,OPEN_EXISTING,dwIOCTLAttr,NULL);

 if(hF==INVALID_HANDLE_VALUE)                          // mmm... no success?
  {
   dwFlags^=GENERIC_WRITE;                             // -> try write toggle
   hF=CreateFile(szFName,dwFlags,FILE_SHARE_READ,      // -> open drive again
                 NULL,OPEN_EXISTING,dwIOCTLAttr,NULL);
   if(hF==INVALID_HANDLE_VALUE) return NULL;
  }

 return hF;
}

/////////////////////////////////////////////////////////
// get a,t,l

void GetIOCTLAdapter(HANDLE hF,int * iDA,int * iDT,int * iDL)
{
 char szBuf[1024];PSCSI_ADDRESS pSA;DWORD dwRet;

 *iDA=*iDT=*iDL=-1;
 if(hF==NULL) return;

 memset(szBuf,0,1024);

 pSA=(PSCSI_ADDRESS)szBuf;
 pSA->Length=sizeof(SCSI_ADDRESS);

 if(!DeviceIoControl(hF,IOCTL_SCSI_GET_ADDRESS,NULL,
                     0,pSA,sizeof(SCSI_ADDRESS),
                     &dwRet,NULL))
  return;

 *iDA = pSA->PortNumber;
 *iDT = pSA->TargetId;
 *iDL = pSA->Lun;
}

/////////////////////////////////////////////////////////
// we fake the aspi call in ioctl scsi mode

DWORD IOCTLSendASPI32Command(LPSRB pSRB)
{
 LPSRB_ExecSCSICmd pSC;DWORD dwRet;BOOL bStat;

 if(!pSRB) return SS_ERR;

 if(hIOCTL==NULL ||
    pSRB->SRB_Cmd!=SC_EXEC_SCSI_CMD)                   // we only fake exec aspi scsi commands
  {
   pSRB->SRB_Status=SS_ERR;
   return SS_ERR;
  }

 pSC=(LPSRB_ExecSCSICmd)pSRB;

 memset(&sptIOCTL,0,sizeof(sptIOCTL));

 sptIOCTL.spt.Length             = sizeof(SCSI_PASS_THROUGH_DIRECT);
 sptIOCTL.spt.CdbLength          = pSC->SRB_CDBLen;
 sptIOCTL.spt.DataTransferLength = pSC->SRB_BufLen;
 sptIOCTL.spt.TimeOutValue       = 60;
 sptIOCTL.spt.DataBuffer         = pSC->SRB_BufPointer;
 sptIOCTL.spt.SenseInfoLength    = 14;
 sptIOCTL.spt.TargetId           = pSC->SRB_Target;
 sptIOCTL.spt.Lun                = pSC->SRB_Lun;
 sptIOCTL.spt.SenseInfoOffset    = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);
 if(pSC->SRB_Flags&SRB_DIR_IN)       sptIOCTL.spt.DataIn = SCSI_IOCTL_DATA_IN;
 else if(pSC->SRB_Flags&SRB_DIR_OUT) sptIOCTL.spt.DataIn = SCSI_IOCTL_DATA_OUT;
 else                                sptIOCTL.spt.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
 memcpy(sptIOCTL.spt.Cdb,pSC->CDBByte,pSC->SRB_CDBLen);

 if(dwIOCTLAttr==FILE_FLAG_OVERLAPPED)                 // async?
  {
   ovcIOCTL.Internal=0;
   ovcIOCTL.InternalHigh=0;
   ovcIOCTL.Offset=0;
   ovcIOCTL.OffsetHigh=0;
   ovcIOCTL.hEvent=hEvent;
   bStat = DeviceIoControl(hIOCTL,
                IOCTL_SCSI_PASS_THROUGH_DIRECT,
                &sptIOCTL,
                sizeof(sptIOCTL),
                &sptIOCTL,
                sizeof(sptIOCTL),
                &dwRet,
                &ovcIOCTL);
  }
 else                                                  // sync?
  {
   bStat = DeviceIoControl(hIOCTL,
                IOCTL_SCSI_PASS_THROUGH_DIRECT,
                &sptIOCTL,
                sizeof(sptIOCTL),
                &sptIOCTL,
                sizeof(sptIOCTL),
                &dwRet,
                NULL);
  }

 if(!bStat)                                            // some err?
  {
   DWORD dwErrCode;
   dwErrCode=GetLastError();
   if(dwErrCode==ERROR_IO_PENDING)                     // -> pending?
    {
     pSC->SRB_Status=SS_COMP;                          // --> ok
     return SS_PENDING;
    }
   pSC->SRB_Status=SS_ERR;                             // -> else error
   return SS_ERR;
  }

 pSC->SRB_Status=SS_COMP;
 return SS_COMP;
}

/////////////////////////////////////////////////////////
// special raw mode... works on TEAC532S, for example

DWORD ReadIOCTL_Raw(BOOL bWait,FRAMEBUF * f)
{
 DWORD dwRet;BOOL bStat;

 if(hIOCTL==NULL) return SS_ERR;

 rawIOCTL.DiskOffset.QuadPart = f->dwFrame*2048;       // 2048 is needed here
 rawIOCTL.SectorCount         = f->dwFrameCnt;
 rawIOCTL.TrackMode           = XAForm2;//CDDA;//YellowMode2;//XAForm2;

 if(dwIOCTLAttr==FILE_FLAG_OVERLAPPED)                 // async?
  {
   ovcIOCTL.Internal=0;
   ovcIOCTL.InternalHigh=0;
   ovcIOCTL.Offset=0;
   ovcIOCTL.OffsetHigh=0;
   ovcIOCTL.hEvent=hEvent;
   ResetEvent(hEvent);
   bStat = DeviceIoControl(hIOCTL,
             IOCTL_CDROM_RAW_READ,
             &rawIOCTL,sizeof(RAW_READ_INFO),
             &(f->BufData[0]),f->dwBufLen,//2048,
             &dwRet, &ovcIOCTL);
  }
 else                                                  // sync?
  {
   bStat = DeviceIoControl(hIOCTL,
             IOCTL_CDROM_RAW_READ,
             &rawIOCTL,sizeof(RAW_READ_INFO),
             &(f->BufData[0]),f->dwBufLen,//2048,
             &dwRet,NULL);
  }

 if(!bStat)
  {
   DWORD dwErrCode;
   dwErrCode=GetLastError();

#ifdef DBGOUT
auxprintf("errorcode %d\n",   dwErrCode);
#endif

   if(dwErrCode==ERROR_IO_PENDING)
    {
     // we do a wait here, not later... no real async mode anyway
     // bDoWaiting=TRUE;

     WaitGenEvent(0xFFFFFFFF);
    }
   else
    {
     sx.SRB_Status=SS_ERR;
     return SS_ERR;
    }
  }

 sx.SRB_Status=SS_COMP;
 return SS_COMP;
}

/////////////////////////////////////////////////////////
// special raw + special sub... dunno if this really
// works on any drive (teac is working, but giving unprecise
// subdata)

DWORD ReadIOCTL_Raw_Sub(BOOL bWait,FRAMEBUF * f)
{
 DWORD dwRet;BOOL bStat;
 SUB_Q_CHANNEL_DATA qd;unsigned char * p;
 CDROM_SUB_Q_DATA_FORMAT qf;

 if(hIOCTL==NULL) return SS_ERR;

 rawIOCTL.DiskOffset.QuadPart = f->dwFrame*2048;
 rawIOCTL.SectorCount         = f->dwFrameCnt;
 rawIOCTL.TrackMode           = XAForm2;

 bStat = DeviceIoControl(hIOCTL,
             IOCTL_CDROM_RAW_READ,
             &rawIOCTL,sizeof(RAW_READ_INFO),
             &(f->BufData[0]),f->dwBufLen,
             &dwRet,NULL);

 if(!bStat) {sx.SRB_Status=SS_ERR;return SS_ERR;}

 qf.Format=IOCTL_CDROM_CURRENT_POSITION;
 qf.Track=1;
 bStat = DeviceIoControl(hIOCTL,
             IOCTL_CDROM_READ_Q_CHANNEL,
             &qf,sizeof(CDROM_SUB_Q_DATA_FORMAT),
             &qd,sizeof(SUB_Q_CHANNEL_DATA),
             &dwRet,NULL);

 p=(unsigned char*)&qd;

 SubCData[12]=(p[5]<<4)|(p[5]>>4);
 SubCData[13]=p[6];
 SubCData[14]=p[7];
 SubCData[15]=p[13];
 SubCData[16]=p[14];
 SubCData[17]=p[15];
 SubCData[18]=0;
 SubCData[19]=p[9];
 SubCData[20]=p[10];
 SubCData[21]=p[11];

 SubCData[15]=itob(SubCData[15]);
 SubCData[16]=itob(SubCData[16]);
 SubCData[17]=itob(SubCData[17]);

 SubCData[19]=itob(SubCData[19]);
 SubCData[20]=itob(SubCData[20]);
 SubCData[21]=itob(SubCData[21]);

 if(!bStat) {sx.SRB_Status=SS_ERR;return SS_ERR;}

 sx.SRB_Status=SS_COMP;
 return  SS_COMP;
}

/////////////////////////////////////////////////////////
