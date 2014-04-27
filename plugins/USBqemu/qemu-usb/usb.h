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

/* Constants related to the USB / PCI interaction */
#define USB_SBRN    0x60 /* Serial Bus Release Number Register */
#define USB_RELEASE_1  0x10 /* USB 1.0 */
#define USB_RELEASE_2  0x20 /* USB 2.0 */
#define USB_RELEASE_3  0x30 /* USB 3.0 */

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
#define USB_SPEED_SUPER 3

#define USB_SPEED_MASK_LOW   (1 << USB_SPEED_LOW)
#define USB_SPEED_MASK_FULL  (1 << USB_SPEED_FULL)
#define USB_SPEED_MASK_HIGH  (1 << USB_SPEED_HIGH)
#define USB_SPEED_MASK_SUPER (1 << USB_SPEED_SUPER)

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
#define ClassInterfaceRequest \
        ((USB_DIR_IN|USB_TYPE_CLASS|USB_RECIP_INTERFACE)<<8)
#define ClassInterfaceOutRequest \
        ((USB_DIR_OUT|USB_TYPE_CLASS|USB_RECIP_INTERFACE)<<8)

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
#define USB_DT_DEVICE_QUALIFIER         0x06
#define USB_DT_OTHER_SPEED_CONFIG       0x07
#define USB_DT_DEBUG                    0x0A
#define USB_DT_INTERFACE_ASSOC          0x0B

#define USB_ENDPOINT_XFER_CONTROL	0
#define USB_ENDPOINT_XFER_ISOC		1
#define USB_ENDPOINT_XFER_BULK		2
#define USB_ENDPOINT_XFER_INT		3

typedef struct USBBus USBBus;
typedef struct USBBusOps USBBusOps;
typedef struct USBPort USBPort;
typedef struct USBDevice USBDevice;
typedef struct USBDeviceInfo USBDeviceInfo;
typedef struct USBPacket USBPacket;

typedef struct USBDesc USBDesc;
typedef struct USBDescID USBDescID;
typedef struct USBDescDevice USBDescDevice;
typedef struct USBDescConfig USBDescConfig;
typedef struct USBDescIfaceAssoc USBDescIfaceAssoc;
typedef struct USBDescIface USBDescIface;
typedef struct USBDescEndpoint USBDescEndpoint;
typedef struct USBDescOther USBDescOther;
typedef struct USBDescString USBDescString;

struct USBDescString {
    uint8_t index;
    char *str;
    QLIST_ENTRY(USBDescString) next;
};

/* definition of a USB device */
struct USBDevice {
    //DeviceState qdev;
    USBDeviceInfo *info;
    USBPort *port;
    char *port_path;
    void *opaque;

    /* Actual connected speed */
    int speed;
    /* Supported speeds, not in info because it may be variable (hostdevs) */
    int speedmask;
    uint8_t addr;
    char product_desc[32];
    int auto_attach;
    int attached;

    int32_t state;
    uint8_t setup_buf[8];
    uint8_t data_buf[4096];
    int32_t remote_wakeup;
    int32_t setup_state;
    int32_t setup_len;
    int32_t setup_index;

    QLIST_HEAD(, USBDescString) strings;
    const USBDescDevice *device;
    const USBDescConfig *config;
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
     * Called when a packet is canceled.
     */
    void (*cancel_packet)(USBDevice *dev, USBPacket *p);

    /*
     * Called when device is destroyed.
     */
    void (*handle_destroy)(USBDevice *dev);

    /*
     * Attach the device
     */
    void (*handle_attach)(USBDevice *dev);

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
    int (*handle_control)(USBDevice *dev, USBPacket *p, int request, int value,
                          int index, int length, uint8_t *data);

    /*
     * Process data transfers (both BULK and ISOC).
     * Called from handle_packet().
     *
     * Returns length or one of the USB_RET_ codes.
     */
    int (*handle_data)(USBDevice *dev, USBPacket *p);

    const char *product_desc;
    const USBDesc *usb_desc;

    /* handle legacy -usbdevice command line options */
    const char *usbdevice_name;
    USBDevice *(*usbdevice_init)(const char *params);
};

typedef struct USBPortOps {
    void (*attach)(USBPort *port);
    void (*detach)(USBPort *port);
    /*
     * This gets called when a device downstream from the device attached to
     * the port (iow attached through a hub) gets detached.
     */
    void (*child_detach)(USBPort *port, USBDevice *child);
    void (*wakeup)(USBPort *port);
    /*
     * Note that port->dev will be different then the device from which
     * the packet originated when a hub is involved, if you want the orginating
     * device use p->owner
     */
    void (*complete)(USBPort *port, USBPacket *p);
} USBPortOps;

/* USB port on which a device can be connected */
struct USBPort {
    USBDevice *dev;
    int speedmask;
    char path[16];
    USBPortOps *ops;
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
    USBDevice *owner;
};

int usb_handle_packet(USBDevice *dev, USBPacket *p);
void usb_packet_complete(USBDevice *dev, USBPacket *p);
void usb_cancel_packet(USBPacket * p);

void usb_attach(USBPort *port, USBDevice *dev);
void usb_wakeup(USBDevice *dev);
int usb_generic_handle_packet(USBDevice *s, USBPacket *p);
void usb_generic_async_ctrl_complete(USBDevice *s, USBPacket *p);
int set_usb_string(uint8_t *buf, const char *str);
void usb_send_msg(USBDevice *dev, int msg);

/* usb-linux.c */
//USBDevice *usb_host_device_open(const char *devname);
//int usb_host_device_close(const char *devname);
//void usb_host_info(Monitor *mon);

/* usb-hid.c */
void usb_hid_datain_cb(USBDevice *dev, void *opaque, void (*datain)(void *));

/* usb-bt.c */
//USBDevice *usb_bt_init(HCIInfo *hci);

/* usb ports of the VM */

#define VM_USB_HUB_SIZE 8

/* usb-musb.c */
enum musb_irq_source_e {
    musb_irq_suspend = 0,
    musb_irq_resume,
    musb_irq_rst_babble,
    musb_irq_sof,
    musb_irq_connect,
    musb_irq_disconnect,
    musb_irq_vbus_request,
    musb_irq_vbus_error,
    musb_irq_rx,
    musb_irq_tx,
    musb_set_vbus,
    musb_set_session,
    __musb_irq_max,
};

typedef struct MUSBState MUSBState;
//MUSBState *musb_init(qemu_irq *irqs);
uint32_t musb_core_intr_get(MUSBState *s);
void musb_core_intr_clear(MUSBState *s, uint32_t mask);
void musb_set_size(MUSBState *s, int epnum, int size, int is_tx);

/* usb-bus.c */

struct USBBus {
    //BusState qbus;
    USBBusOps *ops;
    int busnr;
    int nfree;
    int nused;
    QTAILQ_HEAD(, USBPort) free;
    QTAILQ_HEAD(, USBPort) used;
    QTAILQ_ENTRY(USBBus) next;
};

struct USBBusOps {
    int (*register_companion)(USBBus *bus, USBPort *ports[],
                              uint32_t portcount, uint32_t firstport);
};

//void usb_bus_new(USBBus *bus, USBBusOps *ops, DeviceState *host);
USBBus *usb_bus_find(int busnr);
void usb_qdev_register(USBDeviceInfo *info);
void usb_qdev_register_many(USBDeviceInfo *info);
USBDevice *usb_create(USBBus *bus, const char *name);
USBDevice *usb_create_simple(USBBus *bus, const char *name);
USBDevice *usbdevice_create(const char *cmdline);
void usb_register_port(USBBus *bus, USBPort *port, void *opaque, int index,
                       USBPortOps *ops, int speedmask);
int usb_register_companion(const char *masterbus, USBPort *ports[],
                           uint32_t portcount, uint32_t firstport,
                           void *opaque, USBPortOps *ops, int speedmask);
void usb_port_location(USBPort *downstream, USBPort *upstream, int portnr);
void usb_unregister_port(USBBus *bus, USBPort *port);
int usb_device_attach(USBDevice *dev);
int usb_device_detach(USBDevice *dev);
int usb_device_delete_addr(int busnr, int addr);
//
//static inline USBBus *usb_bus_from_device(USBDevice *d)
//{
//    return DO_UPCAST(USBBus, qbus, d->qdev.parent_bus);
//}

USBDevice *usb_keyboard_init(void);
