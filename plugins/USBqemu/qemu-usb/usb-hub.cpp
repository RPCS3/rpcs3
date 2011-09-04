/*
 * QEMU USB HUB emulation
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
#include "vl.h"

//#define DEBUG

#define MAX_PORTS 8

typedef struct USBHubPort {
    USBPort port;
    uint16_t wPortStatus;
    uint16_t wPortChange;
} USBHubPort;

typedef struct USBHubState {
    USBDevice dev;
    int nb_ports;
    USBHubPort ports[MAX_PORTS];
} USBHubState;

#define ClearHubFeature		(0x2000 | USB_REQ_CLEAR_FEATURE)
#define ClearPortFeature	(0x2300 | USB_REQ_CLEAR_FEATURE)
#define GetHubDescriptor	(0xa000 | USB_REQ_GET_DESCRIPTOR)
#define GetHubStatus		(0xa000 | USB_REQ_GET_STATUS)
#define GetPortStatus		(0xa300 | USB_REQ_GET_STATUS)
#define SetHubFeature		(0x2000 | USB_REQ_SET_FEATURE)
#define SetPortFeature		(0x2300 | USB_REQ_SET_FEATURE)

#define PORT_STAT_CONNECTION	0x0001
#define PORT_STAT_ENABLE	0x0002
#define PORT_STAT_SUSPEND	0x0004
#define PORT_STAT_OVERCURRENT	0x0008
#define PORT_STAT_RESET		0x0010
#define PORT_STAT_POWER		0x0100
#define PORT_STAT_LOW_SPEED	0x0200
#define PORT_STAT_HIGH_SPEED    0x0400
#define PORT_STAT_TEST          0x0800
#define PORT_STAT_INDICATOR     0x1000

#define PORT_STAT_C_CONNECTION	0x0001
#define PORT_STAT_C_ENABLE	0x0002
#define PORT_STAT_C_SUSPEND	0x0004
#define PORT_STAT_C_OVERCURRENT	0x0008
#define PORT_STAT_C_RESET	0x0010

#define PORT_CONNECTION	        0
#define PORT_ENABLE		1
#define PORT_SUSPEND		2
#define PORT_OVERCURRENT	3
#define PORT_RESET		4
#define PORT_POWER		8
#define PORT_LOWSPEED		9
#define PORT_HIGHSPEED		10
#define PORT_C_CONNECTION	16
#define PORT_C_ENABLE		17
#define PORT_C_SUSPEND		18
#define PORT_C_OVERCURRENT	19
#define PORT_C_RESET		20
#define PORT_TEST               21
#define PORT_INDICATOR          22

/* same as Linux kernel root hubs */

static const uint8_t qemu_hub_dev_descriptor[] = {
	0x12,       /*  u8 bLength; */
	0x01,       /*  u8 bDescriptorType; Device */
	0x10, 0x01, /*  u16 bcdUSB; v1.1 */

	0x09,	    /*  u8  bDeviceClass; HUB_CLASSCODE */
	0x00,	    /*  u8  bDeviceSubClass; */
	0x00,       /*  u8  bDeviceProtocol; [ low/full speeds only ] */
	0x08,       /*  u8  bMaxPacketSize0; 8 Bytes */

	0x00, 0x00, /*  u16 idVendor; */
 	0x00, 0x00, /*  u16 idProduct; */
	0x01, 0x01, /*  u16 bcdDevice */

	0x03,       /*  u8  iManufacturer; */
	0x02,       /*  u8  iProduct; */
	0x01,       /*  u8  iSerialNumber; */
	0x01        /*  u8  bNumConfigurations; */
};

/* XXX: patch interrupt size */
static const uint8_t qemu_hub_config_descriptor[] = {

	/* one configuration */
	0x09,       /*  u8  bLength; */
	0x02,       /*  u8  bDescriptorType; Configuration */
	0x19, 0x00, /*  u16 wTotalLength; */
	0x01,       /*  u8  bNumInterfaces; (1) */
	0x01,       /*  u8  bConfigurationValue; */
	0x00,       /*  u8  iConfiguration; */
	0xc0,       /*  u8  bmAttributes; 
				 Bit 7: must be set,
				     6: Self-powered,
				     5: Remote wakeup,
				     4..0: resvd */
	0x00,       /*  u8  MaxPower; */
      
	/* USB 1.1:
	 * USB 2.0, single TT organization (mandatory):
	 *	one interface, protocol 0
	 *
	 * USB 2.0, multiple TT organization (optional):
	 *	two interfaces, protocols 1 (like single TT)
	 *	and 2 (multiple TT mode) ... config is
	 *	sometimes settable
	 *	NOT IMPLEMENTED
	 */

	/* one interface */
	0x09,       /*  u8  if_bLength; */
	0x04,       /*  u8  if_bDescriptorType; Interface */
	0x00,       /*  u8  if_bInterfaceNumber; */
	0x00,       /*  u8  if_bAlternateSetting; */
	0x01,       /*  u8  if_bNumEndpoints; */
	0x09,       /*  u8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,       /*  u8  if_bInterfaceSubClass; */
	0x00,       /*  u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
	0x00,       /*  u8  if_iInterface; */
     
	/* one endpoint (status change endpoint) */
	0x07,       /*  u8  ep_bLength; */
	0x05,       /*  u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x03,       /*  u8  ep_bmAttributes; Interrupt */
 	0x02, 0x00, /*  u16 ep_wMaxPacketSize; 1 + (MAX_ROOT_PORTS / 8) */
	0xff        /*  u8  ep_bInterval; (255ms -- usb 2.0 spec) */
};

static const uint8_t qemu_hub_hub_descriptor[] =
{
	0x00,			/*  u8  bLength; patched in later */
	0x29,			/*  u8  bDescriptorType; Hub-descriptor */
	0x00,			/*  u8  bNbrPorts; (patched later) */
	0x0a,			/* u16  wHubCharacteristics; */
	0x00,			/*   (per-port OC, no power switching) */
	0x01,			/*  u8  bPwrOn2pwrGood; 2ms */
	0x00			/*  u8  bHubContrCurrent; 0 mA */

        /* DeviceRemovable and PortPwrCtrlMask patched in later */
};

static void usb_hub_attach(USBPort *port1, USBDevice *dev)
{
    USBHubState *s =(USBHubState *) port1->opaque;
    USBHubPort *port = (USBHubPort *)&s->ports[port1->index];
    
    if (dev) {
        if (port->port.dev)
            usb_attach(port1, NULL);
        
        port->wPortStatus |= PORT_STAT_CONNECTION;
        port->wPortChange |= PORT_STAT_C_CONNECTION;
        if (dev->speed == USB_SPEED_LOW)
            port->wPortStatus |= PORT_STAT_LOW_SPEED;
        else
            port->wPortStatus &= ~PORT_STAT_LOW_SPEED;
        port->port.dev = dev;
        /* send the attach message */
        dev->handle_packet(dev, 
                           USB_MSG_ATTACH, 0, 0, NULL, 0);
    } else {
        dev = port->port.dev;
        if (dev) {
            port->wPortStatus &= ~PORT_STAT_CONNECTION;
            port->wPortChange |= PORT_STAT_C_CONNECTION;
            if (port->wPortStatus & PORT_STAT_ENABLE) {
                port->wPortStatus &= ~PORT_STAT_ENABLE;
                port->wPortChange |= PORT_STAT_C_ENABLE;
            }
            /* send the detach message */
            dev->handle_packet(dev, 
                               USB_MSG_DETACH, 0, 0, NULL, 0);
            port->port.dev = NULL;
        }
    }
}

static void usb_hub_handle_reset(USBDevice *dev)
{
    /* XXX: do it */
}

static int usb_hub_handle_control(USBDevice *dev, int request, int value,
                                  int index, int length, uint8_t *data)
{
    USBHubState *s = (USBHubState *)dev;
    int ret;

    switch(request) {
    case DeviceRequest | USB_REQ_GET_STATUS:
        data[0] = (1 << USB_DEVICE_SELF_POWERED) |
            (dev->remote_wakeup << USB_DEVICE_REMOTE_WAKEUP);
        data[1] = 0x00;
        ret = 2;
        break;
    case DeviceOutRequest | USB_REQ_CLEAR_FEATURE:
        if (value == USB_DEVICE_REMOTE_WAKEUP) {
            dev->remote_wakeup = 0;
        } else {
            goto fail;
        }
        ret = 0;
        break;
    case EndpointOutRequest | USB_REQ_CLEAR_FEATURE:
        if (value == 0 && index != 0x81) { /* clear ep halt */
            goto fail;
        }
        ret = 0;
        break;
    case DeviceOutRequest | USB_REQ_SET_FEATURE:
        if (value == USB_DEVICE_REMOTE_WAKEUP) {
            dev->remote_wakeup = 1;
        } else {
            goto fail;
        }
        ret = 0;
        break;
    case DeviceOutRequest | USB_REQ_SET_ADDRESS:
        dev->addr = value;
        ret = 0;
        break;
    case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
        switch(value >> 8) {
        case USB_DT_DEVICE:
            memcpy(data, qemu_hub_dev_descriptor, 
                   sizeof(qemu_hub_dev_descriptor));
            ret = sizeof(qemu_hub_dev_descriptor);
            break;
        case USB_DT_CONFIG:
            memcpy(data, qemu_hub_config_descriptor, 
                   sizeof(qemu_hub_config_descriptor));

            /* status change endpoint size based on number
             * of ports */
            data[22] = (s->nb_ports + 1 + 7) / 8;

            ret = sizeof(qemu_hub_config_descriptor);
            break;
        case USB_DT_STRING:
            switch(value & 0xff) {
            case 0:
                /* language ids */
                data[0] = 4;
                data[1] = 3;
                data[2] = 0x09;
                data[3] = 0x04;
                ret = 4;
                break;
            case 1:
                /* serial number */
                ret = set_usb_string(data, "314159");
                break;
            case 2:
                /* product description */
                ret = set_usb_string(data, "QEMU USB Hub");
                break;
            case 3:
                /* vendor description */
                ret = set_usb_string(data, "PCSX2/QEMU");
                break;
            default:
                goto fail;
            }
            break;
        default:
            goto fail;
        }
        break;
    case DeviceRequest | USB_REQ_GET_CONFIGURATION:
        data[0] = 1;
        ret = 1;
        break;
    case DeviceOutRequest | USB_REQ_SET_CONFIGURATION:
        ret = 0;
        break;
    case DeviceRequest | USB_REQ_GET_INTERFACE:
        data[0] = 0;
        ret = 1;
        break;
    case DeviceOutRequest | USB_REQ_SET_INTERFACE:
        ret = 0;
        break;
        /* usb specific requests */
    case GetHubStatus:
        data[0] = 0;
        data[1] = 0;
        data[2] = 0;
        data[3] = 0;
        ret = 4;
        break;
    case GetPortStatus:
        {
            unsigned int n = index - 1;
            USBHubPort *port;
            if (n >= s->nb_ports)
                goto fail;
            port = &s->ports[n];
            data[0] = port->wPortStatus;
            data[1] = port->wPortStatus >> 8;
            data[2] = port->wPortChange;
            data[3] = port->wPortChange >> 8;
            ret = 4;
        }
        break;
    case SetHubFeature:
    case ClearHubFeature:
        if (value == 0 || value == 1) {
        } else {
            goto fail;
        }
        ret = 0;
        break;
    case SetPortFeature:
        {
            unsigned int n = index - 1;
            USBHubPort *port;
            USBDevice *dev;
            if (n >= s->nb_ports)
                goto fail;
            port = &s->ports[n];
            dev = port->port.dev;
            switch(value) {
            case PORT_SUSPEND:
                port->wPortStatus |= PORT_STAT_SUSPEND;
                break;
            case PORT_RESET:
                if (dev) {
                    dev->handle_packet(dev, 
                                       USB_MSG_RESET, 0, 0, NULL, 0);
                    port->wPortChange |= PORT_STAT_C_RESET;
                    /* set enable bit */
                    port->wPortStatus |= PORT_STAT_ENABLE;
                }
                break;
            case PORT_POWER:
                break;
            default:
                goto fail;
            }
            ret = 0;
        }
        break;
    case ClearPortFeature:
        {
            unsigned int n = index - 1;
            USBHubPort *port;
            USBDevice *dev;
            if (n >= s->nb_ports)
                goto fail;
            port = &s->ports[n];
            dev = port->port.dev;
            switch(value) {
            case PORT_ENABLE:
                port->wPortStatus &= ~PORT_STAT_ENABLE;
                break;
            case PORT_C_ENABLE:
                port->wPortChange &= ~PORT_STAT_C_ENABLE;
                break;
            case PORT_SUSPEND:
                port->wPortStatus &= ~PORT_STAT_SUSPEND;
                break;
            case PORT_C_SUSPEND:
                port->wPortChange &= ~PORT_STAT_C_SUSPEND;
                break;
            case PORT_C_CONNECTION:
                port->wPortChange &= ~PORT_STAT_C_CONNECTION;
                break;
            case PORT_C_OVERCURRENT:
                port->wPortChange &= ~PORT_STAT_C_OVERCURRENT;
                break;
            case PORT_C_RESET:
                port->wPortChange &= ~PORT_STAT_C_RESET;
                break;
            default:
                goto fail;
            }
            ret = 0;
        }
        break;
    case GetHubDescriptor:
        {
            unsigned int n, limit, var_hub_size = 0;
            memcpy(data, qemu_hub_hub_descriptor, 
                   sizeof(qemu_hub_hub_descriptor));
            data[2] = s->nb_ports;

            /* fill DeviceRemovable bits */
            limit = ((s->nb_ports + 1 + 7) / 8) + 7;
            for (n = 7; n < limit; n++) {
                data[n] = 0x00;
                var_hub_size++;
            }

            /* fill PortPwrCtrlMask bits */
            limit = limit + ((s->nb_ports + 7) / 8);
            for (;n < limit; n++) {
                data[n] = 0xff;
                var_hub_size++;
            }

            ret = sizeof(qemu_hub_hub_descriptor) + var_hub_size;
            data[0] = ret;
            break;
        }
    default:
    fail:
        ret = USB_RET_STALL;
        break;
    }
    return ret;
}

static int usb_hub_handle_data(USBDevice *dev, int pid, 
                               uint8_t devep, uint8_t *data, int len)
{
    USBHubState *s = (USBHubState *)dev;
    int ret;

    switch(pid) {
    case USB_TOKEN_IN:
        if (devep == 1) {
            USBHubPort *port;
            unsigned int status;
            int i, n;
            n = (s->nb_ports + 1 + 7) / 8;
            if (len == 1) { /* FreeBSD workaround */
                n = 1;
            } else if (n > len) {
                return USB_RET_BABBLE;
            }
            status = 0;
            for(i = 0; i < s->nb_ports; i++) {
                port = &s->ports[i];
                if (port->wPortChange)
                    status |= (1 << (i + 1));
            }
            if (status != 0) {
                for(i = 0; i < n; i++) {
                    data[i] = status >> (8 * i);
                }
                ret = n;
            } else {
                ret = USB_RET_NAK; /* usb11 11.13.1 */
            }
        } else {
            goto fail;
        }
        break;
    case USB_TOKEN_OUT:
    default:
    fail:
        ret = USB_RET_STALL;
        break;
    }
    return ret;
}

static int usb_hub_broadcast_packet(USBHubState *s, int pid, 
                                    uint8_t devaddr, uint8_t devep,
                                    uint8_t *data, int len)
{
    USBHubPort *port;
    USBDevice *dev;
    int i, ret;

    for(i = 0; i < s->nb_ports; i++) {
        port = &s->ports[i];
        dev = port->port.dev;
        if (dev && (port->wPortStatus & PORT_STAT_ENABLE)) {
            ret = dev->handle_packet(dev, pid, 
                                     devaddr, devep,
                                     data, len);
            if (ret != USB_RET_NODEV) {
                return ret;
            }
        }
    }
    return USB_RET_NODEV;
}

static int usb_hub_handle_packet(USBDevice *dev, int pid, 
                                 uint8_t devaddr, uint8_t devep,
                                 uint8_t *data, int len)
{
    USBHubState *s = (USBHubState *)dev;

#if defined(DEBUG) && 0
    printf("usb_hub: pid=0x%x\n", pid);
#endif
    if (dev->state == USB_STATE_DEFAULT &&
        dev->addr != 0 &&
        devaddr != dev->addr &&
        (pid == USB_TOKEN_SETUP || 
         pid == USB_TOKEN_OUT || 
         pid == USB_TOKEN_IN)) {
        /* broadcast the packet to the devices */
        return usb_hub_broadcast_packet(s, pid, devaddr, devep, data, len);
    }
    return usb_generic_handle_packet(dev, pid, devaddr, devep, data, len);
}

static void usb_hub_handle_destroy(USBDevice *dev)
{
    USBHubState *s = (USBHubState *)dev;

    free(s);
}

USBDevice *usb_hub_init(int nb_ports)
{
    USBHubState *s;
    USBHubPort *port;
    int i;

    if (nb_ports > MAX_PORTS)
        return NULL;
    s = (USBHubState *)qemu_mallocz(sizeof(USBHubState));
    if (!s)
        return NULL;
    s->dev.speed = USB_SPEED_FULL;
    s->dev.handle_packet = usb_hub_handle_packet;

    /* generic USB device init */
    s->dev.handle_reset = usb_hub_handle_reset;
    s->dev.handle_control = usb_hub_handle_control;
    s->dev.handle_data = usb_hub_handle_data;
    s->dev.handle_destroy = usb_hub_handle_destroy;

    strncpy(s->dev.devname, "QEMU USB Hub", sizeof(s->dev.devname));

    s->nb_ports = nb_ports;
    for(i = 0; i < s->nb_ports; i++) {
        port = &s->ports[i];
		port->port.opaque = s;
		port->port.index = i;
		port->port.attach = usb_hub_attach;
        port->wPortStatus = PORT_STAT_POWER;
        port->wPortChange = 0;
    }
    return (USBDevice *)s;
}
