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
#include "USBinternal.h"

//#define DEBUG

#define NUM_PORTS 8

typedef struct USBHubPort {
    USBPort port;
    uint16_t wPortStatus;
    uint16_t wPortChange;
} USBHubPort;

typedef struct USBHubState {
    USBDevice dev;
    USBHubPort ports[NUM_PORTS];
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

enum {
    STR_MANUFACTURER = 1,
    STR_PRODUCT,
    STR_SERIALNUMBER,
};

static const USBDescStrings desc_strings = {
    [STR_MANUFACTURER] = "QEMU " QEMU_VERSION,
    [STR_PRODUCT]      = "QEMU USB Hub",
    [STR_SERIALNUMBER] = "314159",
};

static const USBDescIface desc_iface_hub = {
    .bInterfaceNumber              = 0,
    .bNumEndpoints                 = 1,
    .bInterfaceClass               = USB_CLASS_HUB,
    .eps = (USBDescEndpoint[]) {
        {
            .bEndpointAddress      = USB_DIR_IN | 0x01,
            .bmAttributes          = USB_ENDPOINT_XFER_INT,
            .wMaxPacketSize        = 1 + (NUM_PORTS + 7) / 8,
            .bInterval             = 0xff,
        },
    }
};

static const USBDescDevice desc_device_hub = {
    .bcdUSB                        = 0x0110,
    .bDeviceClass                  = USB_CLASS_HUB,
    .bMaxPacketSize0               = 8,
    .bNumConfigurations            = 1,
    .confs = (USBDescConfig[]) {
        {
            .bNumInterfaces        = 1,
            .bConfigurationValue   = 1,
            .bmAttributes          = 0xe0,
            .nif = 1,
            .ifs = &desc_iface_hub,
        },
    },
};

static const USBDesc desc_hub = {
    .id = {
        .idVendor          = 0,
        .idProduct         = 0,
        .bcdDevice         = 0x0101,
        .iManufacturer     = STR_MANUFACTURER,
        .iProduct          = STR_PRODUCT,
        .iSerialNumber     = STR_SERIALNUMBER,
    },
    .full = &desc_device_hub,
    .str  = desc_strings,
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

static void usb_hub_attach(USBPort *port1)
{
    USBHubState *s = port1->opaque;
    USBHubPort *port = &s->ports[port1->index];

    port->wPortStatus |= PORT_STAT_CONNECTION;
    port->wPortChange |= PORT_STAT_C_CONNECTION;
    if (port->port.dev->speed == USB_SPEED_LOW) {
        port->wPortStatus |= PORT_STAT_LOW_SPEED;
    } else {
        port->wPortStatus &= ~PORT_STAT_LOW_SPEED;
    }
}

static void usb_hub_detach(USBPort *port1)
{
    USBHubState *s = port1->opaque;
    USBHubPort *port = &s->ports[port1->index];

    /* Let upstream know the device on this port is gone */
    s->dev.port->ops->child_detach(s->dev.port, port1->dev);

    port->wPortStatus &= ~PORT_STAT_CONNECTION;
    port->wPortChange |= PORT_STAT_C_CONNECTION;
    if (port->wPortStatus & PORT_STAT_ENABLE) {
        port->wPortStatus &= ~PORT_STAT_ENABLE;
        port->wPortChange |= PORT_STAT_C_ENABLE;
    }
}

static void usb_hub_child_detach(USBPort *port1, USBDevice *child)
{
    USBHubState *s = port1->opaque;

    /* Pass along upstream */
    s->dev.port->ops->child_detach(s->dev.port, child);
}

static void usb_hub_wakeup(USBPort *port1)
{
    USBHubState *s = port1->opaque;
    USBHubPort *port = &s->ports[port1->index];

    if (port->wPortStatus & PORT_STAT_SUSPEND) {
        port->wPortChange |= PORT_STAT_C_SUSPEND;
        usb_wakeup(&s->dev);
    }
}

static void usb_hub_complete(USBPort *port, USBPacket *packet)
{
    USBHubState *s = port->opaque;

    /*
     * Just pass it along upstream for now.
     *
     * If we ever inplement usb 2.0 split transactions this will
     * become a little more complicated ...
     */
    usb_packet_complete(&s->dev, packet);
}

static void usb_hub_handle_attach(USBDevice *dev)
{
    USBHubState *s = DO_UPCAST(USBHubState, dev, dev);
    int i;

    for (i = 0; i < NUM_PORTS; i++) {
        usb_port_location(&s->ports[i].port, dev->port, i+1);
    }
}

static void usb_hub_handle_reset(USBDevice *dev)
{
    /* XXX: do it */
}

static int usb_hub_handle_control(USBDevice *dev, USBPacket *p,
               int request, int value, int index, int length, uint8_t *data)
{
    USBHubState *s = (USBHubState *)dev;
    int ret;

    ret = usb_desc_handle_control(dev, p, request, value, index, length, data);
    if (ret >= 0) {
        return ret;
    }

    switch(request) {
    case EndpointOutRequest | USB_REQ_CLEAR_FEATURE:
        if (value == 0 && index != 0x81) { /* clear ep halt */
            goto fail;
        }
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
            if (n >= NUM_PORTS) {
                goto fail;
            }
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
            if (n >= NUM_PORTS) {
                goto fail;
            }
            port = &s->ports[n];
            dev = port->port.dev;
            switch(value) {
            case PORT_SUSPEND:
                port->wPortStatus |= PORT_STAT_SUSPEND;
                break;
            case PORT_RESET:
                if (dev) {
                    usb_send_msg(dev, USB_MSG_RESET);
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

            if (n >= NUM_PORTS) {
                goto fail;
            }
            port = &s->ports[n];
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
            data[2] = NUM_PORTS;

            /* fill DeviceRemovable bits */
            limit = ((NUM_PORTS + 1 + 7) / 8) + 7;
            for (n = 7; n < limit; n++) {
                data[n] = 0x00;
                var_hub_size++;
            }

            /* fill PortPwrCtrlMask bits */
            limit = limit + ((NUM_PORTS + 7) / 8);
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

static int usb_hub_handle_data(USBDevice *dev, USBPacket *p)
{
    USBHubState *s = (USBHubState *)dev;
    int ret;

    switch(p->pid) {
    case USB_TOKEN_IN:
        if (p->devep == 1) {
            USBHubPort *port;
            unsigned int status;
            int i, n;
            n = (NUM_PORTS + 1 + 7) / 8;
            if (p->len == 1) { /* FreeBSD workaround */
                n = 1;
            } else if (n > p->len) {
                return USB_RET_BABBLE;
            }
            status = 0;
            for(i = 0; i < NUM_PORTS; i++) {
                port = &s->ports[i];
                if (port->wPortChange)
                    status |= (1 << (i + 1));
            }
            if (status != 0) {
                for(i = 0; i < n; i++) {
                    p->data[i] = status >> (8 * i);
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

static int usb_hub_broadcast_packet(USBHubState *s, USBPacket *p)
{
    USBHubPort *port;
    USBDevice *dev;
    int i, ret;

    for(i = 0; i < NUM_PORTS; i++) {
        port = &s->ports[i];
        dev = port->port.dev;
        if (dev && (port->wPortStatus & PORT_STAT_ENABLE)) {
            ret = usb_handle_packet(dev, p);
            if (ret != USB_RET_NODEV) {
                return ret;
            }
        }
    }
    return USB_RET_NODEV;
}

static int usb_hub_handle_packet(USBDevice *dev, USBPacket *p)
{
    USBHubState *s = (USBHubState *)dev;

#if defined(DEBUG) && 0
    printf("usb_hub: pid=0x%x\n", pid);
#endif
    if (dev->state == USB_STATE_DEFAULT &&
        dev->addr != 0 &&
        p->devaddr != dev->addr &&
        (p->pid == USB_TOKEN_SETUP ||
         p->pid == USB_TOKEN_OUT ||
         p->pid == USB_TOKEN_IN)) {
        /* broadcast the packet to the devices */
        return usb_hub_broadcast_packet(s, p);
    }
    return usb_generic_handle_packet(dev, p);
}

static void usb_hub_handle_destroy(USBDevice *dev)
{
    USBHubState *s = (USBHubState *)dev;
    int i;

    for (i = 0; i < NUM_PORTS; i++) {
        usb_unregister_port(usb_bus_from_device(dev),
                            &s->ports[i].port);
    }
}

static USBPortOps usb_hub_port_ops = {
    .attach = usb_hub_attach,
    .detach = usb_hub_detach,
    .child_detach = usb_hub_child_detach,
    .wakeup = usb_hub_wakeup,
    .complete = usb_hub_complete,
};

static int usb_hub_initfn(USBDevice *dev)
{
    USBHubState *s = DO_UPCAST(USBHubState, dev, dev);
    USBHubPort *port;
    int i;

    usb_desc_init(dev);
    for (i = 0; i < NUM_PORTS; i++) {
        port = &s->ports[i];
        usb_register_port(usb_bus_from_device(dev),
                          &port->port, s, i, &usb_hub_port_ops,
                          USB_SPEED_MASK_LOW | USB_SPEED_MASK_FULL);
        port->wPortStatus = PORT_STAT_POWER;
        port->wPortChange = 0;
    }
    return 0;
}

static const VMStateDescription vmstate_usb_hub_port = {
    .name = "usb-hub-port",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField []) {
        VMSTATE_UINT16(wPortStatus, USBHubPort),
        VMSTATE_UINT16(wPortChange, USBHubPort),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription vmstate_usb_hub = {
    .name = "usb-hub",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField []) {
        VMSTATE_USB_DEVICE(dev, USBHubState),
        VMSTATE_STRUCT_ARRAY(ports, USBHubState, NUM_PORTS, 0,
                             vmstate_usb_hub_port, USBHubPort),
        VMSTATE_END_OF_LIST()
    }
};

static struct USBDeviceInfo hub_info = {
    .product_desc   = "QEMU USB Hub",
    .qdev.name      = "usb-hub",
    .qdev.fw_name    = "hub",
    .qdev.size      = sizeof(USBHubState),
    .qdev.vmsd      = &vmstate_usb_hub,
    .usb_desc       = &desc_hub,
    .init           = usb_hub_initfn,
    .handle_packet  = usb_hub_handle_packet,
    .handle_attach  = usb_hub_handle_attach,
    .handle_reset   = usb_hub_handle_reset,
    .handle_control = usb_hub_handle_control,
    .handle_data    = usb_hub_handle_data,
    .handle_destroy = usb_hub_handle_destroy,
};

static void usb_hub_register_devices(void)
{
    usb_qdev_register(&hub_info);
}
device_init(usb_hub_register_devices)
