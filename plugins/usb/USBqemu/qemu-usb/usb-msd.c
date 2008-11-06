/* 
 * USB Mass Storage Device emulation
 *
 * Copyright (c) 2006 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licenced under the LGPL.
 */

#include "vl.h"

//#define DEBUG_MSD

#ifdef DEBUG_MSD
#define DPRINTF(fmt, args...) \
do { printf("usb-msd: " fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
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
    uint32_t data_len;
    uint32_t tag;
    SCSIDevice *scsi_dev;
    int result;
} MSDState;

static const uint8_t qemu_msd_dev_descriptor[] = {
	0x12,       /*  u8 bLength; */
	0x01,       /*  u8 bDescriptorType; Device */
	0x10, 0x00, /*  u16 bcdUSB; v1.0 */

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

static void usb_msd_command_complete(void *opaque, uint32_t tag, int fail)
{
    MSDState *s = (MSDState *)opaque;

    DPRINTF("Command complete\n");
    s->result = fail;
    s->mode = USB_MSDM_CSW;
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

static int usb_msd_handle_data(USBDevice *dev, int pid, uint8_t devep,
                               uint8_t *data, int len)
{
    MSDState *s = (MSDState *)dev;
    int ret = 0;
    struct usb_msd_cbw cbw;
    struct usb_msd_csw csw;

    switch (pid) {
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
            scsi_send_command(s->scsi_dev, s->tag, cbw.cmd, 0);
            ret = len;
            break;

        case USB_MSDM_DATAOUT:
            DPRINTF("Data out %d/%d\n", len, s->data_len);
            if (len > s->data_len)
                goto fail;

            if (scsi_write_data(s->scsi_dev, data, len))
                goto fail;

            s->data_len -= len;
            if (s->data_len == 0)
                s->mode = USB_MSDM_CSW;
            ret = len;
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
        case USB_MSDM_CSW:
            DPRINTF("Command status %d tag 0x%x, len %d\n",
                    s->result, s->tag, len);
            if (len < 13)
                goto fail;

            csw.sig = cpu_to_le32(0x53425355);
            csw.tag = cpu_to_le32(s->tag);
            csw.residue = 0;
            csw.status = s->result;
            memcpy(data, &csw, 13);
            ret = 13;
            s->mode = USB_MSDM_CBW;
            break;

        case USB_MSDM_DATAIN:
            DPRINTF("Data in %d/%d\n", len, s->data_len);
            if (len > s->data_len)
                len = s->data_len;

            if (scsi_read_data(s->scsi_dev, data, len))
                goto fail;

            s->data_len -= len;
            if (s->data_len == 0)
                s->mode = USB_MSDM_CSW;
            ret = len;
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

static void usb_msd_handle_destroy(USBDevice *dev)
{
    MSDState *s = (MSDState *)dev;

    scsi_disk_destroy(s->scsi_dev);
    qemu_free(s);
}

USBDevice *usb_msd_init(const char *filename)
{
    MSDState *s;
    BlockDriverState *bdrv;

    s = qemu_mallocz(sizeof(MSDState));
    if (!s)
        return NULL;

    bdrv = bdrv_new("usb");
    bdrv_open(bdrv, filename, 0);

    s->dev.speed = USB_SPEED_FULL;
    s->dev.handle_packet = usb_generic_handle_packet;

    s->dev.handle_reset = usb_msd_handle_reset;
    s->dev.handle_control = usb_msd_handle_control;
    s->dev.handle_data = usb_msd_handle_data;
    s->dev.handle_destroy = usb_msd_handle_destroy;

    snprintf(s->dev.devname, sizeof(s->dev.devname), "QEMU USB MSD(%.16s)",
             filename);

    s->scsi_dev = scsi_disk_init(bdrv, usb_msd_command_complete, s);
    usb_msd_handle_reset((USBDevice *)s);
    return (USBDevice *)s;
}
