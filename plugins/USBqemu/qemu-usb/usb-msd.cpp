/*
 * USB Mass Storage Device emulation
 *
 * Copyright (c) 2006 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the LGPL.
 */

#include "qemu-common.h"
#include "qemu-option.h"
#include "qemu-config.h"
#include "usb.h"
#include "usb-desc.h"
#include "scsi.h"
#include "console.h"
#include "monitor.h"
#include "sysemu.h"
#include "blockdev.h"

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
    USB_MSDM_DATAOUT, /* Transfer data to device.  */
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
    SCSIRequest *req;
    SCSIBus bus;
    BlockConf conf;
    char *serial;
    SCSIDevice *scsi_dev;
    uint32_t removable;
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

enum {
    STR_MANUFACTURER = 1,
    STR_PRODUCT,
    STR_SERIALNUMBER,
    STR_CONFIG_FULL,
    STR_CONFIG_HIGH,
};

static const USBDescStrings desc_strings = {
    [STR_MANUFACTURER] = "QEMU " QEMU_VERSION,
    [STR_PRODUCT]      = "QEMU USB HARDDRIVE",
    [STR_SERIALNUMBER] = "1",
    [STR_CONFIG_FULL]  = "Full speed config (usb 1.1)",
    [STR_CONFIG_HIGH]  = "High speed config (usb 2.0)",
};

static const USBDescIface desc_iface_full = {
    .bInterfaceNumber              = 0,
    .bNumEndpoints                 = 2,
    .bInterfaceClass               = USB_CLASS_MASS_STORAGE,
    .bInterfaceSubClass            = 0x06, /* SCSI */
    .bInterfaceProtocol            = 0x50, /* Bulk */
    .eps = (USBDescEndpoint[]) {
        {
            .bEndpointAddress      = USB_DIR_IN | 0x01,
            .bmAttributes          = USB_ENDPOINT_XFER_BULK,
            .wMaxPacketSize        = 64,
        },{
            .bEndpointAddress      = USB_DIR_OUT | 0x02,
            .bmAttributes          = USB_ENDPOINT_XFER_BULK,
            .wMaxPacketSize        = 64,
        },
    }
};

static const USBDescDevice desc_device_full = {
    .bcdUSB                        = 0x0200,
    .bMaxPacketSize0               = 8,
    .bNumConfigurations            = 1,
    .confs = (USBDescConfig[]) {
        {
            .bNumInterfaces        = 1,
            .bConfigurationValue   = 1,
            .iConfiguration        = STR_CONFIG_FULL,
            .bmAttributes          = 0xc0,
            .nif = 1,
            .ifs = &desc_iface_full,
        },
    },
};

static const USBDescIface desc_iface_high = {
    .bInterfaceNumber              = 0,
    .bNumEndpoints                 = 2,
    .bInterfaceClass               = USB_CLASS_MASS_STORAGE,
    .bInterfaceSubClass            = 0x06, /* SCSI */
    .bInterfaceProtocol            = 0x50, /* Bulk */
    .eps = (USBDescEndpoint[]) {
        {
            .bEndpointAddress      = USB_DIR_IN | 0x01,
            .bmAttributes          = USB_ENDPOINT_XFER_BULK,
            .wMaxPacketSize        = 512,
        },{
            .bEndpointAddress      = USB_DIR_OUT | 0x02,
            .bmAttributes          = USB_ENDPOINT_XFER_BULK,
            .wMaxPacketSize        = 512,
        },
    }
};

static const USBDescDevice desc_device_high = {
    .bcdUSB                        = 0x0200,
    .bMaxPacketSize0               = 64,
    .bNumConfigurations            = 1,
    .confs = (USBDescConfig[]) {
        {
            .bNumInterfaces        = 1,
            .bConfigurationValue   = 1,
            .iConfiguration        = STR_CONFIG_HIGH,
            .bmAttributes          = 0xc0,
            .nif = 1,
            .ifs = &desc_iface_high,
        },
    },
};

static const USBDesc desc = {
    .id = {
        .idVendor          = 0,
        .idProduct         = 0,
        .bcdDevice         = 0,
        .iManufacturer     = STR_MANUFACTURER,
        .iProduct          = STR_PRODUCT,
        .iSerialNumber     = STR_SERIALNUMBER,
    },
    .full = &desc_device_full,
    .high = &desc_device_high,
    .str  = desc_strings,
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
    if (s->scsi_len == 0 || s->data_len == 0) {
        scsi_req_continue(s->req);
    }
}

static void usb_msd_send_status(MSDState *s, USBPacket *p)
{
    struct usb_msd_csw csw;
    int len;

    csw.sig = cpu_to_le32(0x53425355);
    csw.tag = cpu_to_le32(s->tag);
    csw.residue = s->residue;
    csw.status = s->result;

    len = MIN(sizeof(csw), p->len);
    memcpy(p->data, &csw, len);
}

static void usb_msd_transfer_data(SCSIRequest *req, uint32_t len)
{
    MSDState *s = DO_UPCAST(MSDState, dev.qdev, req->bus->qbus.parent);
    USBPacket *p = s->packet;

    assert((s->mode == USB_MSDM_DATAOUT) == (req->cmd.mode == SCSI_XFER_TO_DEV));
    s->scsi_len = len;
    s->scsi_buf = scsi_req_get_buf(req);
    if (p) {
        usb_msd_copy_data(s);
        if (s->packet && s->usb_len == 0) {
            /* Set s->packet to NULL before calling usb_packet_complete
               because another request may be issued before
               usb_packet_complete returns.  */
            DPRINTF("Packet complete %p\n", p);
            s->packet = NULL;
            usb_packet_complete(&s->dev, p);
        }
    }
}

static void usb_msd_command_complete(SCSIRequest *req, uint32_t status)
{
    MSDState *s = DO_UPCAST(MSDState, dev.qdev, req->bus->qbus.parent);
    USBPacket *p = s->packet;

    DPRINTF("Command complete %d\n", status);
    s->residue = s->data_len;
    s->result = status != 0;
    if (s->packet) {
        if (s->data_len == 0 && s->mode == USB_MSDM_DATAOUT) {
            /* A deferred packet with no write data remaining must be
               the status read packet.  */
            usb_msd_send_status(s, p);
            s->mode = USB_MSDM_CBW;
        } else {
            if (s->data_len) {
                s->data_len -= s->usb_len;
                if (s->mode == USB_MSDM_DATAIN) {
                    memset(s->usb_buf, 0, s->usb_len);
                }
                s->usb_len = 0;
            }
            if (s->data_len == 0) {
                s->mode = USB_MSDM_CSW;
            }
        }
        s->packet = NULL;
        usb_packet_complete(&s->dev, p);
    } else if (s->data_len == 0) {
        s->mode = USB_MSDM_CSW;
    }
    scsi_req_unref(req);
    s->req = NULL;
}

static void usb_msd_request_cancelled(SCSIRequest *req)
{
    MSDState *s = DO_UPCAST(MSDState, dev.qdev, req->bus->qbus.parent);

    if (req == s->req) {
        scsi_req_unref(s->req);
        s->req = NULL;
        s->packet = NULL;
        s->scsi_len = 0;
    }
}

static void usb_msd_handle_reset(USBDevice *dev)
{
    MSDState *s = (MSDState *)dev;

    DPRINTF("Reset\n");
    s->mode = USB_MSDM_CBW;
}

static int usb_msd_handle_control(USBDevice *dev, USBPacket *p,
               int request, int value, int index, int length, uint8_t *data)
{
    MSDState *s = (MSDState *)dev;
    int ret;

    ret = usb_desc_handle_control(dev, p, request, value, index, length, data);
    if (ret >= 0) {
        return ret;
    }

    ret = 0;
    switch (request) {
    case DeviceRequest | USB_REQ_GET_INTERFACE:
        data[0] = 0;
        ret = 1;
        break;
    case DeviceOutRequest | USB_REQ_SET_INTERFACE:
        ret = 0;
        break;
    case EndpointOutRequest | USB_REQ_CLEAR_FEATURE:
        ret = 0;
        break;
    case InterfaceOutRequest | USB_REQ_SET_INTERFACE:
        ret = 0;
        break;
        /* Class specific requests.  */
    case ClassInterfaceOutRequest | MassStorageReset:
        /* Reset state ready for the next CBW.  */
        s->mode = USB_MSDM_CBW;
        ret = 0;
        break;
    case ClassInterfaceRequest | GetMaxLun:
        data[0] = 0;
        ret = 1;
        break;
    default:
        ret = USB_RET_STALL;
        break;
    }
    return ret;
}

static void usb_msd_cancel_io(USBDevice *dev, USBPacket *p)
{
    MSDState *s = DO_UPCAST(MSDState, dev, dev);
    scsi_req_cancel(s->req);
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
            s->scsi_len = 0;
            s->req = scsi_req_new(s->scsi_dev, s->tag, 0, NULL);
            scsi_req_enqueue(s->req, cbw.cmd);
            /* ??? Should check that USB and SCSI data transfer
               directions match.  */
            if (s->mode != USB_MSDM_CSW && s->residue == 0) {
                scsi_req_continue(s->req);
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
            s->packet = p;
            ret = USB_RET_ASYNC;
            break;

        case USB_MSDM_CSW:
            DPRINTF("Command status %d tag 0x%x, len %d\n",
                    s->result, s->tag, len);
            if (len < 13)
                goto fail;

            usb_msd_send_status(s, p);
            s->mode = USB_MSDM_CBW;
            ret = 13;
            break;

        case USB_MSDM_DATAIN:
            DPRINTF("Data in %d/%d, scsi_len %d\n", len, s->data_len, s->scsi_len);
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
        err = usb_device_attach(&s->dev);

    if (err)
        qdev_unplug(&s->dev.qdev);
}

static const struct SCSIBusOps usb_msd_scsi_ops = {
    .transfer_data = usb_msd_transfer_data,
    .complete = usb_msd_command_complete,
    .cancel = usb_msd_request_cancelled
};

static int usb_msd_initfn(USBDevice *dev)
{
    MSDState *s = DO_UPCAST(MSDState, dev, dev);
    BlockDriverState *bs = s->conf.bs;
    DriveInfo *dinfo;

    if (!bs) {
        error_report("usb-msd: drive property not set");
        return -1;
    }

    /*
     * Hack alert: this pretends to be a block device, but it's really
     * a SCSI bus that can serve only a single device, which it
     * creates automatically.  But first it needs to detach from its
     * blockdev, or else scsi_bus_legacy_add_drive() dies when it
     * attaches again.
     *
     * The hack is probably a bad idea.
     */
    bdrv_detach(bs, &s->dev.qdev);
    s->conf.bs = NULL;

    if (!s->serial) {
        /* try to fall back to value set with legacy -drive serial=... */
        dinfo = drive_get_by_blockdev(bs);
        if (*dinfo->serial) {
            s->serial = strdup(dinfo->serial);
        }
    }
    if (s->serial) {
        usb_desc_set_string(dev, STR_SERIALNUMBER, s->serial);
    }

    usb_desc_init(dev);
    scsi_bus_new(&s->bus, &s->dev.qdev, 0, 1, &usb_msd_scsi_ops);
    s->scsi_dev = scsi_bus_legacy_add_drive(&s->bus, bs, 0, !!s->removable);
    if (!s->scsi_dev) {
        return -1;
    }
    s->bus.qbus.allow_hotplug = 0;
    usb_msd_handle_reset(dev);

    if (bdrv_key_required(bs)) {
        if (cur_mon) {
            monitor_read_bdrv_key_start(cur_mon, bs, usb_msd_password_cb, s);
            s->dev.auto_attach = 0;
        } else {
            autostart = 0;
        }
    }

    add_boot_device_path(s->conf.bootindex, &dev->qdev, "/disk@0,0");
    return 0;
}

static USBDevice *usb_msd_init(const char *filename)
{
    static int nr=0;
    char id[8];
    QemuOpts *opts;
    DriveInfo *dinfo;
    USBDevice *dev;
    const char *p1;
    char fmt[32];

    /* parse -usbdevice disk: syntax into drive opts */
    snprintf(id, sizeof(id), "usb%d", nr++);
    opts = qemu_opts_create(qemu_find_opts("drive"), id, 0);

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
    dinfo = drive_init(opts, 0);
    if (!dinfo) {
        qemu_opts_del(opts);
        return NULL;
    }

    /* create guest device */
    dev = usb_create(NULL /* FIXME */, "usb-storage");
    if (!dev) {
        return NULL;
    }
    if (qdev_prop_set_drive(&dev->qdev, "drive", dinfo->bdrv) < 0) {
        qdev_free(&dev->qdev);
        return NULL;
    }
    if (qdev_init(&dev->qdev) < 0)
        return NULL;

    return dev;
}

static struct USBDeviceInfo msd_info = {
    .product_desc   = "QEMU USB MSD",
    .qdev.name      = "usb-storage",
    .qdev.fw_name      = "storage",
    .qdev.size      = sizeof(MSDState),
    .usb_desc       = &desc,
    .init           = usb_msd_initfn,
    .handle_packet  = usb_generic_handle_packet,
    .cancel_packet  = usb_msd_cancel_io,
    .handle_attach  = usb_desc_attach,
    .handle_reset   = usb_msd_handle_reset,
    .handle_control = usb_msd_handle_control,
    .handle_data    = usb_msd_handle_data,
    .usbdevice_name = "disk",
    .usbdevice_init = usb_msd_init,
    .qdev.props     = (Property[]) {
        DEFINE_BLOCK_PROPERTIES(MSDState, conf),
        DEFINE_PROP_STRING("serial", MSDState, serial),
        DEFINE_PROP_BIT("removable", MSDState, removable, 0, false),
        DEFINE_PROP_END_OF_LIST(),
    },
};

static void usb_msd_register_devices(void)
{
    usb_qdev_register(&msd_info);
}
device_init(usb_msd_register_devices)
