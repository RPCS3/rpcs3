/***************************************************************************
                            spu.c  -  description
                             -------------------
    begin                : Sun Jan 12 2003
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
// 2005/08/29 - Pete
// - changed to 48Khz output
//
// 2003/03/01 - Pete
// - added mono mode
//
// 2003/01/12 - Pete
// - added recording funcs (win version only)
//
//*************************************************************************//

#include "stdafx.h"

#ifdef _WINDOWS

#include <mmsystem.h>
#include "resource.h"
#include "externals.h"

#define _IN_RECORD

#include "record.h"

////////////////////////////////////////////////////////////////////////

int      iDoRecord=0;
HMMIO    hWaveFile=NULL;
MMCKINFO mmckMain;
MMCKINFO mmckData;
char     szFileName[256]={"C:\\PEOPS.WAV"};

////////////////////////////////////////////////////////////////////////

void RecordStart()
{
 WAVEFORMATEX pcmwf;

 // setup header in the same format as our directsound stream
 memset(&pcmwf,0,sizeof(WAVEFORMATEX));
 pcmwf.wFormatTag      = WAVE_FORMAT_PCM;

 pcmwf.nChannels       = 2;
 pcmwf.nBlockAlign     = 4;

 pcmwf.nSamplesPerSec  = 48000;
 pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;
 pcmwf.wBitsPerSample  = 16;

 // create file
 hWaveFile=mmioOpen(szFileName,NULL,MMIO_CREATE|MMIO_WRITE|MMIO_EXCLUSIVE | MMIO_ALLOCBUF);
 if(!hWaveFile) return;
 
 // setup WAVE, fmt and data chunks
 memset(&mmckMain,0,sizeof(MMCKINFO));
 mmckMain.fccType = mmioFOURCC('W','A','V','E');

 mmioCreateChunk(hWaveFile,&mmckMain,MMIO_CREATERIFF);

 memset(&mmckData,0,sizeof(MMCKINFO));
 mmckData.ckid    = mmioFOURCC('f','m','t',' ');
 mmckData.cksize  = sizeof(WAVEFORMATEX);

 mmioCreateChunk(hWaveFile,&mmckData,0);
 mmioWrite(hWaveFile,(char*)&pcmwf,sizeof(WAVEFORMATEX)); 
 mmioAscend(hWaveFile,&mmckData,0);

 mmckData.ckid = mmioFOURCC('d','a','t','a');
 mmioCreateChunk(hWaveFile,&mmckData,0);
}

////////////////////////////////////////////////////////////////////////

void RecordStop()
{
 // first some check, if recording is running
 iDoRecord=0;
 if(!hWaveFile) return;

 // now finish writing & close the wave file
 mmioAscend(hWaveFile,&mmckData,0);
 mmioAscend(hWaveFile,&mmckMain,0);
 mmioClose(hWaveFile,0);

 // init var
 hWaveFile=NULL;
}

////////////////////////////////////////////////////////////////////////

void RecordBuffer(unsigned char* pSound,long lBytes)
{
 // write the samples
 if(hWaveFile) mmioWrite(hWaveFile,pSound,lBytes);
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

BOOL CALLBACK RecordDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch(uMsg)
  {
   //--------------------------------------------------// init
   case WM_INITDIALOG:
    {
     SetDlgItemText(hW,IDC_WAVFILE,szFileName);   // init filename edit
     ShowCursor(TRUE);                                 // mmm... who is hiding it? main emu? tsts
     return TRUE;
    }
   //--------------------------------------------------// destroy
   case WM_DESTROY:
    {
     RecordStop();
    }break;
   //--------------------------------------------------// command
   case WM_COMMAND:
    {
     if(wParam==IDCANCEL) iRecordMode=2;               // cancel? raise flag for destroying the dialog

     if(wParam==IDC_RECORD)                            // record start/stop?
      {
       if(IsWindowEnabled(GetDlgItem(hW,IDC_WAVFILE))) // not started yet (edit is not disabled):
        {
         GetDlgItemText(hW,IDC_WAVFILE,szFileName,255);// get filename

         RecordStart();                                // start recording

         if(hWaveFile)                                 // start was ok?
          {                                            // -> disable filename edit, change text, raise flag
           EnableWindow(GetDlgItem(hW,IDC_WAVFILE),FALSE);
           SetDlgItemText(hW,IDC_RECORD,"Stop recording");
           iDoRecord=1;
          }
         else MessageBeep(0xFFFFFFFF);                 // error starting recording? BEEP
        }
       else                                            // stop recording?
        {
         RecordStop();                                 // -> just do it
         EnableWindow(GetDlgItem(hW,IDC_WAVFILE),TRUE);// -> enable filename edit again
         SetDlgItemText(hW,IDC_RECORD,"Start recording");
        }
       SetFocus(hWMain);
      }

    }break;
   //--------------------------------------------------// size
   case WM_SIZE:
    if(wParam==SIZE_MINIMIZED) SetFocus(hWMain);       // if we get minimized, set the foxus to the main window
    break;
   //--------------------------------------------------// setcursor
   case WM_SETCURSOR:
    {
     SetCursor(LoadCursor(NULL,IDC_ARROW));            // force the arrow 
     return TRUE;
    }
   //--------------------------------------------------//
  }
 return FALSE;
}

////////////////////////////////////////////////////////////////////////
#endif
