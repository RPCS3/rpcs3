/*----------------------------------------------------------------------------
 *      U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 *      Name:    ADCUSER.H
 *      Purpose: Audio Device Class Custom User Definitions
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

#ifndef __ADCUSER_H__
#define __ADCUSER_H__


/* Audio Device Class Requests Callback Functions */
extern BOOL ADC_IF_GetRequest (void);
extern BOOL ADC_IF_SetRequest (void);
extern BOOL ADC_EP_GetRequest (void);
extern BOOL ADC_EP_SetRequest (void);


#endif  /* __ADCUSER_H__ */
