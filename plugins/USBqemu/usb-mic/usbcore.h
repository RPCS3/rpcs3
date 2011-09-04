/*----------------------------------------------------------------------------
 *      U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 *      Name:    USBCORE.H
 *      Purpose: USB Core Definitions
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

#ifndef __USBCORE_H__
#define __USBCORE_H__


/* USB Endpoint Data Structure */
typedef struct _USB_EP_DATA {
  BYTE  *pData;
  WORD   Count;
} USB_EP_DATA;

/* USB Core Global Variables */
extern WORD  USB_DeviceStatus;
extern BYTE  USB_DeviceAddress;
extern BYTE  USB_Configuration;
extern DWORD USB_EndPointMask;
extern DWORD USB_EndPointHalt;
extern BYTE  USB_AltSetting[USB_IF_NUM];

/* USB Endpoint 0 Buffer */
extern BYTE  EP0Buf[USB_MAX_PACKET0];

/* USB Endpoint 0 Data Info */
extern USB_EP_DATA EP0Data;

/* USB Setup Packet */
extern USB_SETUP_PACKET SetupPacket;

/* USB Core Functions */
extern void  USB_ResetCore (void);


#endif  /* __USBCORE_H__ */
