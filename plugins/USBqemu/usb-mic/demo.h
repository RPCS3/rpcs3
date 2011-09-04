/*----------------------------------------------------------------------------
 *      Name:    DEMO.H
 *      Purpose: USB Audio Demo Definitions
 *      Version: V1.10
 *----------------------------------------------------------------------------
 *      This software is supplied "AS IS" without any warranties, express,
 *      implied or statutory, including but not limited to the implied
 *      warranties of fitness for purpose, satisfactory quality and
 *      noninfringement. Keil extends you a royalty-free right to reproduce
 *      and distribute executable files created using this software for use
 *      on Philips LPC2xxx microcontroller devices only. Nothing else gives
 *      you the right to use this software.
 *
 *      Copyright (c) 2005-2006 Keil Software.
 *---------------------------------------------------------------------------*/

/* Clock Definitions */
#define CPU_CLOCK 60000000              /* CPU Clock */
#define VPB_CLOCK (CPU_CLOCK/1)         /* VPB Clock */

/* Audio Definitions */
#define DATA_FREQ 32000                 /* Audio Data Frequency */
#define P_S       32                    /* Packet Size */
#if USB_DMA
#define P_C       4                     /* Packet Count */
#else
#define P_C       1                     /* Packet Count */
#endif
#define B_S       (8*P_C*P_S)           /* Buffer Size */

/* Push Button Definitions */
#define PBINT     0x00004000            /* P0.14 */

/* LED Definitions */
#define LEDMSK    0x00FF0000            /* P1.16..23 */

/* Audio Demo Variables */
extern BYTE  Mute;                      /* Mute State */
extern DWORD Volume;                    /* Volume Level */
extern WORD  VolCur;                    /* Volume Current Value */
extern DWORD InfoBuf[P_C];              /* Packet Info Buffer */
extern short DataBuf[B_S];              /* Data Buffer */
extern WORD  DataOut;                   /* Data Out Index */
extern WORD  DataIn;                    /* Data In Index */
extern BYTE  DataRun;                   /* Data Stream Run State */
