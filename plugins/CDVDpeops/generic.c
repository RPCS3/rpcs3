/***************************************************************************
                           generic.c  -  description
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
// 2004/12/25 - Pete
// - repaired time2addr/addr2time
//
// 2003/11/16 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

/////////////////////////////////////////////////////////

#include "stdafx.h"
#define _IN_GENERIC
#include "externals.h"

/////////////////////////////////////////////////////////

int        iCD_AD=-1;                                  // drive address
int        iCD_TA=-1;               
int        iCD_LU=-1;
int        iRType=0;                                   // read mode
int        iUseSpeedLimit=0;                           // speed limit use
int        iSpeedLimit=2;                              // speed 2x
int        iNoWait=0;                                  // wait
int        iMaxRetry=5;                                // retry on error
int        iShowReadErr=0;                             // show msg on error
HANDLE     hEvent=NULL;                                // global event
HINSTANCE  hASPI=NULL;                                 // aspi lib
READFUNC   pReadFunc=NULL;                             // read func
DEINITFUNC pDeInitFunc=NULL;                           // deinit func
int        iInterfaceMode=1;                           // interface (aspi/ioctrlscsi/ioctrlraw)
int        iWantedBlockSize=2352;
int        iUsedBlockSize=2352;
int        iUsedMode=CDVD_MODE_2352;
int        iBlockDump=0;

DWORD (*pGetASPI32SupportInfo)(void);                  // ptrs to aspi funcs
DWORD (*pSendASPI32Command)(LPSRB);

/////////////////////////////////////////////////////////

void addr2time(unsigned long addr, unsigned char *time)
{
 addr+=150;
 time[3] = (unsigned char)(addr%75);
 addr/=75;
 time[2]=(unsigned char)(addr%60);
 addr/=60;
 time[1]=(unsigned char)(addr%100);
 time[0]=(unsigned char)(addr/100);
}

void addr2timeB(unsigned long addr, unsigned char *time)
{
 time[3] = itob((unsigned char)(addr%75));
 addr/=75;
 time[2]=itob((unsigned char)(addr%60));
 addr/=60;
 time[1]=itob((unsigned char)(addr%100));
 time[0]=itob((unsigned char)(addr/100));
}

unsigned long time2addr(unsigned char *time)
{
 unsigned long addr;

 addr =  time[0]*100;
 addr += time[1];
 addr *= 60;
 
 addr = (addr + time[2])*75;
 addr += time[3];
 addr -= 150;
 return addr;
}

unsigned long time2addrB(unsigned char *time)
{
 unsigned long addr;

 addr =  (btoi(time[0]))*100;
 addr += btoi(time[1]);
 addr *= 60;
 addr = (addr + btoi(time[2]))*75;
 addr += btoi(time[3]);
 addr -= 150;
 return addr;
}

#ifndef _GCC

#ifdef __x86_64__
unsigned long reOrder(unsigned long value)
{
    return ((value&0xff)<<24)|((value&0xff00)<<8)|((value&0xff0000)>>8)|(value>>24);
}
#else
unsigned long reOrder(unsigned long value)
{
#pragma warning (disable: 4035)
   __asm 
    {
     mov eax,value
     bswap eax
    }
}
#endif
#endif

/////////////////////////////////////////////////////////

void CreateGenEvent(void)
{
 hEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
}

/////////////////////////////////////////////////////////

void FreeGenEvent(void)
{
 if(hEvent) CloseHandle(hEvent);
 hEvent=0;
}

/////////////////////////////////////////////////////////

DWORD WaitGenEvent(DWORD dwMS)
{
 if(hASPI)                                             // aspi event
  return WaitForSingleObject(hEvent,dwMS);
 else
  {                                                    // ioctl overlapped (always waiting til finished, dwMS not used)
   DWORD dwR=0;
   GetOverlappedResult(hIOCTL,&ovcIOCTL,&dwR,TRUE);
   return 0;
  }
}

/////////////////////////////////////////////////////////

void LockGenCDAccess(void)
{
 if(hReadThread)                                       // thread mode?
  WaitForSingleObject(hThreadMutex[0],INFINITE);       // -> wait until all reading is done
 else                                                  // async prefetch?
 if(bDoWaiting)                                        // -> async operation has to finish first
  {WaitGenEvent(0xFFFFFFFF);bDoWaiting=FALSE;}
}

/////////////////////////////////////////////////////////

void UnlockGenCDAccess(void)
{
 if(hReadThread)                                       // thread mode?
  ReleaseMutex(hThreadMutex[0]);                       // -> we are finished with our special command, now reading can go on
}

/////////////////////////////////////////////////////////

void WaitUntilDriveIsReady(void)
{
 if(iNoWait==0)
  {
   while(TestSCSIUnitReady()==0) Sleep(500);
  } 
}

/////////////////////////////////////////////////////////

void SetGenCDSpeed(int iReset)
{
 if(iUseSpeedLimit)
  {
   if(bDoWaiting)                                      // still a command running? wait 
    {WaitGenEvent(0xFFFFFFFF);bDoWaiting=FALSE;}

   if(iReset) SetSCSISpeed(0xFFFF);
   else
   if(SetSCSISpeed(176*iSpeedLimit)<=0)
    {
     MessageBox(GetActiveWindow(),
                "Failure: cannot change the drive speed!",
                "cdvdPeops... speed limitation",
                MB_OK|MB_ICONEXCLAMATION);
     iUseSpeedLimit=0;
    }
  }
}

/////////////////////////////////////////////////////////
// checks, which direct subchannel reading type is supported

void GetBESubReadFunc(void)
{
 unsigned char * pB=(unsigned char *)malloc(4096);

 pReadFunc  = ReadSCSI_BE_Sub;                         // pre-init read func

 WaitUntilDriveIsReady();                              // wait before first read

 ReadSub_BE_2(0,pB,1);                                 // first (unchecked) read
 if(!ReadSub_BE_2(0,pB,1))                             // read again, and check this time
  {                                                    // -> read failed?
   if(ReadSub_BE_2_1(0,pB,1))                          // -> try different sub reading
    {                                                  // --> success? mmm... let us check the data
     DecodeSub_BE_2_1(pB+2352);                        // --> decode sub
     if(*(pB+2352)==0x41)                              // --> check the first decoded byte
      pReadFunc  = ReadSCSI_BE_Sub_1;                  // ---> wow, byte is ok
    }
  }
 free(pB);                                            
}

/////////////////////////////////////////////////////////

int GetGenReadFunc(int iRM)
{
 switch(iRM)                                           // scsi read mode
  {
   // ------------------------------------------------ //
   case MODE_BE_1:
   case MODE_BE_2:
    {
     if(iUseSubReading==1)
           GetBESubReadFunc();
      else pReadFunc = ReadSCSI_BE;
     } break;
   // ------------------------------------------------ //
   case MODE_28_1:
   case MODE_28_2:
    {
     if(iUseSubReading==1)
          pReadFunc  = ReadSCSI_28_Sub;
     else pReadFunc  = ReadSCSI_28;
    } break;
   // ------------------------------------------------ //
   case MODE_28_2048:
    {
     pReadFunc       = ReadSCSI_28_2048;
    } break;
   // ------------------------------------------------ //
   case MODE_28_2048_Ex:
    {
     pReadFunc       = ReadSCSI_28_2048_Ex;
    } break;
   // ------------------------------------------------ //
   default:
    {
     pReadFunc       = ReadSCSI_Dummy;
    } return -3;
   // ------------------------------------------------ //
  }
 return 1;
}

/////////////////////////////////////////////////////////

int OpenGenCD(int iA,int iT,int iL)
{
 pDeInitFunc = NULL;                                   // init de-init func
 pReadFunc   = ReadSCSI_Dummy;                         // init (dummy) read func

 if(iA==-1) return -1;                                 // not configured properly

 // -------------------------------------------------- //

 if(iInterfaceMode>1)                                  // ioctrl interfaces?
  {
   OpenIOCTLHandle(iA,iT,iL);                          // open w2k/xp ioctrl device
   if(hIOCTL==NULL) return -2;                         // no cdrom available

   if(iInterfaceMode==3)                               // special ioctl RAW mode?
    {                                                  // -> get special reading funcs (non-scsi!)
     if(iUseSubReading==1)                         
          pReadFunc  = ReadIOCTL_Raw_Sub;
     else pReadFunc  = ReadIOCTL_Raw;

     WaitUntilDriveIsReady();
     return 1;
    }
  }
 else                                                  // aspi interface?
  {
   int iDevice=GetSCSIDevice(iA,iT,iL);                // get device type
   if(iDevice!=DTYPE_CDROM) return -2;                 // no cdrom? bye
  }

 if(CheckSCSIReadMode()==SS_COMP)
  WaitUntilDriveIsReady();
 else 
  {
   CloseIOCTLHandle();
   return -3;
  }

 return 1;
}

/////////////////////////////////////////////////////////

void CloseGenCD(void)
{
 iCDROK=0;                                             // no more cd available
 if(pDeInitFunc) pDeInitFunc();                        // deinit, if needed
 pDeInitFunc = NULL;                
 pReadFunc   = ReadSCSI_Dummy;
 CloseIOCTLHandle();                                   // close ioctl drive file (if used)
}

/////////////////////////////////////////////////////////

void OpenGenInterface(void)
{
 hASPI=NULL;

 if(iInterfaceMode==0) return;                         // no interface? no fun
 else
 if(iInterfaceMode==1)                                 // aspi
  {
   hASPI=LoadLibrary("WNASPI32.DLL");
   if(hASPI)
    {
     pGetASPI32SupportInfo =
      (DWORD(*)(void)) GetProcAddress(hASPI,"GetASPI32SupportInfo");
     pSendASPI32Command =
      (DWORD(*)(LPSRB))GetProcAddress(hASPI,"SendASPI32Command");

     if(!pGetASPI32SupportInfo || !pSendASPI32Command)
      {
       iInterfaceMode=0;
       return;
      }
    }
  }
 else                                                  // ioctl
  {
   if(iInterfaceMode<2 || iInterfaceMode>3) iInterfaceMode=2;
   pGetASPI32SupportInfo = NULL;
   pSendASPI32Command    = IOCTLSendASPI32Command;
  }
}

/////////////////////////////////////////////////////////

void CloseGenInterface(void)
{
 pGetASPI32SupportInfo=NULL;                           // clear funcs
 pSendASPI32Command=NULL;

 if(hASPI)                                             // free aspi
  {
   FreeLibrary(hASPI);
   hASPI=NULL;
  }
 else CloseIOCTLHandle();                              // or close ioctl file
}

/////////////////////////////////////////////////////////

int GetGenCDDrives(char * pDList)
{
 if(hASPI) return GetSCSICDDrives(pDList);             // aspi? use it
 return GetIOCTLCDDrives(pDList);                      // or use ioctl
}

/////////////////////////////////////////////////////////
