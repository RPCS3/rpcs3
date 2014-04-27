/***************************************************************************
                            ppf.c  -  description
                             -------------------
    begin                : Wed Sep 18 2002
    copyright            : (C) 2002 by Pete Bernert
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
// 2003/02/14 - Pete
// - fixed a bug reading PPF3 patches reported by Zydio
//
// 2002/09/19 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

/////////////////////////////////////////////////////////

#include "stdafx.h"
#define _IN_PPF
#include "externals.h"

/////////////////////////////////////////////////////////

int         iUsePPF=0;
char        szPPF[260];
PPF_CACHE * ppfCache=NULL;
PPF_DATA  * ppfHead=NULL;
int         iPPFNum=0;

/////////////////////////////////////////////////////////
// works like sub cache... using a linked data list, and address array

void FillPPFCache(void)
{
 PPF_DATA * p;PPF_CACHE * pc;
 long lastaddr;

 p=ppfHead;
 lastaddr=-1;
 iPPFNum=0;

 while(p)
  {
   if(p->addr!=lastaddr) iPPFNum++;
   lastaddr=p->addr;
   p=(PPF_DATA *)p->pNext;
  }

 if(!iPPFNum) return;

 pc=ppfCache=(PPF_CACHE *)malloc(iPPFNum*sizeof(PPF_CACHE));

 iPPFNum--;
 p=ppfHead;
 lastaddr=-1;

 while(p)
  {
   if(p->addr!=lastaddr)
    {
     pc->addr=p->addr;
     pc->pNext=(void *)p;
     pc++;
    }
   lastaddr=p->addr;
   p=(PPF_DATA *)p->pNext;
  }
}

/////////////////////////////////////////////////////////

void FreePPFCache(void)
{
 PPF_DATA * p=ppfHead;
 void * pn;

 while(p)
  {
   pn=p->pNext;
   free(p);
   p=(PPF_DATA *)pn;
  }
 ppfHead=NULL;

 if(ppfCache) free(ppfCache);
 ppfCache=NULL;
}

/////////////////////////////////////////////////////////

void CheckPPFCache(long addr,unsigned char * pB)
{
 PPF_CACHE * pcstart, * pcend, * pcpos;

 pcstart=ppfCache;
 if(addr<pcstart->addr) return;
 pcend=ppfCache+iPPFNum;
 if(addr>pcend->addr)   return;

 while(1)
  {
   if(addr==pcend->addr) {pcpos=pcend;break;}

   pcpos=pcstart+(pcend-pcstart)/2;
   if(pcpos==pcstart) break;
   if(addr<pcpos->addr)
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

 if(addr==pcpos->addr)
  {
   PPF_DATA * p=(PPF_DATA *)pcpos->pNext;
   while(p && p->addr==addr)
    {
     memcpy(pB+p->pos,p+1,p->anz);
     p=(PPF_DATA *)p->pNext;
    }
  }
}

/////////////////////////////////////////////////////////

void AddToPPF(long ladr,long pos,long anz,char * ppfmem)
{
 if(!ppfHead)
  {
   ppfHead=(PPF_DATA *)malloc(sizeof(PPF_DATA)+anz);
   ppfHead->addr=ladr;
   ppfHead->pNext=NULL;
   ppfHead->pos=pos;
   ppfHead->anz=anz;
   memcpy(ppfHead+1,ppfmem,anz);
   iPPFNum=1;
  }
 else
  {
   PPF_DATA * p=ppfHead;
   PPF_DATA * plast=NULL;
   PPF_DATA * padd;
   while(p)
    {
     if(ladr<p->addr) break;
     if(ladr==p->addr)
      {
       while(p && ladr==p->addr && pos>p->pos)
        {
         plast=p;
         p=(PPF_DATA *)p->pNext;
        }
       break;
      }
     plast=p;
     p=(PPF_DATA *)p->pNext;
    }
   padd=(PPF_DATA *)malloc(sizeof(PPF_DATA)+anz);
   padd->addr=ladr;
   padd->pNext=(void *)p;
   padd->pos=pos;
   padd->anz=anz;
   memcpy(padd+1,ppfmem,anz);
   iPPFNum++;
   if(plast==NULL)
        ppfHead=padd;
   else plast->pNext=(void *)padd;
  }
}

/////////////////////////////////////////////////////////
// build ppf cache, if wanted

void BuildPPFCache(void)
{
 FILE * ppffile;
 char buffer[5];
 char method,undo=0,blockcheck=0;
 int  dizlen, dizyn, dizlensave=0;
 char ppfmem[512];
 int  count,seekpos, pos;
 //unsigned char anz;
 unsigned int anz;                                     // new! avoids stupid overflows
 long ladr,off,anx;

 ppfHead=NULL;

 if(iUsePPF==0)  return;                               // no ppf cache wanted?
 if(szPPF[0]==0) return;                               // no ppf file given?

 ppffile=fopen(szPPF, "rb");
 if(ppffile==0)
  {
   MessageBox(NULL,"No PPF file found!",libraryName,MB_OK);
   return;
  }

 memset(buffer,0,5);
 fread(buffer, 3, 1, ppffile);

 if(strcmp(buffer,"PPF"))
  {
   MessageBox(NULL,"No PPF file format!",libraryName,MB_OK);
   fclose(ppffile);
   return;
  }

 fseek(ppffile, 5, SEEK_SET);
 fread(&method, 1, 1,ppffile);

 switch(method)
  {
   case 0:                                             // ppf1
    fseek(ppffile, 0, SEEK_END);
    count=ftell(ppffile);
    count-=56;
    seekpos=56;
   break;

   case 1:                                             // ppf2
    fseek(ppffile, -8,SEEK_END);

    memset(buffer,0,5);
    fread(buffer, 4, 1,ppffile);
    if(strcmp(".DIZ", buffer))
     {
      dizyn=0;
     }
    else
     {
      fread(&dizlen, 4, 1, ppffile);
      dizyn=1;
      dizlensave=dizlen;
     }

    fseek(ppffile, 56, SEEK_SET);
    fread(&dizlen, 4, 1,ppffile);
    fseek(ppffile, 0, SEEK_END);
    count=ftell(ppffile);
    if(dizyn==0)
     {
      count-=1084;
      seekpos=1084;
     }
    else
     {
      count-=1084;
      count-=38;
      count-=dizlensave;
      seekpos=1084;
     }
    break;

   case 2:                                             // ppf3
    fseek(ppffile, 57, SEEK_SET);
    fread(&blockcheck, 1, 1,ppffile);
    fseek(ppffile, 58, SEEK_SET);
    fread(&undo, 1, 1,ppffile);

    fseek(ppffile, -6,SEEK_END);
    memset(buffer,0,5);
    fread(buffer, 4, 1,ppffile);
    dizlen=0;
    if(!strcmp(".DIZ", buffer))
     {
      fseek(ppffile, -2,SEEK_END);
      fread(&dizlen, 2, 1, ppffile);
      dizlen+=36;
     }

    fseek(ppffile, 0, SEEK_END);
    count=ftell(ppffile);
    count-=dizlen;

    if(blockcheck)
     {
      seekpos=1084;
      count-=1084;
     }
    else
     {
      seekpos=60;
      count-=60;
     }

   break;

   default:
    fclose(ppffile);
    MessageBox(NULL,"Unknown PPF format!",libraryName,MB_OK);
    return;
  }

 do                                                    // now do the data reading
  {
   fseek(ppffile, seekpos, SEEK_SET);
   fread(&pos, 4, 1, ppffile);

   if(method==2) fread(buffer, 4, 1, ppffile);         // skip 4 bytes on ppf3 (no int64 support here)

   anz=0;                                              // new! init anz (since it's no unsigned char anymore)
   fread(&anz, 1, 1, ppffile);
   fread(ppfmem, anz, 1, ppffile);

   ladr=pos/2352;
   off=pos%2352;

   if(off+anz>2352)
    {
     anx=off+anz-2352;
     anz-=(unsigned char)anx;
     AddToPPF(ladr+1,0,anx,ppfmem+anz);
    }

   AddToPPF(ladr,off,anz,ppfmem);                      // add to link list

   if(method==2)                                       // adjust ppf3 size
    {
     if(undo) anz+=anz;
     anz+=4;
    }

   seekpos=seekpos+5+anz;
   count=count-5-anz;
  }
 while(count!=0);                                      // loop til end

 fclose(ppffile);

 FillPPFCache();                                       // build address array
}

/////////////////////////////////////////////////////////
