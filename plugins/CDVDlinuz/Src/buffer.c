/*  buffer.c

 *  Copyright (C) 2002-2005  CDVDlinuz Team

 *

 *  This program is free software; you can redistribute it and/or modify

 *  it under the terms of the GNU General Public License as published by

 *  the Free Software Foundation; either version 2 of the License, or

 *  (at your option) any later version.

 *

 *  This program is distributed in the hope that it will be useful,

 *  but WITHOUT ANY WARRANTY; without even the implied warranty of

 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

 *  GNU General Public License for more details.

 *

 *  You should have received a copy of the GNU General Public License

 *  along with this program; if not, write to the Free Software

 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */



#include <errno.h> // errno

#include <stddef.h> // NULL

#include <stdio.h> // printf()

#include <string.h> // strerror()



#ifndef __LINUX__

#ifdef __linux__

#define __LINUX__

#endif /* __linux__ */

#endif /* No __LINUX__ */



#define CDVDdefs

#include "PS2Edefs.h"



#include "logfile.h"



#include "buffer.h"





// Globals



struct BufferSortEmpty {

  u16 sortptr;

};



struct BufferSort {

  u16 upptr;

  u16 uppos;



  u32 mask;

  u32 divisor;



  struct {

    u8 isdata;

    u16 ptr;

  } lower[256];

  u16 ptrcount;

  u16 firstptr;

};



struct BufferSortEmpty buffersortempty[BUFFERMAX];

u16 buffersortemptystart;

u16 buffersortemptyend;



struct BufferSort buffersort[BUFFERMAX];

u8 buffersortstartisdata;

u16 buffersortstart;



struct BufferList bufferlist[BUFFERMAX];

u16 userbuffer;

u16 replacebuffer;





void InitBuffer() {

  u16 i;

  u16 j;



#ifdef VERBOSE_FUNCTION_BUFFER

  PrintLog("CDVD buffer: InitBuffer()");

#endif /* VERBOSE_FUNCTION_BUFFER */



  buffersortemptystart = 0;

  buffersortemptyend = 0;

  for(i = 0; i < BUFFERMAX; i++) {

    buffersortempty[i].sortptr = i;

  } // NEXT i- Saying all buffersort[] entries are open.



  buffersortstart = 0xffff;

  buffersortstartisdata = 2;

  for(i = 0; i < BUFFERMAX; i++) {

    for(j = 0; j < 256; j++)  buffersort[i].lower[j].isdata = 2;

    // buffersort[i].ptrcount = 0;

  } // NEXT i- Saying all buffersort[] entries are open.



  for(i = 0; i < BUFFERMAX; i++) {

    bufferlist[i].upsort = 0xffff;

  } // NEXT i- Clearing out the buffer pointers

  userbuffer = 0xffff;

  replacebuffer = BUFFERMAX - 1;

} // END InitBuffer();





u16 AllocSortBuffer() {

  u16 newbuffer;



#ifdef VERBOSE_FUNCTION_BUFFER

  PrintLog("CDVD buffer: AllocSortBuffer()");

#endif /* VERBOSE_FUNCTION_BUFFER */



  newbuffer = buffersortempty[buffersortemptyend].sortptr;

  buffersortempty[buffersortemptyend].sortptr = BUFFERMAX;

  buffersortemptyend++;

  if(buffersortemptyend > BUFFERMAX - 1)  buffersortemptyend = 0;



#ifdef VERBOSE_FUNCTION_BUFFER

  PrintLog("CDVD buffer:   Sort Buffer %u", newbuffer);

#endif /* VERBOSE_FUNCTION_BUFFER */



#ifdef VERBOSE_WARNINGS_BUFFER

  if(buffersortemptyend == buffersortemptystart) {

    PrintLog("Completely out of Sort Buffers to allocate now!");

  } // ENDIF- Out of Sort Buffers? Say so!

#endif /* VERBOSE_WARNINGS_BUFFER */



  return(newbuffer);

} // END AllocSortBuffer()





void ReleaseSortBuffer(u16 oldbuffer) {

#ifdef VERBOSE_FUNCTION_BUFFER

  PrintLog("CDVD buffer: ReleaseSortBuffer(%u)", oldbuffer);

#endif /* VERBOSE_FUNCTION_BUFFER */



  buffersortempty[buffersortemptystart].sortptr = oldbuffer;

  buffersortemptystart++;

  if(buffersortemptystart > BUFFERMAX - 1)  buffersortemptystart = 0;

} // END ReleaseSortBuffer()





// Returns either index in buffersort... or closest insertion point.

// Make lsn == buffersort[int].lsn test for exact match

u16 FindListBuffer(u32 lsn) {

  u16 current;

  u8 isdata;

  u32 index;



#ifdef VERBOSE_FUNCTION_BUFFER

  PrintLog("CDVD buffer: FindListBuffer(%u)", lsn);

#endif /* VERBOSE_FUNCTION_BUFFER */



  if(buffersortstart == 0xffff)  return(0xffff); // Buffer empty? Exit



  if(buffersortstartisdata == 1) {

    if(bufferlist[buffersortstart].lsn != lsn)  return(0xffff);

    return(buffersortstart);

  } // ENDIF- Only one Record in Buffer?



#ifdef VERBOSE_FUNCTION_BUFFER

  PrintLog("CDVD buffer:   Searching...");

#endif /* VERBOSE_FUNCTION_BUFFER */



  current = buffersortstart;

  isdata = 0;

  while(isdata == 0) {

    index = lsn;

    index &= buffersort[current].mask;

    index /= buffersort[current].divisor;

    isdata = buffersort[current].lower[index].isdata;

    current = buffersort[current].lower[index].ptr;

  } // ENDWHILE- still haven't found data



  if(isdata == 2)  return(0xffff); // Pointing at empty entry?

  if(bufferlist[current].lsn != lsn)  return(0xffff); // LSNs don't match?



#ifdef VERBOSE_FUNCTION_BUFFER

  PrintLog("CDVD buffer:   Found.");

#endif /* VERBOSE_FUNCTION_BUFFER */

  return(current);

} // END FindListBuffer()





// Removes buffer from the buffersort list

// bufnum = The bufferlist pointer

void RemoveListBuffer(u16 bufnum) {

  u16 current;

  u16 currentpos;



  u16 upperlink;

  u16 upperindex;



  u16 lowerlink;

  u8 lowerisdata;



  u32 index;



#ifdef VERBOSE_FUNCTION_BUFFER

  PrintLog("CDVD buffer: RemoveListBuffer(%u)", bufnum);

#endif /* VERBOSE_FUNCTION_BUFFER */



  if(bufferlist[bufnum].upsort == 0xffff)  return; // No link to break.



  current = bufferlist[bufnum].upsort;

  currentpos = bufferlist[bufnum].uppos;

  bufferlist[bufnum].upsort = 0xffff;



  if(current == 0xfffe) {

    buffersortstart = 0xffff;

    buffersortstartisdata = 2;

#ifdef VERBOSE_FUNCTION_BUFFER

    PrintLog("CDVD buffer:   Buffer emptied");

#endif /* VERBOSE_FUNCTION_BUFFER */

    return;

  } // ENDIF- Last link broken... empty buffer now.



  lowerlink = 0xffff;

  lowerisdata = 2;



  // Remove Lower Pointer

  buffersort[current].lower[currentpos].isdata = 2;

  if(currentpos == buffersort[current].firstptr) {

    index = currentpos + 1;

    while((index < 256) && (buffersort[current].lower[index].isdata == 2))  index++;

    buffersort[current].firstptr = index;

  } // ENDIF- Do we need to move firstptr to an active entry?

  buffersort[current].ptrcount--;

#ifdef VERBOSE_FUNCTION_BUFFER

  PrintLog("CDVD buffer:   Pointer count for sort buffer %u: %u",

           current, buffersort[current].ptrcount);

#endif /* VERBOSE_FUNCTION_BUFFER */

  if(buffersort[current].ptrcount > 1)  return; // Still 2+ branches left



  // Find Lower Link

  index = buffersort[current].firstptr;

  lowerlink = buffersort[current].lower[index].ptr;

  lowerisdata = buffersort[current].lower[index].isdata;

  buffersort[current].lower[index].isdata = 2;



  // Find and Break Upper Link

  upperlink = buffersort[current].upptr;

  upperindex = buffersort[current].uppos;



  if(upperlink == 0xffff) {

    buffersortstart = lowerlink;

    buffersortstartisdata = lowerisdata;

  } else {

    buffersort[upperlink].lower[upperindex].ptr = lowerlink;

    buffersort[upperlink].lower[upperindex].isdata = lowerisdata;

  } // ENDIF- Did we hit the top of the web?



  // Break Lower Link

  if(lowerisdata == 1) {

    if(upperlink == 0xffff) {

      bufferlist[lowerlink].upsort = 0xfffe;

#ifdef VERBOSE_FUNCTION_BUFFER

      PrintLog("CDVD buffer:   Buffer down to 1 record now");

#endif /* VERBOSE_FUNCTION_BUFFER */

    } else {

      bufferlist[lowerlink].upsort = upperlink;

      bufferlist[lowerlink].uppos = upperindex;

    } // ENDIF- Is this the last active buffersort?

  } else {

    buffersort[lowerlink].upptr = upperlink;

    buffersort[lowerlink].uppos = upperindex;

  } // ENDIF- Was this a BufferList link?



  // Cleanup in aisle 5....

  ReleaseSortBuffer(current);

  return;

} // END RemoveListBuffer()





// Adds buffer to the buffersort list

// bufnum = The bufferlist pointer

void AddListBuffer(u16 bufnum) {

  u32 newmask;

  u32 newdivisor;



  u16 newbuffer;

  u32 newvalue;



  u16 current;

  u32 currentvalue;

  u8 currentisdata;



  u16 prevbuffer;

  u32 prevvalue;



  u16 comparebuffer;

  u8 compareisdata;

  u32 comparevalue;



#ifdef VERBOSE_FUNCTION_BUFFER

  PrintLog("CDVD buffer: AddListBuffer(%u)", bufnum);

#endif /* VERBOSE_FUNCTION_BUFFER */



  // Already in list? Oh, re-sorting? Got it covered.

  if(bufferlist[bufnum].upsort != 0xffff)  RemoveListBuffer(bufnum);



  if(buffersortstartisdata == 2) {

    buffersortstart = bufnum;

    buffersortstartisdata = 1;

    bufferlist[bufnum].upsort = 0xfffe;

#ifdef VERBOSE_FUNCTION_BUFFER

    PrintLog("CDVD buffer:   Buffer up to 1 record now");

#endif /* VERBOSE_FUNCTION_BUFFER */

    return;

  } // ENDIF- Empty list? Set for just 1 entry.



  if(buffersortstartisdata == 1) {

    newmask = 0xff000000;

    newdivisor = 0x01000000;

    newvalue = (bufferlist[bufnum].lsn & newmask) / newdivisor;

    currentvalue = (bufferlist[buffersortstart].lsn & newmask) / newdivisor;

    while((newdivisor != 0x00000001) && (newvalue == currentvalue)) {

      newmask /= 0x0100;

      newdivisor /= 0x0100;

      newvalue = (bufferlist[bufnum].lsn & newmask) / newdivisor;

      currentvalue = (bufferlist[buffersortstart].lsn & newmask) / newdivisor;

    } // ENDWHILE- trying to find a difference between the LSNs



    if(newvalue == currentvalue) {

      bufferlist[buffersortstart].upsort = 0xffff;

      bufferlist[bufnum].upsort = 0xfffe;

      buffersortstart = bufnum;

      return;



    } else {

      newbuffer = AllocSortBuffer();

      buffersort[newbuffer].upptr = 0xffff;

      buffersort[newbuffer].mask = newmask;

      buffersort[newbuffer].divisor = newdivisor;

      buffersort[newbuffer].lower[currentvalue].isdata = 1;

      buffersort[newbuffer].lower[currentvalue].ptr = buffersortstart;

      buffersort[newbuffer].lower[newvalue].isdata = 1;

      buffersort[newbuffer].lower[newvalue].ptr = bufnum;

      buffersort[newbuffer].ptrcount = 2;

      buffersort[newbuffer].firstptr = currentvalue;

      if(newvalue < buffersort[newbuffer].firstptr)

        buffersort[newbuffer].firstptr = newvalue;



      bufferlist[buffersortstart].upsort = newbuffer;

      bufferlist[buffersortstart].uppos = currentvalue;

      buffersortstart = newbuffer;

      buffersortstartisdata = 0;



      bufferlist[bufnum].upsort = newbuffer;

      bufferlist[bufnum].uppos = newvalue;

#ifdef VERBOSE_FUNCTION_BUFFER

      PrintLog("CDVD buffer:   Buffer up to 2 records now");

#endif /* VERBOSE_FUNCTION_BUFFER */

      return;

    } // ENDIF- Same LSN? Shift pointer in response. Else, add a Sort Record.

  } // ENDIF- Only 1 record? Check if we need a Sort Record.



  newmask = 0xff000000;

  newdivisor = 0x01000000;

  prevbuffer = 0xffff;

  prevvalue = 0;

  current = buffersortstart;

  currentisdata = 0;

  while(currentisdata == 0) {

    newvalue = (bufferlist[bufnum].lsn & newmask) / newdivisor;



    if(buffersort[current].mask != newmask) {

      comparebuffer = current;

      compareisdata = 0;

      while(compareisdata == 0) {

        comparevalue = buffersort[comparebuffer].firstptr;

        compareisdata = buffersort[comparebuffer].lower[comparevalue].isdata;

        comparebuffer = buffersort[comparebuffer].lower[comparevalue].ptr;

      } // ENDWHILE- looking for an another buffer to compare to...



      comparevalue = (bufferlist[comparebuffer].lsn & newmask) / newdivisor;

      if(newvalue != comparevalue) {

        // Add buffersort here (comparevalue/newvalue)

        newbuffer = AllocSortBuffer();

        buffersort[newbuffer].upptr = buffersort[current].upptr;

        buffersort[newbuffer].uppos = buffersort[current].uppos;

        buffersort[newbuffer].mask = newmask;

        buffersort[newbuffer].divisor = newdivisor;

        buffersort[newbuffer].lower[comparevalue].isdata = 0;

        buffersort[newbuffer].lower[comparevalue].ptr = current;

        buffersort[newbuffer].lower[newvalue].isdata = 1;

        buffersort[newbuffer].lower[newvalue].ptr = bufnum;

        buffersort[newbuffer].ptrcount = 2;

        buffersort[newbuffer].firstptr = comparevalue;

        if(newvalue < buffersort[newbuffer].firstptr)

          buffersort[newbuffer].firstptr = newvalue;



        if(buffersort[newbuffer].upptr == 0xffff) {

          buffersortstart = newbuffer;

        } else {

          buffersort[prevbuffer].lower[prevvalue].isdata = 0;

          buffersort[prevbuffer].lower[prevvalue].ptr = newbuffer;

          if(prevvalue < buffersort[prevbuffer].firstptr)

            buffersort[prevbuffer].firstptr = prevvalue;

        } // ENDIF- Do we need to adjust to buffersortstart connection?

        buffersort[current].upptr = newbuffer;

        buffersort[current].uppos = comparevalue;

        bufferlist[bufnum].upsort = newbuffer;

        bufferlist[bufnum].uppos = newvalue;

        return;

      } // ENDIF- Don't match? Add a buffersort here to say that!



      compareisdata = 0;

      newmask /= 0x0100;

      newdivisor /= 0x0100;



    } else {

      currentisdata = buffersort[current].lower[newvalue].isdata;

      prevbuffer = current;

      prevvalue = newvalue;

      if(currentisdata == 0) {

        current = buffersort[current].lower[newvalue].ptr;

        newmask /= 0x0100;

        newdivisor /= 0x0100;

        if(newdivisor == 0) {

#ifdef VERBOSE_WARNINGS_BUFFER

          PrintLog("CDVD buffer:   Mask/Divisor at 0! Index corruption! (detected in search)");

#endif /* VERBOSE_WARNINGS_BUFFER */

          return;

        } // ENDIF- The Mask went too far! Abort! (Sanity Check)

      } // ENDIF- Do we have to go through another level of sort data?

    } // ENDIF- We don't have a comparison on this byte?

  } // ENDWHILE- looking for his level...



  if(buffersort[current].lower[newvalue].isdata == 2) {

    buffersort[current].lower[newvalue].isdata = 1;

    buffersort[current].lower[newvalue].ptr = bufnum;

    buffersort[current].ptrcount++;

    if(newvalue < buffersort[current].firstptr)

      buffersort[current].firstptr = newvalue;

    bufferlist[bufnum].upsort = current;

    bufferlist[bufnum].uppos = newvalue;

    return;

  } // ENDIF- an empty slot? Fill it in.



  comparebuffer = buffersort[current].lower[newvalue].ptr;

  if(bufferlist[bufnum].lsn == bufferlist[comparebuffer].lsn) {

    buffersort[current].lower[newvalue].ptr = bufnum;

    bufferlist[bufnum].upsort = current;

    bufferlist[bufnum].uppos = newvalue;

    bufferlist[comparebuffer].upsort = 0xffff;

    return;

  } // ENDIF- Same LSN? Replace!



  // Calc new position based on new separation markers...

  newmask /= 0x0100;

  newdivisor /= 0x0100;

  if(newdivisor == 0) {

#ifdef VERBOSE_WARNINGS_BUFFER

    PrintLog("CDVD buffer:   Mask/Divisor at 0! Index corruption! (bottom add initial)");

#endif /* VERBOSE_WARNINGS_BUFFER */

    return;

  } // ENDIF- The Mask went too far! Abort! (Sanity Check)

  newvalue = (bufferlist[bufnum].lsn & newmask) / newdivisor;

  comparevalue = (bufferlist[comparebuffer].lsn & newmask) / newdivisor;

  while((newmask != 0x000000ff) && (comparevalue == newvalue)) {

    newmask /= 0x0100;

    newdivisor /= 0x0100;

    if(newdivisor == 0) {

#ifdef VERBOSE_WARNINGS_BUFFER

      PrintLog("CDVD buffer:   Mask/Divisor at 0! Index corruption! (bottom add loop)");

#endif /* VERBOSE_WARNINGS_BUFFER */

      return;

    } // ENDIF- The Mask went too far! Abort! (Sanity Check)

    newvalue = (bufferlist[bufnum].lsn & newmask) / newdivisor;

    comparevalue = (bufferlist[comparebuffer].lsn & newmask) / newdivisor;

  } // ENDWHILE- continuing to scan for difference between the two numbers



  newbuffer = AllocSortBuffer();

  buffersort[newbuffer].upptr = prevbuffer;

  buffersort[newbuffer].uppos = prevvalue;

  buffersort[newbuffer].mask = newmask;

  buffersort[newbuffer].divisor = newdivisor;

  buffersort[newbuffer].lower[comparevalue].isdata = 1;

  buffersort[newbuffer].lower[comparevalue].ptr = comparebuffer;

  buffersort[newbuffer].lower[newvalue].isdata = 1;

  buffersort[newbuffer].lower[newvalue].ptr = bufnum;

  buffersort[newbuffer].ptrcount = 2;

  buffersort[newbuffer].firstptr = comparevalue;

  if(newvalue < buffersort[newbuffer].firstptr)

    buffersort[newbuffer].firstptr = newvalue;



  buffersort[prevbuffer].lower[prevvalue].isdata = 0;

  buffersort[prevbuffer].lower[prevvalue].ptr = newbuffer;

  bufferlist[comparebuffer].upsort = newbuffer;

  bufferlist[comparebuffer].uppos = comparevalue;

  bufferlist[bufnum].upsort = newbuffer;

  bufferlist[bufnum].uppos = newvalue;

} // END AddListBuffer()



#ifdef VERBOSE_WARNINGS_BUFFER

void PrintSortBuffers() {

  u16 h;

  u16 i;

  u16 j;



  printf("CDVD buffer: Sort Buffer Dump\n");

  printf("CDVD buffer:   Top Pointer: isdata %u ptr %u\n",

         buffersortstartisdata, buffersortstart);

  for(i = 0; i < BUFFERMAX; i++) {



    h = 0;

    while((h < BUFFERMAX) && (buffersortempty[h].sortptr != i))  h++;



    if(h == BUFFERMAX) {

      printf("CDVD buffer:   Sort Buffer:%u   Mask:%x   Divisor:%x\n",

             i, buffersort[i].mask, buffersort[i].divisor);

      printf("CDVD buffer:     Up Ptr:%u   Up Pos:%u   First Down Ptr:%u   Ptr Count: %u\n",

             buffersort[i].upptr,

             buffersort[i].uppos,

             buffersort[i].firstptr,

             buffersort[i].ptrcount);

      printf("CDVD buffer:   ");

      for(j = 0; j < 256; j++) {

        if(buffersort[i].lower[j].isdata == 1)  printf("  D");

        if(buffersort[i].lower[j].isdata == 0)  printf("  L");

        if(buffersort[i].lower[j].isdata < 2) {

          printf("%u:%u", j, buffersort[i].lower[j].ptr);

        } // ENDIF- We have active data? Print it.

      } // NEXT j- Scanning lower 256 pointers for active ones...

      printf("\n");

    } // ENDIF- Not found in inactive list? Must be active! Print it.

  } // NEXT i- looking at all the Allocated Buffers...

} // END PrintSortBuffers()

#endif /* VERBOSE_WARNINGS_BUFFER */

