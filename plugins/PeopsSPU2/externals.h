/***************************************************************************
                         externals.h  -  description
                             -------------------
    begin                : Wed May 15 2002
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
// 2004/04/04 - Pete
// - changed plugin to emulate PS2 spu
//
// 2002/04/04 - Pete
// - increased channel struct for interpolation
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//


/////////////////////////////////////////////////////////
// generic defines
/////////////////////////////////////////////////////////

//#define PSE_LT_SPU                  4
//#define PSE_SPU_ERR_SUCCESS         0
//#define PSE_SPU_ERR                 -60
//#define PSE_SPU_ERR_NOTCONFIGURED   PSE_SPU_ERR - 1
//#define PSE_SPU_ERR_INIT            PSE_SPU_ERR - 2

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

////////////////////////////////////////////////////////////////////////
// spu defines
////////////////////////////////////////////////////////////////////////

// sound buffer sizes
// 500 ms complete sound buffer
#define SOUNDSIZE   76800
                    
// 200 ms test buffer... if less than that is buffered, a new upload will happen
#define TESTSIZE    26304//13152
                    
// num of channels
#define MAXCHAN     48
#define HLFCHAN     24

// ~ 1 ms of data
#define NSSIZE 48

///////////////////////////////////////////////////////////
// struct defines
///////////////////////////////////////////////////////////
// ADMA Channels
extern EXPORT_GCC void CALLBACK SPU2async(unsigned long cycle);
typedef struct
{
 unsigned short * MemAddr;
 unsigned short * TempMem;
 unsigned int	  Index;
 unsigned int	  AmountLeft;
 unsigned int     ADMAPos;
 unsigned int     TransferAmount;
 int			  IRQ;
 int		      TempAmount;
} ADMA;

typedef struct
{
  int Left;
  int Right;
} DINPUT;

// ADSR INFOS PER CHANNEL
typedef struct
{
 int            AttackModeExp;
 long           AttackTime;
 long           DecayTime;
 long           SustainLevel;
 int            SustainModeExp;
 long           SustainModeDec;
 long           SustainTime;
 int            ReleaseModeExp;
 unsigned long  ReleaseVal;
 long           ReleaseTime;
 long           ReleaseStartTime; 
 long           ReleaseVol; 
 long           lTime;
 long           lVolume;
} ADSRInfo;

typedef struct
{
 int            State;
 int            AttackModeExp;
 int            AttackRate;
 int            DecayRate;
 int            SustainLevel;
 int            SustainModeExp;
 int            SustainIncrease;
 int            SustainRate;
 int            ReleaseModeExp;
 int            ReleaseRate;
 int            EnvelopeVol;
 long           lVolume;
 long           lDummy1;
 long           lDummy2;
} ADSRInfoEx;
              
///////////////////////////////////////////////////////////

// Tmp Flags

// used for debug channel muting
#define FLAG_MUTE  1

// used for simple interpolation
#define FLAG_IPOL0 2
#define FLAG_IPOL1 4

///////////////////////////////////////////////////////////

// MAIN CHANNEL STRUCT
typedef struct
{
 // no mutexes used anymore... don't need them to sync access
 //HANDLE            hMutex;

 int               bNew;                               // start flag

 int               iSBPos;                             // mixing stuff
 int               spos;
 int               sinc;
 int               SB[32+32];                          // Pete added another 32 dwords in 1.6 ... prevents overflow issues with gaussian/cubic interpolation (thanx xodnizel!), and can be used for even better interpolations, eh? :)
 int               sval;

 unsigned char *   pStart;                             // start ptr into sound mem
 unsigned char *   pCurr;                              // current pos in sound mem
 unsigned char *   pLoop;                              // loop ptr in sound mem

 int               iStartAdr;
 int               iLoopAdr; 
 int               iNextAdr; 

 int               bOn;                                // is channel active (sample playing?)
 int               bStop;                              // is channel stopped (sample _can_ still be playing, ADSR Release phase)
 int               bEndPoint;                          // end point reached
 int               bReverbL;                           // can we do reverb on this channel? must have ctrl register bit, to get active
 int               bReverbR; 
 
 int               bVolumeL;                           // Volume on/off
 int               bVolumeR;
 
 int               iActFreq;                           // current psx pitch
 int               iUsedFreq;                          // current pc pitch
 int               iLeftVolume;                        // left volume
 int               iLeftVolRaw;                        // left psx volume value
 int               bIgnoreLoop;                        // ignore loop bit, if an external loop address is used
 int               iMute;                              // mute mode
 int               iRightVolume;                       // right volume
 int               iRightVolRaw;                       // right psx volume value
 int               iRawPitch;                          // raw pitch (0...3fff)
 int               iIrqDone;                           // debug irq done flag
 int               s_1;                                // last decoding infos
 int               s_2;
 int               bRVBActive;                         // reverb active flag
 int               bNoise;                             // noise active flag
 int               bFMod;                              // freq mod (0=off, 1=sound channel, 2=freq channel)
 int               iOldNoise;                          // old noise val for this channel   
 ADSRInfo          ADSR;                               // active ADSR settings
 ADSRInfoEx        ADSRX;                              // next ADSR settings (will be moved to active on sample start)

} SPUCHAN;

///////////////////////////////////////////////////////////

typedef struct
{
 int StartAddr;      // reverb area start addr in samples
 int EndAddr;        // reverb area end addr in samples
 int CurrAddr;       // reverb area curr addr in samples

 int VolLeft;
 int VolRight;
 int iLastRVBLeft;
 int iLastRVBRight;
 int iRVBLeft;
 int iRVBRight;
 int iCnt;

 int FB_SRC_A;       // (offset)
 int FB_SRC_B;       // (offset)
 int IIR_ALPHA;      // (coef.)
 int ACC_COEF_A;     // (coef.)
 int ACC_COEF_B;     // (coef.)
 int ACC_COEF_C;     // (coef.)
 int ACC_COEF_D;     // (coef.)
 int IIR_COEF;       // (coef.)
 int FB_ALPHA;       // (coef.)
 int FB_X;           // (coef.)
 int IIR_DEST_A0;    // (offset)
 int IIR_DEST_A1;    // (offset)
 int ACC_SRC_A0;     // (offset)
 int ACC_SRC_A1;     // (offset)
 int ACC_SRC_B0;     // (offset)
 int ACC_SRC_B1;     // (offset)
 int IIR_SRC_A0;     // (offset)
 int IIR_SRC_A1;     // (offset)
 int IIR_DEST_B0;    // (offset)
 int IIR_DEST_B1;    // (offset)
 int ACC_SRC_C0;     // (offset)
 int ACC_SRC_C1;     // (offset)
 int ACC_SRC_D0;     // (offset)
 int ACC_SRC_D1;     // (offset)
 int IIR_SRC_B1;     // (offset)
 int IIR_SRC_B0;     // (offset)
 int MIX_DEST_A0;    // (offset)
 int MIX_DEST_A1;    // (offset)
 int MIX_DEST_B0;    // (offset)
 int MIX_DEST_B1;    // (offset)
 int IN_COEF_L;      // (coef.)
 int IN_COEF_R;      // (coef.)
} REVERBInfo;

#ifdef _WINDOWS
extern HINSTANCE hInst;
#define WM_MUTE (WM_USER+543)
#endif

///////////////////////////////////////////////////////////
// SPU.C globals
///////////////////////////////////////////////////////////

#ifndef _IN_SPU

// psx buffers / addresses

extern unsigned short  regArea[];                        
extern unsigned short  spuMem[];
extern unsigned char * spuMemC;
extern unsigned char * pSpuIrq[];
extern unsigned char * pSpuBuffer;

// user settings

extern int        iUseXA;
extern int        iVolume;
extern int        iXAPitch;
extern int        iUseTimer;
extern int        iDebugMode;
extern int        iRecordMode;
extern int        iUseReverb;
extern int        iUseInterpolation;
extern int        iDisStereo;
extern int        aSync;
// MISC

extern SPUCHAN s_chan[];
extern REVERBInfo rvb[];

extern unsigned long dwNoiseVal;
extern unsigned short spuCtrl2[];
extern unsigned short spuStat2[];
extern unsigned long  spuIrq2[];
extern unsigned long  spuAddr2[];
extern unsigned long   spuRvbAddr2[];
extern unsigned long   spuRvbAEnd2[];

extern int      bEndThread; 
extern int      bThreadEnded;
extern int      bSpuInit;

extern int      SSumR[];
extern int      SSumL[];
extern int      iCycle;
extern short *  pS;
extern unsigned long dwNewChannel2[];
extern unsigned long dwEndChannel2[];

extern int iSpuAsyncWait;

#ifdef _WINDOWS
extern HWND    hWMain;                               // window handle
extern HWND    hWDebug;
#endif

extern void (CALLBACK *cddavCallback)(unsigned short,unsigned short);

#endif

///////////////////////////////////////////////////////////
// DSOUND.C globals
///////////////////////////////////////////////////////////

#ifndef _IN_DSOUND

#ifdef _WINDOWS
extern unsigned long LastWrite;
extern unsigned long LastPlay;
#endif

#endif

///////////////////////////////////////////////////////////
// RECORD.C globals
///////////////////////////////////////////////////////////

#ifndef _IN_RECORD

#ifdef _WINDOWS
extern int iDoRecord;
#endif

#endif

///////////////////////////////////////////////////////////
// XA.C globals
///////////////////////////////////////////////////////////

#ifndef _IN_XA

extern xa_decode_t   * xapGlobal;

extern unsigned long * XAFeed;
extern unsigned long * XAPlay;
extern unsigned long * XAStart;
extern unsigned long * XAEnd;

extern unsigned long   XARepeat;
extern unsigned long   XALastVal;

extern int           iLeftXAVol;
extern int           iRightXAVol;

#endif

///////////////////////////////////////////////////////////
// REVERB.C globals
///////////////////////////////////////////////////////////

#ifndef _IN_REVERB

extern int *          sRVBPlay[];
extern int *          sRVBEnd[];
extern int *          sRVBStart[];

#endif
