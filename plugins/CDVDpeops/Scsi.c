/***************************************************************************
                            scsi.c  -  description
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
#define _IN_SCSI
#include "externals.h"

/////////////////////////////////////////////////////////

SRB_ExecSCSICmd sx;     // used with all (non-waiting) read funcs
BOOL bDoWaiting=FALSE;  // flag for async reads

/////////////////////////////////////////////////////////
// returns device type

int GetSCSIDevice(int iA,int iT,int iL)
{
 SRB_GDEVBlock s;DWORD dwStatus;

 memset(&s,0,sizeof(SRB_GDEVBlock));
 s.SRB_Cmd    = SC_GET_DEV_TYPE;
 s.SRB_HaID   = iA;
 s.SRB_Target = iT;
 s.SRB_Lun    = iL;

 ResetEvent(hEvent);

 dwStatus=pSendASPI32Command((LPSRB)&s);

 if(dwStatus==SS_PENDING) 
  {WaitGenEvent(30000);dwStatus=s.SRB_Status;}

 if(dwStatus==SS_COMP) return s.SRB_DeviceType;

 return -1;
}

/////////////////////////////////////////////////////////

int GetSCSIStatus(int iA,int iT,int iL)
{
 SRB_ExecSCSICmd s;DWORD dwStatus;
 char ret[0x324];

 memset(&s,0,sizeof(s));

 s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 s.SRB_HaId       = iCD_AD;
 s.SRB_Target     = iCD_TA;
 s.SRB_Lun        = iCD_LU;
 s.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 s.SRB_BufLen     = 0x324;
 s.SRB_BufPointer = (BYTE FAR *)ret;
 s.SRB_SenseLen   = 0x0E;
 s.SRB_CDBLen     = 0x0A;
 s.SRB_PostProc   = (LPVOID)hEvent;
 s.CDBByte[0]     = 0x00;

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&s);

 if(dwStatus==SS_PENDING) WaitGenEvent(30000);

 return s.SRB_Status;
}

/////////////////////////////////////////////////////////
// fills toc infos

DWORD GetSCSITOC(LPTOC toc)
{
 SRB_ExecSCSICmd s;DWORD dwStatus;

 memset(&s,0,sizeof(s));

 s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 s.SRB_HaId       = iCD_AD;
 s.SRB_Target     = iCD_TA;
 s.SRB_Lun        = iCD_LU;
 s.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 s.SRB_BufLen     = 0x324;
 s.SRB_BufPointer = (BYTE FAR *)toc;
 s.SRB_SenseLen   = 0x0E;
 s.SRB_CDBLen     = 0x0A;
 s.SRB_PostProc   = (LPVOID)hEvent;
 s.CDBByte[0]     = 0x43;
 s.CDBByte[1]     = 0x00; // 0x02 for MSF
 s.CDBByte[7]     = 0x03;
 s.CDBByte[8]     = 0x24;

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&s);

 if(dwStatus==SS_PENDING) WaitGenEvent(30000);

 if(s.SRB_Status!=SS_COMP) return SS_ERR;

 return SS_COMP;
}

/////////////////////////////////////////////////////////
// enum all cd drives into 32k buffer, return num of drives

int GetSCSICDDrives(char * pDList)
{
 int iCnt=0,iA,iT,iL;char * p=pDList;
 SRB_HAInquiry si;SRB_GDEVBlock sd;
 SRB_ExecSCSICmd s;int iNumA;char szBuf[100];
 DWORD dw,dwStatus;

 if(!pGetASPI32SupportInfo) return 0;

 dw=pGetASPI32SupportInfo();

 if(HIBYTE(LOWORD(dw))!=SS_COMP) return 0;
 iNumA=(int)LOBYTE(LOWORD(dw));

 for(iA=0;iA<iNumA;iA++)
  {
   memset(&si,0,sizeof(SRB_HAInquiry));
   si.SRB_Cmd  = SC_HA_INQUIRY;
   si.SRB_HaId = iA;
   pSendASPI32Command((LPSRB)&si);
   if(si.SRB_Status!=SS_COMP) continue;
   if(!si.HA_Unique[3]) si.HA_Unique[3]=8;
   for(iT=0;iT<si.HA_Unique[3];iT++)
    {
     for(iL=0;iL<8;iL++)
      {
       memset(&sd,0,sizeof(SRB_GDEVBlock));
       sd.SRB_Cmd    = SC_GET_DEV_TYPE;
       sd.SRB_HaID   = iA;
       sd.SRB_Target = iT;
       sd.SRB_Lun    = iL;

       pSendASPI32Command((LPSRB)&sd);
       if(sd.SRB_Status==SS_COMP &&
          sd.SRB_DeviceType==DTYPE_CDROM)
        {
         memset(&s,0,sizeof(s));
 
         s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
         s.SRB_HaId       = iA;
         s.SRB_Target     = iT;
         s.SRB_Lun        = iL;
         s.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
         s.SRB_BufLen     = 100;
         s.SRB_BufPointer = szBuf;
         s.SRB_SenseLen   = SENSE_LEN;
         s.SRB_CDBLen     = 6;
         s.SRB_PostProc   = (LPVOID)hEvent;
         s.CDBByte[0]     = SCSI_INQUIRY;
         s.CDBByte[4]     = 100;
         ResetEvent(hEvent);
         dwStatus=pSendASPI32Command((LPSRB)&s);
         if(dwStatus==SS_PENDING)
          WaitGenEvent(WAITFOREVER);
         if(s.SRB_Status==SS_COMP)
          {
           int i;
           for(i=8 ;i<15;i++) if(szBuf[i]==' ') {szBuf[i]=0;break;}
           szBuf[15]=0;
           for(i=16;i<32;i++) if(szBuf[i]==' ') {szBuf[i]=0;break;}
           szBuf[31]=0;
           for(i=32;i<37;i++) if(szBuf[i]==' ') {szBuf[i]=0;break;}
           szBuf[36]=0;
           wsprintf(p,"[%d:%d:%d] %s %s V%s", 
            iA,iT,iL,&szBuf[8],&szBuf[16],&szBuf[32]);
           iCnt++;
           p+=strlen(p)+1;
          }
        }
      }
    }
  }
 return iCnt;
}

/////////////////////////////////////////////////////////
// play audio

DWORD PlaySCSIAudio(unsigned long start,unsigned long len)
{
 SRB_ExecSCSICmd s;DWORD dwStatus;

 memset(&s,0,sizeof(s));
 s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 s.SRB_HaId       = iCD_AD;
 s.SRB_Target     = iCD_TA;
 s.SRB_Lun        = iCD_LU;
 s.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 s.SRB_BufLen     = 0;
 s.SRB_BufPointer = 0;
 s.SRB_SenseLen   = SENSE_LEN;
 s.SRB_CDBLen     = 12;
 s.SRB_PostProc   = (LPVOID)hEvent;

 if(start==0)                                          // mmm... that stop doesn't seem to work on all drives... pfff
  {
   s.CDBByte[0]     = 0x4b;
   s.CDBByte[1]     = (iCD_LU << 5) | 01;
   s.CDBByte[2]     = 0;
   s.CDBByte[3]     = 0;
   s.CDBByte[4]     = 0;
   s.CDBByte[5]     = 0;
   s.CDBByte[6]     = 0;
   s.CDBByte[7]     = 0;
   s.CDBByte[8]     = 0;
   s.CDBByte[9]     = 0;
  }
 else                                                  // start playing
  {
   s.CDBByte[0]     = 0xa5;
   s.CDBByte[1]     = iCD_LU << 5;
   s.CDBByte[2]     = (unsigned char)((start >> 24) & 0xFF);
   s.CDBByte[3]     = (unsigned char)((start >> 16) & 0xFF);
   s.CDBByte[4]     = (unsigned char)((start >> 8) & 0xFF);
   s.CDBByte[5]     = (unsigned char)((start & 0xFF));
   s.CDBByte[6]     = (unsigned char)((len >> 24) & 0xFF);
   s.CDBByte[7]     = (unsigned char)((len >> 16) & 0xFF);
   s.CDBByte[8]     = (unsigned char)((len >> 8) & 0xFF);
   s.CDBByte[9]     = (unsigned char)(len & 0xFF);
  }

 ResetEvent(hEvent);

 dwStatus = pSendASPI32Command((LPSRB)&s);

 if(dwStatus==SS_PENDING) WaitGenEvent(10000);

 return s.SRB_Status;
}

/////////////////////////////////////////////////////////
// do (unprecise) sub channel read on audio play

unsigned char * GetSCSIAudioSub(void)
{
 SRB_ExecSCSICmd s;DWORD dwStatus;
 unsigned char cB[20];

 memset(cB,0,20);
 memset(&s,0,sizeof(s));

 s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 s.SRB_HaId       = iCD_AD;
 s.SRB_Target     = iCD_TA;
 s.SRB_Lun        = iCD_LU;
 s.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 s.SRB_SenseLen   = SENSE_LEN;

 s.SRB_BufLen     = 20;//44;
 s.SRB_BufPointer = cB;
 s.SRB_CDBLen     = 10;

 s.CDBByte[0]     = 0x42;
 s.CDBByte[1]     = (iCD_LU<<5)|2;   // lun & msf
 s.CDBByte[2]     = 0x40;            // subq
 s.CDBByte[3]     = 0x01;            // curr pos info
 s.CDBByte[6]     = 0;               // track number (only in isrc mode, ignored)
 s.CDBByte[7]     = 0;               // alloc len
 s.CDBByte[8]     = 20;//44;

 ResetEvent(hEvent);

 dwStatus = pSendASPI32Command((LPSRB)&s);

 if(dwStatus==SS_PENDING) WaitGenEvent(WAITFOREVER);

 if(s.SRB_Status!=SS_COMP) return NULL;

 SubAData[12]=(cB[5]<<4)|(cB[5]>>4);
 SubAData[13]=cB[6];
 SubAData[14]=cB[7];
 SubAData[15]=itob(cB[13]);
 SubAData[16]=itob(cB[14]);
 SubAData[17]=itob(cB[15]);
 SubAData[18]=0;
 SubAData[19]=itob(cB[9]);
 SubAData[20]=itob(cB[10]);
 SubAData[21]=itob(cB[11]);

 return SubAData;
}

/////////////////////////////////////////////////////////
// test, if drive is ready (doesn't work on all drives)

BOOL TestSCSIUnitReady(void)
{
 SRB_ExecSCSICmd s;DWORD dwStatus;

 memset(&s,0,sizeof(s));
 s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 s.SRB_HaId       = iCD_AD;
 s.SRB_Target     = iCD_TA;
 s.SRB_Lun        = iCD_LU;
 s.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 s.SRB_BufLen     = 0;
 s.SRB_BufPointer = 0;
 s.SRB_SenseLen   = SENSE_LEN;
 s.SRB_CDBLen     = 6;
 s.SRB_PostProc   = (LPVOID)hEvent;
 s.CDBByte[0]     = 0x00;
 s.CDBByte[1]     = iCD_LU << 5;

 ResetEvent(hEvent);
 dwStatus = pSendASPI32Command((LPSRB)&s);

 if(dwStatus==SS_PENDING)
  WaitGenEvent(1000);

 if(s.SRB_Status!=SS_COMP)
  return FALSE;

 if(s.SRB_TargStat==STATUS_GOOD) return TRUE;          // will always be GOOD with ioctl, so no problem here

 return FALSE;
}

/////////////////////////////////////////////////////////
// change the read speed (not supported on all drives)

DWORD SetSCSISpeed(DWORD dwSpeed)
{
 SRB_ExecSCSICmd s;DWORD dwStatus;

 memset(&s,0,sizeof(s));

 s.SRB_Cmd      = SC_EXEC_SCSI_CMD;
 s.SRB_HaId     = iCD_AD;
 s.SRB_Target   = iCD_TA;
 s.SRB_Lun      = iCD_LU;
 s.SRB_Flags    = SRB_DIR_OUT | SRB_EVENT_NOTIFY;
 s.SRB_SenseLen = SENSE_LEN;
 s.SRB_CDBLen   = 12;
 s.SRB_PostProc = (LPVOID)hEvent;
 s.CDBByte[0]   = 0xBB;
 s.CDBByte[2]   = (BYTE)(dwSpeed >> 8);
 s.CDBByte[3]   = (BYTE)dwSpeed;

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&s);
 
 if(dwStatus==SS_PENDING)
  WaitGenEvent(WAITFOREVER);

 if(s.SRB_Status!=SS_COMP) return SS_ERR;

 return SS_COMP;
}

/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// all the different SCSI read commands can be found here
// 'bWait' is a flag, if the command should wait until
// completed, or if the func can return as soon as possible
// (async reading). Attention: 'bWait' is not really used 
// in the Sub-channel commands yet (sub is done always 
// blocking, and just 'one sector' reads are done, caching=0)
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// BE: used by most ATAPI drives
/////////////////////////////////////////////////////////

DWORD ReadSCSI_BE(BOOL bWait,FRAMEBUF * f)
{
 DWORD dwStatus;

 memset(&sx,0,sizeof(sx));

 sx.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 sx.SRB_HaId       = iCD_AD;
 sx.SRB_Target     = iCD_TA;
 sx.SRB_Lun        = iCD_LU;
 sx.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 sx.SRB_BufLen     = f->dwBufLen;
 sx.SRB_BufPointer = &(f->BufData[0]);
 sx.SRB_SenseLen   = SENSE_LEN;
 sx.SRB_CDBLen     = 12;
 sx.SRB_PostProc   = (LPVOID)hEvent;
 sx.CDBByte[0]     = 0xBE;
 //s.CDBByte[1]     = 0x04;
 sx.CDBByte[2]     = (unsigned char)((f->dwFrame >> 24) & 0xFF);
 sx.CDBByte[3]     = (unsigned char)((f->dwFrame >> 16) & 0xFF);
 sx.CDBByte[4]     = (unsigned char)((f->dwFrame >> 8) & 0xFF);
 sx.CDBByte[5]     = (unsigned char)(f->dwFrame & 0xFF);
 sx.CDBByte[8]     = (unsigned char)(f->dwFrameCnt & 0xFF);
 sx.CDBByte[9]     = (iRType==MODE_BE_1)?0x10:0xF8;//F0!!!!!!!!!!!

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&sx);

 if(dwStatus==SS_PENDING)
  {
   if(bWait) WaitGenEvent(WAITFOREVER);
   else
    {
     bDoWaiting=TRUE;
     return SS_COMP;
    }
  }

 if(sx.SRB_Status!=SS_COMP)
  return SS_ERR;

 return SS_COMP;
}

/////////////////////////////////////////////////////////

DWORD ReadSCSI_BE_Sub(BOOL bWait,FRAMEBUF * f)
{
 DWORD dwStatus;

 memset(&sx,0,sizeof(sx));
 sx.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 sx.SRB_HaId       = iCD_AD;
 sx.SRB_Target     = iCD_TA;
 sx.SRB_Lun        = iCD_LU;
 sx.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 sx.SRB_BufLen     = f->dwBufLen + 16;
 sx.SRB_BufPointer = &(f->BufData[0]);
 sx.SRB_SenseLen   = SENSE_LEN;
 sx.SRB_CDBLen     = 12;
 sx.SRB_PostProc   = (LPVOID)hEvent;
 sx.CDBByte[0]     = 0xBE;
 //s.CDBByte[1]     = 0x04;
 sx.CDBByte[2]     = (unsigned char)((f->dwFrame >> 24) & 0xFF);
 sx.CDBByte[3]     = (unsigned char)((f->dwFrame >> 16) & 0xFF);
 sx.CDBByte[4]     = (unsigned char)((f->dwFrame >> 8) & 0xFF);
 sx.CDBByte[5]     = (unsigned char)(f->dwFrame & 0xFF);
 sx.CDBByte[8]     = (unsigned char)(f->dwFrameCnt & 0xFF);
 sx.CDBByte[9]     = (iRType==MODE_BE_1)?0x10:0xF8;//F0!!!!!!!!!!!
 sx.CDBByte[10]    = 0x2;

 ResetEvent(hEvent );
 dwStatus=pSendASPI32Command((LPSRB)&sx);

 if(dwStatus==SS_PENDING)
  WaitGenEvent(WAITSUB);

 if(sx.SRB_Status!=SS_COMP)
  return SS_ERR;

 memcpy(&SubCData[12],&f->BufData[2352],16);

 SubCData[15]=itob(SubCData[15]);
 SubCData[16]=itob(SubCData[16]);
 SubCData[17]=itob(SubCData[17]);

 SubCData[19]=itob(SubCData[19]);
 SubCData[20]=itob(SubCData[20]);
 SubCData[21]=itob(SubCData[21]);

 return SS_COMP;
}

/////////////////////////////////////////////////////////
// different sub reading for lite-on ltd163d...
// 16 bytes subc data is encoded in 96 bytes...

DWORD ReadSCSI_BE_Sub_1(BOOL bWait,FRAMEBUF * f)
{
 DWORD dwStatus;

 memset(&sx,0,sizeof(sx));

 sx.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 sx.SRB_HaId       = iCD_AD;
 sx.SRB_Target     = iCD_TA;
 sx.SRB_Lun        = iCD_LU;
 sx.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 sx.SRB_BufLen     = f->dwBufLen + 96;
 sx.SRB_BufPointer = &(f->BufData[0]);
 sx.SRB_SenseLen   = SENSE_LEN;
 sx.SRB_CDBLen     = 12;
 sx.SRB_PostProc   = (LPVOID)hEvent;
 sx.CDBByte[0]     = 0xBE;
 sx.CDBByte[2]     = (unsigned char)((f->dwFrame >> 24) & 0xFF);
 sx.CDBByte[3]     = (unsigned char)((f->dwFrame >> 16) & 0xFF);
 sx.CDBByte[4]     = (unsigned char)((f->dwFrame >> 8) & 0xFF);
 sx.CDBByte[5]     = (unsigned char)(f->dwFrame & 0xFF);
 sx.CDBByte[8]     = (unsigned char)(f->dwFrameCnt & 0xFF);
 sx.CDBByte[9]     = 0xF8;
 sx.CDBByte[10]    = 0x1;

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&sx);
 
 if(dwStatus==SS_PENDING)
  WaitGenEvent(WAITSUB);

 if(sx.SRB_Status!=SS_COMP)
  return SS_ERR;

 DecodeSub_BE_2_1(&f->BufData[2352]);

 memcpy(&SubCData[12],&f->BufData[2352],16);

 return SS_COMP;
}

/////////////////////////////////////////////////////////
// 28: used by most SCSI drives
/////////////////////////////////////////////////////////
              
DWORD InitSCSI_28_2(void)
{
 SRB_ExecSCSICmd s;DWORD dwStatus;
 int i;
 BYTE init1[] = { 0, 0, 0, 0x08, 0, 0, 0, 0, 0, 0, 0x09, 0x30, 0x23, 6, 0, 0, 0, 0, 0, 0x80 };
 BYTE init2[] = { 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 9, 48, 1, 6, 32, 7, 0, 0, 0, 0 };

 for(i=0;i<2;i++)
  {
   memset( &s, 0, sizeof( s ) );
   s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
   s.SRB_HaId       = iCD_AD;
   s.SRB_Target     = iCD_TA;
   s.SRB_Lun        = iCD_LU;
   s.SRB_Flags      = SRB_EVENT_NOTIFY;
   s.SRB_BufLen     = 0x14;
   s.SRB_BufPointer = (i==0)?init1:init2;
   s.SRB_SenseLen   = SENSE_LEN;
   s.SRB_CDBLen     = 6;
   s.SRB_PostProc   = (LPVOID)hEvent;
   s.CDBByte[0]     = 0x15;
   s.CDBByte[1]     = 0x10;
   s.CDBByte[4]     = 0x14;
   
   ResetEvent(hEvent);

   dwStatus=pSendASPI32Command((LPSRB)&s);
   if (dwStatus == SS_PENDING)
    WaitGenEvent(WAITFOREVER);

   if(s.SRB_Status!=SS_COMP)
    return SS_ERR;
  }
                 
 pDeInitFunc = DeInitSCSI_28;

 return s.SRB_Status;
}

/////////////////////////////////////////////////////////

DWORD InitSCSI_28_1(void)
{
 SRB_ExecSCSICmd s;DWORD dwStatus;
 BYTE init1[] = { 0, 0, 0, 0x08, 0, 0, 0, 0, 0, 0, 0x09, 0x30 };

 memset(&s,0,sizeof(s));
 s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 s.SRB_HaId       = iCD_AD;
 s.SRB_Target     = iCD_TA;
 s.SRB_Lun        = iCD_LU;
 s.SRB_Flags      = SRB_EVENT_NOTIFY;
 s.SRB_BufLen     = 0x0C;
 s.SRB_BufPointer = init1;
 s.SRB_SenseLen   = SENSE_LEN;
 s.SRB_CDBLen     = 6;
 s.SRB_PostProc   = (LPVOID)hEvent;
 s.CDBByte[0]     = 0x15;
 s.CDBByte[4]     = 0x0C;

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&s);
 if(dwStatus==SS_PENDING)
  WaitGenEvent(WAITFOREVER);

 if(s.SRB_Status!=SS_COMP)
  return SS_ERR;

 pDeInitFunc = DeInitSCSI_28;

 return s.SRB_Status;
}

/////////////////////////////////////////////////////////

DWORD InitSCSI_28_2048(void)
{  
 SRB_ExecSCSICmd s;DWORD dwStatus;
 BYTE init1[] = { 0, 0, 0, 0x08, 0, 0, 0, 0, 0, 0, 0x08, 0x0 };

 pDeInitFunc = DeInitSCSI_28;

 memset(&s,0,sizeof(s));
 s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 s.SRB_HaId       = iCD_AD;
 s.SRB_Target     = iCD_TA;
 s.SRB_Lun        = iCD_LU;
 s.SRB_Flags      = SRB_EVENT_NOTIFY;
 s.SRB_BufLen     = 0x0C;
 s.SRB_BufPointer = init1;
 s.SRB_SenseLen   = SENSE_LEN;
 s.SRB_CDBLen     = 6;
 s.SRB_PostProc   = (LPVOID)hEvent;
 s.CDBByte[0]     = 0x15;
 s.CDBByte[4]     = 0x0C;

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&s);
 if(dwStatus==SS_PENDING)
  WaitGenEvent(WAITFOREVER);

 if(s.SRB_Status!=SS_COMP)
  return SS_ERR;

 return s.SRB_Status;
}

/////////////////////////////////////////////////////////

DWORD DeInitSCSI_28(void)
{
 SRB_ExecSCSICmd s;DWORD dwStatus;
 BYTE init1[] = { 0, 0, 0, 8, 83, 0, 0, 0, 0, 0, 8, 0 };

 memset(&s,0,sizeof(s));
 s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 s.SRB_HaId       = iCD_AD;
 s.SRB_Target     = iCD_TA;
 s.SRB_Lun        = iCD_LU;
 s.SRB_Flags      = SRB_EVENT_NOTIFY | SRB_ENABLE_RESIDUAL_COUNT;
 s.SRB_BufLen     = 0x0C;
 s.SRB_BufPointer = init1;
 s.SRB_SenseLen   = SENSE_LEN;
 s.SRB_CDBLen     = 6;
 s.SRB_PostProc   = (LPVOID)hEvent;
 s.CDBByte[0]     = 0x15;
 s.CDBByte[4]     = 0x0C;

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&s);
 if(dwStatus==SS_PENDING)
  WaitGenEvent(WAITFOREVER);

 if(s.SRB_Status!=SS_COMP)
  return SS_ERR;

 return s.SRB_Status;
}

/////////////////////////////////////////////////////////

DWORD ReadSCSI_28(BOOL bWait,FRAMEBUF * f)
{
 DWORD dwStatus;

 if(!pDeInitFunc)
  {
   if(iRType==MODE_28_2)
    {
     if(InitSCSI_28_2()!=SS_COMP) return SS_ERR;
    }
   else
    {
     if(InitSCSI_28_1()!=SS_COMP) return SS_ERR;
    }
  }

 memset(&sx,0,sizeof(sx));
 sx.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 sx.SRB_HaId       = iCD_AD;
 sx.SRB_Target     = iCD_TA;
 sx.SRB_Lun        = iCD_LU;
 sx.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 sx.SRB_BufLen     = f->dwBufLen;
 sx.SRB_BufPointer = &(f->BufData[0]);
 sx.SRB_SenseLen   = SENSE_LEN;
 sx.SRB_CDBLen     = 10;
 sx.SRB_PostProc   = (LPVOID)hEvent;
 sx.CDBByte[0]     = 0x28;    // read10 command
 sx.CDBByte[1]     = iCD_LU << 5;
 sx.CDBByte[2]     = (unsigned char)((f->dwFrame >> 24) & 0xFF);
 sx.CDBByte[3]     = (unsigned char)((f->dwFrame >> 16) & 0xFF);
 sx.CDBByte[4]     = (unsigned char)((f->dwFrame >> 8) & 0xFF);
 sx.CDBByte[5]     = (unsigned char)(f->dwFrame & 0xFF);
 sx.CDBByte[8]     = (unsigned char)(f->dwFrameCnt & 0xFF);

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&sx);
 if(dwStatus==SS_PENDING)
  {
   if(bWait) WaitGenEvent(WAITFOREVER);
   else
    {
     bDoWaiting=TRUE;
     return SS_COMP;
    }
  }

 if(sx.SRB_Status!=SS_COMP)
  return SS_ERR;

 return SS_COMP;
}

/////////////////////////////////////////////////////////
// DVD MODE

DWORD ReadSCSI_28_2048(BOOL bWait,FRAMEBUF * f)
{
 DWORD dwStatus;

 if(!pDeInitFunc)
  {
   InitSCSI_28_2048();

   //if(InitSCSI_28_2048()!=SS_COMP) return SS_ERR;
  }

 memset(&sx,0,sizeof(sx));
 sx.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 sx.SRB_HaId       = iCD_AD;
 sx.SRB_Target     = iCD_TA;
 sx.SRB_Lun        = iCD_LU;
 sx.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 sx.SRB_BufLen     = f->dwBufLen;
 sx.SRB_BufPointer = &(f->BufData[0]);
 sx.SRB_SenseLen   = SENSE_LEN;
 sx.SRB_CDBLen     = 10;
 sx.SRB_PostProc   = (LPVOID)hEvent;
 sx.CDBByte[0]     = 0x28;    // read10 command
 sx.CDBByte[1]     = iCD_LU << 5;
 sx.CDBByte[2]     = (unsigned char)((f->dwFrame >> 24) & 0xFF);
 sx.CDBByte[3]     = (unsigned char)((f->dwFrame >> 16) & 0xFF);
 sx.CDBByte[4]     = (unsigned char)((f->dwFrame >> 8) & 0xFF);
 sx.CDBByte[5]     = (unsigned char)(f->dwFrame & 0xFF);
 sx.CDBByte[8]     = (unsigned char)(f->dwFrameCnt & 0xFF);
 sx.CDBByte[9]     = 0xF8;

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&sx);
 if(dwStatus==SS_PENDING)
  {
   if(bWait) WaitGenEvent(WAITFOREVER);
   else
    {
     bDoWaiting=TRUE;
     return SS_COMP;
    }
  }

 if(sx.SRB_Status!=SS_COMP)
  return SS_ERR;

 return SS_COMP;
}

DWORD ReadSCSI_28_2048_Ex(BOOL bWait,FRAMEBUF * f)
{
 DWORD dwStatus;

 if(!pDeInitFunc)
  {
   InitSCSI_28_2048();

   //if(InitSCSI_28_2048()!=SS_COMP) return SS_ERR;
  }

 memset(&sx,0,sizeof(sx));
 sx.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 sx.SRB_HaId       = iCD_AD;
 sx.SRB_Target     = iCD_TA;
 sx.SRB_Lun        = iCD_LU;
 sx.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 sx.SRB_BufLen     = f->dwBufLen;
 sx.SRB_BufPointer = &(f->BufData[0]);
 sx.SRB_SenseLen   = SENSE_LEN;
 sx.SRB_CDBLen     = 10;
 sx.SRB_PostProc   = (LPVOID)hEvent;
 sx.CDBByte[0]     = 0x28;    // read10 command
 sx.CDBByte[1]     = iCD_LU << 5;
 sx.CDBByte[2]     = (unsigned char)((f->dwFrame >> 24) & 0xFF);
 sx.CDBByte[3]     = (unsigned char)((f->dwFrame >> 16) & 0xFF);
 sx.CDBByte[4]     = (unsigned char)((f->dwFrame >> 8) & 0xFF);
 sx.CDBByte[5]     = (unsigned char)(f->dwFrame & 0xFF);
 sx.CDBByte[8]     = (unsigned char)(f->dwFrameCnt & 0xFF);
 // NO F8
 //sx.CDBByte[9]     = 0xF8;

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&sx);
 if(dwStatus==SS_PENDING)
  {
   if(bWait) WaitGenEvent(WAITFOREVER);
   else
    {
     bDoWaiting=TRUE;
     return SS_COMP;
    }
  }

 if(sx.SRB_Status!=SS_COMP)
  return SS_ERR;

 return SS_COMP;
}


/////////////////////////////////////////////////////////
// stupid subc reading on Teac 532S

char tbuf[2368];

DWORD ReadSCSI_28_Sub(BOOL bWait,FRAMEBUF * f)
{
 DWORD dwStatus;

 if(!pDeInitFunc)
  {
   if(iRType==MODE_28_2)
    {
     if(InitSCSI_28_2()!=SS_COMP) return SS_ERR;
    }
   else
    {
     if(InitSCSI_28_1()!=SS_COMP) return SS_ERR;
    }
  }

 memset(&sx,0,sizeof(sx));
 sx.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 sx.SRB_HaId       = iCD_AD;
 sx.SRB_Target     = iCD_TA;
 sx.SRB_Lun        = iCD_LU;
 sx.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 sx.SRB_BufLen     = f->dwBufLen;
 sx.SRB_BufPointer = &(f->BufData[0]);
 sx.SRB_SenseLen   = SENSE_LEN;
 sx.SRB_CDBLen     = 10;
 sx.SRB_PostProc   = (LPVOID)hEvent;
 sx.CDBByte[0]     = 0x28;                 // read10
 sx.CDBByte[1]     = iCD_LU << 5;
 sx.CDBByte[2]     = (unsigned char)((f->dwFrame >> 24) & 0xFF);
 sx.CDBByte[3]     = (unsigned char)((f->dwFrame >> 16) & 0xFF);
 sx.CDBByte[4]     = (unsigned char)((f->dwFrame >> 8) & 0xFF);
 sx.CDBByte[5]     = (unsigned char)(f->dwFrame & 0xFF);
 sx.CDBByte[8]     = (unsigned char)(f->dwFrameCnt & 0xFF);

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&sx);
 if(dwStatus==SS_PENDING)
  WaitGenEvent(WAITFOREVER);

 if(sx.SRB_Status!=SS_COMP)
  return SS_ERR;

 memset(&sx,0,sizeof(sx));
 sx.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 sx.SRB_HaId       = iCD_AD;
 sx.SRB_Target     = iCD_TA;
 sx.SRB_Lun        = iCD_LU;
 sx.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 sx.SRB_BufLen     = 2368;
 sx.SRB_BufPointer = tbuf;
 sx.SRB_SenseLen   = SENSE_LEN;
 sx.SRB_CDBLen     = 12;
 sx.SRB_PostProc   = (LPVOID)hEvent;
 sx.CDBByte[0]     = 0xD8;
 sx.CDBByte[2]     = (unsigned char)((f->dwFrame >> 24) & 0xFF);
 sx.CDBByte[3]     = (unsigned char)((f->dwFrame >> 16) & 0xFF);
 sx.CDBByte[4]     = (unsigned char)((f->dwFrame >> 8) & 0xFF);
 sx.CDBByte[5]     = (unsigned char)(f->dwFrame & 0xFF);
 sx.CDBByte[9]     = (unsigned char)(f->dwFrameCnt & 0xFF);
 sx.CDBByte[10]    = 1;

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&sx);
 if(dwStatus==SS_PENDING)
  WaitGenEvent(WAITFOREVER);

 if(sx.SRB_Status!=SS_COMP)
  return SS_ERR;

 memcpy(&SubCData[12],&tbuf[2352],16);

 return SS_COMP;
}                            


/////////////////////////////////////////////////////////
// various simple scsi sub data read funcs... used for 
// ripping and subread checking... first 2352 bytes can
// be trash after read, only the bytes after are important
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////

int ReadSub_BE_2(unsigned long addr,unsigned char * pBuf,int iNum)
{
 DWORD dwStatus;

 memset(&sx,0,sizeof(sx));
 sx.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 sx.SRB_HaId       = iCD_AD;
 sx.SRB_Target     = iCD_TA;
 sx.SRB_Lun        = iCD_LU;
 sx.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 sx.SRB_BufLen     = 2368*iNum;
 sx.SRB_BufPointer = pBuf;
 sx.SRB_SenseLen   = SENSE_LEN;
 sx.SRB_CDBLen     = 12;
 sx.SRB_PostProc   = (LPVOID)hEvent;
 sx.CDBByte[0]     = 0xBE;
 sx.CDBByte[2]     = (unsigned char)((addr >> 24) & 0xFF);
 sx.CDBByte[3]     = (unsigned char)((addr >> 16) & 0xFF);
 sx.CDBByte[4]     = (unsigned char)((addr >> 8) & 0xFF);
 sx.CDBByte[5]     = (unsigned char)(addr & 0xFF);
 sx.CDBByte[8]     = iNum;
 sx.CDBByte[9]     = 0xF8;
 sx.CDBByte[10]    = 0x2;

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command( (LPSRB)&sx );
 if(dwStatus==SS_PENDING)
  WaitGenEvent(WAITSUB);

 if(sx.SRB_Status!=SS_COMP)
  return 0;

 return 1;
}

/////////////////////////////////////////////////////////

int ReadSub_BE_2_1(unsigned long addr,unsigned char * pBuf,int iNum)
{
 DWORD dwStatus;

 memset(&sx,0,sizeof(sx));
 sx.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 sx.SRB_HaId       = iCD_AD;
 sx.SRB_Target     = iCD_TA;
 sx.SRB_Lun        = iCD_LU;
 sx.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 sx.SRB_BufLen     = 2448*iNum;                        // special! 96 bytes instead of 16
 sx.SRB_BufPointer = pBuf;
 sx.SRB_SenseLen   = SENSE_LEN;
 sx.SRB_CDBLen     = 12;
 sx.SRB_PostProc   = (LPVOID)hEvent;
 sx.CDBByte[0]     = 0xBE;
 sx.CDBByte[2]     = (unsigned char)((addr >> 24) & 0xFF);
 sx.CDBByte[3]     = (unsigned char)((addr >> 16) & 0xFF);
 sx.CDBByte[4]     = (unsigned char)((addr >> 8) & 0xFF);
 sx.CDBByte[5]     = (unsigned char)(addr & 0xFF);
 sx.CDBByte[8]     = iNum;
 sx.CDBByte[9]     = 0xF8;
 sx.CDBByte[10]    = 0x1;

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&sx);
 if(dwStatus==SS_PENDING)
  WaitGenEvent(WAITSUB);

 if(sx.SRB_Status!=SS_COMP)
  return 0;

 return 1;
}

/////////////////////////////////////////////////////////

int ReadSub_D8(unsigned long addr,unsigned char * pBuf,int iNum)
{
 DWORD dwStatus;

 memset(&sx,0,sizeof(sx));
 sx.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 sx.SRB_HaId       = iCD_AD;
 sx.SRB_Target     = iCD_TA;
 sx.SRB_Lun        = iCD_LU;
 sx.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 sx.SRB_BufLen     = 2368*iNum;
 sx.SRB_BufPointer = pBuf;
 sx.SRB_SenseLen   = SENSE_LEN;
 sx.SRB_CDBLen     = 12;
 sx.SRB_PostProc   = (LPVOID)hEvent;
 sx.CDBByte[0]     = 0xD8;
 sx.CDBByte[2]     = (unsigned char)((addr >> 24) & 0xFF);
 sx.CDBByte[3]     = (unsigned char)((addr >> 16) & 0xFF);
 sx.CDBByte[4]     = (unsigned char)((addr >> 8) & 0xFF);
 sx.CDBByte[5]     = (unsigned char)(addr & 0xFF);
 sx.CDBByte[9]     = iNum;
 sx.CDBByte[10]    = 1;

 ResetEvent(hEvent);
 dwStatus=pSendASPI32Command((LPSRB)&sx);
 if(dwStatus==SS_PENDING)
  WaitGenEvent(WAITFOREVER);

 if(sx.SRB_Status!=SS_COMP)
  return 0;

 return 1;
}

/////////////////////////////////////////////////////////
// liteon subdata decoding

void DecodeSub_BE_2_1(unsigned char * pBuf)
{
 int i,j;
 unsigned char * pS=pBuf;
 unsigned char c;

 for(i=0;i<12;i++)
  {
   c=0;
   for(j=7;j>=0;j--,pS++)
    {
     if(*pS & 0x40) c|=(1<<j);
    }
   *(pBuf+i)=c;
  }

 for(i=12;i<16;i++) *(pBuf+i)=0;
}

/////////////////////////////////////////////////////////
// auto-detection of read mode (without subc), called
// on plugin startup

#define MAXMODES 6

DWORD CheckSCSIReadMode(void)
{
 DWORD dwStatus;int i,j,k,iCnt;char * p;
 int iModes[MAXMODES]={MODE_BE_2,MODE_BE_1,MODE_28_1,MODE_28_2,MODE_28_2048,MODE_28_2048_Ex};
 int iBlock[MAXMODES]={2352,     2352,     2352,     2352,     2048,        2048};
 unsigned char cdb[3000];   
 FRAMEBUF * f=(FRAMEBUF *)cdb;

 for(i=0;i<MAXMODES;i++)                               // loop avail read modes
  {
   f->dwFrame    = 16;                                 // we check on addr 16 (should be available on all ps2 cds/dvds)
   f->dwFrameCnt = 1;  
   f->dwBufLen   = iBlock[i];

   pDeInitFunc = NULL;
   iRType=iModes[i];                                   // set global read mode
   GetGenReadFunc(iRType);                             // get read func pointer

   for(j=0;j<3;j++)                                    // try it 3 times
    {
     memset(f->BufData,0xAA,f->dwBufLen);              // fill buf with AA
     dwStatus=pReadFunc(TRUE,f);                       // do the read

#ifdef DBGOUT  	 
auxprintf("status %d\n",dwStatus);
#endif

     if(dwStatus!=SS_COMP) continue;                   // error? try again

     p=&(f->BufData[0]);                               

#ifdef DBGOUT  	 
auxprintf("check mode %d\n",i);
#endif

     for(k=0,iCnt=0;k<(int)f->dwBufLen;k+=4,p+=4)      // now check the returned data
      {
#ifdef DBGOUT  	 
// auxprintf("%08x ",*((DWORD *)p));
#endif

       if(*((DWORD *)p)==0xAAAAAAAA)                   // -> still AA? bad
            iCnt++;
       else iCnt=0;

       if(iCnt>=8) {dwStatus=SS_ERR;break;}            // -> if we have found many AA's, the reading was bad
      }

     if(dwStatus==SS_COMP)                             // reading was a success, no AA's?
      {
       iRType         = iModes[i];                     // -> set found mode
       iUsedBlockSize = iBlock[i];
       if(iUsedBlockSize==2352)
            iUsedMode=CDVD_MODE_2352;
       else iUsedMode=CDVD_MODE_2048;
       
#ifdef DBGOUT  	 
auxprintf("mode found %d\n",i);
#endif
       
       return dwStatus;                                // -> bye
      }
    }
   if(pDeInitFunc) pDeInitFunc();                      // deinit, try next mode
  }
  
 return dwStatus;
}

/////////////////////////////////////////////////////////
// dummy read dunc

DWORD ReadSCSI_Dummy(BOOL bWait,FRAMEBUF * f)
{
 return SS_ERR;
}

/////////////////////////////////////////////////////////
