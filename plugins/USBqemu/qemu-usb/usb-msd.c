/*
 * USB Mass Storage Device emulation
 *
 * Copyright (c) 2006 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licenced under the LGPL.
 */

#include "qemu-common.h"
#include "qemu-option.h"
#include "qemu-config.h"
#include "usb.h"
#include "block.h"
#include "scsi.h"
#include "console.h"
#include "monitor.h"

//#define DEBUG_MSD

#ifdef DEBUG_MSD
#define DPRINTF(fmt, ...) \
do { printf("usb-msd: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#endif

/* USB requests.  */
#define MassStorageReset  0xff
#define GetMaxLun         0xfe

enum USBMSDMode {
    USB_MSDM_CBW, /* Command Block.  */
    USB_MSDM_DATAOUT, /* Tranfer data to device.  */
    USB_MSDM_DATAIN, /* Transfer data from device.  */
    USB_MSDM_CSW /* Command Status.  */
};

typedef struct {
    USBDevice dev;
    enum USBMSDMode mode;
    uint32_t scsi_len;
    uint8_t *scsi_buf;
    uint32_t usb_len;
    uint8_t *usb_buf;
    uint32_t data_len;
    uint32_t residue;
    uint32_t tag;
    SCSIBus bus;
    DriveInfo *dinfo;
    SCSIDevice *scsi_dev;
    int result;
    /* For async completion.  */
    USBPacket *packet;
} MSDState;

struct usb_msd_cbw {
    uint32_t sig;
    uint32_t tag;
    uint32_t data_len;
    uint8_t flags;
    uint8_t lun;
    uint8_t cmd_len;
    uint8_t cmd[16];
};

struct usb_msd_csw {
    uint32_t sig;
    uint32_t tag;
    uint32_t residue;
    uint8_t status;
};

static const uint8_t qemu_msd_dev_descriptor[] = {
	0x12,       /*  u8 bLength; */
	0x01,       /*  u8 bDescriptorType; Device */
	0x00, 0x01, /*  u16 bcdUSB; v1.0 */

	0x00,	    /*  u8  bDeviceClass; */
	0x00,	    /*  u8  bDeviceSubClass; */
	0x00,       /*  u8  bDeviceProtocol; [ low/full speeds only ] */
	0x08,       /*  u8  bMaxPacketSize0; 8 Bytes */

        /* Vendor and product id are arbitrary.  */
	0x00, 0x00, /*  u16 idVendor; */
 	0x00, 0x00, /*  u16 idProduct; */
	0x00, 0x00, /*  u16 bcdDevice */

	0x01,       /*  u8  iManufacturer; */
	0x02,       /*  u8  iProduct; */
	0x03,       /*  u8  iSerialNumber; */
	0x01        /*  u8  bNumConfigurations; */
};

static const uint8_t qemu_msd_config_descriptor[] = {

	/* one configuration */
	0x09,       /*  u8  bLength; */
	0x02,       /*  u8  bDescriptorType; Configuration */
	0x20, 0x00, /*  u16 wTotalLength; */
	0x01,       /*  u8  bNumInterfaces; (1) */
	0x01,       /*  u8  bConfigurationValue; */
	0x00,       /*  u8  iConfiguration; */
	0xc0,       /*  u8  bmAttributes;
				 Bit 7: must be set,
				     6: Self-powered,
				     5: Remote wakeup,
				     4..0: resvd */
	0x00,       /*  u8  MaxPower; */

	/* one interface */
	0x09,       /*  u8  if_bLength; */
	0x04,       /*  u8  if_bDescriptorType; Interface */
	0x00,       /*  u8  if_bInterfaceNumber; */
	0x00,       /*  u8  if_bAlternateSetting; */
	0x02,       /*  u8  if_bNumEndpoints; */
	0x08,       /*  u8  if_bInterfaceClass; MASS STORAGE */
	0x06,       /*  u8  if_bInterfaceSubClass; SCSI */
	0x50,       /*  u8  if_bInterfaceProtocol; Bulk Only */
	0x00,       /*  u8  if_iInterface; */

	/* Bulk-In endpoint */
	0x07,       /*  u8  ep_bLength; */
	0x05,       /*  u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x02,       /*  u8  ep_bmAttributes; Bulk */
 	0x40, 0x00, /*  u16 ep_wMaxPacketSize; */
	0x00,       /*  u8  ep_bInterval; */

	/* Bulk-Out endpoint */
	0x07,       /*  u8  ep_bLength; */
	0x05,       /*  u8  ep_bDescriptorType; Endpoint */
	0x02,       /*  u8  ep_bEndpointAddress; OUT Endpoint 2 */
 	0x02,       /*  u8  ep_bmAttributes; Bulk */
 	0x40, 0x00, /*  u16 ep_wMaxPacketSize; */
	0x00        /*  u8  ep_bInterval; */
};

static void usb_msd_copy_data(MSDState *s)
{
    uint32_t len;
    len = s->usb_len;
    if (len > s->scsi_len)
        len = s->scsi_len;
    if (s->mode == USB_MSDM_DATAIN) {
        memcpy(s->usb_buf, s->scsi_buf, len);
    } else {
        memcpy(s->scsi_buf, s->usb_buf, len);
    }
    s->usb_len -= len;
    s->scsi_len -= len;
    s->usb_buf += len;
    s->scsi_buf += len;
    s->data_len -= len;
    if (s->scsi_len == 0) {
        if (s->mode == USB_MSDM_DATAIN) {
            s->scsi_dev->info->read_data(s->scsi_dev, s->tag);
        } else if (s->mode == USB_MSDM_DATAOUT) {
            s->scsi_dev->info->write_data(s->scsi_dev, s->tag);
        }
    }
}

static void usb_msd_send_status(MSDState *s)
{
    struct usb_msd_csw csw;

    csw.sig = cpu_to_le32(0x53425355);
    csw.tag = cpu_to_le32(s->tag);
    csw.residue = s->residue;
    csw.status = s->result;
    memcpy(s->usb_buf, &csw, 13);
}

static void usb_msd_command_complete(SCSIBus *bus, int reason, uint32_t tag,
                                     uint32_t arg)
{
    MSDState *s = DO_UPCAST(MSDState, dev.qdev, bus->qbus.parent);
    USBPacket *p = s->packet;

    if (tag != s->tag) {
        fprintf(stderr, "usb-msd: Unexpected SCSI Tag 0x%x\n", tag);
    }
    if (reason == SCSI_REASON_DONE) {
        DPRINTF("Command complete %d\n", arg);
        s->residue = s->data_len;
        s->result = arg != 0;
        if (s->packet) {
            if (s->data_len == 0 && s->mode == USB_MSDM_DATAOUT) {
                /* A deferred packet with no write data remaining must be
                   the status read packet.  */
                usb_msd_send_status(s);
                s->mode = USB_MSDM_CBW;
            } else {
                if (s->data_len) {
                    s->data_len -= s->usb_len;
                    if (s->mode == USB_MSDM_DATAIN)
                        memset(s->usb_buf, 0, s->usb_len);
                    s->usb_len = 0;
                }
                if (s->data_len == 0)
                    s->mode = USB_MSDM_CSW;
            }
            s->packet = NULL;
            usb_packet_complete(p);
        } else if (s->data_len == 0) {
            s->mode = USB_MSDM_CSW;
        }
        return;
    }
    s->scsi_len = arg;
    s->scsi_buf = s->scsi_dev->info->get_buf(s->scsi_dev, tag);
    if (p) {
        usb_msd_copy_data(s);
        if (s->usb_len == 0) {
            /* Set s->packet to NULL before calling usb_packet_complete
               because annother request may be issued before
               usb_packet_complete returns.  */
            DPRINTF("Packet complete %p\n", p);
            s->packet = NULL;
            usb_packet_complete(p);
        }
    }
}

static void usb_msd_handle_reset(USBDevice *dev)
{
    MSDState *s = (MSDState *)dev;

    DPRINTF("Reset\n");
    s->mode = USB_MSDM_CBW;
}

static int usb_msd_handle_control(USBDevice *dev, int request, int value,
                                  int index, int length, uint8_t *data)
{
    MSDState *s = (MSDState *)dev;
    int ret = 0;

    switch (request) {
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
            memcpy(data, qemu_msd_dev_descriptor,
                   sizeof(qemu_msd_dev_descriptor));
            ret = sizeof(qemu_msd_dev_descriptor);
            break;
        case USB_DT_CONFIG:
            memcpy(data, qemu_msd_config_descriptor,
                   sizeof(qemu_msd_config_descriptor));
            ret = sizeof(qemu_msd_config_descriptor);
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
                /* vendor description */
                ret = set_usb_string(data, "QEMU " QEMU_VERSION);
                break;
            case 2:
                /* product description */
                ret = set_usb_string(data, "QEMU USB HARDDRIVE");
                break;
            case 3:
                /* serial number */
                ret = set_usb_string(data, "1");
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
    case EndpointOutRequest | USB_REQ_CLEAR_FEATURE:
        if (value == 0 && index != 0x81) { /* clear ep halt */
            goto fail;
        }
        ret = 0;
        break;
        /* Class specific requests.  */
    case MassStorageReset:
        /* Reset state ready for the next CBW.  */
        s->mode = USB_MSDM_CBW;
        ret = 0;
        break;
    case GetMaxLun:
        data[0] = 0;
        ret = 1;
        break;
    default:
    fail:
        ret = USB_RET_STALL;
        break;
    }
    return ret;
}

static void usb_msd_cancel_io(USBPacket *p, void *opaque)
{
    MSDState *s = opaque;
    s->scsi_dev->info->cancel_io(s->scsi_dev, s->tag);
    s->packet = NULL;
    s->scsi_len = 0;
}

static int usb_msd_handle_data(USBDevice *dev, USBPacket *p)
{
    MSDState *s = (MSDState *)dev;
    int ret = 0;
    struct usb_msd_cbw cbw;
    uint8_t devep = p->devep;
    uint8_t *data = p->data;
    int len = p->len;

    switch (p->pid) {
    case USB_TOKEN_OUT:
        if (devep != 2)
            goto fail;

        switch (s->mode) {
        case USB_MSDM_CBW:
            if (len != 31) {
                fprintf(stderr, "usb-msd: Bad CBW size");
                goto fail;
            }
            memcpy(&cbw, data, 31);
            if (le32_to_cpu(cbw.sig) != 0x43425355) {
                fprintf(stderr, "usb-msd: Bad signature %08x\n",
                        le32_to_cpu(cbw.sig));
                goto fail;
            }
            DPRINTF("Command on LUN %d\n", cbw.lun);
            if (cbw.lun != 0) {
                fprintf(stderr, "usb-msd: Bad LUN %d\n", cbw.lun);
                goto fail;
            }
            s->tag = le32_to_cpu(cbw.tag);
            s->data_len = le32_to_cpu(cbw.data_len);
            if (s->data_len == 0) {
                s->mode = USB_MSDM_CSW;
            } else if (cbw.flags & 0x80) {
                s->mode = USB_MSDM_DATAIN;
            } else {
                s->mode = USB_MSDM_DATAOUT;
            }
            DPRINTF("Command tag 0x%x flags %08x len %d data %d\n",
                    s->tag, cbw.flags, cbw.cmd_len, s->data_len);
            s->residue = 0;
            s->scsi_dev->info->send_command(s->scsi_dev, s->tag, cbw.cmd, 0);
            /* ??? Should check that USB and SCSI data transfer
               directions match.  */
            if (s->residue == 0) {
                if (s->mode == USB_MSDM_DATAIN) {
                    s->scsi_dev->info->read_data(s->scsi_dev, s->tag);
                } else if (s->mode == USB_MSDM_DATAOUT) {
                    s->scsi_dev->info->write_data(s->scsi_dev, s->tag);
                }
            }
            ret = len;
            break;

        case USB_MSDM_DATAOUT:
            DPRINTF("Data out %d/%d\n", len, s->data_len);
            if (len > s->data_len)
                goto fail;

            s->usb_buf = data;
            s->usb_len = len;
            if (s->scsi_len) {
                usb_msd_copy_data(s);
            }
            if (s->residue && s->usb_len) {
                s->data_len -= s->usb_len;
                if (s->data_len == 0)
                    s->mode = USB_MSDM_CSW;
                s->usb_len = 0;
            }
            if (s->usb_len) {
                DPRINTF("Deferring packet %p\n", p);
                usb_defer_packet(p, usb_msd_cancel_io, s);
                s->packet = p;
                ret = USB_RET_ASYNC;
            } else {
                ret = len;
            }
            break;

        default:
            DPRINTF("Unexpected write (len %d)\n", len);
            goto fail;
        }
        break;

    case USB_TOKEN_IN:
        if (devep != 1)
            goto fail;

        switch (s->mode) {
        case USB_MSDM_DATAOUT:
            if (s->data_len != 0 || len < 13)
                goto fail;
            /* Waiting for SCSI write to complete.  */
            usb_defer_packet(p, usb_msd_cancel_io, s);
            s->packet = p;
            ret = USB_RET_ASYNC;
            break;

        case USB_MSDM_CSW:
            DPRINTF("Command status %d tag 0x%x, len %d\n",
                    s->result, s->tag, len);
            if (len < 13)
                goto fail;

            s->usb_len = len;
            s->usb_buf = data;
            usb_msd_send_status(s);
            s->mode = USB_MSDM_CBW;
            ret = 13;
            break;

        case USB_MSDM_DATAIN:
            DPRINTF("Data in %d/%d\n", len, s->data_len);
            if (len > s->data_len)
                len = s->data_len;
            s->usb_buf = data;
            s->usb_len = len;
            if (s->scsi_len) {
                usb_msd_copy_data(s);
            }
            if (s->residue && s->usb_len) {
                s->data_len -= s->usb_len;
                memset(s->usb_buf, 0, s->usb_len);
                if (s->data_len == 0)
                    s->mode = USB_MSDM_CSW;
                s->usb_len = 0;
            }
            if (s->usb_len) {
                DPRINTF("Deferring packet %p\n", p);
                usb_defer_packet(p, usb_msd_cancel_io, s);
                s->packet = p;
                ret = USB_RET_ASYNC;
            } else {
                ret = len;
            }
            break;

        default:
            DPRINTF("Unexpected read (len %d)\n", len);
            goto fail;
        }
        break;

    default:
        DPRINTF("Bad token\n");
    fail:
        ret = USB_RET_STALL;
        break;
    }

    return ret;
}

static void usb_msd_password_cb(void *opaque, int err)
{
    MSDState *s = opaque;

    if (!err)
        usb_device_attach(&s->dev);
    else
        qdev_unplug(&s->dev.qdev);
}

static int usb_msd_initfn(USBDevice *dev)
{
    MSDState *s = DO_UPCAST(MSDState, dev, dev);

    if (!s->dinfo || !s->dinfo->bdrv) {
        qemu_error("usb-msd: drive property not set\n");
        return -1;
    }

    s->dev.speed = USB_SPEED_FULL;
    scsi_bus_new(&s->bus, &s->dev.qdev, 0, 1, usb_msd_command_complete);
    s->scsi_dev = scsi_bus_legacy_add_drive(&s->bus, s->dinfo, 0);
    s->bus.qbus.allow_hotplug = 0;
    usb_msd_handle_reset(dev);

    if (bdrv_key_required(s->dinfo->bdrv)) {
        if (s->dev.qdev.hotplugged) {
            monitor_read_bdrv_key_start(cur_mon, s->dinfo->bdrv,
                                        usb_msd_password_cb, s);
            s->dev.auto_attach = 0;
        } else {
            autostart = 0;
        }
    }

    return 0;
}

static USBDevice *usb_msd_init(const char *filename)
{
    static int nr=0;
    char id[8];
    QemuOpts *opts;
    DriveInfo *dinfo;
    USBDevice *dev;
    int fatal_error;
    const char *p1;
    char fmt[32];

    /* parse -usbdevice disk: syntax into drive opts */
    snprintf(id, sizeof(id), "usb%d", nr++);
    opts = qemu_opts_create(&qemu_drive_opts, id, 0);

    p1 = strchr(filename, ':');
    if (p1++) {
        const char *p2;

        if (strstart(filename, "format=", &p2)) {
            int len = MIN(p1 - p2, sizeof(fmt));
            pstrcpy(fmt, len, p2);
            qemu_opt_set(opts, "format", fmt);
        } else if (*filename != ':') {
            printf("unrecognized USB mass-storage option %s\n", filename);
            return NULL;
        }
        filename = p1;
    }
    if (!*filename) {
        printf("block device specification needed\n");
        return NULL;
    }
    qemu_opt_set(opts, "file", filename);
    qemu_opt_set(opts, "if", "none");

    /* create host drive */
    dinfo = drive_init(opts, NULL, &fatal_error);
    if (!dinfo) {
        qemu_opts_del(opts);
        return NULL;
    }

    /* create guest device */
    dev = usb_create(NULL /* FIXME */, "usb-storage");
    if (!dev) {
        return NULL;
    }
    qdev_prop_set_drive(&dev->qdev, "drive", dinfo);
    if (qdev_init(&dev->qdev) < 0)
        return NULL;

    return dev;
}

static struct USBDeviceInfo msd_info = {
    .product_desc   = "QEMU USB MSD",
    .qdev.name      = "usb-storage",
    .qdev.size      = sizeof(MSDState),
    .init           = usb_msd_initfn,
    .handle_packet  = usb_generic_handle_packet,
    .handle_reset   = usb_msd_handle_reset,
    .handle_control = usb_msd_handle_control,
    .handle_data    = usb_msd_handle_data,
    .usbdevice_name = "disk",
    .usbdevice_init = usb_msd_init,
    .qdev.props     = (Property[]) {
        DEFINE_PROP_DRIVE("drive", MSDState, dinfo),
        DEFINE_PROP_END_OF_LIST(),
    },
};

static void usb_msd_register_devices(void)
{
    usb_qdev_register(&msd_info);
}
device_init(usb_msd_register_devices)
