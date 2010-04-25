/***************************************************************************
                           read.c  -  description
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
#define _IN_READ
#include "externals.h"

/////////////////////////////////////////////////////////

READTRACKFUNC   pReadTrackFunc=NULL;
GETPTRFUNC      pGetPtrFunc=NULL;

int iUseCaching=0;
int iTryAsync=0;
int iBufSel=0;

unsigned char * pMainBuffer=0;
unsigned char * pCurrReadBuf=0;
unsigned char * pFirstReadBuf=0;
unsigned char * pAsyncBuffer=0;

unsigned long   lMaxAddr=0;
unsigned long   lLastAddr         = 0xFFFFFFFF;
unsigned long   lLastAsyncAddr    = 0xFFFFFFFF;
unsigned long   lNeededAddr       = 0xFFFFFFFF;
unsigned long   lLastAccessedAddr = 0xFFFFFFFF;
int             iLastAccessedMode=0;

unsigned char * ptrBuffer[2];
unsigned char * pAsyncFirstReadBuf[2];


#define MAXQSIZE  16
#define MAXQFETCH 8

unsigned long   lAddrQ[MAXQSIZE];
int             iQPos=0;
int             iQIdle=0;
int             iQLockPos=-1;

/////////////////////////////////////////////////////////
// thread helper vars

HANDLE          hReadThread  = NULL;
BOOL            bThreadEnded = FALSE;
HANDLE          hThreadEvent[3];
HANDLE          hThreadMutex[2];

/////////////////////////////////////////////////////////
// internal MAXDATACACHE*64KB (4MB) data cache

#define MAXDATACACHE 64
unsigned long   lDataCacheAddr[MAXDATACACHE];
unsigned char * pDataCacheBuf[MAXDATACACHE];
BOOL            bDataCacheHit=FALSE;
int             iUseDataCache=0;

/////////////////////////////////////////////////////////
// main init func

void CreateREADBufs(void)
{
 switch(iUseCaching)
  {
   case 4:  iUseDataCache  = 2; // use a special data cache on threadex reading
            pReadTrackFunc = DoReadThreadEx;
            pGetPtrFunc    = GetREADThreadExPtr; break;
   case 3:  pReadTrackFunc = DoReadThread;
            pGetPtrFunc    = GetREADThreadPtr;   break;
   case 2:  pReadTrackFunc = DoReadAsync;
            pGetPtrFunc    = NULL;               break;
   default: pReadTrackFunc = DoRead;
            pGetPtrFunc    = NULL;               break;
  }

 hThreadEvent[0]=NULL;                                 // clear events/mutex
 hThreadEvent[1]=NULL;
 hThreadEvent[2]=NULL;
 hThreadMutex[0]=NULL;
 hThreadMutex[1]=NULL;

 AllocDataCache();                                     // build data cache, if wanted

 lLastAddr      = 0xFFFFFFFF;
 lLastAsyncAddr = 0xFFFFFFFF;
 iBufSel        = 0;

 if(iUseCaching)                                       // some caching? need bigger buffer
      pMainBuffer=(unsigned char *)malloc(MAXCDBUFFER);
 else pMainBuffer=(unsigned char *)malloc(CDSECTOR+208+96);

 pCurrReadBuf=pFirstReadBuf=pMainBuffer+FRAMEBUFEXTRA;

 if(iUseCaching>=2)                                    // async/thread mode
  {
   pAsyncBuffer=(unsigned char *)malloc(MAXCDBUFFER);
   ptrBuffer[0]=pMainBuffer;
   ptrBuffer[1]=pAsyncBuffer;
   pAsyncFirstReadBuf[0]=pFirstReadBuf;
   pAsyncFirstReadBuf[1]=pAsyncBuffer+FRAMEBUFEXTRA;

   if(iUseCaching>=3)                                  // thread mode
    {
     DWORD dw;
     bThreadEnded = FALSE;

     for(dw=0;dw<3;dw++)                               // -> create events
      {
       hThreadEvent[dw]=CreateEvent(NULL,TRUE,FALSE,NULL);
       ResetEvent(hThreadEvent[dw]);
      }
     for(dw=0;dw<2;dw++)                               // -> create mutex
      {
       hThreadMutex[dw]=CreateMutex(NULL,FALSE,NULL);
      }
     if(iUseCaching==3)                                // -> create thread
      hReadThread=CreateThread(NULL,0,READThread,0,0,&dw);
     else
      {
       for(dw=0;dw<MAXQSIZE;dw++) lAddrQ[dw]=0xFFFFFFFF;
       iQPos=0;
       iQLockPos=-1;
       hReadThread=CreateThread(NULL,0,READThreadEx,0,0,&dw);
      }
    }
  }
 else
  pAsyncBuffer=0;
}

/////////////////////////////////////////////////////////

void FreeREADBufs(void)
{
 if(hReadThread)                                       // thread?
  {
   SetEvent(hThreadEvent[1]);                          // -> signal: end thread
   while(!bThreadEnded) {Sleep(5L);}                   // -> wait til ended
   WaitForSingleObject(hThreadMutex[1],INFINITE);
   ReleaseMutex(hThreadMutex[1]);
   hReadThread=NULL;                                   // -> clear handle
  }

 if(hThreadEvent[0]) CloseHandle(hThreadEvent[0]);     // kill events/mutex
 if(hThreadEvent[1]) CloseHandle(hThreadEvent[1]);
 if(hThreadEvent[2]) CloseHandle(hThreadEvent[2]);
 if(hThreadMutex[0]) CloseHandle(hThreadMutex[0]);
 if(hThreadMutex[1]) CloseHandle(hThreadMutex[1]);

 if(pMainBuffer) free(pMainBuffer);                    // free main data buf
 pMainBuffer=NULL;
 if(pAsyncBuffer) free(pAsyncBuffer);                  // free async data buf
 pAsyncBuffer=NULL;

 FreeDataCache();
}

/////////////////////////////////////////////////////////
// retry on readng error (blocking)

BOOL bReadRetry(FRAMEBUF * f)
{
 int iRetry=0;

 while (iRetry<iMaxRetry)
  {
   if(pReadFunc(TRUE,f)==SS_COMP) break;               // try sync read
   iRetry++;
  }

 if(iRetry==iMaxRetry)                                 // no success?
  {
   if(iShowReadErr)                                    // -> tell it to user
    {
     char szB[64];
     wsprintf(szB,"Read error on address %08lx!",f->dwFrame);
     MessageBox(NULL,szB,libraryName,MB_OK);
    }
   return FALSE;                                       // -> tell emu: bad
  }

 return TRUE;
}

////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// sync modes (caching 0 and 1)
// just reads one or more blocks, and always waits until
// reading is done
/////////////////////////////////////////////////////////

BOOL DoRead(unsigned long addr)
{
 FRAMEBUF * f;

 ////////////////////////////////////////////////////////

 if(iUseCaching &&                                     // cache block available?
    lLastAddr!=0xFFFFFFFF &&
    addr>=lLastAddr &&                                 // and addr in block?
    addr<=(lLastAddr+MAXCACHEBLOCK))
  {
   pCurrReadBuf=pFirstReadBuf+                         // -> calc data ptr
                 ((addr-lLastAddr)*iUsedBlockSize);
   if(ppfHead) CheckPPFCache(addr,pCurrReadBuf);       // -> apply ppf
   return TRUE;                                        // -> done
  }

 ////////////////////////////////////////////////////////

 if(iUseDataCache && CheckDataCache(addr))             // cache used and data is in cache? set read ptr, if yes
  return TRUE;                                         // -> also fine

 ////////////////////////////////////////////////////////

 f=(FRAMEBUF *)pMainBuffer;                            // setup read for one sector

 f->dwFrameCnt = 1;
 f->dwBufLen   = iUsedBlockSize;
 f->dwFrame    = addr;

 pCurrReadBuf=pFirstReadBuf;

 ////////////////////////////////////////////////////////

 if(iUseCaching)                                       // cache block?
  {
   if((addr+MAXCACHEBLOCK)<lMaxAddr)                   // and big read is possible?
    {
     f->dwFrameCnt = MAXCACHEBLOCK+1;                  // -> set bigger read
     f->dwBufLen   = (MAXCACHEBLOCK+1)*iUsedBlockSize;
     lLastAddr     = addr;                             // -> store addr of block
    }
   else
    {
     lLastAddr=0xFFFFFFFF;                             // no caching, no block addr
    }
  }

 ////////////////////////////////////////////////////////

 if(pReadFunc(TRUE,f)!=SS_COMP)                        // do a waiting read
  {
   if(!bReadRetry(f)) return FALSE;                    // and retry on error
  }

 if(iUseDataCache && lLastAddr!=0xFFFFFFFF)            // data cache used? and whole 64 k read block?
  AddToDataCache(addr,pFirstReadBuf);                  // -> add the complete data to cache

 if(ppfHead) CheckPPFCache(addr,pCurrReadBuf);         // apply ppf

 return TRUE;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// async mode (caching 2)
// this mode works fine with ASPI...
// the first read will be done sync, though, only the
// additional pre-fetching will be done async...
// well, with mdecs most reads will be prefetched, so
// speed is good... with IOCTL this mode is more like
// a 'double sync' reading, since IOCTL seems always
// to be blocking (see also notes for caching mode 3)
/////////////////////////////////////////////////////////

BOOL DoReadAsync(unsigned long addr)
{
 FRAMEBUF * f;

 ////////////////////////////////////////////////////////
 // 1. check if data is in already filled buffer

 if(lLastAddr!=0xFFFFFFFF &&
    addr>=lLastAddr &&
    addr<=(lLastAddr+MAXCACHEBLOCK))
  {
   pCurrReadBuf=pAsyncFirstReadBuf[iBufSel]+
                 ((addr-lLastAddr)*iUsedBlockSize);

   if(ppfHead) CheckPPFCache(addr,pCurrReadBuf);

   iTryAsync=0;
   return TRUE;
  }

 ////////////////////////////////////////////////////////
 // check data cache

 if(iUseDataCache && CheckDataCache(addr))             // cache used and data is in cache? set read ptr, if yes
  return TRUE;                                         // -> also fine

 ////////////////////////////////////////////////////////
 // 2. not in main buffer? wait for async to be finished

 if(bDoWaiting)
  {
   WaitGenEvent(0xFFFFFFFF);bDoWaiting=FALSE;
   if(sx.SRB_Status!=SS_COMP) lLastAsyncAddr=0xFFFFFFFF;
  }

 ////////////////////////////////////////////////////////
 // 3. check in asyncbuffer. if yes, swap buffers and do next async read

 if(lLastAsyncAddr!=0xFFFFFFFF &&
    addr>=lLastAsyncAddr &&
    addr<=(lLastAsyncAddr+MAXCACHEBLOCK))
  {
   int iAsyncSel=iBufSel;                              // store old buf num
   if(iBufSel==0) iBufSel=1; else iBufSel=0;           // toggle to new num

   lLastAddr=lLastAsyncAddr;                           // set adr of block
   pCurrReadBuf=pAsyncFirstReadBuf[iBufSel]+           // set data ptr
                 ((addr-lLastAddr)*iUsedBlockSize);

   if(iUseDataCache)                                   // data cache used?
    AddToDataCache(lLastAddr,pAsyncFirstReadBuf[iBufSel]); // -> add the complete 64k data to cache

   if(ppfHead) CheckPPFCache(addr,pCurrReadBuf);       // apply ppf

   iTryAsync=0;                                        // data was async, reset count
   addr=lLastAddr+MAXCACHEBLOCK+1;                     // calc adr of next prefetch
   if(!((addr+MAXCACHEBLOCK)<lMaxAddr))                // mmm, no whole block can be done... so we do no prefetch at all
    {lLastAsyncAddr=0xFFFFFFFF;return TRUE;}

   f=(FRAMEBUF *)ptrBuffer[iAsyncSel];                 // setup prefetch addr
   f->dwFrameCnt = MAXCACHEBLOCK+1;
   f->dwBufLen   = (MAXCACHEBLOCK+1)*iUsedBlockSize;
   f->dwFrame    = addr;

   lLastAsyncAddr=addr;                                // store prefetch addr

   if(pReadFunc(FALSE,f)!=SS_COMP)                     // start the async read
    lLastAsyncAddr=0xFFFFFFFF;                         // -> if no success, no async prefetch buf available

   return TRUE;
  }

 ////////////////////////////////////////////////////////
 // here we do a sync read

 iBufSel=0;                                            // read in buf 0

 f=(FRAMEBUF *)ptrBuffer[0];
 f->dwFrame = addr;

 pCurrReadBuf=pFirstReadBuf;

 ////////////////////////////////////////////////////////
 // if it's possible, we do a bigger read

 if((addr+MAXCACHEBLOCK)<lMaxAddr)
  {
   f->dwFrameCnt = MAXCACHEBLOCK+1;
   f->dwBufLen   = (MAXCACHEBLOCK+1)*iUsedBlockSize;
   lLastAddr     = addr;
  }
 else
  {
   f->dwFrameCnt = 1;
   f->dwBufLen   = iUsedBlockSize;
   lLastAddr     = 0xFFFFFFFF;
  }

 ////////////////////////////////////////////////////////
 // start read, wait til finished

 if(pReadFunc(TRUE,f)!=SS_COMP)
  {
   if(!bReadRetry(f)) return FALSE;
  }

 if(iUseDataCache && lLastAddr!=0xFFFFFFFF)            // data cache used? and complete 64k block?
  AddToDataCache(addr,pAsyncFirstReadBuf[0]);         // -> add the complete data to cache

 if(ppfHead) CheckPPFCache(addr,pCurrReadBuf);

 ////////////////////////////////////////////////////////
 // start additional async prefetch read, if it's ok

 iTryAsync++;
 if(iTryAsync>1)  {iTryAsync=2;return TRUE;}           // prefetches seems to be useless right now, so turn them off until next real read

 addr+=MAXCACHEBLOCK+1;                                // prefetch addr
 if(!((addr+MAXCACHEBLOCK)<lMaxAddr))                  // not possible? do't do prefetch
  {lLastAsyncAddr=0xFFFFFFFF;return TRUE;}

 f=(FRAMEBUF *)ptrBuffer[1];                           // setup prefetch into buf 1
 f->dwFrameCnt = MAXCACHEBLOCK+1;
 f->dwBufLen   = (MAXCACHEBLOCK+1)*iUsedBlockSize;
 f->dwFrame    = addr;

 lLastAsyncAddr= addr;

 if(pReadFunc(FALSE,f)!=SS_COMP)                       // start the async prefetch
  lLastAsyncAddr=0xFFFFFFFF;

 return TRUE;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// thread mode (caching 3)
// this mode helps with slower drives using the IOCTL
// interface (since that one seems to do always blocking
// reads, even when they are done overlapped).
// With ASPI, the thread mode performance will be more or less
// the same as with async caching mode 2...
// thread reading would be much more powerful, if the main
// emu would do:
// ...
// CDRreadTrack()
// ... do some other stuff here ...
// CDRgetBuffer()
// ...
// but lazy main emu coders seem to prefer:
// ...
// CDRreadTrack()
// CDRgetBuffer()
// ...
// so there is no time between the calls to do a good
// asynchronous read... sad, sad...
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// reading thread... sleeps until a new read is signaled

DWORD WINAPI READThread(LPVOID lpParameter)
{
 FRAMEBUF * f;

 while(WaitForMultipleObjects(2,hThreadEvent,FALSE,    // wait until event to start (or event to end) get raised
       INFINITE)==WAIT_OBJECT_0)
  {
   WaitForSingleObject(hThreadMutex[0],INFINITE);      // read mutex: nobody else is allowed to read now
   WaitForSingleObject(hThreadMutex[1],INFINITE);      // variable access mutex: nobody else can change the vars now
   ResetEvent(hThreadEvent[0]);                        // ok, kick event has been handled
   SetEvent(hThreadEvent[2]);                          // set flag: we have started the read

   lLastAsyncAddr = lNeededAddr;                       // setup read and vars
   f=(FRAMEBUF *)ptrBuffer[!iBufSel];                  // !iSel = async buffer
   f->dwFrame     = lNeededAddr;
   f->dwFrameCnt  = min((lMaxAddr-lNeededAddr+1),(MAXCACHEBLOCK+1));
   f->dwBufLen    = f->dwFrameCnt*iUsedBlockSize;

   ReleaseMutex(hThreadMutex[1]);                      // ok, vars have been changed, now that vars are again available for all

   if(pReadFunc(TRUE,f)!=SS_COMP)                      // do a blocking (sync) read
    {
     bReadRetry(f);                                    // mmm... if reading fails a number of times, we don't have a chance to return 'bad' to emu with tread reading... life is hard :)
    }

   ReleaseMutex(hThreadMutex[0]);                      // ok, read has done
  }

 bThreadEnded=1;
 return 0;
}

/////////////////////////////////////////////////////////
// emu says: we need data at given addr soon...
// so, if we don't have it in any buffer, we kick a read
// ... called on CDRreadTrack()

BOOL DoReadThread(unsigned long addr)
{
 if(!hReadThread) return FALSE;                        // no thread, no fun

 bDataCacheHit=FALSE;                                  // init data cache hit flag (even if no cache is used...)

 if(lLastAddr!=0xFFFFFFFF &&                           // data is in curr data buffer?
    addr>=lLastAddr &&
    addr<=(lLastAddr+MAXCACHEBLOCK))
  return TRUE;                                         // -> fine

 if(iUseDataCache && CheckDataCache(addr))             // data cache used and data is in cache? set read ptr, if yes
  {bDataCacheHit=TRUE;return TRUE;}                    // -> and raise 'hit' flag, so we don't need to do anything in 'getbuffer'

 WaitForSingleObject(hThreadMutex[1],INFINITE);        // wait to access 'buffer 1 vars'

 if(lLastAsyncAddr!=0xFFFFFFFF &&                      // data is (or will be soon if reading is going on in thread now) in async buffer?
    addr>=lLastAsyncAddr &&
    addr<=(lLastAsyncAddr+MAXCACHEBLOCK))
  {
   ReleaseMutex(hThreadMutex[1]);                      // -> fine
   return TRUE;
  }
                                                       // data is not in buf0 and not in buf1:
 lNeededAddr=addr;                                     // set needed adr (mutex is active, so it's safe to change that)
 ResetEvent(hThreadEvent[2]);                          // reset "read has started" flag
 SetEvent(hThreadEvent[0]);                            // set "start read" flag... the read will start reading soon after
 ReleaseMutex(hThreadMutex[1]);                        // done with var access
 return TRUE;
}

/////////////////////////////////////////////////////////
// emu says: gimme ptr to needed data... this will
// automatically do an async data prefetch read as well
// ... called on CDRgetBuffer()

void GetREADThreadPtr(void)
{
 unsigned long addr=lLastAccessedAddr;

 if(bDataCacheHit) return;                             // if we had a data cache hit, the readbuf ptr is already fine, nothing else to do

 if(lLastAddr!=0xFFFFFFFF &&                           // data is in buffer 0?
    addr>=lLastAddr &&
    addr<=(lLastAddr+MAXCACHEBLOCK))
  {
   pCurrReadBuf=pAsyncFirstReadBuf[iBufSel]+           // -> ok, return curr data buffer ptr
                  ((addr-lLastAddr)*iUsedBlockSize);
   if(ppfHead) CheckPPFCache(addr,pCurrReadBuf);
   return;
  }

 WaitForSingleObject(hThreadEvent[2],INFINITE);        // wait until reading has started (it will take a small time after the read start kick, so we have to go sure that it has _really_ started)
 WaitForSingleObject(hThreadMutex[0],INFINITE);        // wait until reading has finished

 lLastAddr=lLastAsyncAddr;                             // move data to from async data to curr data buffer (by toggling iSel)
 iBufSel=!iBufSel;
 lLastAsyncAddr=0xFFFFFFFF;                            // nothing in async data buffer now

 lNeededAddr=addr+MAXCACHEBLOCK+1;                     // prefetch read addr
 ResetEvent(hThreadEvent[2]);                          // reset "read has started" flag
 SetEvent(hThreadEvent[0]);                            // signal for start next read
 ReleaseMutex(hThreadMutex[0]);                        // ok, now reading in buffer 1 can start

 if(iUseDataCache)                                     // data cache used? can be less then 64 kb with thread reading, but that doesn't matter here... will be either 64 k or (max-addr) sectors
  AddToDataCache(lLastAddr,
                 pAsyncFirstReadBuf[iBufSel]);         // -> add the complete data to cache

 pCurrReadBuf=pAsyncFirstReadBuf[iBufSel]+             // -> return the curr data buffer ptr
                ((addr-lLastAddr)*iUsedBlockSize);
 if(ppfHead) CheckPPFCache(addr,pCurrReadBuf);
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// special thread mode (caching 4)
// this mode helps with certain drives
// basically it does the following:
// It has a queue for n prefetch reads. If the main emu is
// asking for an addr, the mode will check, if
// this addr is a) getting read right now, b) already
// in our 4 MB cache, c) already in the q.
// If no condition matches, it will add it in q...
// the same is done with the next n/2 addr blocks, so
// the q will keep the drive busy... also, if everything
// is cached (and the q is empty), we will add additional
// addresses to read, also to keep the drive busy, and to
// do the needed reading as soon as possible :)

/////////////////////////////////////////////////////////
// reading thread...

DWORD WINAPI READThreadEx(LPVOID lpParameter)
{
 FRAMEBUF * f;

 while(WaitForMultipleObjects(2,hThreadEvent,FALSE,    // wait until event to start (or event to end) get raised
       INFINITE)==WAIT_OBJECT_0)
  {
   while(1)
    {
     //------------------------------------------------//
     WaitForSingleObject(hThreadMutex[1],INFINITE);    // variable access mutex: nobody else can change the vars now
     ResetEvent(hThreadEvent[0]);                      // ok, kick event has been handled

     if(lAddrQ[iQPos]==0xFFFFFFFF)                     // nothing to do? strange :)
      {ReleaseMutex(hThreadMutex[1]);break;}

     f=(FRAMEBUF *)ptrBuffer[0];
     lNeededAddr    = lAddrQ[iQPos];                   // store it in 'Neededaddr' for checks outside the thread
     f->dwFrame     = lNeededAddr;
     f->dwFrameCnt  = min((lMaxAddr-f->dwFrame+1),(MAXCACHEBLOCK+1));
     f->dwBufLen    = f->dwFrameCnt*iUsedBlockSize;

     lAddrQ[iQPos++]=0xFFFFFFFF;                       // set this slot as 'done'
     if(iQPos>=MAXQSIZE) iQPos=0;                      // amnd inc the head pos

     ReleaseMutex(hThreadMutex[1]);                    // ok, vars have been changed, now that vars are again available for all
     //------------------------------------------------//
     WaitForSingleObject(hThreadMutex[0],INFINITE);    // read mutex: nobody else is allowed to read now
     if(!iCDROK)
      {
       ReleaseMutex(hThreadMutex[0]);
       break;
      }

     if(bCDDAPlay)                                     // some cdda security...
      {                                                // it should just prevent prefetch reads happening in cdda mode, if this one breaks a 'needed' read, we are lost...
       lNeededAddr=0xFFFFFFFF;                         // so maybe we should remove this check? mmm, we will see
       ReleaseMutex(hThreadMutex[0]);
       break;
      }

     if(pReadFunc(TRUE,f)!=SS_COMP)                    // do a blocking (sync) read
      {                                                // mmm... if reading fails a number of times, we don't have a chance to return 'bad' to emu with thread reading... life is hard :)
       bReadRetry(f);                                  // but at least our 'wait for data in cache' getptr will not wait forever (just returning wrong data, ehehe)
      }

     ReleaseMutex(hThreadMutex[0]);                    // ok, read has done
     //------------------------------------------------//
     WaitForSingleObject(hThreadMutex[1],INFINITE);    // variable access mutex: nobody else can change the vars now
     lNeededAddr=0xFFFFFFFF;                           // no read is now active
     AddToDataCache(f->dwFrame,pFirstReadBuf);         // add the complete data to cache
     ReleaseMutex(hThreadMutex[1]);                    // ok, vars have been changed, now that vars are again available for all
     //------------------------------------------------//
     if(WaitForSingleObject(hThreadEvent[0],0)!=WAIT_OBJECT_0)
      Sleep(1);                                        // if nobody has started a new kick, let's sleep awhile to give Windows more room to breath
    }
  }

 bThreadEnded=1;
 return 0;
}

/////////////////////////////////////////////////////////
// emu says: we need data at given addr soon...
// so, if we don't have it in any buffer, we kick a read
// ... called on CDRreadTrack()

//#define THREADEX_STRAIGHT

BOOL DoReadThreadEx(unsigned long addr)
{
 int i,k,j=0;

 if(!hReadThread) return FALSE;                        // no thread, no fun

 WaitForSingleObject(hThreadMutex[1],INFINITE);        // wait for data access

//-----------------------------------------------------//
// straight reading try... should have been faster, but
// in 'real life' this approach keeps the cdrom drive
// spinning too much, giving other pc resources no room
// to breath... by increasing the thread 'Sleep' value
// the performance can get better, but then the annoying
// breaks we wanted to fight will show up again...
// so this type is disabled as long as nobody enables the
// define again :)

#ifdef THREADEX_STRAIGHT

 if(addr>=lNeededAddr &&
    addr<=(lNeededAddr+MAXCACHEBLOCK))
  {
   ReleaseMutex(hThreadMutex[1]);
   return TRUE;
  }

 for(k=0;k<MAXQSIZE;k++)                               // loop max of prefetch blocks
  {
   for(i=0;i<MAXDATACACHE;i++)                         // loop whole cache
    {
     if(addr>=lDataCacheAddr[i] &&                     // -> addr found?
        addr<=(lDataCacheAddr[i]+MAXCACHEBLOCK))
      {
       if(k==0) iQLockPos=i;                           // -> if it's the current main addr, lock it, so no prefetch read overwrites its content
       break;
      }
    }
   if(i!=MAXDATACACHE)                                 // found in data cache?
    {addr=addr+MAXCACHEBLOCK+1;continue;}              // -> do nothing with this addr, we have its content
   else break;                                         // here is the first unknown addr
  }

 if(addr>=lMaxAddr)                                    // check, if addr too big
  {
   ReleaseMutex(hThreadMutex[1]);
   if(k==0) return FALSE;                              // -> if it's the main addr, there is an error
   return TRUE;                                        // -> otherwise we can't simply cache that addr
  }

 for(i=0;i<MAXQSIZE;i++)                               // loop q list
  {
   if(addr>=lAddrQ[i] &&                               // -> addr will be read soon?
      addr<=(lAddrQ[i]+MAXCACHEBLOCK))
    {
     addr=lAddrQ[i];                                   // --> take this aligned addr for new header
     break;
    }
  }

 for(i=0;i<MAXQSIZE;i++)                               // loop q list
  {
   lAddrQ[i]=addr;
   addr=addr+MAXCACHEBLOCK+1;
   if(addr>=lMaxAddr) break;
  }

 for(;i<MAXQSIZE;i++)                                  // loop q list
  {
   lAddrQ[i]=0xFFFFFFFF;
  }

 iQPos=0;

 SetEvent(hThreadEvent[0]);                            // kick a read, if neccessary

#else

//-----------------------------------------------------//
// ok, here is the current ReadThreadEx mode: more
// complex, and it doesn't arrange the prefetch sectors
// as straight as the type above, but the final result is
// still smoother (no more pauses on the tested LG drive)

 for(k=0;k<MAXQFETCH;k++)                              // loop max of prefetch blocks
  {
   if(addr>=lNeededAddr &&                             // addr is getting read right now?
      addr<=(lNeededAddr+MAXCACHEBLOCK))               // -> ok, we do nothing with it
    {addr=addr+MAXCACHEBLOCK+1;continue;}

   for(i=0;i<MAXDATACACHE;i++)                         // loop whole cache
    {
     if(addr>=lDataCacheAddr[i] &&                     // -> addr found?
        addr<=(lDataCacheAddr[i]+MAXCACHEBLOCK))
      {
       if(k==0) iQLockPos=i;                           // -> if it's the current main addr, lock it, so no other prefetch read overwrites its content
       break;
      }
    }
   if(i!=MAXDATACACHE)                                 // found in data cache?
    {addr=addr+MAXCACHEBLOCK+1;continue;}              // -> do nothing with this addr, we have its content

   for(i=0;i<MAXQSIZE;i++)                             // loop prefetch q list
    {
     if(addr>=lAddrQ[i] &&                             // -> addr will be read soon?
        addr<=(lAddrQ[i]+MAXCACHEBLOCK))
      {
       if(k==0 && i!=iQPos)                            // curr needed addr is not on top of the q?
        {
         addr=lAddrQ[i];                               // -> get the addr (our main addr is in it, but that one is more aligned to prev reads)
         for(i=0;i<MAXQSIZE;i++) lAddrQ[i]=0xFFFFFFFF; // -> clear whole q (we will fill the slots in that loop again)
         i=MAXQSIZE;                                   // -> sign for storing the addr in q
        }
       break;
      }
    }
   if(i!=MAXQSIZE)                                     // found in q?
    {addr=addr+MAXCACHEBLOCK+1;continue;}              // -> do nothing with this addr, we will have its content soon

                                                       // not in q or data cache?
   if(k==0) lAddrQ[iQPos]=addr;                        // -> if it's the main addr, store it on top of list
   else                                                // -> if it's a prefetch addr, try to store it elsewhere at the end of the q
    {
     j=iQPos;
     for(i=0;i<MAXQSIZE;i++)
      {
       if(lAddrQ[j]==0xFFFFFFFF) {lAddrQ[j]=addr;break;}
       j++;if(j>=MAXQSIZE) j=0;
      }
    }

   SetEvent(hThreadEvent[0]);                          // kick a read, if neccessary
   addr=addr+MAXCACHEBLOCK+1;                          // next prefetch addr
   if(addr>=lMaxAddr) break;                           // security, for detecting if we are at the end of cd
  }

 //----------------------------------------------------// ok, and here's something to keep the drive busy...

 if(lAddrQ[iQPos]==0xFFFFFFFF && addr<lMaxAddr)        // nothing in prefetch q?
  {
   iQIdle++;                                           // count how many empty q's in-a-row are happening
   if(iQIdle>10)                                       // more then x times?
    {
     iQIdle=0;
     lAddrQ[iQPos]=addr;                               // we add the farest prefetch addr
     SetEvent(hThreadEvent[0]);                        // and do an additional kick
    }
  }
 else iQIdle=0;                                        // not idling? ok

 //----------------------------------------------------//

#endif

 ReleaseMutex(hThreadMutex[1]);

 return TRUE;
}

/////////////////////////////////////////////////////////
// emu says: gimme ptr to needed data... this will
// automatically do an async data prefetch read as well
// ... called on CDRgetBuffer()

void GetREADThreadExPtr(void)
{
 unsigned long addr=lLastAccessedAddr;

 while(1)
  {
   if(bThreadEnded) return;                            // main emu is already closing (thread is down)? bye
   WaitForSingleObject(hThreadMutex[1],INFINITE);      // wait for data access
   if(CheckDataCache(addr))                            // data now in cache?
    {
     ReleaseMutex(hThreadMutex[1]);                    // -> ok, done
     return;
    }
   ReleaseMutex(hThreadMutex[1]);                      // else try again (no sleep here, we are blocking everything anyway)
  }

}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// simple data cache

void AllocDataCache(void)
{
 bDataCacheHit=FALSE;                                  // init thread cache hit flag
 if(!iUseCaching) iUseDataCache=0;                     // security: no additinal data cache, if no caching active
 if(iUseDataCache)
  {
   int i;
   for(i=0;i<MAXDATACACHE;i++)                         // init all cache slots
    {
     lDataCacheAddr[i] = 0xFFFFFFFF;
     pDataCacheBuf[i]  = malloc(MAXCDBUFFER-FRAMEBUFEXTRA);
    }
  }
}

/////////////////////////////////////////////////////////

void FreeDataCache(void)
{
 if(iUseDataCache)
  {
   int i;
   for(i=0;i<MAXDATACACHE;i++)
    {
     free(pDataCacheBuf[i]);
     pDataCacheBuf[i]=NULL;
    }
  }
}

/////////////////////////////////////////////////////////
// easy data cache: stores data blocks

void AddToDataCache(unsigned long addr,unsigned char * pB)
{
 static int iPos=0;
 if(iPos==iQLockPos)                                   // special thread mode lock?
  {iPos++;if(iPos>=MAXDATACACHE) iPos=0;}              // -> don't use that pos, use next one
 lDataCacheAddr[iPos]=addr;
 memcpy(pDataCacheBuf[iPos],pB,
        MAXCDBUFFER-FRAMEBUFEXTRA);
 iPos++; if(iPos>=MAXDATACACHE) iPos=0;
}

/////////////////////////////////////////////////////////
// easy data cache check: loop MAXDATACACHE blocks, set ptr if addr found

BOOL CheckDataCache(unsigned long addr)
{
 int i;

 for(i=0;i<MAXDATACACHE;i++)
  {
   if(addr>=lDataCacheAddr[i] &&
      addr<=(lDataCacheAddr[i]+MAXCACHEBLOCK))
    {
     pCurrReadBuf=pDataCacheBuf[i]+
                   ((addr-lDataCacheAddr[i])*iUsedBlockSize);
     if(ppfHead) CheckPPFCache(addr,pCurrReadBuf);
     return TRUE;
    }
  }
 return FALSE;
}

/////////////////////////////////////////////////////////

