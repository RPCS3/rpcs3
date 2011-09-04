/*
 * QEMU USB API
 *
 * Copyright (c) 2005 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu-queue.h"

#define USB_TOKEN_SETUP 0x2d
#define USB_TOKEN_IN    0x69 /* device -> host */
#define USB_TOKEN_OUT   0xe1 /* host -> device */

/* specific usb messages, also sent in the 'pid' parameter */
#define USB_MSG_ATTACH   0x100
#define USB_MSG_DETACH   0x101
#define USB_MSG_RESET    0x102

#define USB_RET_NODEV  (-1)
#define USB_RET_NAK    (-2)
#define USB_RET_STALL  (-3)
#define USB_RET_BABBLE (-4)
#define USB_RET_ASYNC  (-5)

#define USB_SPEED_LOW   0
#define USB_SPEED_FULL  1
#define USB_SPEED_HIGH  2

#define USB_STATE_NOTATTACHED 0
#define USB_STATE_ATTACHED    1
//#define USB_STATE_POWERED     2
#define USB_STATE_DEFAULT     3
//#define USB_STATE_ADDRESS     4
//#define	USB_STATE_CONFIGURED  5
#define USB_STATE_SUSPENDED   6

#define USB_CLASS_AUDIO			1
#define USB_CLASS_COMM			2
#define USB_CLASS_HID			3
#define USB_CLASS_PHYSICAL		5
#define USB_CLASS_STILL_IMAGE		6
#define USB_CLASS_PRINTER		7
#define USB_CLASS_MASS_STORAGE		8
#define USB_CLASS_HUB			9
#define USB_CLASS_CDC_DATA		0x0a
#define USB_CLASS_CSCID			0x0b
#define USB_CLASS_CONTENT_SEC		0x0d
#define USB_CLASS_APP_SPEC		0xfe
#define USB_CLASS_VENDOR_SPEC		0xff

#define USB_DIR_OUT			0
#define USB_DIR_IN			0x80

#define USB_TYPE_MASK			(0x03 << 5)
#define USB_TYPE_STANDARD		(0x00 << 5)
#define USB_TYPE_CLASS			(0x01 << 5)
#define USB_TYPE_VENDOR			(0x02 << 5)
#define USB_TYPE_RESERVED		(0x03 << 5)

#define USB_RECIP_MASK			0x1f
#define USB_RECIP_DEVICE		0x00
#define USB_RECIP_INTERFACE		0x01
#define USB_RECIP_ENDPOINT		0x02
#define USB_RECIP_OTHER			0x03

#define DeviceRequest ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_DEVICE)<<8)
#define DeviceOutRequest ((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_DEVICE)<<8)
#define InterfaceRequest \
        ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8)
#define InterfaceOutRequest \
        ((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8)
#define EndpointRequest ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_ENDPOINT)<<8)
#define EndpointOutRequest \
        ((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_ENDPOINT)<<8)

#define USB_REQ_GET_STATUS		0x00
#define USB_REQ_CLEAR_FEATURE		0x01
#define USB_REQ_SET_FEATURE		0x03
#define USB_REQ_SET_ADDRESS		0x05
#define USB_REQ_GET_DESCRIPTOR		0x06
#define USB_REQ_SET_DESCRIPTOR		0x07
#define USB_REQ_GET_CONFIGURATION	0x08
#define USB_REQ_SET_CONFIGURATION	0x09
#define USB_REQ_GET_INTERFACE		0x0A
#define USB_REQ_SET_INTERFACE		0x0B
#define USB_REQ_SYNCH_FRAME		0x0C

#define USB_DEVICE_SELF_POWERED		0
#define USB_DEVICE_REMOTE_WAKEUP	1

#define USB_DT_DEVICE			0x01
#define USB_DT_CONFIG			0x02
#define USB_DT_STRING			0x03
#define USB_DT_INTERFACE		0x04
#define USB_DT_ENDPOINT			0x05

#define USB_ENDPOINT_XFER_CONTROL	0
#define USB_ENDPOINT_XFER_ISOC		1
#define USB_ENDPOINT_XFER_BULK		2
#define USB_ENDPOINT_XFER_INT		3

typedef struct USBBus USBBus;
typedef struct USBPort USBPort;
typedef struct USBDevice USBDevice;
typedef struct USBDeviceInfo USBDeviceInfo;
typedef struct USBPacket USBPacket;

/* definition of a USB device */
struct USBDevice {
    //DeviceState qdev;
    USBDeviceInfo *info;
    void *opaque;

    int speed;
    uint8_t addr;
    char product_desc[32];
    int auto_attach;
    int attached;

    int state;
    uint8_t setup_buf[8];
    uint8_t data_buf[1024];
    int remote_wakeup;
    int setup_state;
    int setup_len;
    int setup_index;
};

struct USBDeviceInfo {
    //DeviceInfo qdev;
    int (*init)(USBDevice *dev);

    /*
     * Process USB packet.
     * Called by the HC (Host Controller).
     *
     * Returns length of the transaction
     * or one of the USB_RET_XXX codes.
     */
    int (*handle_packet)(USBDevice *dev, USBPacket *p);

    /*
     * Called when device is destroyed.
     */
    void (*handle_destroy)(USBDevice *dev);

    /*
     * Reset the device
     */
    void (*handle_reset)(USBDevice *dev);

    /*
     * Process control request.
     * Called from handle_packet().
     *
     * Returns length or one of the USB_RET_ codes.
     */
    int (*handle_control)(USBDevice *dev, int request, int value,
                          int index, int length, uint8_t *data);

    /*
     * Process data transfers (both BULK and ISOC).
     * Called from handle_packet().
     *
     * Returns length or one of the USB_RET_ codes.
     */
    int (*handle_data)(USBDevice *dev, USBPacket *p);

    const char *product_desc;

    /* handle legacy -usbdevice command line options */
    const char *usbdevice_name;
    USBDevice *(*usbdevice_init)(const char *params);
};

typedef void (*usb_attachfn)(USBPort *port, USBDevice *dev);

/* USB port on which a device can be connected */
struct USBPort {
    USBDevice *dev;
    usb_attachfn attach;
    void *opaque;
    int index; /* internal port index, may be used with the opaque */
    QTAILQ_ENTRY(USBPort) next;
};

typedef void USBCallback(USBPacket * packet, void *opaque);

/* Structure used to hold information about an active USB packet.  */
struct USBPacket {
    /* Data fields for use by the driver.  */
    int pid;
    uint8_t devaddr;
    uint8_t devep;
    uint8_t *data;
    int len;
    /* Internal use by the USB layer.  */
    USBCallback *complete_cb;
    void *complete_opaque;
    USBCallback *cancel_cb;
    void *cancel_opaque;
};

/* Defer completion of a USB packet.  The hadle_packet routine should then
   return USB_RET_ASYNC.  Packets that complete immediately (before
   handle_packet returns) should not call this method.  */
static inline void usb_defer_packet(USBPacket *p, USBCallback *cancel,
                                    void * opaque)
{
    p->cancel_cb = cancel;
    p->cancel_opaque = opaque;
}

/* Notify the controller that an async packet is complete.  This should only
   be called for packets previously deferred with usb_defer_packet, and
   should never be called from within handle_packet.  */
static inline void usb_packet_complete(USBPacket *p)
{
    p->complete_cb(p, p->complete_opaque);
}

/* Cancel an active packet.  The packed must have been deferred with
   usb_defer_packet,  and not yet completed.  */
static inline void usb_cancel_packet(USBPacket * p)
{
    p->cancel_cb(p, p->cancel_opaque);
}

void usb_attach(USBPort *port, USBDevice *dev);
int usb_generic_handle_packet(USBDevice *s, USBPacket *p);
int set_usb_string(uint8_t *buf, const char *str);
void usb_send_msg(USBDevice *dev, int msg);

/* usb-hid.c */
void usb_hid_datain_cb(USBDevice *dev, void *opaque, void (*datain)(void *));

/* usb ports of the VM */

#define VM_USB_HUB_SIZE 2

/* usb-kbd.cpp */
USBDevice *usb_keyboard_init(void);