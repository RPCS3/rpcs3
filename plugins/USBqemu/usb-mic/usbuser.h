/*----------------------------------------------------------------------------
 *      U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 *      Name:    USBUSER.H
 *      Purpose: USB Custom User Definitions
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

#ifndef __USBUSER_H__
#define __USBUSER_H__


/* USB Device Events Callback Functions */
extern void USB_Power_Event     (BOOL  power);
extern void USB_Reset_Event     (void);
extern void USB_Suspend_Event   (void);
extern void USB_Resume_Event    (void);
extern void USB_WakeUp_Event    (void);
extern void USB_SOF_Event       (void);
extern void USB_Error_Event     (DWORD error);

/* USB Endpoint Callback Events */
#define USB_EVT_SETUP       1   /* Setup Packet */
#define USB_EVT_OUT         2   /* OUT Packet */
#define USB_EVT_IN          3   /*  IN Packet */
#define USB_EVT_OUT_NAK     4   /* OUT Packet - Not Acknowledged */
#define USB_EVT_IN_NAK      5   /*  IN Packet - Not Acknowledged */
#define USB_EVT_OUT_STALL   6   /* OUT Packet - Stalled */
#define USB_EVT_IN_STALL    7   /*  IN Packet - Stalled */
#define USB_EVT_OUT_DMA_EOT 8   /* DMA OUT EP - End of Transfer */
#define USB_EVT_IN_DMA_EOT  9   /* DMA  IN EP - End of Transfer */
#define USB_EVT_OUT_DMA_NDR 10  /* DMA OUT EP - New Descriptor Request */
#define USB_EVT_IN_DMA_NDR  11  /* DMA  IN EP - New Descriptor Request */
#define USB_EVT_OUT_DMA_ERR 12  /* DMA OUT EP - Error */
#define USB_EVT_IN_DMA_ERR  13  /* DMA  IN EP - Error */

/* USB Endpoint Events Callback Pointers */
extern void (* const USB_P_EP[16])(DWORD event);

/* USB Endpoint Events Callback Functions */
extern void USB_EndPoint0  (DWORD event);
extern void USB_EndPoint1  (DWORD event);
extern void USB_EndPoint2  (DWORD event);
extern void USB_EndPoint3  (DWORD event);
extern void USB_EndPoint4  (DWORD event);
extern void USB_EndPoint5  (DWORD event);
extern void USB_EndPoint6  (DWORD event);
extern void USB_EndPoint7  (DWORD event);
extern void USB_EndPoint8  (DWORD event);
extern void USB_EndPoint9  (DWORD event);
extern void USB_EndPoint10 (DWORD event);
extern void USB_EndPoint11 (DWORD event);
extern void USB_EndPoint12 (DWORD event);
extern void USB_EndPoint13 (DWORD event);
extern void USB_EndPoint14 (DWORD event);
extern void USB_EndPoint15 (DWORD event);

/* USB Core Events Callback Functions */
extern void USB_Configure_Event (void);
extern void USB_Interface_Event (void);
extern void USB_Feature_Event   (void);


#endif  /* __USBUSER_H__ */
