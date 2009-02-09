/***************************************************************************
                            sub.c  -  description
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
#define _IN_SUB
#include "externals.h"

/* TODO (#1#): SUB CHANNEL STUFF */

/////////////////////////////////////////////////////////
              
unsigned char * pCurrSubBuf=NULL;                      // ptr to emu sub data (or NULL)
int             iUseSubReading=0;                      // subdata support (by file or directly)
char            szSUBF[260];                           // sub file name
SUB_CACHE *     subCache=NULL;                         // cache memory
SUB_DATA  *     subHead=NULL;
int             iSUBNum=0;                             // number of subdata cached
unsigned char   SubCData[96];                          // global data subc buffer
unsigned char   SubAData[96];                          // global audio subc buffer

/////////////////////////////////////////////////////////

void BuildSUBCache(void)
{
 FILE * subfile;
 unsigned char buffer[16], * p;
 SUB_DATA * plast=NULL,* px;

 if(iUseSubReading)                                    // some subreading wanted?
  {
   if(iUseSubReading==1) iUseCaching=0;                // -> direct read? no caching done, only 1 sector reads
   memset(SubCData,0,96);                              // -> init subc
   pCurrSubBuf=SubCData;                               // -> set global ptr
  }
 else                                                  // no plugin subreading?
  {
   pCurrSubBuf=NULL;                                   // -> return NULL as subc buffer to emu
  }

 memset(SubAData,0,96);                                // init audio subc buffer

 if(iUseSubReading!=2)  return;                        // no subfile wanted?
 if(szSUBF[0]==0)       return;                        // no filename given?

 subfile=fopen(szSUBF, "rb");                          // open subfile
 if(subfile==0)
  {
   MessageBox(NULL,"No SBI/M3S file found!",libraryName,MB_OK);
   return;
  }

 memset(buffer,0,5);                                   // read header
 fread(buffer, 4, 1, subfile);

 iSUBNum=0;subHead=NULL;

 if(strcmp((char *)buffer,"SBI")==0)                   // ah, SBI file
  {
   while(fread(buffer, 4, 1, subfile)==1)              // -> read record header
    {
     iSUBNum++;                                        // -> one more sub cache block
     px=(SUB_DATA *)malloc(sizeof(SUB_DATA));          // -> get cache buff
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!                                                       // -> and fill it...

/* TODO (#1#): addr2time subchannel */


     px->addr=time2addrB(buffer);

     px->pNext=NULL;

     px->subq[0]=0x41;
     px->subq[1]=0x01;
     px->subq[2]=0x01;
     p=&px->subq[3];
     addr2timeB(px->addr,p);
     px->subq[6]=0x00;
     p=&px->subq[7];
     addr2timeB(px->addr+150,p);

     if(buffer[3]==1)
      {
       fread(px->subq,10, 1, subfile);
      }
     else if(buffer[3]==2)
      {
       fread(&px->subq[3],3, 1, subfile);
      }
     else if(buffer[3]==3)
      {
       fread(&px->subq[7],3, 1, subfile);
      }
    
     if(plast==NULL)                                   // add new cache block to linked list
      {
       plast=subHead=px;
      } 
     else
      {
       plast->pNext=px;plast=px;
      } 
    }
  }
 else                                                  // M3S file?
  {                                                    // -> read data, and store all
   unsigned char min,sec,frame,xmin,xsec,xframe;       // -> subs which are different from
   BOOL b1,b2,goon=TRUE;int iNum=0;                    // -> the expected calculated values

   xmin=2;       
   xsec=58;
   xframe=0;
   min=3;        
   sec=0;
   frame=0;

   fread(buffer+4, 12, 1, subfile);
   do
    {
     if(itod(min)    != buffer[7]||
        itod(sec)    != buffer[8]||
        itod(frame)  != buffer[9])
      b1=TRUE; else b1=FALSE;

     if(itod(xmin)   != buffer[3]||
        itod(xsec)   != buffer[4]||
        itod(xframe) != buffer[5])
      b2=TRUE; else b2=FALSE;

     if(buffer[1]!=1) b1=b2=TRUE;
     if(buffer[2]!=1) b1=b2=TRUE;

     if(b1 || b2)
      {
       iSUBNum++;
       px=(SUB_DATA *)malloc(sizeof(SUB_DATA));
       px->pNext=NULL;
       memcpy(px->subq,buffer,10);
       buffer[7]=itod(min);
       buffer[8]=itod(sec);
       buffer[9]=itod(frame);
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
       px->addr=time2addrB(&buffer[7]);

       if(plast==NULL)
            {plast=subHead=px;} 
       else {plast->pNext=px;plast=px;} 
      }

     xframe=xframe+1;
     if(xframe>74)
      {
       xsec+=1;
       xframe-=75;
       if(xsec>59)
        {
         xmin+=1;
         xsec-=60;
         if(xmin>99) 
          {
           goon=FALSE;
          }
        }
      }
        
     frame=frame+1;
     if(frame>74)
      {
       sec+=1;
       frame-=75;
       if(sec>59)
        {
         min+=1;
         sec-=60;
         if(min>99) 
          {
           goon=FALSE;
          }
        }
      }
     iNum++;
     if(iNum>(60*75)) goon=FALSE;
    }
   while(goon && (fread(buffer, 16, 1, subfile)==1));
   
   if(iNum!=(60*75))    goon=FALSE;
   else
   if(iSUBNum==(60*75)) goon=FALSE;

   if(!goon)
    {
     MessageBox(NULL,"Bad SBI/M3S file!",libraryName,MB_OK);
     fclose(subfile);
     FreeSUBCache();
     return;
    }
  }

 subCache=NULL;                                       
 if(iSUBNum)                                           // something in cache?
  {                                                    // -> create an array with the used addresses, for fast searching access
   SUB_CACHE * psc;int i;
   subCache=(SUB_CACHE *)malloc(iSUBNum*sizeof(SUB_CACHE));
   psc=subCache;px=subHead;
   for(i=0;i<iSUBNum;i++,psc++,px=(SUB_DATA *)px->pNext)
    {
     psc->addr  = px->addr;
     psc->pNext = (void *)px;
    }
   iSUBNum--;
  }

 fclose(subfile);
}

/////////////////////////////////////////////////////////
// func for calculating 'right' subdata

void FakeSubData(unsigned long adr)
{
 SubCData[12]=0x41;
 SubCData[13]=0x01;
 SubCData[14]=0x01;
 
 /* TODO (#1#): addr2time fake sub data
 */
 
 
//!!!!!!!!!!!!!!!!!!!!!!!???? 
 addr2timeB(adr,    &SubCData[15]);
// SubCData[18]=0x00;
 addr2timeB(adr+150,&SubCData[19]);
}

/////////////////////////////////////////////////////////
// check, if for a given addr we have special subdata in cache

void CheckSUBCache(long addr)
{
 SUB_CACHE * pcstart, * pcend, * pcpos;

 pcstart=subCache;                                     // ptrs to address arrays (start/end)
 pcend  =subCache+iSUBNum;

 if(addr>=pcstart->addr &&                             // easy check, if given addr is between start/end
    addr<=pcend->addr)
  {
   while(1)                                            // now search for sub
    {
     if(addr==pcend->addr) {pcpos=pcend;break;}        // got it! break

     pcpos=pcstart+(pcend-pcstart)/2;                  // get the 'middle' address
     if(pcpos==pcstart) break;                         // no more checks can be done
     if(addr<pcpos->addr)                              // look further...
      {
       pcend=pcpos;
       continue;
      }
     if(addr>pcpos->addr)
      {
       pcstart=pcpos;
       continue;
      }
     break;
    }

   if(addr==pcpos->addr)                               // found some cached data?
    {
     SUB_DATA * p=(SUB_DATA *)pcpos->pNext;            // -> ptr to data
     memcpy(&SubCData[12],p->subq,10);                 // -> get the data
     return;                                           // -> done
    }
  }

 FakeSubData(addr);                                    // no subcdata avail, so fake right one
}

/////////////////////////////////////////////////////////
// free all sub cache bufs

void FreeSUBCache(void)
{
 SUB_DATA * p=subHead;
 void * pn;
 while(p)
  {
   pn=p->pNext;
   free(p);
   p=(SUB_DATA *)pn;
  }
 subHead=NULL;

 if(subCache) free(subCache);
 subCache=NULL;
}

/////////////////////////////////////////////////////////
