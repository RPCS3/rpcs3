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

//#define DEBUG

#define MAX_PORTS 8

#include "../qemu-usb/vl.h"

/* HID interface requests */
#define GET_REPORT   0xa101
#define GET_IDLE     0xa102
#define GET_PROTOCOL 0xa103
#define SET_IDLE     0x210a
#define SET_PROTOCOL 0x210b

typedef struct EYETOYState {
    USBDevice dev;
	//nothing yet
} EYETOYState;

/* same as Linux kernel root hubs */

/* mostly the same values as the Bochs USB Mouse device */
static const uint8_t eyetoy_dev_descriptor[] = {
    0x12, /* bLength */
    0x01, /* bDescriptorType */
    0x10, 0x01, /* bcdUSB */
    0x00, /* bDeviceClass */
    0x00, /* bDeviceSubClass */
    0x00, /* bDeviceProtocol */
    0x08, /* bMaxPacketSize0 */
    0x4c, 0x05, /* idVendor */
    0x55, 0x01, /* idProduct */
    0x00, 0x01, /* bcdDevice */
    0x01, /* iManufacturer */
    0x02, /* iProduct */
    0x00, /* iSerialNumber */
    0x01, /* bNumConfigurations */
};

/* XXX: patch interrupt size */
static const uint8_t eyetoy_config_descriptor[] = {

	/* one configuration */
	0x09,       /*  u8  bLength; */
	0x02,       /*  u8  bDescriptorType; Configuration */
	0xb4, 0x00, /*  u16 wTotalLength; */
	0x03,       /*  u8  bNumInterfaces; (3) */
	0x01,       /*  u8  bConfigurationValue; */
	0x00,       /*  u8  iConfiguration; */
	0x90,       /*  u8  bmAttributes; 
				 Bit 7: must be set,
				     6: Self-powered,
				     5: Remote wakeup,
				     4..0: resvd */
	0xfa,       /*  u8  MaxPower; */

	/* interface #0 alternate setting #0 */
	0x09,       /*  u8  if_bLength; */
	0x04,       /*  u8  if_bDescriptorType; Interface */
	0x00,       /*  u8  if_bInterfaceNumber; */
	0x00,       /*  u8  if_bAlternateSetting; */
	0x01,       /*  u8  if_bNumEndpoints; */
	0xff,       /*  u8  if_bInterfaceClass; Vendor Specific */
	0x00,       /*  u8  if_bInterfaceSubClass; */
	0x00,       /*  u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
	0x00,       /*  u8  if_iInterface; */

	/* interface #0 alternate setting #0 endpoint */
	0x07,       /*  u8  ep_bLength; */
	0x05,       /*  u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x01,       /*  u8  ep_bmAttributes; */
 	0x00, 0x00, /*  u16 ep_wMaxPacketSize; 1 + (MAX_ROOT_PORTS / 8) */
	0x01,       /*  u8  ep_bInterval; */

	/* interface #0 alternate setting #1 */
	0x09, 0x04, 0x00, 0x01, 0x01, 0xff, 0x00, 0x00, 0x00,
	0x07, 0x05, 0x81, 0x01, 0x80, 0x01, 0x01,

	/* interface #0 alternate setting #2*/
	0x09, 0x04, 0x00, 0x02, 0x01, 0xff, 0x00, 0x00, 0x00,
	0x07, 0x05, 0x81, 0x01, 0x00, 0x02, 0x01,

	/* interface #0 alternate setting #3 */
	0x09, 0x04, 0x00, 0x03, 0x01, 0xff, 0x00, 0x00, 0x00,
	0x07, 0x05, 0x81, 0x01, 0x00, 0x03, 0x01,

	/* interface #0 alternate setting #4 */
	0x09, 0x04, 0x00, 0x04, 0x01, 0xff, 0x00, 0x00, 0x00,
	0x07, 0x05, 0x81, 0x01, 0x80, 0x03, 0x01,

	/* interface #1  alternate setting #0 */
	0x09,       /*  u8  if_bLength; */
	0x04,       /*  u8  if_bDescriptorType; Interface */
	0x01,       /*  u8  if_bInterfaceNumber; */
	0x00,       /*  u8  if_bAlternateSetting; */
	0x00,       /*  u8  if_bNumEndpoints; */
	0x01,       /*  u8  if_bInterfaceClass; AUDIO */
	0x01,       /*  u8  if_bInterfaceSubClass; AUDIOCONTROL*/
	0x00,       /*  u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
	0x00,       /*  u8  if_iInterface; */

	/* interface #1 alternate setting #0 classes */
	0x09, 0x24, 0x01, 0x00, 0x01, 0x1e, 0x00, 0x01, 0x02,
	0x0c, 0x24, 0x02, 0x01, 0x01, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x09, 0x24,	0x03, 0x02, 0x01, 0x01, 0x00, 0x01, 0x00,

	/* interface #2 alternate setting #0 (AUDIO class, AUDIOSTREAMING subclass) */
	0x09, 0x04, 0x02, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00,

	/* interface #2 alternate setting #1 (AUDIO class, AUDIOSTREAMING subclass) */
	0x09, 0x04, 0x02, 0x01, 0x01, 0x01, 0x02, 0x00, 0x00, 

	/* interface #1 class-specific interface (HEADER) */
	0x07, 0x24, 0x01, 0x02, 0x01, 0x01, 0x00,

	/* interface #1 class-specific interface (INPUT TERMINAL) */
	0x0b, 0x24, 0x02, 0x01, 0x01, 0x02, 0x10, 0x01, 0x80, 0x3e, 0x00,

	/* interface #1 alternate setting #1 endpoint */
	0x09, 0x05, 0x82, 0x05, 0x28, 0x00, 0x01, 0x00, 0x00, 

	/* interface #1 class-specific endpoint */
	0x07, 0x25, 0x01, 0x00, 0x00, 0x00, 0x00
};

static void eyetoy_handle_reset(USBDevice *dev)
{
    /* XXX: do it */
	return;
}

static int eyetoy_handle_control(USBDevice *dev, int request, int value,
                                  int index, int length, uint8_t *data)
{
    EYETOYState *s = (EYETOYState *)dev;
    int ret = 0;

    switch(request) {
    case DeviceRequest | USB_REQ_GET_STATUS:
        data[0] = (dev->remote_wakeup << USB_DEVICE_REMOTE_WAKEUP);
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
            memcpy(data, eyetoy_dev_descriptor, 
                   sizeof(eyetoy_dev_descriptor));
            ret = sizeof(eyetoy_dev_descriptor);
            break;
        case USB_DT_CONFIG:
			memcpy(data, eyetoy_config_descriptor, 
				sizeof(eyetoy_config_descriptor));
			ret = sizeof(eyetoy_config_descriptor);
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
                ret = set_usb_string(data, "3X0420811");
                break;
            case 2:
                /* product description */
			    ret = set_usb_string(data, "EyeToy USB camera Namtai");
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
        /* hid specific requests */
    case InterfaceRequest | USB_REQ_GET_DESCRIPTOR:
        //switch(value >> 8) {
        //((case 0x22:
		//	memcpy(data, qemu_mouse_hid_report_descriptor, 
		//		sizeof(qemu_mouse_hid_report_descriptor));
		//	ret = sizeof(qemu_mouse_hid_report_descriptor);
		//    break;
        //default:
            goto fail;
        //}
        break;
    case GET_REPORT:
		ret = 0;
        break;
    case SET_IDLE:
        ret = 0;
        break;
    default:
    fail:
        ret = USB_RET_STALL;
        break;
    }
    return ret;
}

static int eyetoy_handle_data(USBDevice *dev, int pid, 
                               uint8_t devep, uint8_t *data, int len)
{
    EYETOYState *s = (EYETOYState *)dev;
    int ret = 0;

    switch(pid) {
    case USB_TOKEN_IN:
        if (devep == 1) {
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


static void eyetoy_handle_destroy(USBDevice *dev)
{
    EYETOYState *s = (EYETOYState *)dev;

    free(s);
}

int eyetoy_handle_packet(USBDevice *s, int pid, 
                              uint8_t devaddr, uint8_t devep,
                              uint8_t *data, int len)
{
	fprintf(stderr,"usb-eyetoy: packet received with pid=%x, devaddr=%x, devep=%x and len=%x\n",pid,devaddr,devep,len);
	usb_generic_handle_packet(s,pid,devaddr,devep,data,len);
}

USBDevice *eyetoy_init()
{
    EYETOYState *s;

    s = qemu_mallocz(sizeof(EYETOYState));
    if (!s)
        return NULL;
    s->dev.speed = USB_SPEED_FULL;
    s->dev.handle_packet = eyetoy_handle_packet;

    s->dev.handle_reset = eyetoy_handle_reset;
    s->dev.handle_control = eyetoy_handle_control;
    s->dev.handle_data = eyetoy_handle_data;
    s->dev.handle_destroy = eyetoy_handle_destroy;

    strncpy(s->dev.devname, "EyeToy USB camera Namtai", sizeof(s->dev.devname));

    return (USBDevice *)s;

}

#ifdef X_DRIVER
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

/* Video-to-USB Bridge Driver for OmniVision OV511/OV511+/OV518/OV518+/OV519
 *
 * Copyright (c) 1999-2005 Mark W. McClelland
 * Support for OV519, OV8610 Copyright (c) 2003 Joerg Heckenbach
 *
 * Original decompression code Copyright 1998-2000 OmniVision Technologies
 * Many improvements by Bret Wallach <bwallac1@san.rr.com>
 * Color fixes by by Orion Sky Lawlor <olawlor@acm.org> (2/26/2000)
 * Snapshot code by Kevin Moore
 * OV7620 fixes by Charl P. Botha <cpbotha@ieee.org>
 * Changes by Claudio Matsuoka <claudio@conectiva.com>
 * Original SAA7111A code by Dave Perks <dperks@ibm.net>
 * URB error messages from pwc driver by Nemosoft
 * generic_ioctl() code from videodev.c by Gerd Knorr and Alan Cox
 * Memory management (rvmalloc) code from bttv driver, by Gerd Knorr and others
 * OV7x3x/7x4x detection by Franz Reinhardt
 * 2004/01/25: Added OV7640 and EyeToy support (Mark McClelland)
 *
 * Based on the Linux CPiA driver written by Peter Pregler,
 * Scott J. Bertin and Johannes Erdfelt.
 *
 * Please see the file: doc/ov51x.txt
 * and the website at:  http://alpha.dyndns.org/ov511
 * for more info.
 * For Questions on OV519 or OV8610 please contact <joerg@heckenbach-aw.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef OV511_ALLOW_CONVERSION
	/* Pixel count * 3 bytes for RGB */
	#define MAX_FRAME_SIZE(w, h) ((w) * (h) * 3)
#else
	/* Pixel count * bytes per YUV420 pixel (1.5) */
	#define MAX_FRAME_SIZE(w, h) ((w) * (h) * 3 / 2)
#endif

#define MAX_DATA_SIZE(w, h) (MAX_FRAME_SIZE(w, h) + sizeof(struct timeval))

/* Max size * bytes per YUV420 pixel (1.5) + one extra isoc frame for safety */
#define MAX_RAW_DATA_SIZE(w, h) ((w) * (h) * 3 / 2 + 1024)

#define FATAL_ERROR(rc) ((rc) < 0 && (rc) != -EPERM)

/* URB error codes: */
static struct symbolic_list urb_errlist[] = {
	{ -ENOSR,		"Buffer error (overrun)" },
	{ -EPIPE,		"Stalled (device not responding)" },
	{ -EOVERFLOW,	"Babble (bad cable?)" },
	{ -EPROTO,		"Bit-stuff error (bad cable?)" },
	{ -EILSEQ,		"CRC/Timeout" },
	{ -ETIMEDOUT,	"NAK (device does not respond)" },
	{ -1, NULL }
};


/**********************************************************************
 * /proc interface
 * Based on the CPiA driver version 0.7.4 -claudio
 **********************************************************************/

#if defined(CONFIG_VIDEO_PROC_FS)

static struct proc_dir_entry *ov511_proc_entry = NULL;
extern struct proc_dir_entry *video_proc_entry;

/* Prototypes */
static void ov51x_clear_snapshot(struct usb_ov511 *);
static int sensor_get_picture(struct usb_ov511 *, struct video_picture *);
static int sensor_get_exposure(struct usb_ov511 *, unsigned char *);
static int ov51x_check_snapshot(struct usb_ov511 *);
static int ov51x_control_ioctl(struct inode *, struct file *, unsigned int,
			       unsigned long);

static struct file_operations ov511_control_fops = {
	.ioctl =	ov51x_control_ioctl,
};

#define YES_NO(x) ((x) ? "yes" : "no")


/**********************************************************************
 *
 * Register I/O
 *
 **********************************************************************/

/* Write an OV51x register


ov->cbuf[0] = value;

usb_control_msg:
pipe:			((PIPE_CONTROL << 30) | (dev->devnum << 8) | (0 << 15)),
request:		1,
requesttype:	USB_TYPE_VENDOR | USB_RECIP_DEVICE,
value:			0,
index:			(__u16)reg,
bytes:			&ov->cbuf[0],
size:			1,
timeout:		1000

*/

/* Read from an OV51x register 

usb_control_msg:
pipe:			((PIPE_CONTROL << 30) | (dev->devnum << 8) | (0 << 15)),
request:		1,
requesttype:	USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
value:			0,
index:			(__u16)reg,
bytes:			&ov->cbuf[0],
size:			1,
timeout:		1000

if result<0 then error
else value = ov->cbuf[0];

*/

/*
 * Writes a multbyte value to a single register. Only valid with certain
 * registers (0x30 and 0xc4 - 0xce).

    ov->cbuf = value (in le order)

	rc = usb_control_msg(ov->dev,
			     usb_sndctrlpipe(ov->dev, 0)
			     1 // REG_IO
			     USB_TYPE_VENDOR | USB_RECIP_DEVICE
			     0
				 (__u16)reg
				 ov->cbuf
				 n
				 1000

 */

static int
ov511_upload_quan_tables(struct usb_ov511 *ov)
{
	unsigned char *pYTable = yQuanTable511;
	unsigned char *pUVTable = uvQuanTable511;
	unsigned char val0, val1;
	int i, rc, reg = R511_COMP_LUT_BEGIN;

	PDEBUG(4, "Uploading quantization tables");

	for (i = 0; i < OV511_QUANTABLESIZE / 2; i++) {
		if (ENABLE_Y_QUANTABLE)	{
			val0 = *pYTable++;
			val1 = *pYTable++;
			val0 &= 0x0f;
			val1 &= 0x0f;
			val0 |= val1 << 4;
			rc = reg_w(ov, reg, val0);
			if (rc < 0)
				return rc;
		}

		if (ENABLE_UV_QUANTABLE) {
			val0 = *pUVTable++;
			val1 = *pUVTable++;
			val0 &= 0x0f;
			val1 &= 0x0f;
			val0 |= val1 << 4;
			rc = reg_w(ov, reg + OV511_QUANTABLESIZE/2, val0);
			if (rc < 0)
				return rc;
		}

		reg++;
	}

	return 0;
}

/* OV518 quantization tables are 8x4 (instead of 8x8) */
static int
ov518_upload_quan_tables(struct usb_ov511 *ov)
{
	unsigned char *pYTable = yQuanTable518;
	unsigned char *pUVTable = uvQuanTable518;
	unsigned char val0, val1;
	int i, rc, reg = R511_COMP_LUT_BEGIN;

	PDEBUG(4, "Uploading quantization tables");

	for (i = 0; i < OV518_QUANTABLESIZE / 2; i++) {
		if (ENABLE_Y_QUANTABLE) {
			val0 = *pYTable++;
			val1 = *pYTable++;
			val0 &= 0x0f;
			val1 &= 0x0f;
			val0 |= val1 << 4;
			rc = reg_w(ov, reg, val0);
			if (rc < 0)
				return rc;
		}

		if (ENABLE_UV_QUANTABLE) {
			val0 = *pUVTable++;
			val1 = *pUVTable++;
			val0 &= 0x0f;
			val1 &= 0x0f;
			val0 |= val1 << 4;
			rc = reg_w(ov, reg + OV518_QUANTABLESIZE/2, val0);
			if (rc < 0)
				return rc;
		}

		reg++;
	}

	return 0;
}

static int
ov51x_reset(struct usb_ov511 *ov, unsigned char reset_type)
{
	int rc = -1;

	if (ov->bclass == BCL_OV519) {
		//~~~
		PDEBUG(1, "Reset: type=0x0f");
		switch (reset_type) {
		case OV511_RESET_NOREGS:
			rc = reg_w(ov, OV519_SYS_RESET1, 0x0f);
//			rc = reg_w(ov, OV519_SYS_RESET0, 0xdc);
//			rc = reg_w(ov, OV519_SYS_RESET0, 0);
			rc = reg_w(ov, OV519_SYS_RESET1, 0);
			break;
		}
	} else {
		/* Setting bit 0 not allowed on 518/518Plus */
		if (ov->bclass == BCL_OV518)
			reset_type &= 0xfe;

		PDEBUG(4, "Reset: type=0x%02X", reset_type);

		rc = reg_w(ov, R51x_SYS_RESET, reset_type);
		rc = reg_w(ov, R51x_SYS_RESET, 0);
	}
	if (rc < 0)
		err("reset: command failed");

	return rc;
}

/**********************************************************************
 *
 * Low-level I2C I/O functions
 *
 **********************************************************************/

/* NOTE: Do not call this function directly!
 * The OV518 I2C I/O procedure is different, hence, this function.
 * This is normally only called from i2c_w(). Note that this function
 * always succeeds regardless of whether the sensor is present and working.
 */
static int
ov518_i2c_write_internal(struct usb_ov511 *ov,
			 unsigned char reg,
			 unsigned char value)
{
	int rc;

	PDEBUG(5, "0x%02X:0x%02X", reg, value);

	/* Select camera register */
	rc = reg_w(ov, R51x_I2C_SADDR_3, reg);
	if (rc < 0) return rc;

	/* Write "value" to I2C data port of OV511 */
	rc = reg_w(ov, R51x_I2C_DATA, value);
	if (rc < 0) return rc;

	/* Initiate 3-byte write cycle */
	rc = reg_w(ov, R518_I2C_CTL, 0x01);
	if (rc < 0) return rc;

	return 0;
}

/* NOTE: Do not call this function directly! */
static int
ov511_i2c_write_internal(struct usb_ov511 *ov,
			 unsigned char reg,
			 unsigned char value)
{
	int rc, retries;

	PDEBUG(5, "0x%02X:0x%02X", reg, value);

	/* Three byte write cycle */
	for (retries = OV511_I2C_RETRIES; ; ) {
		/* Select camera register */
		rc = reg_w(ov, R51x_I2C_SADDR_3, reg);
		if (rc < 0) return rc;

		/* Write "value" to I2C data port of OV511 */
		rc = reg_w(ov, R51x_I2C_DATA, value);
		if (rc < 0) return rc;

		/* Initiate 3-byte write cycle */
		rc = reg_w(ov, R511_I2C_CTL, 0x01);
		if (rc < 0) return rc;

		do rc = reg_r(ov, R511_I2C_CTL);
		while (rc > 0 && ((rc&1) == 0)); /* Retry until idle */
		if (rc < 0) return rc;

		if ((rc&2) == 0) /* Ack? */
			break;
#if 0
		/* I2C abort */
		reg_w(ov, R511_I2C_CTL, 0x10);
#endif
		if (--retries < 0) {
			PDEBUG(5, "i2c write retries exhausted");
			return -1;
		}
	}

	return 0;
}

/* NOTE: Do not call this function directly!
 * The OV518 I2C I/O procedure is different, hence, this function.
 * This is normally only called from i2c_r(). Note that this function
 * always succeeds regardless of whether the sensor is present and working.
 */
static int
ov518_i2c_read_internal(struct usb_ov511 *ov, unsigned char reg)
{
	int rc, value;

	/* Select camera register */
	rc = reg_w(ov, R51x_I2C_SADDR_2, reg);
	if (rc < 0) return rc;

	/* Initiate 2-byte write cycle */
	rc = reg_w(ov, R518_I2C_CTL, 0x03);
	if (rc < 0) return rc;

	/* Initiate 2-byte read cycle */
	rc = reg_w(ov, R518_I2C_CTL, 0x05);
	if (rc < 0) return rc;

	value = reg_r(ov, R51x_I2C_DATA);

	PDEBUG(5, "0x%02X:0x%02X", reg, value);

	return value;
}

/* NOTE: Do not call this function directly!
 * returns: negative is error, pos or zero is data */
static int
ov511_i2c_read_internal(struct usb_ov511 *ov, unsigned char reg)
{
	int rc, value, retries;

	/* Two byte write cycle */
	for (retries = OV511_I2C_RETRIES; ; ) {
		/* Select camera register */
		rc = reg_w(ov, R51x_I2C_SADDR_2, reg);
		if (rc < 0) return rc;

		/* Initiate 2-byte write cycle */
		rc = reg_w(ov, R511_I2C_CTL, 0x03);
		if (rc < 0) return rc;

		do rc = reg_r(ov, R511_I2C_CTL);
		while (rc > 0 && ((rc&1) == 0)); /* Retry until idle */
		if (rc < 0) return rc;

		if ((rc&2) == 0) /* Ack? */
			break;

		/* I2C abort */
		reg_w(ov, R511_I2C_CTL, 0x10);

		if (--retries < 0) {
			PDEBUG(5, "i2c write retries exhausted");
			return -1;
		}
	}

	/* Two byte read cycle */
	for (retries = OV511_I2C_RETRIES; ; ) {
		/* Initiate 2-byte read cycle */
		rc = reg_w(ov, R511_I2C_CTL, 0x05);
		if (rc < 0) return rc;

		do rc = reg_r(ov, R511_I2C_CTL);
		while (rc > 0 && ((rc&1) == 0)); /* Retry until idle */
		if (rc < 0) return rc;

		if ((rc&2) == 0) /* Ack? */
			break;

		/* I2C abort */
		rc = reg_w(ov, R511_I2C_CTL, 0x10);
		if (rc < 0) return rc;

		if (--retries < 0) {
			PDEBUG(5, "i2c read retries exhausted");
			return -1;
		}
	}

	value = reg_r(ov, R51x_I2C_DATA);

	PDEBUG(5, "0x%02X:0x%02X", reg, value);

	/* This is needed to make i2c_w() work */
	rc = reg_w(ov, R511_I2C_CTL, 0x05);
	if (rc < 0)
		return rc;

	return value;
}

/* returns: negative is error, pos or zero is data */
static int
i2c_r(struct usb_ov511 *ov, unsigned char reg)
{
	int rc;

	down(&ov->i2c_lock);

	switch (ov->bclass) {
		case BCL_OV511:
			rc = ov511_i2c_read_internal(ov, reg);
			break;
		case BCL_OV518:
		case BCL_OV519:
			rc = ov518_i2c_read_internal(ov, reg);
			break;
		default:
			err("i2c_r: Invalid bridge class");
			rc = -EINVAL;
	}
	up(&ov->i2c_lock);

	return rc;
}

static int
i2c_w(struct usb_ov511 *ov, unsigned char reg, unsigned char value)
{
	int rc;

	down(&ov->i2c_lock);

	switch (ov->bclass) {
		case BCL_OV511:
			rc = ov511_i2c_write_internal(ov, reg, value);
			break;
		case BCL_OV518:
		case BCL_OV519:
			rc = ov518_i2c_write_internal(ov, reg, value);
			break;
		default:
			err("ic2_w: Invalid bridge class");
			rc = -EINVAL;
	}

	up(&ov->i2c_lock);

	return rc;
}

/* Do not call this function directly! */
static int
ov51x_i2c_write_mask_internal(struct usb_ov511 *ov,
			      unsigned char reg,
			      unsigned char value,
			      unsigned char mask)
{
	int rc;
	unsigned char oldval, newval;

	if (mask == 0xff) {
		newval = value;
	} else {
		switch (ov->bclass) {
			case BCL_OV511:
					rc = ov511_i2c_read_internal(ov, reg);
				break;
			case BCL_OV518:
			case BCL_OV519:
				rc = ov518_i2c_read_internal(ov, reg);
				break;
			default:
				err("ov51x_i2c_write_mask_internal: Invalid bridge class");
				rc = -EINVAL;
		}
		if (rc < 0)
			return rc;

		oldval = (unsigned char) rc;
		oldval &= (~mask);		/* Clear the masked bits */
		value &= mask;			/* Enforce mask on value */
		newval = oldval | value;	/* Set the desired bits */
	}

	switch (ov->bclass) {
		case BCL_OV511:
			return (ov511_i2c_write_internal(ov, reg, newval));
			break;
		case BCL_OV518:
		case BCL_OV519:
			return (ov518_i2c_write_internal(ov, reg, newval));
			break;
		default:
			return -EINVAL;
	}
}

/* Writes bits at positions specified by mask to an I2C reg. Bits that are in
 * the same position as 1's in "mask" are cleared and set to "value". Bits
 * that are in the same position as 0's in "mask" are preserved, regardless
 * of their respective state in "value".
 */
static int
i2c_w_mask(struct usb_ov511 *ov,
	   unsigned char reg,
	   unsigned char value,
	   unsigned char mask)
{
	int rc;

	down(&ov->i2c_lock);
	rc = ov51x_i2c_write_mask_internal(ov, reg, value, mask);
	up(&ov->i2c_lock);

	return rc;
}

/* Do not call this function directly! */
static int
ov51x_i2c_setbit_internal(struct usb_ov511 *ov,
			      unsigned char reg,
			      unsigned char value,
			      unsigned char bitaddr)
{
	int rc;
	unsigned char newval;

	switch (ov->bclass) {
		case BCL_OV511:
			rc = ov511_i2c_read_internal(ov, reg);
			break;
		case BCL_OV518:
		case BCL_OV519:
			rc = ov518_i2c_read_internal(ov, reg);
			break;
		default:
			err("ov51x_i2c_setbit_internal: Invalid bridge class");
			rc = -EINVAL;
	}
	if (rc < 0)
		return rc;

	newval = ((unsigned char)rc & ~(1 << bitaddr)) | (value ? (1 << bitaddr) : 0);	/* Set the desired bit */

	switch (ov->bclass) {
		case BCL_OV511:
			return (ov511_i2c_write_internal(ov, reg, newval));
			break;
		case BCL_OV518:
		case BCL_OV519:
			return (ov518_i2c_write_internal(ov, reg, newval));
			break;
		default:
			return -EINVAL;
	}
}

/* Writes bits at positions specified by bitaddr to an I2C reg. Bits are cleared
 * if value = 0 and set if value = 1.
 */
static int
i2c_setbit(struct usb_ov511 *ov,
	   unsigned char reg,
	   unsigned char value,
	   unsigned char bitaddr)
{
	int rc;

	down(&ov->i2c_lock);
	rc = ov51x_i2c_setbit_internal(ov, reg, value, bitaddr);
	up(&ov->i2c_lock);

	return rc;
}

/* Set the read and write slave IDs. The "slave" argument is the write slave,
 * and the read slave will be set to (slave + 1). ov->i2c_lock should be held
 * when calling this. This should not be called from outside the i2c I/O
 * functions.
 */
static int
i2c_set_slave_internal(struct usb_ov511 *ov, unsigned char slave)
{
	int rc;

	rc = reg_w(ov, R51x_I2C_W_SID, slave);
	if (rc < 0) return rc;

	rc = reg_w(ov, R51x_I2C_R_SID, slave + 1);
	if (rc < 0) return rc;

	return 0;
}

/* Write to a specific I2C slave ID and register, using the specified mask */
static int
i2c_w_slave(struct usb_ov511 *ov,
	    unsigned char slave,
	    unsigned char reg,
	    unsigned char value,
	    unsigned char mask)
{
	int rc = 0;

	down(&ov->i2c_lock);

	/* Set new slave IDs */
	rc = i2c_set_slave_internal(ov, slave);
	if (rc < 0) goto out;

	rc = ov51x_i2c_write_mask_internal(ov, reg, value, mask);

out:
	/* Restore primary IDs */
	if (i2c_set_slave_internal(ov, ov->primary_i2c_slave) < 0)
		err("Couldn't restore primary I2C slave");

	up(&ov->i2c_lock);
	return rc;
}

/* Read from a specific I2C slave ID and register */
static int
i2c_r_slave(struct usb_ov511 *ov,
	    unsigned char slave,
	    unsigned char reg)
{
	int rc;

	down(&ov->i2c_lock);

	/* Set new slave IDs */
	rc = i2c_set_slave_internal(ov, slave);
	if (rc < 0) goto out;

	switch (ov->bclass) {
		case BCL_OV511:
			rc = ov511_i2c_read_internal(ov, reg);
			break;
		case BCL_OV518:
		case BCL_OV519:
			rc = ov518_i2c_read_internal(ov, reg);
			break;
		default:
			err("i2c_r_slave: Invalid bridge class");
			rc = -EINVAL;
	}
out:
	/* Restore primary IDs */
	if (i2c_set_slave_internal(ov, ov->primary_i2c_slave) < 0)
		err("Couldn't restore primary I2C slave");

	up(&ov->i2c_lock);
	return rc;
}

/* Sets I2C read and write slave IDs. Returns <0 for error */
static int
ov51x_set_slave_ids(struct usb_ov511 *ov, unsigned char sid)
{
	int rc;

	down(&ov->i2c_lock);

	rc = i2c_set_slave_internal(ov, sid);
	if (rc < 0) goto out;

	// FIXME: Is this actually necessary?
	if (ov->bclass != BCL_OV519)
		rc = ov51x_reset(ov, OV511_RESET_NOREGS);
	if (rc < 0) goto out;

out:
	up(&ov->i2c_lock);
	return rc;
}

static int
write_regvals(struct usb_ov511 *ov, struct ov511_regvals * pRegvals)
{
	int rc;

	while (pRegvals->bus != OV511_DONE_BUS) {
		if (pRegvals->bus == OV511_REG_BUS) {
			if ((rc = reg_w(ov, pRegvals->reg, pRegvals->val)) < 0)
				return rc;
		} else if (pRegvals->bus == OV511_I2C_BUS) {
			if ((rc = i2c_w(ov, pRegvals->reg, pRegvals->val)) < 0)
				return rc;
		} else {
			err("Bad regval array");
			return -1;
		}
		pRegvals++;
	}
	return 0;
}

#ifdef OV511_DEBUG
static void
dump_i2c_range(struct usb_ov511 *ov, int reg1, int regn)
{
	int i, rc;

	for (i = reg1; i <= regn; i++) {
		rc = i2c_r(ov, i);
		info("Sensor[0x%02X] = 0x%02X", i, rc);
	}
}

static void
dump_i2c_regs(struct usb_ov511 *ov)
{
	info("I2C REGS");
	dump_i2c_range(ov, 0x00, 0x7C);
}

static void
dump_reg_range(struct usb_ov511 *ov, int reg1, int regn)
{
	int i, rc;

	for (i = reg1; i <= regn; i++) {
		rc = reg_r(ov, i);
		info("OV511[0x%02X] = 0x%02X", i, rc);
	}
}

static void
ov511_dump_regs(struct usb_ov511 *ov)
{
	info("CAMERA INTERFACE REGS");
	dump_reg_range(ov, 0x10, 0x1f);
	info("DRAM INTERFACE REGS");
	dump_reg_range(ov, 0x20, 0x23);
	info("ISO FIFO REGS");
	dump_reg_range(ov, 0x30, 0x31);
	info("PIO REGS");
	dump_reg_range(ov, 0x38, 0x39);
	dump_reg_range(ov, 0x3e, 0x3e);
	info("I2C REGS");
	dump_reg_range(ov, 0x40, 0x49);
	info("SYSTEM CONTROL REGS");
	dump_reg_range(ov, 0x50, 0x55);
	dump_reg_range(ov, 0x5e, 0x5f);
	info("OmniCE REGS");
	dump_reg_range(ov, 0x70, 0x79);
	/* NOTE: Quantization tables are not readable. You will get the value
	 * in reg. 0x79 for every table register */
	dump_reg_range(ov, 0x80, 0x9f);
	dump_reg_range(ov, 0xa0, 0xbf);

}

static void
ov518_dump_regs(struct usb_ov511 *ov)
{
	info("VIDEO MODE REGS");
	dump_reg_range(ov, 0x20, 0x2f);
	info("DATA PUMP AND SNAPSHOT REGS");
	dump_reg_range(ov, 0x30, 0x3f);
	info("I2C REGS");
	dump_reg_range(ov, 0x40, 0x4f);
	info("SYSTEM CONTROL AND VENDOR REGS");
	dump_reg_range(ov, 0x50, 0x5f);
	info("60 - 6F");
	dump_reg_range(ov, 0x60, 0x6f);
	info("70 - 7F");
	dump_reg_range(ov, 0x70, 0x7f);
	info("Y QUANTIZATION TABLE");
	dump_reg_range(ov, 0x80, 0x8f);
	info("UV QUANTIZATION TABLE");
	dump_reg_range(ov, 0x90, 0x9f);
	info("A0 - BF");
	dump_reg_range(ov, 0xa0, 0xbf);
	info("CBR");
	dump_reg_range(ov, 0xc0, 0xcf);
}
#endif

/*****************************************************************************/

/* Temporarily stops OV511 from functioning. Must do this before changing
 * registers while the camera is streaming */
static inline int
ov51x_stop(struct usb_ov511 *ov)
{
	PDEBUG(4, "stopping");
	ov->stopped = 1;
	switch (ov->bclass) {
		case BCL_OV511:
			return (reg_w(ov, R51x_SYS_RESET, 0x3d));
			break;
		case BCL_OV518:
			return (reg_w_mask(ov, R51x_SYS_RESET, 0x3a, 0x3a));
			break;
		case BCL_OV519:
			return (reg_w(ov, OV519_SYS_RESET1, 0x0f));
			break;
		default:
			err("ov51x_stop: invalid bridge type");
	}
	return -1;
}

/* Restarts OV511 after ov511_stop() is called. Has no effect if it is not
 * actually stopped (for performance). */
static inline int
ov51x_restart(struct usb_ov511 *ov)
{
	int rc = 0;

	if (ov->stopped) {
		PDEBUG(4, "restarting");
		ov->stopped = 0;

		/* Reinitialize the stream */
		switch (ov->bclass) {
			case BCL_OV511:
				rc = reg_w(ov, R51x_SYS_RESET, 0x00);
				break;
			case BCL_OV518:
				rc = reg_w(ov, 0x2f, 0x80);
				rc = reg_w(ov, R51x_SYS_RESET, 0x00);
				break;
			case BCL_OV519:
				rc = reg_w(ov, OV519_SYS_RESET1, 0x00);
				break;
			default:
				err("ov51x_restart: invalid bridge type");
				rc = -EINVAL;
		}
	}

	return rc;
}

/* Sleeps until no frames are active. Returns !0 if got signal */
static int
ov51x_wait_frames_inactive(struct usb_ov511 *ov)
{
	return wait_event_interruptible(ov->wq, ov->curframe < 0);
}

/* Resets the hardware snapshot button */
static void
ov51x_clear_snapshot(struct usb_ov511 *ov)
{
	if (ov->bclass == BCL_OV511) {
		reg_w(ov, R51x_SYS_SNAP, 0x00);
		reg_w(ov, R51x_SYS_SNAP, 0x02);
		reg_w(ov, R51x_SYS_SNAP, 0x00);
	} else if (ov->bclass == BCL_OV518) {
		warn("snapshot reset not supported yet on OV518(+)");
	} else {
		err("clear snap: invalid bridge type");
	}
}

#if defined(CONFIG_VIDEO_PROC_FS)
/* Checks the status of the snapshot button. Returns 1 if it was pressed since
 * it was last cleared, and zero in all other cases (including errors) */
static int
ov51x_check_snapshot(struct usb_ov511 *ov)
{
	int ret, status = 0;

	if (ov->bclass == BCL_OV511) {
		ret = reg_r(ov, R51x_SYS_SNAP);
		if (ret < 0) {
			err("Error checking snspshot status (%d)", ret);
		} else if (ret & 0x08) {
			status = 1;
		}
	} else if (ov->bclass == BCL_OV518) {
		warn("snapshot check not supported yet on OV518(+)");
	} else {
		err("check snap: invalid bridge type");
	}

	return status;
}
#endif

/* This does an initial reset of an OmniVision sensor and ensures that I2C
 * is synchronized. Returns <0 for failure.
 */
static int
init_ov_sensor(struct usb_ov511 *ov)
{
	int i, success;

	/* Reset the sensor */
	if (i2c_w(ov, 0x12, 0x80) < 0) return -EIO;

	/* Wait for it to initialize */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 9)
	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(1 + 150 * HZ / 1000);
#else
	msleep(150);
#endif

	for (i = 0, success = 0; i < i2c_detect_tries && !success; i++) {
		if ((i2c_r(ov, OV7610_REG_ID_HIGH) == 0x7F) &&
		    (i2c_r(ov, OV7610_REG_ID_LOW) == 0xA2)) {
			success = 1;
			continue;
		}

		/* Reset the sensor */
		if (i2c_w(ov, 0x12, 0x80) < 0) return -EIO;
		/* Wait for it to initialize */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 9)
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(1 + 150 * HZ / 1000);
#else
		msleep(150);
#endif
		/* Dummy read to sync I2C */
		if (i2c_r(ov, 0x00) < 0) return -EIO;
	}

	if (!success)
		return -EIO;

	PDEBUG(1, "I2C synced in %d attempt(s)", i);

	return 0;
}

static int
ov511_set_packet_size(struct usb_ov511 *ov, int size)
{
	int alt, mult;

	if (ov51x_stop(ov) < 0)
		return -EIO;

	mult = size >> 5;

	if (ov->bridge == BRG_OV511) {
		if (size == 0) alt = OV511_ALT_SIZE_0;
		else if (size == 257) alt = OV511_ALT_SIZE_257;
		else if (size == 513) alt = OV511_ALT_SIZE_513;
		else if (size == 769) alt = OV511_ALT_SIZE_769;
		else if (size == 993) alt = OV511_ALT_SIZE_993;
		else {
			err("Set packet size: invalid size (%d)", size);
			return -EINVAL;
		}
	} else if (ov->bridge == BRG_OV511PLUS) {
		if (size == 0) alt = OV511PLUS_ALT_SIZE_0;
		else if (size == 33) alt = OV511PLUS_ALT_SIZE_33;
		else if (size == 129) alt = OV511PLUS_ALT_SIZE_129;
		else if (size == 257) alt = OV511PLUS_ALT_SIZE_257;
		else if (size == 385) alt = OV511PLUS_ALT_SIZE_385;
		else if (size == 513) alt = OV511PLUS_ALT_SIZE_513;
		else if (size == 769) alt = OV511PLUS_ALT_SIZE_769;
		else if (size == 961) alt = OV511PLUS_ALT_SIZE_961;
		else {
			err("Set packet size: invalid size (%d)", size);
			return -EINVAL;
		}
	} else {
		err("Set packet size: Invalid bridge type");
		return -EINVAL;
	}

	PDEBUG(3, "%d, mult=%d, alt=%d", size, mult, alt);

	if (reg_w(ov, R51x_FIFO_PSIZE, mult) < 0)
		return -EIO;

	if (usb_set_interface(ov->dev, ov->iface, alt) < 0) {
		err("Set packet size: set interface error");
		return -EBUSY;
	}

	if (ov51x_reset(ov, OV511_RESET_NOREGS) < 0)
		return -EIO;

	ov->packet_size = size;

	if (ov51x_restart(ov) < 0)
		return -EIO;

	return 0;
}

/* Note: Unlike the OV511/OV511+, the size argument does NOT include the
 * optional packet number byte. The actual size *is* stored in ov->packet_size,
 * though. */
static int
ov518_set_packet_size(struct usb_ov511 *ov, int size)
{
	int alt;

	if (ov51x_stop(ov) < 0)
		return -EIO;

	if (ov->bclass == BCL_OV518) {
		if (size == 0) alt = OV518_ALT_SIZE_0;
		else if (size == 128) alt = OV518_ALT_SIZE_128;
		else if (size == 256) alt = OV518_ALT_SIZE_256;
		else if (size == 384) alt = OV518_ALT_SIZE_384;
		else if (size == 512) alt = OV518_ALT_SIZE_512;
		else if (size == 640) alt = OV518_ALT_SIZE_640;
		else if (size == 768) alt = OV518_ALT_SIZE_768;
		else if (size == 896) alt = OV518_ALT_SIZE_896;
		else {
			err("Set packet size: invalid size (%d)", size);
			return -EINVAL;
		}
	} else {
		err("Set packet size: Invalid bridge type");
		return -EINVAL;
	}

	PDEBUG(3, "%d, alt=%d", size, alt);

	ov->packet_size = size;
	if (size > 0) {
		/* Program ISO FIFO size reg (packet number isn't included) */
		ov518_reg_w32(ov, 0x30, size, 2);

		if (ov->packet_numbering)
			++ov->packet_size;
	}

	if (usb_set_interface(ov->dev, ov->iface, alt) < 0) {
		err("Set packet size: set interface error");
		return -EBUSY;
	}

	/* Initialize the stream */
	if (reg_w(ov, 0x2f, 0x80) < 0)
		return -EIO;

	if (ov51x_restart(ov) < 0)
		return -EIO;

	if (ov51x_reset(ov, OV511_RESET_NOREGS) < 0)
		return -EIO;

	return 0;
}

/* Note: Unlike the OV511/OV511+, the size argument does NOT include the
 * optional packet number byte. The actual size *is* stored in ov->packet_size,
 * though. */
static int
ov519_set_packet_size(struct usb_ov511 *ov, int size)
{
	int alt;

	if (ov51x_stop(ov) < 0)
		return -EIO;

	if (ov->bclass == BCL_OV519) {
		if      (size ==   0) alt = OV519_ALT_SIZE_0;
		else if (size == 384) alt = OV519_ALT_SIZE_384;
		else if (size == 512) alt = OV519_ALT_SIZE_512;
		else if (size == 768) alt = OV519_ALT_SIZE_768;
		else if (size == 896) alt = OV519_ALT_SIZE_896;
		else {
			err("Set packet size: invalid size (%d)", size);
			return -EINVAL;
		}
	} else {
		err("Set packet size: Invalid bridge class");
		return -EINVAL;
	}

	PDEBUG(3, "%d, alt=%d", size, alt);

	ov->packet_size = size;
	if (size > 0) {
		/* Program ISO FIFO size reg (packet number isn't included) */
	//~~~ is this nessecary?	ov518_reg_w32(ov, 0x30, size, 2);

		if (ov->packet_numbering)
			++ov->packet_size;
	}

	if (usb_set_interface(ov->dev, ov->iface, alt) < 0) {
		err("Set packet size: set interface error");
		return -EBUSY;
	}

	/* Initialize the stream */

	if (size > 0) {
		if (ov51x_restart(ov) < 0)
			return -EIO;
	}

//	if (ov51x_reset(ov, OV511_RESET_NOREGS) < 0)
//		return -EIO;

	return 0;
}

/* Upload compression params and quantization tables. Returns 0 for success. */
static int
ov511_init_compression(struct usb_ov511 *ov)
{
	int rc = 0;

	if (!ov->compress_inited) {
		reg_w(ov, 0x70, phy);
		reg_w(ov, 0x71, phuv);
		reg_w(ov, 0x72, pvy);
		reg_w(ov, 0x73, pvuv);
		reg_w(ov, 0x74, qhy);
		reg_w(ov, 0x75, qhuv);
		reg_w(ov, 0x76, qvy);
		reg_w(ov, 0x77, qvuv);

		if (ov511_upload_quan_tables(ov) < 0) {
			err("Error uploading quantization tables");
			rc = -EIO;
			goto out;
		}
	}

	ov->compress_inited = 1;
out:
	return rc;
}

/* Upload compression params and quantization tables. Returns 0 for success. */
static int
ov518_init_compression(struct usb_ov511 *ov)
{
	int rc = 0;

	if (!ov->compress_inited) {
		if (ov518_upload_quan_tables(ov) < 0) {
			err("Error uploading quantization tables");
			rc = -EIO;
			goto out;
		}
	}

	ov->compress_inited = 1;
out:
	return rc;
}

/* Switch on standard JPEG compression. Returns 0 for success. */
static int
ov519_init_compression(struct usb_ov511 *ov)
{
	int rc = 0;

	if (!ov->compress_inited) {
		if (reg_setbit(ov, OV519_SYS_EN_CLK1, 1, 2 ) < 0) {
			err("Error switching to compressed mode");
			rc = -EIO;
			goto out;
		}
	}

	ov->compress_inited = 1;
out:
	return rc;
}

/* -------------------------------------------------------------------------- */

/* Sets sensor's contrast setting to "val" */
static int
sensor_set_contrast(struct usb_ov511 *ov, unsigned short val)
{
	int rc;

	PDEBUG(3, "%d", val);

	if (ov->stop_during_set)
		if (ov51x_stop(ov) < 0)
			return -EIO;

	switch (ov->sensor) {
	case SEN_OV7610:
	case SEN_OV6620:
	{
		rc = i2c_w(ov, OV7610_REG_CNT, val >> 8);
		if (rc < 0)
			goto out;
		break;
	}
	case SEN_OV6630:
	{
		rc = i2c_w_mask(ov, OV7610_REG_CNT, val >> 12, 0x0f);
		if (rc < 0)
			goto out;
		break;
	}
	case SEN_OV8610:
	{
		static unsigned char ctab[] = {
			0x03, 0x09, 0x0b, 0x0f, 0x53, 0x6f, 0x35, 0x7f
		};

		/* Use Y gamma control instead. Bit 0 enables it. */
		rc = i2c_w(ov, 0x64, ctab[val>>13]);
		if (rc < 0)
			goto out;
		break;
	}
	case SEN_OV7620:
	{
		static unsigned char ctab[] = {
			0x01, 0x05, 0x09, 0x11, 0x15, 0x35, 0x37, 0x57,
			0x5b, 0xa5, 0xa7, 0xc7, 0xc9, 0xcf, 0xef, 0xff
		};

		/* Use Y gamma control instead. Bit 0 enables it. */
		rc = i2c_w(ov, 0x64, ctab[val>>12]);
		if (rc < 0)
			goto out;
		break;
	}
	case SEN_OV7640:
	{
		/* Use gain control instead. */
		rc = i2c_w(ov, OV7610_REG_GAIN, val>>10);
		if (rc < 0)
			goto out;
		break;
	}
	case SEN_SAA7111A:
	{
		rc = i2c_w(ov, 0x0b, val >> 9);
		if (rc < 0)
			goto out;
		break;
	}
	default:
	{
		PDEBUG(3, "Unsupported with this sensor");
		rc = -EPERM;
		goto out;
	}
	}

	rc = 0;		/* Success */
	ov->contrast = val;
out:
	if (ov51x_restart(ov) < 0)
		return -EIO;

	return rc;
}

/* Gets sensor's contrast setting */
static int
sensor_get_contrast(struct usb_ov511 *ov, unsigned short *val)
{
	int rc;

	switch (ov->sensor) {
	case SEN_OV7610:
	case SEN_OV6620:
		rc = i2c_r(ov, OV7610_REG_CNT);
		if (rc < 0)
			return rc;
		else
			*val = rc << 8;
		break;
	case SEN_OV6630:
		rc = i2c_r(ov, OV7610_REG_CNT);
		if (rc < 0)
			return rc;
		else
			*val = rc << 12;
		break;
	case SEN_OV8610:
	{
		static unsigned char ctab[] = {
			0x03, 0x09, 0x0b, 0x0f, 0x53, 0x6f, 0x35, 0x7f
		};
		static int cidx = 0;

		/* Use Y gamma control instead. Bit 0 enables it. */
		rc = i2c_r(ov, 0x64);
		if (rc < 0) {
			return rc;
		} else {
			for (cidx = 0; cidx < 8; cidx++) {
				if (ctab[cidx] == rc) {
					*val = cidx << 13;
					break;
				}
			}
			if (cidx == 8) // could not find value in table
				return -EINVAL;
		}
		break;
	}
	case SEN_OV7620:
		/* Use Y gamma reg instead. Bit 0 is the enable bit. */
		rc = i2c_r(ov, 0x64);
		if (rc < 0)
			return rc;
		else
			*val = (rc & 0xfe) << 8;
		break;
	case SEN_OV7640:
		/* Use gain control instead. */
		rc = i2c_r(ov, OV7610_REG_GAIN);
		if (rc < 0)
			return rc;
		else
			*val = rc << 10;
		break;

	case SEN_SAA7111A:
		*val = ov->contrast;
		break;
	default:
		PDEBUG(3, "Unsupported with this sensor");
		return -EPERM;
	}

	PDEBUG(3, "%d", *val);
	ov->contrast = *val;

	return 0;
}

/* -------------------------------------------------------------------------- */

/* Sets sensor's brightness setting to "val" */
static int
sensor_set_brightness(struct usb_ov511 *ov, unsigned short val)
{
	int rc;

	PDEBUG(4, "%d", val);

	if (ov->stop_during_set)
		if (ov51x_stop(ov) < 0)
			return -EIO;

	switch (ov->sensor) {
	case SEN_OV8610:
	case SEN_OV7610:
	case SEN_OV76BE:
	case SEN_OV6620:
	case SEN_OV6630:
	case SEN_OV7640:
		rc = i2c_w(ov, OV7610_REG_BRT, val >> 8);
		if (rc < 0)
			goto out;
		break;
	case SEN_OV7620:
		/* 7620 doesn't like manual changes when in auto mode */
		if (!ov->auto_brt) {
			rc = i2c_w(ov, OV7610_REG_BRT, val >> 8);
			if (rc < 0)
				goto out;
		}
		break;
	case SEN_SAA7111A:
		rc = i2c_w(ov, 0x0a, val >> 8);
		if (rc < 0)
			goto out;
		break;
	default:
		PDEBUG(3, "Unsupported with this sensor");
		rc = -EPERM;
		goto out;
	}

	rc = 0;		/* Success */
	ov->brightness = val;
out:
	if (ov51x_restart(ov) < 0)
		return -EIO;

	return rc;
}

/* Gets sensor's brightness setting */
static int
sensor_get_brightness(struct usb_ov511 *ov, unsigned short *val)
{
	int rc;

	switch (ov->sensor) {
	case SEN_OV8610:
	case SEN_OV7610:
	case SEN_OV76BE:
	case SEN_OV7620:
	case SEN_OV6620:
	case SEN_OV6630:
	case SEN_OV7640:
		rc = i2c_r(ov, OV7610_REG_BRT);
		if (rc < 0)
			return rc;
		else
			*val = rc << 8;
		break;
	case SEN_SAA7111A:
		*val = ov->brightness;
		break;
	default:
		PDEBUG(3, "Unsupported with this sensor");
		return -EPERM;
	}

	PDEBUG(3, "%d", *val);
	ov->brightness = *val;

	return 0;
}

/* -------------------------------------------------------------------------- */

/* Sets sensor's saturation (color intensity) setting to "val" */
static int
sensor_set_saturation(struct usb_ov511 *ov, unsigned short val)
{
	int rc;

	PDEBUG(3, "%d", val);

	if (ov->stop_during_set)
		if (ov51x_stop(ov) < 0)
			return -EIO;

	switch (ov->sensor) {
	case SEN_OV8610:
	case SEN_OV7610:
	case SEN_OV76BE:
	case SEN_OV6620:
	case SEN_OV6630:
		rc = i2c_w(ov, OV7610_REG_SAT, val >> 8);
		if (rc < 0)
			goto out;
		break;
	case SEN_OV7620:
//		/* Use UV gamma control instead. Bits 0 & 7 are reserved. */
//		rc = ov_i2c_write(ov->dev, 0x62, (val >> 9) & 0x7e);
//		if (rc < 0)
//			goto out;
		rc = i2c_w(ov, OV7610_REG_SAT, val >> 8);
		if (rc < 0)
			goto out;
		break;
	case SEN_OV7640:
		rc = i2c_w(ov, OV7610_REG_SAT, (val >> 8) & 0xf0);
		if (rc < 0)
			goto out;
	case SEN_SAA7111A:
		rc = i2c_w(ov, 0x0c, val >> 9);
		if (rc < 0)
			goto out;
		break;
	default:
		PDEBUG(3, "Unsupported with this sensor");
		rc = -EPERM;
		goto out;
	}

	rc = 0;		/* Success */
	ov->colour = val;
out:
	if (ov51x_restart(ov) < 0)
		return -EIO;

	return rc;
}

/* Gets sensor's saturation (color intensity) setting */
static int
sensor_get_saturation(struct usb_ov511 *ov, unsigned short *val)
{
	int rc;

	switch (ov->sensor) {
	case SEN_OV8610:
	case SEN_OV7610:
	case SEN_OV76BE:
	case SEN_OV6620:
	case SEN_OV6630:
	case SEN_OV7640:
		rc = i2c_r(ov, OV7610_REG_SAT);
		if (rc < 0)
			return rc;
		else
			*val = rc << 8;
		break;
	case SEN_OV7620:
//		/* Use UV gamma reg instead. Bits 0 & 7 are reserved. */
//		rc = i2c_r(ov, 0x62);
//		if (rc < 0)
//			return rc;
//		else
//			*val = (rc & 0x7e) << 9;
		rc = i2c_r(ov, OV7610_REG_SAT);
		if (rc < 0)
			return rc;
		else
			*val = rc << 8;
		break;
	case SEN_SAA7111A:
		*val = ov->colour;
		break;
	default:
		PDEBUG(3, "Unsupported with this sensor");
		return -EPERM;
	}

	PDEBUG(3, "%d", *val);
	ov->colour = *val;

	return 0;
}

/* -------------------------------------------------------------------------- */

/* Sets sensor's hue (red/blue balance) setting to "val" */
static int
sensor_set_hue(struct usb_ov511 *ov, unsigned short val)
{
	int rc;

	PDEBUG(3, "%d", val);

	if (ov->stop_during_set)
		if (ov51x_stop(ov) < 0)
			return -EIO;

	switch (ov->sensor) {
	case SEN_OV7610:
	case SEN_OV6620:
	case SEN_OV6630:
		rc = i2c_w(ov, OV7610_REG_RED, 0xFF - (val >> 8));
		if (rc < 0)
			goto out;

		rc = i2c_w(ov, OV7610_REG_BLUE, val >> 8);
		if (rc < 0)
			goto out;
		break;
	case SEN_OV8610:
	case SEN_OV7640:
		rc = i2c_w(ov, OV8610_REG_HUE, (val >> 11) | 0x20);
		if (rc < 0)
			goto out;
		break;
	case SEN_OV7620:
// Hue control is causing problems. I will enable it once it's fixed.
#if 0
		rc = i2c_w(ov, 0x7a, (unsigned char)(val >> 8) + 0xb);
		if (rc < 0)
			goto out;

		rc = i2c_w(ov, 0x79, (unsigned char)(val >> 8) + 0xb);
		if (rc < 0)
			goto out;
#endif
		break;
	case SEN_SAA7111A:
		rc = i2c_w(ov, 0x0d, (val + 32768) >> 8);
		if (rc < 0)
			goto out;
		break;
	default:
		PDEBUG(3, "Unsupported with this sensor");
		rc = -EPERM;
		goto out;
	}

	rc = 0;		/* Success */
	ov->hue = val;
out:
	if (ov51x_restart(ov) < 0)
		return -EIO;

	return rc;
}

/* Gets sensor's hue (red/blue balance) setting */
static int
sensor_get_hue(struct usb_ov511 *ov, unsigned short *val)
{
	int rc;

	switch (ov->sensor) {
	case SEN_OV7610:
	case SEN_OV6620:
	case SEN_OV6630:
		rc = i2c_r(ov, OV7610_REG_BLUE);
		if (rc < 0)
			return rc;
		else
			*val = rc << 8;
		break;
	case SEN_OV8610:
	case SEN_OV7640:
		rc = i2c_r(ov, OV8610_REG_HUE);
		if (rc < 0)
			return rc;
		else
			*val = (rc & 0x1f) << 11;
		break;
	case SEN_OV7620:
		rc = i2c_r(ov, 0x7a);
		if (rc < 0)
			return rc;
		else
			*val = rc << 8;
		break;
	case SEN_SAA7111A:
		*val = ov->hue;
		break;
	default:
		PDEBUG(3, "Unsupported with this sensor");
		return -EPERM;
	}

	PDEBUG(3, "%d", *val);
	ov->hue = *val;

	return 0;
}

/* -------------------------------------------------------------------------- */

static int
sensor_set_picture(struct usb_ov511 *ov, struct video_picture *p)
{
	int rc;

	PDEBUG(4, "sensor_set_picture");

	ov->whiteness = p->whiteness;

	/* Don't return error if a setting is unsupported, or rest of settings
         * will not be performed */

	rc = sensor_set_contrast(ov, p->contrast);
	if (FATAL_ERROR(rc))
		return rc;

	rc = sensor_set_brightness(ov, p->brightness);
	if (FATAL_ERROR(rc))
		return rc;

	rc = sensor_set_saturation(ov, p->colour);
	if (FATAL_ERROR(rc))
		return rc;

	rc = sensor_set_hue(ov, p->hue);
	if (FATAL_ERROR(rc))
		return rc;

	return 0;
}

static int
sensor_get_picture(struct usb_ov511 *ov, struct video_picture *p)
{
	int rc;

	PDEBUG(4, "sensor_get_picture");

	/* Don't return error if a setting is unsupported, or rest of settings
         * will not be performed */

	rc = sensor_get_contrast(ov, &(p->contrast));
	if (FATAL_ERROR(rc))
		return rc;

	rc = sensor_get_brightness(ov, &(p->brightness));
	if (FATAL_ERROR(rc))
		return rc;

	rc = sensor_get_saturation(ov, &(p->colour));
	if (FATAL_ERROR(rc))
		return rc;

	rc = sensor_get_hue(ov, &(p->hue));
	if (FATAL_ERROR(rc))
		return rc;

	p->whiteness = 105 << 8;

	return 0;
}

#if defined(CONFIG_VIDEO_PROC_FS)
// FIXME: Exposure range is only 0x00-0x7f in interlace mode
/* Sets current exposure for sensor. This only has an effect if auto-exposure
 * is off */
static inline int
sensor_set_exposure(struct usb_ov511 *ov, unsigned char val)
{
	int rc;

	PDEBUG(3, "%d", val);

	if (ov->stop_during_set)
		if (ov51x_stop(ov) < 0)
			return -EIO;

	switch (ov->sensor) {
	case SEN_OV6620:
	case SEN_OV6630:
	case SEN_OV7610:
	case SEN_OV7620:
	case SEN_OV7640:
	case SEN_OV76BE:
	case SEN_OV8600:
	case SEN_OV8610:
		rc = i2c_w(ov, 0x10, val);
		if (rc < 0)
			goto out;

		break;
	case SEN_SAA7111A:
		PDEBUG(3, "Unsupported with this sensor");
		return -EPERM;
	default:
		err("Sensor not supported for set_exposure");
		return -EINVAL;
	}

	rc = 0;		/* Success */
	ov->exposure = val;
out:
	if (ov51x_restart(ov) < 0)
		return -EIO;

	return rc;
}

/* Gets current exposure level from sensor, regardless of whether it is under
 * manual control. */
static int
sensor_get_exposure(struct usb_ov511 *ov, unsigned char *val)
{
	int rc;

	switch (ov->sensor) {
	case SEN_OV7610:
	case SEN_OV6620:
	case SEN_OV6630:
	case SEN_OV7620:
	case SEN_OV7640:
	case SEN_OV76BE:
	case SEN_OV8600:
	case SEN_OV8610:
		rc = i2c_r(ov, 0x10);
		if (rc < 0)
			return rc;
		else
			*val = rc;
		break;
	case SEN_SAA7111A:
		val = 0;
		PDEBUG(3, "Unsupported with this sensor");
		return -EPERM;
	default:
		err("Sensor not supported for get_exposure");
		return -EINVAL;
	}

	PDEBUG(3, "%d", *val);
	ov->exposure = *val;

	return 0;
}
#endif /* CONFIG_VIDEO_PROC_FS */

/* Turns on or off the LED. Only has an effect with OV511+/OV518(+)/OV519 */
static void
ov51x_led_control(struct usb_ov511 *ov, int on)
{
	PDEBUG(4, " (%s)", on ? "turn on" : "turn off");

	if (ov->bridge == BRG_OV511PLUS)
		reg_w(ov, R511_SYS_LED_CTL, on ? 1 : 0);
	else if (ov->bridge == BRG_OV519)
		reg_w_mask(ov, OV519_GPIO_DATA_OUT0, on ? 0x01 : 0x00, 0x01);
	else if (ov->bclass == BCL_OV518)
		reg_w_mask(ov, R518_GPIO_OUT, on ? 0x02 : 0x00, 0x02);

	return;
}

/* Matches the sensor's internal frame rate to the lighting frequency.
 * Valid frequencies are:
 *	50 - 50Hz, for European and Asian lighting
 *	60 - 60Hz, for American lighting
 *
 * Tested with: OV7610, OV7620, OV76BE, OV6620
 * Unsupported: KS0127, KS0127B, SAA7111A
 * Returns: 0 for success
 */
static int
sensor_set_light_freq(struct usb_ov511 *ov, int freq)
{
	int sixty;

	PDEBUG(4, "%d Hz", freq);

	if (freq == 60)
		sixty = 1;
	else if (freq == 50)
		sixty = 0;
	else {
		err("Invalid light freq (%d Hz)", freq);
		return -EINVAL;
	}

	switch (ov->sensor) {
	case SEN_OV8610:
		i2c_w(ov, 0x2b, sixty?0xcc:0xc0);
		i2c_w(ov, 0x2a, sixty?0x80:0xa0);
		break;
	case SEN_OV7610:
		i2c_w_mask(ov, 0x2a, sixty?0x00:0x80, 0x80);
		i2c_w(ov, 0x2b, sixty?0x00:0xac);
		i2c_w_mask(ov, 0x13, 0x10, 0x10);
		i2c_w_mask(ov, 0x13, 0x00, 0x10);
		break;
	case SEN_OV7620:
	case SEN_OV76BE:
	case SEN_OV8600:
		i2c_w_mask(ov, 0x2a, sixty?0x00:0x80, 0x80);
		i2c_w(ov, 0x2b, sixty?0x00:0xac);
		i2c_w_mask(ov, 0x76, 0x01, 0x01);
		break;
	case SEN_OV7640:
		i2c_w_mask(ov, 0x2a, sixty?0x00:0x80, 0x80);
		i2c_w(ov, 0x2b, sixty?0x00:0xac);
	case SEN_OV6620:
	case SEN_OV6630:
		i2c_w(ov, 0x2b, sixty?0xa8:0x28);
		i2c_w(ov, 0x2a, sixty?0x84:0xa4);
		break;
	case SEN_SAA7111A:
		PDEBUG(5, "Unsupported with this sensor");
		return -EPERM;
	default:
		err("Sensor not supported for set_light_freq");
		return -EINVAL;
	}

	ov->lightfreq = freq;

	return 0;
}

/* If enable is true, turn on the sensor's banding filter, otherwise turn it
 * off. This filter tries to reduce the pattern of horizontal light/dark bands
 * caused by some (usually fluorescent) lighting. The light frequency must be
 * set either before or after enabling it with ov51x_set_light_freq().
 *
 * Tested with: OV7610, OV7620, OV76BE, OV6620.
 * Unsupported: KS0127, KS0127B, SAA7111A
 * Returns: 0 for success
 */
static int
sensor_set_banding_filter(struct usb_ov511 *ov, int enable)
{
	int rc;

	PDEBUG(4, " (%s)", enable ? "turn on" : "turn off");

	if (ov->sensor == SEN_KS0127 || ov->sensor == SEN_KS0127B
		|| ov->sensor == SEN_SAA7111A) {
		PDEBUG(5, "Unsupported with this sensor");
		return -EPERM;
	}

	rc = i2c_w_mask(ov, 0x2d, enable?0x04:0x00, 0x04);
	if (rc < 0)
		return rc;

	ov->bandfilt = enable;

	return 0;
}

/* If enable is true, turn on the sensor's auto brightness control, otherwise
 * turn it off.
 *
 * Unsupported: KS0127, KS0127B, SAA7111A, OV7640
 * Returns: 0 for success
 */
static int
sensor_set_auto_brightness(struct usb_ov511 *ov, int enable)
{
	int rc;

	PDEBUG(4, " (%s)", enable ? "turn on" : "turn off");

	if (ov->sensor == SEN_KS0127 || ov->sensor == SEN_KS0127B
		|| ov->sensor == SEN_SAA7111A || ov->sensor == SEN_OV7640) {
		PDEBUG(5, "Unsupported with this sensor");
		return -EPERM;
	}

	rc = i2c_w_mask(ov, 0x2d, enable?0x10:0x00, 0x10);
	if (rc < 0)
		return rc;

	ov->auto_brt = enable;

	return 0;
}

/* If enable is true, turn on the sensor's auto exposure control, otherwise
 * turn it off.
 *
 * Unsupported: KS0127, KS0127B, SAA7111A
 * Returns: 0 for success
 */
static int
sensor_set_auto_exposure(struct usb_ov511 *ov, int enable)
{
	PDEBUG(4, " (%s)", enable ? "turn on" : "turn off");

	switch (ov->sensor) {
	case SEN_OV7610:
		i2c_w_mask(ov, 0x29, enable?0x00:0x80, 0x80);
		break;
	case SEN_OV6620:
	case SEN_OV7620:
	case SEN_OV7640:
	case SEN_OV76BE:
	case SEN_OV8600:
		i2c_w_mask(ov, 0x13, enable?0x01:0x00, 0x01);
		break;
	case SEN_OV6630:
	case SEN_OV8610:
		i2c_w_mask(ov, 0x28, enable?0x00:0x10, 0x10);
		break;
	case SEN_SAA7111A:
		PDEBUG(5, "Unsupported with this sensor");
		return -EPERM;
	default:
		err("Sensor not supported for set_auto_exposure");
		return -EINVAL;
	}

	ov->auto_exp = enable;

	return 0;
}

/* If enable is true, turn on the sensor's auto gain control, otherwise
 * turn it off.
 *
 * Returns: 0 for success
 */
static int
sensor_set_auto_gain(struct usb_ov511 *ov, int enable)
{
	PDEBUG(4, " (%s)", enable ? "turn on" : "turn off");

	switch (ov->sensor) {
	case SEN_OV7640:
		i2c_w_mask(ov, 0x13, enable?0x02:0x00, 0x02);
		break;
	default:
		PDEBUG(5, "Unsupported with this sensor");
		return -EPERM;
//	default:
//		err("Sensor not supported for set_auto_gain");
//		return -EINVAL;
	}

	ov->auto_gain = enable;

	return 0;
}

/* Modifies the sensor's exposure algorithm to allow proper exposure of objects
 * that are illuminated from behind.
 *
 * Tested with: OV6620, OV7620
 * Unsupported: OV7610, OV7640, OV76BE, KS0127, KS0127B, SAA7111A
 * Returns: 0 for success
 */
static int
sensor_set_backlight(struct usb_ov511 *ov, int enable)
{
	PDEBUG(4, " (%s)", enable ? "turn on" : "turn off");

	switch (ov->sensor) {
	case SEN_OV8610:
		// change AEC/AGC Reference level
		i2c_w_mask(ov, 0x68, enable?0xef:0xcf, 0xff);
		// select central 1/4 image to calculate AEC/AGC
		i2c_w_mask(ov, 0x29, enable?0x08:0x00, 0x08);
		// increase gain 3dB
		i2c_w_mask(ov, 0x28, enable?0x02:0x00, 0x02);
		break;
	case SEN_OV7620:
	case SEN_OV8600:
		// change AEC/AGC Reference level
		i2c_w_mask(ov, 0x68, enable?0xe0:0xc0, 0xe0);
		// select central 1/4 image to calculate AEC/AGC
		i2c_w_mask(ov, 0x29, enable?0x08:0x00, 0x08);
		// increase gain 3dB
		i2c_w_mask(ov, 0x28, enable?0x02:0x00, 0x02);
		break;
	case SEN_OV6620:
		i2c_w_mask(ov, 0x4e, enable?0xe0:0xc0, 0xe0);
		i2c_w_mask(ov, 0x29, enable?0x08:0x00, 0x08);
		i2c_w_mask(ov, 0x0e, enable?0x80:0x00, 0x80);
		break;
	case SEN_OV6630:
		i2c_w_mask(ov, 0x4e, enable?0x80:0x60, 0xe0);
		i2c_w_mask(ov, 0x29, enable?0x08:0x00, 0x08);
		i2c_w_mask(ov, 0x28, enable?0x02:0x00, 0x02);
		break;
	case SEN_OV7610:
	case SEN_OV7640:
	case SEN_OV76BE:
	case SEN_SAA7111A:
		PDEBUG(5, "Unsupported with this sensor");
		return -EPERM;
	default:
		err("Sensor not supported for set_backlight");
		return -EINVAL;
	}

	ov->backlight = enable;

	return 0;
}

static int
sensor_set_mirror(struct usb_ov511 *ov, int enable)
{
	PDEBUG(4, " (%s)", enable ? "turn on" : "turn off");

	switch (ov->sensor) {
	case SEN_OV6620:
	case SEN_OV6630:
	case SEN_OV7610:
	case SEN_OV7620:
	case SEN_OV76BE:
	case SEN_OV7640:
	case SEN_OV8600:
	case SEN_OV8610:
		i2c_w_mask(ov, 0x12, enable?0x40:0x00, 0x40);
		break;
	case SEN_SAA7111A:
		PDEBUG(5, "Unsupported with this sensor");
		return -EPERM;
	default:
		err("Sensor not supported for set_mirror");
		return -EINVAL;
	}

	ov->mirror = enable;

	return 0;
}

/* Returns number of bits per pixel (regardless of where they are located;
 * planar or not), or zero for unsupported format.
 */
static inline int
get_depth(int palette)
{
	switch (palette) {
	case VIDEO_PALETTE_GREY:    return 8;
	case VIDEO_PALETTE_YUV420:  return 12;
	case VIDEO_PALETTE_YUV420P: return 12; /* Planar */
#ifdef OV511_ALLOW_CONVERSION
	case VIDEO_PALETTE_RGB565:  return 16;
	case VIDEO_PALETTE_RGB24:   return 24;
	case VIDEO_PALETTE_YUV422:  return 16;
	case VIDEO_PALETTE_YUYV:    return 16;
	case VIDEO_PALETTE_YUV422P: return 16; /* Planar */
#endif
	default:		    return 0;  /* Invalid format */
	}
}

/* Bytes per frame. Used by read(). Return of 0 indicates error */
static inline long int
get_frame_length(struct usb_ov511 *ov, struct ov511_frame *frame)
{
	if (!frame) {
		return 0;
	} else {
		if (ov->bclass == BCL_OV519) {
			return (frame->bytes_recvd);
		} else {
			return ((frame->width * frame->height
				 * get_depth(frame->format)) >> 3);
		}
	}
}

static int
mode_init_ov_sensor_regs(struct usb_ov511 *ov, struct ovsensor_window *win)
{
	int qvga = win->quarter;

	/******** Mode (VGA/QVGA) and sensor specific regs ********/

	switch (ov->sensor) {
	case SEN_OV8610:
		// For OV8610 qvga means qsvga
		i2c_setbit(ov, OV7610_REG_COM_C, qvga?1:0, 5);
	// FIXME: Does this improve the image quality or frame rate?
#if 0
		i2c_w_mask(ov, 0x28, qvga?0x00:0x20, 0x20);
		i2c_w(ov, 0x24, 0x10);
		i2c_w(ov, 0x25, qvga?0x40:0x8a);
		i2c_w(ov, 0x2f, qvga?0x30:0xb0);
		i2c_w(ov, 0x35, qvga?0x1c:0x9c);
#endif
		break;
	case SEN_OV7610:
		i2c_w_mask(ov, 0x14, qvga?0x20:0x00, 0x20);
// FIXME: Does this improve the image quality or frame rate?
#if 0
		i2c_w_mask(ov, 0x28, qvga?0x00:0x20, 0x20);
		i2c_w(ov, 0x24, 0x10);
		i2c_w(ov, 0x25, qvga?0x40:0x8a);
		i2c_w(ov, 0x2f, qvga?0x30:0xb0);
		i2c_w(ov, 0x35, qvga?0x1c:0x9c);
#endif
		break;
	case SEN_OV7620:
//		i2c_w(ov, 0x2b, 0x00);
		i2c_w_mask(ov, 0x14, qvga?0x20:0x00, 0x20);
		i2c_w_mask(ov, 0x28, qvga?0x00:0x20, 0x20);
		i2c_w(ov, 0x24, qvga?0x20:0x3a);
		i2c_w(ov, 0x25, qvga?0x30:0x60);
		i2c_w_mask(ov, 0x2d, qvga?0x40:0x00, 0x40);
		i2c_w_mask(ov, 0x67, qvga?0xf0:0x90, 0xf0);
		i2c_w_mask(ov, 0x74, qvga?0x20:0x00, 0x20);
		break;
	case SEN_OV76BE:
//		i2c_w(ov, 0x2b, 0x00);
		i2c_w_mask(ov, 0x14, qvga?0x20:0x00, 0x20);
// FIXME: Enable this once 7620AE uses 7620 initial settings
#if 0
		i2c_w_mask(ov, 0x28, qvga?0x00:0x20, 0x20);
		i2c_w(ov, 0x24, qvga?0x20:0x3a);
		i2c_w(ov, 0x25, qvga?0x30:0x60);
		i2c_w_mask(ov, 0x2d, qvga?0x40:0x00, 0x40);
		i2c_w_mask(ov, 0x67, qvga?0xb0:0x90, 0xf0);
		i2c_w_mask(ov, 0x74, qvga?0x20:0x00, 0x20);
#endif
		break;
	case SEN_OV7640:
//		i2c_w(ov, 0x2b, 0x00);
		i2c_w_mask(ov, 0x14, qvga?0x20:0x00, 0x20);
		i2c_w_mask(ov, 0x28, qvga?0x00:0x20, 0x20);
//		i2c_w(ov, 0x24, qvga?0x20:0x3a);
//		i2c_w(ov, 0x25, qvga?0x30:0x60);
//		i2c_w_mask(ov, 0x2d, qvga?0x40:0x00, 0x40);
//		i2c_w_mask(ov, 0x67, qvga?0xf0:0x90, 0xf0);
//		i2c_w_mask(ov, 0x74, qvga?0x20:0x00, 0x20);
		break;
	case SEN_OV6620:
		i2c_w_mask(ov, 0x14, qvga?0x20:0x00, 0x20);
		break;
	case SEN_OV6630:
		i2c_w_mask(ov, 0x14, qvga?0x20:0x00, 0x20);
		break;
	default:
		return -EINVAL;
	}

	/******** Palette-specific regs ********/

	if (win->format == VIDEO_PALETTE_GREY) {
		if (ov->sensor == SEN_OV7610 || ov->sensor == SEN_OV76BE) {
			/* these aren't valid on the OV6620/OV7620/6630? */
			i2c_w_mask(ov, 0x0e, 0x40, 0x40);
		}

		/* OV6630 default reg 0x13 value is always right */
		/* OV7640 is 8-bit only */
		if (ov->sensor != SEN_OV6630 && ov->sensor != SEN_OV7640)
			i2c_w_mask(ov, 0x13, 0x20, 0x20);
		else
			return -EINVAL;	/* No OV6630 greyscale support yet */
	} else {
		if (ov->sensor == SEN_OV7610 || ov->sensor == SEN_OV76BE) {
			/* not valid on the OV6620/OV7620/6630? */
			i2c_w_mask(ov, 0x0e, 0x00, 0x40);
		}

		/* The OV518 needs special treatment. Although both the OV518
		 * and the OV6630 support a 16-bit video bus, only the 8 bit Y
		 * bus is actually used. The UV bus is tied to ground.
		 * Therefore, the OV6630 needs to be in 8-bit multiplexed
		 * output mode */

		/* OV7640 is 8-bit only */

		if (ov->sensor != SEN_OV6630 && ov->sensor != SEN_OV7640)
			i2c_w_mask(ov, 0x13, 0x00, 0x20);
	}

	/******** Clock programming ********/

	/* The OV6620 needs special handling. This prevents the
	 * severe banding that normally occurs */
	if (ov->sensor == SEN_OV6620) {
		/* Clock down */

		i2c_w(ov, 0x2a, 0x04);
		i2c_w(ov, 0x11, win->clockdiv);
		i2c_w(ov, 0x2a, 0x84);
		/* This next setting is critical. It seems to improve
		 * the gain or the contrast. The "reserved" bits seem
		 * to have some effect in this case. */
		i2c_w(ov, 0x2d, 0x85);
	} else if (win->clockdiv >= 0) {
		i2c_w(ov, 0x11, win->clockdiv);
	}

	/******** Special Features ********/

	if (framedrop >= 0 && ov->sensor != SEN_OV7640)
		i2c_w(ov, 0x16, framedrop);

	/* Test Pattern */
	if (ov->sensor != SEN_OV7640)
		i2c_w_mask(ov, 0x12, (testpat?0x02:0x00), 0x02);

	/* Enable auto white balance */
	i2c_w_mask(ov, 0x12, 0x04, 0x04);

	// This will go away as soon as ov51x_mode_init_sensor_regs()
	// is fully tested.
	/* 7620/6620/6630? don't have register 0x35, so play it safe */
	if (ov->sensor == SEN_OV7610 || ov->sensor == SEN_OV76BE) {
		if (win->width == 640 && win->height == 480)
			i2c_w(ov, 0x35, 0x9e);
		else
			i2c_w(ov, 0x35, 0x1e);
	}

	return 0;
}

static int
set_ov_sensor_window(struct usb_ov511 *ov, struct ovsensor_window *win)
{
	int hwsbase, hwebase, vwsbase, vwebase, hwscale, vwscale, ret;

	/* The different sensor ICs handle setting up of window differently.
	 * IF YOU SET IT WRONG, YOU WILL GET ALL ZERO ISOC DATA FROM OV51x!!! */
	switch (ov->sensor) {
	case SEN_OV8610:
		hwsbase = 0x1e;
		hwebase = 0x1e;
		vwsbase = 0x02;
		vwebase = 0x02;
		break;
	case SEN_OV7610:
	case SEN_OV76BE:
		hwsbase = 0x38;
		hwebase = 0x3a;
		vwsbase = vwebase = 0x05;
		break;
	case SEN_OV6620:
	case SEN_OV6630:
		hwsbase = 0x38;
		hwebase = 0x3a;
		vwsbase = 0x05;
		vwebase = 0x06;
		break;
	case SEN_OV7620:
		hwsbase = 0x2f;		/* From 7620.SET (spec is wrong) */
		hwebase = 0x2f;
		vwsbase = vwebase = 0x05;
		break;
	case SEN_OV7640:
		hwsbase = 0x1a;
		hwebase = 0x1a;
		vwsbase = vwebase = 0x03;
		break;
	default:
		return -EINVAL;
	}

	switch (ov->sensor) {
		case SEN_OV6620:
		case SEN_OV6630:
			if (win->quarter) {	/* QCIF */
				hwscale = 0;
				vwscale = 0;
			} else {		/* CIF */
				hwscale = 1;
				vwscale = 1;  /* The datasheet says 0; it's wrong */
			}
			break;
		case SEN_OV8610:
			if (win->quarter) {	/* QSVGA */
				hwscale = 1;
				vwscale = 1;
			} else {		/* SVGA */
				hwscale = 2;
				vwscale = 2;
			}
			break;
		default: //SEN_OV7xx0
			if (win->quarter) {	/* QVGA */
				hwscale = 1;
				vwscale = 0;
			} else {		/* VGA */
				hwscale = 2;
				vwscale = 1;
			}
	}

	ret = mode_init_ov_sensor_regs(ov, win);
	if (ret < 0)
		return ret;

	if (ov->sensor == SEN_OV8610) {
		i2c_w_mask(ov, 0x2d, 0x05, 0x40);  /* old 0x95, new 0x05 from windrv 090403 *//* bits 5-7: reserved */
		i2c_w_mask(ov, 0x28, 0x20, 0x20); /* bit 5: progressive mode on */
	}

	i2c_w(ov, 0x17, hwsbase + (win->x >> hwscale));
	i2c_w(ov, 0x18, hwebase + ((win->x + win->width) >> hwscale));
	i2c_w(ov, 0x19, vwsbase + (win->y >> vwscale));
	i2c_w(ov, 0x1a, vwebase + ((win->y + win->height) >> vwscale));

#ifdef OV511_DEBUG
	if (dump_sensor)
		dump_i2c_regs(ov);
#endif

	return 0;
}

static int
ov_sensor_mode_setup(struct usb_ov511 *ov,
		     int width, int height, int mode, int sub_flag)
{
	struct ovsensor_window win;
	int half_w = ov->maxwidth / 2;
	int half_h = ov->maxheight / 2;

	win.format = mode;

	/* Unless subcapture is enabled, center the image window and downsample
	 * if possible to increase the field of view */
	if (sub_flag) {
		win.x = ov->subx;
		win.y = ov->suby;
		win.width = ov->subw;
		win.height = ov->subh;
		win.quarter = 0;
	} else {
		/* NOTE: OV518(+) and OV519 does downsampling on its own */
		if ((width > half_w && height > half_h)
		    || (ov->bclass == BCL_OV518)
			|| (ov->bclass == BCL_OV519)) {
			win.width = ov->maxwidth;
			win.height = ov->maxheight;
			win.quarter = 0;
		} else if (width > half_w || height > half_h) {
			err("Illegal dimensions");
			return -EINVAL;
		} else {
			win.width = half_w;
			win.height = half_h;
			win.quarter = 1;
		}

		/* Center it */
		win.x = (win.width - width) / 2;
		win.y = (win.height - height) / 2;
	}

	if (clockdiv >= 0) {
		/* Manual override */
		win.clockdiv = clockdiv;
	} else if (ov->bridge == BRG_OV518) {
		/* OV518 controls the clock externally */
		win.clockdiv = 0;
	} else if (ov->bridge == BRG_OV518PLUS) {
		/* OV518+ controls the clock externally */
		win.clockdiv = 1;
	} else if (ov->bridge == BRG_OV519) {
		/* Clock is determined by OV519 frame rate code */
		win.clockdiv = ov->clockdiv;
	} else if (ov->compress) {
		/* Use the highest possible rate, to maximize FPS */
		switch (ov->sensor) {
		case SEN_OV6620:
			/* ...except with this sensor, which doesn't like
			 * higher rates yet */
			win.clockdiv = 3;
			break;
		case SEN_OV6630:
			win.clockdiv = 0;
			break;
		case SEN_OV76BE:
		case SEN_OV7610:
		case SEN_OV7620:
			win.clockdiv = 1;
			break;
		case SEN_OV8610:
			win.clockdiv = 0;
			break;
		default:
			err("Invalid sensor");
			return -EINVAL;
		}
	} else {
		switch (ov->sensor) {
		case SEN_OV6620:
		case SEN_OV6630:
			win.clockdiv = 3;
			break;
		case SEN_OV76BE:
		case SEN_OV7610:
		case SEN_OV7620:
			/* Use slowest possible clock without sacrificing
			 * frame rate */
			win.clockdiv = ((sub_flag ? ov->subw * ov->subh
			      : width * height)
			     * (win.format == VIDEO_PALETTE_GREY ? 2 : 3) / 2)
			    / 66000;
			break;
		default:
			err("Invalid sensor");
			return -EINVAL;
		}
	}

	PDEBUG(4, "Setting clock divider to %d", win.clockdiv);

	return set_ov_sensor_window(ov, &win);
}

/* Set up the OV511/OV511+ with the given image parameters.
 *
 * Do not put any sensor-specific code in here (including I2C I/O functions)
 */
static int
ov511_mode_init_regs(struct usb_ov511 *ov,
		     int width, int height, int mode, int sub_flag)
{
	int hsegs, vsegs;

	if (sub_flag) {
		width = ov->subw;
		height = ov->subh;
	}

	PDEBUG(3, "width:%d, height:%d, mode:%d, sub:%d",
	       width, height, mode, sub_flag);

	// FIXME: This should be moved to a 7111a-specific function once
	// subcapture is dealt with properly
	if (ov->sensor == SEN_SAA7111A) {
		if (width == 320 && height == 240) {
			/* No need to do anything special */
		} else if (width == 640 && height == 480) {
			/* Set the OV511 up as 320x480, but keep the
			 * V4L resolution as 640x480 */
			width = 320;
		} else {
			err("SAA7111A only allows 320x240 or 640x480");
			return -EINVAL;
		}
	}

	/* Make sure width and height are a multiple of 8 */
	if (width % 8 || height % 8) {
		err("Invalid size (%d, %d) (mode = %d)", width, height, mode);
		return -EINVAL;
	}

	if (width < ov->minwidth || height < ov->minheight) {
		err("Requested dimensions are too small");
		return -EINVAL;
	}

	if (ov51x_stop(ov) < 0)
		return -EIO;

	if (mode == VIDEO_PALETTE_GREY) {
		reg_w(ov, R511_CAM_UV_EN, 0x00);
		reg_w(ov, R511_SNAP_UV_EN, 0x00);
		reg_w(ov, R511_SNAP_OPTS, 0x01);
	} else {
		reg_w(ov, R511_CAM_UV_EN, 0x01);
		reg_w(ov, R511_SNAP_UV_EN, 0x01);
		reg_w(ov, R511_SNAP_OPTS, 0x03);
	}

	/* Here I'm assuming that snapshot size == image size.
	 * I hope that's always true. --claudio
	 */
	hsegs = (width >> 3) - 1;
	vsegs = (height >> 3) - 1;

	reg_w(ov, R511_CAM_PXCNT, hsegs);
	reg_w(ov, R511_CAM_LNCNT, vsegs);
	reg_w(ov, R511_CAM_PXDIV, 0x00);
	reg_w(ov, R511_CAM_LNDIV, 0x00);

	/* YUV420, low pass filter on */
	reg_w(ov, R511_CAM_OPTS, 0x03);

	/* Snapshot additions */
	reg_w(ov, R511_SNAP_PXCNT, hsegs);
	reg_w(ov, R511_SNAP_LNCNT, vsegs);
	reg_w(ov, R511_SNAP_PXDIV, 0x00);
	reg_w(ov, R511_SNAP_LNDIV, 0x00);

	if (ov->compress) {
		/* Enable Y and UV quantization and compression */
		reg_w(ov, R511_COMP_EN, 0x07);
		reg_w(ov, R511_COMP_LUT_EN, 0x03);
		ov51x_reset(ov, OV511_RESET_OMNICE);
	}

	if (ov51x_restart(ov) < 0)
		return -EIO;

	return 0;
}

/* Sets up the OV518/OV518+ with the given image parameters
 *
 * OV518 needs a completely different approach, until we can figure out what
 * the individual registers do. Also, only 15 FPS is supported now.
 *
 * Do not put any sensor-specific code in here (including I2C I/O functions)
 */
static int
ov518_mode_init_regs(struct usb_ov511 *ov,
		     int width, int height, int mode, int sub_flag)
{
	int hsegs, vsegs, hi_res;

	if (sub_flag) {
		width = ov->subw;
		height = ov->subh;
	}

	PDEBUG(3, "width:%d, height:%d, mode:%d, sub:%d",
	       width, height, mode, sub_flag);

	if (width % 16 || height % 8) {
		err("Invalid size (%d, %d)", width, height);
		return -EINVAL;
	}

	if (width < ov->minwidth || height < ov->minheight) {
		err("Requested dimensions are too small");
		return -EINVAL;
	}

	if (width >= 320 && height >= 240) {
		hi_res = 1;
	} else if (width >= 320 || height >= 240) {
		err("Invalid width/height combination (%d, %d)", width, height);
		return -EINVAL;
	} else {
		hi_res = 0;
	}

	if (ov51x_stop(ov) < 0)
		return -EIO;

	/******** Set the mode ********/

	reg_w(ov, 0x2b, 0);
	reg_w(ov, 0x2c, 0);
	reg_w(ov, 0x2d, 0);
	reg_w(ov, 0x2e, 0);
	reg_w(ov, 0x3b, 0);
	reg_w(ov, 0x3c, 0);
	reg_w(ov, 0x3d, 0);
	reg_w(ov, 0x3e, 0);

	if (ov->bridge == BRG_OV518 && ov518_color) {
	 	if (mode == VIDEO_PALETTE_GREY) {
			/* Set 16-bit input format (UV data are ignored) */
			reg_w_mask(ov, 0x20, 0x00, 0x08);

			/* Set 8-bit (4:0:0) output format */
			reg_w_mask(ov, 0x28, 0x00, 0xf0);
			reg_w_mask(ov, 0x38, 0x00, 0xf0);
		} else {
			/* Set 8-bit (YVYU) input format */
			reg_w_mask(ov, 0x20, 0x08, 0x08);

			/* Set 12-bit (4:2:0) output format */
			reg_w_mask(ov, 0x28, 0x80, 0xf0);
			reg_w_mask(ov, 0x38, 0x80, 0xf0);
		}
	} else {
		reg_w(ov, 0x28, (mode == VIDEO_PALETTE_GREY) ? 0x00:0x80);
		reg_w(ov, 0x38, (mode == VIDEO_PALETTE_GREY) ? 0x00:0x80);
	}

	hsegs = width / 16;
	vsegs = height / 4;

	reg_w(ov, 0x29, hsegs);
	reg_w(ov, 0x2a, vsegs);

	reg_w(ov, 0x39, hsegs);
	reg_w(ov, 0x3a, vsegs);

	/* Windows driver does this here; who knows why */
	reg_w(ov, 0x2f, 0x80);

	/******** Set the framerate (to 30 FPS) ********/

	/* Mode independent, but framerate dependent, regs */
	reg_w(ov, 0x51, 0x04);	/* Clock divider; lower==faster */
	reg_w(ov, 0x22, 0x18);
	reg_w(ov, 0x23, 0xff);

	if (ov->bridge == BRG_OV518PLUS)
		reg_w(ov, 0x21, 0x19);
	else
		reg_w(ov, 0x71, 0x17);	/* Compression-related? */

	// FIXME: Sensor-specific
	/* Bit 5 is what matters here. Of course, it is "reserved" */
	i2c_w(ov, 0x54, 0x23);

	reg_w(ov, 0x2f, 0x80);

	if (ov->bridge == BRG_OV518PLUS) {
		reg_w(ov, 0x24, 0x94);
		reg_w(ov, 0x25, 0x90);
		ov518_reg_w32(ov, 0xc4,    400, 2);	/* 190h   */
		ov518_reg_w32(ov, 0xc6,    540, 2);	/* 21ch   */
		ov518_reg_w32(ov, 0xc7,    540, 2);	/* 21ch   */
		ov518_reg_w32(ov, 0xc8,    108, 2);	/* 6ch    */
		ov518_reg_w32(ov, 0xca, 131098, 3);	/* 2001ah */
		ov518_reg_w32(ov, 0xcb,    532, 2);	/* 214h   */
		ov518_reg_w32(ov, 0xcc,   2400, 2);	/* 960h   */
		ov518_reg_w32(ov, 0xcd,     32, 2);	/* 20h    */
		ov518_reg_w32(ov, 0xce,    608, 2);	/* 260h   */
	} else {
		reg_w(ov, 0x24, 0x9f);
		reg_w(ov, 0x25, 0x90);
		ov518_reg_w32(ov, 0xc4,    400, 2);	/* 190h   */
		ov518_reg_w32(ov, 0xc6,    381, 2);	/* 17dh   */
		ov518_reg_w32(ov, 0xc7,    381, 2);	/* 17dh   */
		ov518_reg_w32(ov, 0xc8,    128, 2);	/* 80h    */
		ov518_reg_w32(ov, 0xca, 183331, 3);	/* 2cc23h */
		ov518_reg_w32(ov, 0xcb,    746, 2);	/* 2eah   */
		ov518_reg_w32(ov, 0xcc,   1750, 2);	/* 6d6h   */
		ov518_reg_w32(ov, 0xcd,     45, 2);	/* 2dh    */
		ov518_reg_w32(ov, 0xce,    851, 2);	/* 353h   */
	}

	reg_w(ov, 0x2f, 0x80);

	if (ov51x_restart(ov) < 0)
		return -EIO;

	/* Reset it just for good measure */
	if (ov51x_reset(ov, OV511_RESET_NOREGS) < 0)
		return -EIO;

	return 0;
}

/* Sets up the OV519 with the given image parameters
 *
 * OV519 needs a completely different approach, until we can figure out what
 * the individual registers do.
 *
 * Do not put any sensor-specific code in here (including I2C I/O functions)
 */
static int
ov519_mode_init_regs(struct usb_ov511 *ov,
		     int width, int height, int mode, int sub_flag)
{
	static struct ov511_regvals regvals_mode_init_519[] = {
		{ OV511_REG_BUS, 0x5d,	0x03 }, /* Turn off suspend mode */
		{ OV511_REG_BUS, 0x53,	0x9f }, /* was 9b in 1.65-1.08 */
		{ OV511_REG_BUS, 0x54,	0x0f }, /* bit2 (jpeg enable) */
		{ OV511_REG_BUS, 0xa2,	0x20 }, /* a2-a5 are undocumented */
		{ OV511_REG_BUS, 0xa3,	0x18 },
		{ OV511_REG_BUS, 0xa4,	0x04 },
		{ OV511_REG_BUS, 0xa5,	0x28 },
		{ OV511_REG_BUS, 0x37,	0x00 },	/* SetUsbInit */
		{ OV511_REG_BUS, 0x55,	0x02 }, /* 4.096 Mhz audio clock */
		/* Enable both fields, YUV Input, disable defect comp (why?) */
		{ OV511_REG_BUS, 0x22,	0x1d },
		{ OV511_REG_BUS, 0x17,	0x50 }, /* undocumented */
		{ OV511_REG_BUS, 0x37,	0x00 }, /* undocumented */
		{ OV511_REG_BUS, 0x40,	0xff }, /* I2C timeout counter */
		{ OV511_REG_BUS, 0x46,	0x00 }, /* I2C clock prescaler */
		{ OV511_REG_BUS, 0x59,	0x04 },	/* new from windrv 090403 */
		{ OV511_REG_BUS, 0xff,	0x00 }, /* undocumented */
		/* windows reads 0x55 at this point, why? */
		{ OV511_DONE_BUS, 0x0, 0x00},
	};

//	int hi_res;

	if (sub_flag) {
		width = ov->subw;
		height = ov->subh;
	}

	PDEBUG(3, "width:%d, height:%d, mode:%d, sub:%d",
	       width, height, mode, sub_flag);

	if ((width % 16) || (height % 8)) {
		err("Invalid size (%d, %d)", width, height);
		return -EINVAL;
	}

	if (width < ov->minwidth || height < ov->minheight) {
		err("Requested dimensions are too small %dx%d", width, height);
		return -EINVAL;
	}

//	if (width >= 800 && height >= 600) {
//		hi_res = 1;
//	} else
	if (width > ov->maxwidth || height > ov->maxheight) {
		err("Requested dimensions are too big %dx%d", width, height);
		return -EINVAL;
	}
//	 else {
//		hi_res = 0;
//	}

	if (ov51x_stop(ov) < 0)
		return -EIO;

	/******** Set the mode ********/

	if (write_regvals(ov, regvals_mode_init_519))
		return -EIO;

	if (ov->sensor == SEN_OV7640) {
		/* Select 8-bit input mode */
		reg_w_mask(ov, OV519_CAM_DFR, 0x10, 0x10);
	}

	reg_w(ov, OV519_CAM_H_SIZE,	width>>4);
	reg_w(ov, OV519_CAM_V_SIZE,	height>>3);
	reg_w(ov, OV519_CAM_X_OFFSETL,	0x00);
	reg_w(ov, OV519_CAM_X_OFFSETH,	0x00);
	reg_w(ov, OV519_CAM_Y_OFFSETL,	0x00);
	reg_w(ov, OV519_CAM_Y_OFFSETH,	0x00);
	reg_w(ov, OV519_CAM_DIVIDER,	0x00);
	reg_w(ov, OV519_CAM_FORMAT,	0x03); /* YUV422 */
	reg_w(ov, 0x26,			0x00); /* Undocumented */


	/******** Set the framerate ********/
	if (framerate > 0) {
		ov->framerate = framerate;
	}

// FIXME: These are only valid at the max resolution.
	if (ov->sensor == SEN_OV7640) {
		switch (ov->framerate) {
		case 30:
			reg_w(ov, 0xa4, 0x0c);
			reg_w(ov, 0x23, 0xff);
			ov->clockdiv = 0;
			break;
		case 25:
			reg_w(ov, 0xa4, 0x0c);
			reg_w(ov, 0x23, 0x1f);
			ov->clockdiv = 0;
			break;
		case 20:
			reg_w(ov, 0xa4, 0x0c);
			reg_w(ov, 0x23, 0x1b);
			ov->clockdiv = 0;
			break;
		case 15:
			reg_w(ov, 0xa4, 0x04);
			reg_w(ov, 0x23, 0xff);
			ov->clockdiv = 1;
			break;
		case 10:
			reg_w(ov, 0xa4, 0x04);
			reg_w(ov, 0x23, 0x1f);
			ov->clockdiv = 1;
			break;
		case 5:
			reg_w(ov, 0xa4, 0x04);
			reg_w(ov, 0x23, 0x1b);
			ov->clockdiv = 1;
			break;
		default:	// 30 fps
			reg_w(ov, 0xa4, 0x0c);
			reg_w(ov, 0x23, 0xff);
			ov->clockdiv = 0;
		}
	} else if (ov->sensor == SEN_OV8610) {
		switch (ov->framerate) {
		case 15:
			reg_w(ov, 0xa4, 0x06);
			reg_w(ov, 0x23, 0xff);
			break;
		case 10:
			reg_w(ov, 0xa4, 0x06);
			reg_w(ov, 0x23, 0x1f);
			break;
		case 5:
			reg_w(ov, 0xa4, 0x06);
			reg_w(ov, 0x23, 0x1b);
			break;
		default:	// 15 fps
			reg_w(ov, 0xa4, 0x06);
			reg_w(ov, 0x23, 0xff);
		}

		ov->clockdiv = 0;
	} else {
		err("Sensor not supported for OV519!");
		return -EINVAL;
	}

	if (ov51x_restart(ov) < 0)
		return -EIO;

	/* Reset it just for good measure */
	if (ov51x_reset(ov, OV511_RESET_NOREGS) < 0)
		return -EIO;

	return 0;
}

/* This is a wrapper around the OV511, OV518, and sensor specific functions */
static int
mode_init_regs(struct usb_ov511 *ov,
	       int width, int height, int mode, int sub_flag)
{
	int rc = 0;

	if (!ov || !ov->dev)
		return -EFAULT;

	switch (ov->bclass) {
		case BCL_OV511:
			rc = ov511_mode_init_regs(ov, width, height, mode, sub_flag);
			break;
		case BCL_OV518:
			rc = ov518_mode_init_regs(ov, width, height, mode, sub_flag);
			break;
		case BCL_OV519:
			rc = ov519_mode_init_regs(ov, width, height, mode, sub_flag);
			break;
		default:
			err("mode_init_regs: Invalid bridge class");
			rc = -EINVAL;
	}

	if (FATAL_ERROR(rc))
		return rc;

	switch (ov->sensor) {
		case SEN_OV8610:
		case SEN_OV7610:
		case SEN_OV7620:
		case SEN_OV76BE:
		case SEN_OV7640:
		case SEN_OV8600:
		case SEN_OV6620:
		case SEN_OV6630:
			rc = ov_sensor_mode_setup(ov, width, height, mode, sub_flag);
			break;

		case SEN_SAA7111A:
//			rc = mode_init_saa_sensor_regs(ov, width, height, mode,
//				       sub_flag);

			PDEBUG(1, "SAA status = 0x%02X", i2c_r(ov, 0x1f));
			break;
		default:
			rc = -EINVAL;
	}

	if (FATAL_ERROR(rc))
		return rc;

	/* Sensor-independent settings */
	rc = sensor_set_auto_brightness(ov, ov->auto_brt);
	if (FATAL_ERROR(rc))
		return rc;

	rc = sensor_set_auto_exposure(ov, ov->auto_exp);
	if (FATAL_ERROR(rc))
		return rc;

	rc = sensor_set_auto_gain(ov, ov->auto_gain);
	if (FATAL_ERROR(rc))
		return rc;

	rc = sensor_set_banding_filter(ov, bandingfilter);
	if (FATAL_ERROR(rc))
		return rc;

	if (ov->lightfreq) {
		rc = sensor_set_light_freq(ov, lightfreq);
		if (FATAL_ERROR(rc))
			return rc;
	}

	rc = sensor_set_backlight(ov, ov->backlight);
	if (FATAL_ERROR(rc))
		return rc;

	rc = sensor_set_mirror(ov, ov->mirror);
	if (FATAL_ERROR(rc))
		return rc;

	return 0;
}

/* This sets the default image parameters. This is useful for apps that use
 * read() and do not set these.
 */
static int
ov51x_set_default_params(struct usb_ov511 *ov)
{
	int i;

	/* Set default sizes in case IOCTL (VIDIOCMCAPTURE) is not used
	 * (using read() instead). */
	for (i = 0; i < OV511_NUMFRAMES; i++) {
		ov->frame[i].width = ov->maxwidth;
		ov->frame[i].height = ov->maxheight;
		ov->frame[i].bytes_read = 0;
		if (force_palette)
			ov->frame[i].format = force_palette;
		else
#ifdef OV511_ALLOW_CONVERSION
			ov->frame[i].format = VIDEO_PALETTE_RGB24;
#else
			ov->frame[i].format = VIDEO_PALETTE_YUV420;
#endif
		ov->frame[i].depth = get_depth(ov->frame[i].format);
	}

	PDEBUG(3, "%dx%d, %s", ov->maxwidth, ov->maxheight,
	       symbolic(v4l1_plist, ov->frame[0].format));

	/* Initialize to max width/height, YUV420 or RGB24 (if supported) */
	if (mode_init_regs(ov, ov->maxwidth, ov->maxheight,
			   ov->frame[0].format, 0) < 0)
		return -EINVAL;

	return 0;
}

/**********************************************************************
 *
 * Video decoder stuff
 *
 **********************************************************************/

/* Set analog input port of decoder */
static int
decoder_set_input(struct usb_ov511 *ov, int input)
{
	PDEBUG(4, "port %d", input);

	switch (ov->sensor) {
	case SEN_SAA7111A:
	{
		/* Select mode */
		i2c_w_mask(ov, 0x02, input, 0x07);
		/* Bypass chrominance trap for modes 4..7 */
		i2c_w_mask(ov, 0x09, (input > 3) ? 0x80:0x00, 0x80);
		break;
	}
	default:
		return -EINVAL;
	}

	return 0;
}

/* Get ASCII name of video input */
static int
decoder_get_input_name(struct usb_ov511 *ov, int input, char *name)
{
	switch (ov->sensor) {
	case SEN_SAA7111A:
	{
		if (input < 0 || input > 7)
			return -EINVAL;
		else if (input < 4)
			sprintf(name, "CVBS-%d", input);
		else // if (input < 8)
			sprintf(name, "S-Video-%d", input - 4);
		break;
	}
	default:
		sprintf(name, "%s", "Camera");
	}

	return 0;
}

/* Set norm (NTSC, PAL, SECAM, AUTO) */
static int
decoder_set_norm(struct usb_ov511 *ov, int norm)
{
	PDEBUG(4, "%d", norm);

	switch (ov->sensor) {
	case SEN_SAA7111A:
	{
		int reg_8, reg_e;

		if (norm == VIDEO_MODE_NTSC) {
			reg_8 = 0x40;	/* 60 Hz */
			reg_e = 0x00;	/* NTSC M / PAL BGHI */
		} else if (norm == VIDEO_MODE_PAL) {
			reg_8 = 0x00;	/* 50 Hz */
			reg_e = 0x00;	/* NTSC M / PAL BGHI */
		} else if (norm == VIDEO_MODE_AUTO) {
			reg_8 = 0x80;	/* Auto field detect */
			reg_e = 0x00;	/* NTSC M / PAL BGHI */
		} else if (norm == VIDEO_MODE_SECAM) {
			reg_8 = 0x00;	/* 50 Hz */
			reg_e = 0x50;	/* SECAM / PAL 4.43 */
		} else {
			return -EINVAL;
		}

		i2c_w_mask(ov, 0x08, reg_8, 0xc0);
		i2c_w_mask(ov, 0x0e, reg_e, 0x70);
		break;
	}
	default:
		return -EINVAL;
	}

	return 0;
}

#ifdef OV511_ALLOW_CONVERSION
/**********************************************************************
 *
 * Color correction functions
 *
 **********************************************************************/

/*
 * Turn a YUV4:2:0 block into an RGB block
 *
 * Video4Linux seems to use the blue, green, red channel
 * order convention-- rgb[0] is blue, rgb[1] is green, rgb[2] is red.
 *
 * Color space conversion coefficients taken from the excellent
 * http://www.inforamp.net/~poynton/ColorFAQ.html
 * In his terminology, this is a CCIR 601.1 YCbCr -> RGB.
 * Y values are given for all 4 pixels, but the U (Pb)
 * and V (Pr) are assumed constant over the 2x2 block.
 *
 * To avoid floating point arithmetic, the color conversion
 * coefficients are scaled into 16.16 fixed-point integers.
 * They were determined as follows:
 *
 *	double brightness = 1.0;  (0->black; 1->full scale)
 *	double saturation = 1.0;  (0->greyscale; 1->full color)
 *	double fixScale = brightness * 256 * 256;
 *	int rvScale = (int)(1.402 * saturation * fixScale);
 *	int guScale = (int)(-0.344136 * saturation * fixScale);
 *	int gvScale = (int)(-0.714136 * saturation * fixScale);
 *	int buScale = (int)(1.772 * saturation * fixScale);
 *	int yScale = (int)(fixScale);
 */

/* LIMIT: convert a 16.16 fixed-point value to a byte, with clipping. */
#define LIMIT(x) ((x)>0xffffff?0xff: ((x)<=0xffff?0:((x)>>16)))

static inline void
move_420_block(int yTL, int yTR, int yBL, int yBR, int u, int v,
	       int rowPixels, unsigned char * rgb, int bits)
{
	const int rvScale = 91881;
	const int guScale = -22553;
	const int gvScale = -46801;
	const int buScale = 116129;
	const int yScale  = 65536;
	int r, g, b;

	g = guScale * u + gvScale * v;
	if (force_rgb) {
		r = buScale * u;
		b = rvScale * v;
	} else {
		r = rvScale * v;
		b = buScale * u;
	}

	yTL *= yScale; yTR *= yScale;
	yBL *= yScale; yBR *= yScale;

	if (bits == 24) {
		/* Write out top two pixels */
		rgb[0] = LIMIT(b+yTL); rgb[1] = LIMIT(g+yTL);
		rgb[2] = LIMIT(r+yTL);

		rgb[3] = LIMIT(b+yTR); rgb[4] = LIMIT(g+yTR);
		rgb[5] = LIMIT(r+yTR);

		/* Skip down to next line to write out bottom two pixels */
		rgb += 3 * rowPixels;
		rgb[0] = LIMIT(b+yBL); rgb[1] = LIMIT(g+yBL);
		rgb[2] = LIMIT(r+yBL);

		rgb[3] = LIMIT(b+yBR); rgb[4] = LIMIT(g+yBR);
		rgb[5] = LIMIT(r+yBR);
	} else if (bits == 16) {
		/* Write out top two pixels */
		rgb[0] = ((LIMIT(b+yTL) >> 3) & 0x1F)
			| ((LIMIT(g+yTL) << 3) & 0xE0);
		rgb[1] = ((LIMIT(g+yTL) >> 5) & 0x07)
			| (LIMIT(r+yTL) & 0xF8);

		rgb[2] = ((LIMIT(b+yTR) >> 3) & 0x1F)
			| ((LIMIT(g+yTR) << 3) & 0xE0);
		rgb[3] = ((LIMIT(g+yTR) >> 5) & 0x07)
			| (LIMIT(r+yTR) & 0xF8);

		/* Skip down to next line to write out bottom two pixels */
		rgb += 2 * rowPixels;

		rgb[0] = ((LIMIT(b+yBL) >> 3) & 0x1F)
			| ((LIMIT(g+yBL) << 3) & 0xE0);
		rgb[1] = ((LIMIT(g+yBL) >> 5) & 0x07)
			| (LIMIT(r+yBL) & 0xF8);

		rgb[2] = ((LIMIT(b+yBR) >> 3) & 0x1F)
			| ((LIMIT(g+yBR) << 3) & 0xE0);
		rgb[3] = ((LIMIT(g+yBR) >> 5) & 0x07)
			| (LIMIT(r+yBR) & 0xF8);
	}
}

#endif	/* OV511_ALLOW_CONVERSION */

/**********************************************************************
 *
 * Raw data parsing
 *
 **********************************************************************/

/* Copies a 64-byte segment at pIn to an 8x8 block at pOut. The width of the
 * image at pOut is specified by w.
 */
static inline void
make_8x8(unsigned char *pIn, unsigned char *pOut, int w)
{
	unsigned char *pOut1 = pOut;
	int x, y;

	for (y = 0; y < 8; y++) {
		pOut1 = pOut;
		for (x = 0; x < 8; x++) {
			*pOut1++ = *pIn++;
		}
		pOut += w;
	}
}

/*
 * For RAW BW (YUV 4:0:0) images, data show up in 256 byte segments.
 * The segments represent 4 squares of 8x8 pixels as follows:
 *
 *      0  1 ...  7    64  65 ...  71   ...  192 193 ... 199
 *      8  9 ... 15    72  73 ...  79        200 201 ... 207
 *           ...              ...                    ...
 *     56 57 ... 63   120 121 ... 127        248 249 ... 255
 *
 */ 
static void
yuv400raw_to_yuv400p(struct ov511_frame *frame,
		     unsigned char *pIn0, unsigned char *pOut0)
{
	int x, y;
	unsigned char *pIn, *pOut, *pOutLine;

	/* Copy Y */
	pIn = pIn0;
	pOutLine = pOut0;
	for (y = 0; y < frame->rawheight - 1; y += 8) {
		pOut = pOutLine;
		for (x = 0; x < frame->rawwidth - 1; x += 8) {
			make_8x8(pIn, pOut, frame->rawwidth);
			pIn += 64;
			pOut += 8;
		}
		pOutLine += 8 * frame->rawwidth;
	}
}

/*
 * For YUV 4:2:0 images, the data show up in 384 byte segments.
 * The first 64 bytes of each segment are U, the next 64 are V.  The U and
 * V are arranged as follows:
 *
 *      0  1 ...  7
 *      8  9 ... 15
 *           ...   
 *     56 57 ... 63
 *
 * U and V are shipped at half resolution (1 U,V sample -> one 2x2 block).
 *
 * The next 256 bytes are full resolution Y data and represent 4 squares
 * of 8x8 pixels as follows:
 *
 *      0  1 ...  7    64  65 ...  71   ...  192 193 ... 199
 *      8  9 ... 15    72  73 ...  79        200 201 ... 207
 *           ...              ...                    ...
 *     56 57 ... 63   120 121 ... 127   ...  248 249 ... 255
 *
 * Note that the U and V data in one segment represent a 16 x 16 pixel
 * area, but the Y data represent a 32 x 8 pixel area. If the width is not an
 * even multiple of 32, the extra 8x8 blocks within a 32x8 block belong to the
 * next horizontal stripe.
 *
 * If dumppix module param is set, _parse_data just dumps the incoming segments,
 * verbatim, in order, into the frame. When used with vidcat -f ppm -s 640x480
 * this puts the data on the standard output and can be analyzed with the
 * parseppm.c utility I wrote.  That's a much faster way for figuring out how
 * these data are scrambled.
 */

/* Converts from raw, uncompressed segments at pIn0 to a YUV420P frame at pOut0.
 *
 * FIXME: Currently only handles width and height that are multiples of 16
 */
static void
yuv420raw_to_yuv420p(struct ov511_frame *frame,
		     unsigned char *pIn0, unsigned char *pOut0)
{
	int k, x, y;
	unsigned char *pIn, *pOut, *pOutLine;
	const unsigned int a = frame->rawwidth * frame->rawheight;
	const unsigned int w = frame->rawwidth / 2;

	/* Copy U and V */
	pIn = pIn0;
	pOutLine = pOut0 + a;
	for (y = 0; y < frame->rawheight - 1; y += 16) {
		pOut = pOutLine;
		for (x = 0; x < frame->rawwidth - 1; x += 16) {
			make_8x8(pIn, pOut, w);
			make_8x8(pIn + 64, pOut + a/4, w);
			pIn += 384;
			pOut += 8;
		}
		pOutLine += 8 * w;
	}

	/* Copy Y */
	pIn = pIn0 + 128;
	pOutLine = pOut0;
	k = 0;
	for (y = 0; y < frame->rawheight - 1; y += 8) {
		pOut = pOutLine;
		for (x = 0; x < frame->rawwidth - 1; x += 8) {
			make_8x8(pIn, pOut, frame->rawwidth);
			pIn += 64;
			pOut += 8;
			if ((++k) > 3) {
				k = 0;
				pIn += 128;
			}
		}
		pOutLine += 8 * frame->rawwidth;
	}
}

#ifdef OV511_ALLOW_CONVERSION
/*
 * fixFrameRGBoffset--
 * My camera seems to return the red channel about 1 pixel
 * low, and the blue channel about 1 pixel high. After YUV->RGB
 * conversion, we can correct this easily. OSL 2/24/2000.
 */
static void
fixFrameRGBoffset(struct ov511_frame *frame)
{
	int x, y;
	int rowBytes = frame->width*3, w = frame->width;
	unsigned char *rgb = frame->data;
	const int shift = 1;  /* Distance to shift pixels by, vertically */

	/* Don't bother with little images */
	if (frame->width < 400)
		return;

	/* This only works with RGB24 */
	if (frame->format != VIDEO_PALETTE_RGB24)
		return;

	/* Shift red channel up */
	for (y = shift; y < frame->height; y++)	{
		int lp = (y-shift)*rowBytes;     /* Previous line offset */
		int lc = y*rowBytes;             /* Current line offset */
		for (x = 0; x < w; x++)
			rgb[lp+x*3+2] = rgb[lc+x*3+2]; /* Shift red up */
	}

	/* Shift blue channel down */
	for (y = frame->height-shift-1; y >= 0; y--) {
		int ln = (y + shift) * rowBytes;  /* Next line offset */
		int lc = y * rowBytes;            /* Current line offset */
		for (x = 0; x < w; x++)
			rgb[ln+x*3+0] = rgb[lc+x*3+0]; /* Shift blue down */
	}
}
#endif

/**********************************************************************
 *
 * Decompression
 *
 **********************************************************************/

/* Chooses a decompression module, locks it, and sets ov->decomp_ops
 * accordingly. Returns -ENXIO if decompressor is not available, otherwise
 * returns 0 if no other error.
 */
static int
request_decompressor(struct usb_ov511 *ov)
{
	if (!ov)
		return -ENODEV;

	if (ov->decomp_ops) {
		err("ERROR: Decompressor already requested!");
		return -EINVAL;
	}

	lock_kernel();

	/* Try to get MMX, and fall back on no-MMX if necessary */
	if (ov->bclass == BCL_OV511) {
		if (ov511_mmx_decomp_ops) {
			PDEBUG(3, "Using OV511 MMX decompressor");
			ov->decomp_ops = ov511_mmx_decomp_ops;
		} else if (ov511_decomp_ops) {
			PDEBUG(3, "Using OV511 decompressor");
			ov->decomp_ops = ov511_decomp_ops;
		} else {
			err("No decompressor available");
		}
	} else if (ov->bclass == BCL_OV518) {
		if (ov518_mmx_decomp_ops) {
			PDEBUG(3, "Using OV518 MMX decompressor");
			ov->decomp_ops = ov518_mmx_decomp_ops;
		} else if (ov518_decomp_ops) {
			PDEBUG(3, "Using OV518 decompressor");
			ov->decomp_ops = ov518_decomp_ops;
		} else {
			err("No decompressor available");
		}
	} else if (ov->bclass == BCL_OV519) {
		info("OV519 doesn't need proprietary decompressor. It uses standard JPEG");
	} else {
		err("Decompressor: Unknown bridge");
	}

	if (ov->decomp_ops) {
		if (!ov->decomp_ops->owner) {
			ov->decomp_ops = NULL;
			unlock_kernel();
			return -ENOSYS;
		}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
		if (! try_module_get (ov->decomp_ops->owner)) {
			ov->decomp_ops = NULL;
			unlock_kernel();
			return -ENOSYS;
		}
#else
 		__MOD_INC_USE_COUNT(ov->decomp_ops->owner);
#endif
		unlock_kernel();
		return 0;
	} else {
		unlock_kernel();
		if (ov->bclass == BCL_OV519)
			return 0;
		else
			return -ENOSYS;
	}
}

/* Unlocks decompression module and nulls ov->decomp_ops. Safe to call even
 * if ov->decomp_ops is NULL.
 */
static void
release_decompressor(struct usb_ov511 *ov)
{
	int released = 0;	/* Did we actually do anything? */

	if (!ov)
		return;

	if (ov->bclass == BCL_OV519) {
		ov->compress_inited = 0;
		return;
	}

	lock_kernel();

	if (ov->decomp_ops && ov->decomp_ops->owner) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
		module_put(ov->decomp_ops->owner);
#else
 		__MOD_DEC_USE_COUNT(ov->decomp_ops->owner);
#endif
		released = 1;
	}

	ov->decomp_ops = NULL;

	unlock_kernel();

	if (released)
		PDEBUG(3, "Decompressor released");
}

static void
decompress(struct usb_ov511 *ov, struct ov511_frame *frame,
	   unsigned char *pIn0, unsigned char *pOut0)
{
	if (!ov->decomp_ops)
		if (request_decompressor(ov))
			return;

	PDEBUG(4, "Decompressing %d bytes", frame->bytes_recvd);

	if (frame->format == VIDEO_PALETTE_GREY
	    && ov->decomp_ops->decomp_400) {
		int ret = ov->decomp_ops->decomp_400(
			pIn0,
			pOut0,
			frame->compbuf,
			frame->rawwidth,
			frame->rawheight,
			frame->bytes_recvd);
		PDEBUG(4, "DEBUG: decomp_400 returned %d", ret);
	} else if (frame->format != VIDEO_PALETTE_GREY
		   && ov->decomp_ops->decomp_420) {
		int ret = ov->decomp_ops->decomp_420(
			pIn0,
			pOut0,
			frame->compbuf,
			frame->rawwidth,
			frame->rawheight,
			frame->bytes_recvd);
		PDEBUG(4, "DEBUG: decomp_420 returned %d", ret);
	} else {
		err("Decompressor does not support this format");
	}
}

/**********************************************************************
 *
 * Format conversion
 *
 **********************************************************************/

#ifdef OV511_ALLOW_CONVERSION

/* Converts from planar YUV420 to RGB24. */
static void
yuv420p_to_rgb(struct ov511_frame *frame,
	       unsigned char *pIn0, unsigned char *pOut0, int bits)
{
	const int numpix = frame->width * frame->height;
	const int bytes = bits >> 3;
	int i, j, y00, y01, y10, y11, u, v;
	unsigned char *pY = pIn0;
	unsigned char *pU = pY + numpix;
	unsigned char *pV = pU + numpix / 4;
	unsigned char *pOut = pOut0;

	for (j = 0; j <= frame->height - 2; j += 2) {
		for (i = 0; i <= frame->width - 2; i += 2) {
			y00 = *pY;
			y01 = *(pY + 1);
			y10 = *(pY + frame->width);
			y11 = *(pY + frame->width + 1);
			u = (*pU++) - 128;
			v = (*pV++) - 128;

			move_420_block(y00, y01, y10, y11, u, v,
				       frame->width, pOut, bits);

			pY += 2;
			pOut += 2 * bytes;
		}
		pY += frame->width;
		pOut += frame->width * bytes;
	}
}

/* Converts from planar YUV420 to YUV422 (YUYV). */
static void
yuv420p_to_yuv422(struct ov511_frame *frame,
		  unsigned char *pIn0, unsigned char *pOut0)
{
	const int numpix = frame->width * frame->height;
	int i, j;
	unsigned char *pY = pIn0;
	unsigned char *pU = pY + numpix;
	unsigned char *pV = pU + numpix / 4;
	unsigned char *pOut = pOut0;

	for (i = 0; i < numpix; i++) {
		*pOut = *(pY + i);
		pOut += 2;
	}

	pOut = pOut0 + 1;
	for (j = 0; j <= frame->height - 2 ; j += 2) {
		for (i = 0; i <= frame->width - 2; i += 2) {
			int u = *pU++;
			int v = *pV++;

			*pOut = u;
			*(pOut+2) = v;
			*(pOut+frame->width*2) = u;
			*(pOut+frame->width*2+2) = v;
			pOut += 4;
		}
		pOut += (frame->width * 2);
	}
}

/* Converts pData from planar YUV420 to planar YUV422 **in place**. */
static void
yuv420p_to_yuv422p(struct ov511_frame *frame, unsigned char *pData)
{
	const int numpix = frame->width * frame->height;
	const int w = frame->width;
	int j;
	unsigned char *pIn, *pOut;

	/* Clear U and V */
	memset(pData + numpix + numpix / 2, 127, numpix / 2);

	/* Convert V starting from beginning and working forward */
	pIn = pData + numpix + numpix / 4;
	pOut = pData + numpix +numpix / 2;
	for (j = 0; j <= frame->height - 2; j += 2) {
		memmove(pOut, pIn, w/2);
		memmove(pOut + w/2, pIn, w/2);
		pIn += w/2;
		pOut += w;
	}

	/* Convert U, starting from end and working backward */
	pIn = pData + numpix + numpix / 4;
	pOut = pData + numpix + numpix / 2;
	for (j = 0; j <= frame->height - 2; j += 2) {
		pIn -= w/2;
		pOut -= w;
		memmove(pOut, pIn, w/2);
		memmove(pOut + w/2, pIn, w/2);
	}
}

#endif	/* OV511_ALLOW_CONVERSION */

/* Fuses even and odd fields together, and doubles width.
 * INPUT: an odd field followed by an even field at pIn0, in YUV planar format
 * OUTPUT: a normal YUV planar image, with correct aspect ratio
 */
static void
deinterlace(struct ov511_frame *frame, int rawformat,
            unsigned char *pIn0, unsigned char *pOut0)
{
	const int fieldheight = frame->rawheight / 2;
	const int fieldpix = fieldheight * frame->rawwidth;
	const int w = frame->width;
	int x, y;
	unsigned char *pInEven, *pInOdd, *pOut;

	PDEBUG(5, "fieldheight=%d", fieldheight);

	if (frame->rawheight != frame->height) {
		err("invalid height");
		return;
	}

	if ((frame->rawwidth * 2) != frame->width) {
		err("invalid width");
		return;
	}

	/* Y */
	pInOdd = pIn0;
	pInEven = pInOdd + fieldpix;
	pOut = pOut0;
	for (y = 0; y < fieldheight; y++) {
		for (x = 0; x < frame->rawwidth; x++) {
			*pOut = *pInEven;
			*(pOut+1) = *pInEven++;
			*(pOut+w) = *pInOdd;
			*(pOut+w+1) = *pInOdd++;
			pOut += 2;
		}
		pOut += w;
	}

	if (rawformat == RAWFMT_YUV420) {
	/* U */
		pInOdd = pIn0 + fieldpix * 2;
		pInEven = pInOdd + fieldpix / 4;
		for (y = 0; y < fieldheight / 2; y++) {
			for (x = 0; x < frame->rawwidth / 2; x++) {
				*pOut = *pInEven;
				*(pOut+1) = *pInEven++;
				*(pOut+w/2) = *pInOdd;
				*(pOut+w/2+1) = *pInOdd++;
				pOut += 2;
			}
			pOut += w/2;
		}
	/* V */
		pInOdd = pIn0 + fieldpix * 2 + fieldpix / 2;
		pInEven = pInOdd + fieldpix / 4;
		for (y = 0; y < fieldheight / 2; y++) {
			for (x = 0; x < frame->rawwidth / 2; x++) {
				*pOut = *pInEven;
				*(pOut+1) = *pInEven++;
				*(pOut+w/2) = *pInOdd;
				*(pOut+w/2+1) = *pInOdd++;
				pOut += 2;
			}
			pOut += w/2;
		}
	}
}

static void
ov51x_postprocess_grey(struct usb_ov511 *ov, struct ov511_frame *frame)
{
		/* Deinterlace frame, if necessary */
		if (ov->sensor == SEN_SAA7111A && frame->rawheight >= 480) {
			if (frame->compressed)
				decompress(ov, frame, frame->rawdata,
						 frame->tempdata);
			else
				yuv400raw_to_yuv400p(frame, frame->rawdata,
						     frame->tempdata);

			deinterlace(frame, RAWFMT_YUV400, frame->tempdata,
			            frame->data);
		} else {
			if (frame->compressed)
				decompress(ov, frame, frame->rawdata,
						 frame->data);
			else
				yuv400raw_to_yuv400p(frame, frame->rawdata,
						     frame->data);
		}
}

#ifdef OV511_ALLOW_CONVERSION
/* Process raw YUV420 data into the format requested by the app. Conversion
 * between V4L formats is allowed.
 */
static void
ov51x_postprocess_yuv420(struct usb_ov511 *ov, struct ov511_frame *frame)
{
	/* Process frame->rawdata to frame->tempdata */
	if (frame->compressed)
		decompress(ov, frame, frame->rawdata, frame->tempdata);
	else
		yuv420raw_to_yuv420p(frame, frame->rawdata, frame->tempdata);

	/* Deinterlace frame, if necessary */
	if (ov->sensor == SEN_SAA7111A && frame->rawheight >= 480) {
		memcpy(frame->rawdata, frame->tempdata,
			MAX_RAW_DATA_SIZE(frame->width, frame->height));
		deinterlace(frame, RAWFMT_YUV420, frame->rawdata,
		            frame->tempdata);
	}

	/* Frame should be (width x height) and not (rawwidth x rawheight) at
         * this point. */

	/* Process frame->tempdata to frame->data */
	switch (frame->format) {
	case VIDEO_PALETTE_RGB565:
		yuv420p_to_rgb(frame, frame->tempdata, frame->data, 16);
		break;
	case VIDEO_PALETTE_RGB24:
		yuv420p_to_rgb(frame, frame->tempdata, frame->data, 24);
		break;
	case VIDEO_PALETTE_YUV422:
	case VIDEO_PALETTE_YUYV:
		yuv420p_to_yuv422(frame, frame->tempdata, frame->data);
		break;
	case VIDEO_PALETTE_YUV420:
	case VIDEO_PALETTE_YUV420P:
		memcpy(frame->data, frame->tempdata,
			MAX_RAW_DATA_SIZE(frame->width, frame->height));
		break;
	case VIDEO_PALETTE_YUV422P:
		/* Data is converted in place, so copy it in advance */
		memcpy(frame->data, frame->tempdata,
			MAX_RAW_DATA_SIZE(frame->width, frame->height));

		yuv420p_to_yuv422p(frame, frame->data);
		break;
	default:
		err("Cannot convert YUV420 to %s",
		    symbolic(v4l1_plist, frame->format));
	}

	if (fix_rgb_offset)
		fixFrameRGBoffset(frame);
}

#else /* if conversion not allowed */

/* Process raw YUV420 data into standard YUV420P */
static void
ov51x_postprocess_yuv420(struct usb_ov511 *ov, struct ov511_frame *frame)
{
	/* Deinterlace frame, if necessary */
	if (ov->sensor == SEN_SAA7111A && frame->rawheight >= 480) {
		if (frame->compressed)
			decompress(ov, frame, frame->rawdata, frame->tempdata);
		else
			yuv420raw_to_yuv420p(frame, frame->rawdata,
					     frame->tempdata);

		deinterlace(frame, RAWFMT_YUV420, frame->tempdata,
		            frame->data);
	} else {
		if (frame->compressed)
			decompress(ov, frame, frame->rawdata, frame->data);
		else
			yuv420raw_to_yuv420p(frame, frame->rawdata,
					     frame->data);
	}
}
#endif /* OV511_ALLOW_CONVERSION */

/* Post-processes the specified frame. This consists of:
 * 	1. Decompress frame, if necessary
 *	2. Deinterlace frame and scale to proper size, if necessary
 * 	3. Convert from YUV planar to destination format, if necessary
 * 	4. Fix the RGB offset, if necessary
 */
static void
ov51x_postprocess(struct usb_ov511 *ov, struct ov511_frame *frame)
{
	if (dumppix) {
		memset(frame->data, 0,
			MAX_DATA_SIZE(ov->maxwidth, ov->maxheight));
		PDEBUG(4, "Dumping %d bytes", frame->bytes_recvd);
		memcpy(frame->data, frame->rawdata, frame->bytes_recvd);
	} else {
		switch (frame->format) {
		case VIDEO_PALETTE_GREY:
			ov51x_postprocess_grey(ov, frame);
			break;
		case VIDEO_PALETTE_YUV420:
		case VIDEO_PALETTE_YUV420P:
#ifdef OV511_ALLOW_CONVERSION
		case VIDEO_PALETTE_RGB565:
		case VIDEO_PALETTE_RGB24:
		case VIDEO_PALETTE_YUV422:
		case VIDEO_PALETTE_YUYV:
		case VIDEO_PALETTE_YUV422P:
#endif
			ov51x_postprocess_yuv420(ov, frame);
			break;
		default:
			err("Cannot convert data to %s",
			    symbolic(v4l1_plist, frame->format));
		}
	}
}

/**********************************************************************
 *
 * OV51x data transfer, IRQ handler
 *
 **********************************************************************/

static inline void
ov511_move_data(struct usb_ov511 *ov, unsigned char *in, int n)
{
	int num, offset;
	int pnum = in[ov->packet_size - 1];		/* Get packet number */
	int max_raw = MAX_RAW_DATA_SIZE(ov->maxwidth, ov->maxheight);
	struct ov511_frame *frame = &ov->frame[ov->curframe];
	struct timeval *ts;

	/* SOF/EOF packets have 1st to 8th bytes zeroed and the 9th
	 * byte non-zero. The EOF packet has image width/height in the
	 * 10th and 11th bytes. The 9th byte is given as follows:
	 *
	 * bit 7: EOF
	 *     6: compression enabled
	 *     5: 422/420/400 modes
	 *     4: 422/420/400 modes
	 *     3: 1
	 *     2: snapshot button on
	 *     1: snapshot frame
	 *     0: even/odd field
	 */

	if (printph) {
		info("ph(%3d): %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x",
		     pnum, in[0], in[1], in[2], in[3], in[4], in[5], in[6],
		     in[7], in[8], in[9], in[10], in[11]);
	}

	/* Check for SOF/EOF packet */
	if ((in[0] | in[1] | in[2] | in[3] | in[4] | in[5] | in[6] | in[7]) ||
	    (~in[8] & 0x08))
		goto check_middle;

	/* Frame end */
	if (in[8] & 0x80) {
		ts = (struct timeval *)(frame->data
		      + MAX_FRAME_SIZE(ov->maxwidth, ov->maxheight));
		do_gettimeofday(ts);

		/* Get the actual frame size from the EOF header */
		frame->rawwidth = ((int)(in[9]) + 1) * 8;
		frame->rawheight = ((int)(in[10]) + 1) * 8;

 		PDEBUG(4, "Frame end, frame=%d, pnum=%d, w=%d, h=%d, recvd=%d",
			ov->curframe, pnum, frame->rawwidth, frame->rawheight,
			frame->bytes_recvd);

		/* Validate the header data */
		RESTRICT_TO_RANGE(frame->rawwidth, ov->minwidth, ov->maxwidth);
		RESTRICT_TO_RANGE(frame->rawheight, ov->minheight,
				  ov->maxheight);

		/* Don't allow byte count to exceed buffer size */
		RESTRICT_TO_RANGE(frame->bytes_recvd, 8, max_raw);

		if (frame->scanstate == STATE_LINES) {
	    		int nextf;

			frame->grabstate = FRAME_DONE;
			wake_up_interruptible(&frame->wq);

			/* If next frame is ready or grabbing,
			 * point to it */
			nextf = (ov->curframe + 1) % OV511_NUMFRAMES;
			if (ov->frame[nextf].grabstate == FRAME_READY
			    || ov->frame[nextf].grabstate == FRAME_GRABBING) {
				ov->curframe = nextf;
				ov->frame[nextf].scanstate = STATE_SCANNING;
			} else {
				if (ov->frame[nextf].grabstate == FRAME_DONE) {
					PDEBUG(4, "No empty frames left");
				} else {
					PDEBUG(4, "Frame not ready? state = %d",
						ov->frame[nextf].grabstate);
				}

				ov->curframe = -1;
			}
		} else {
			PDEBUG(5, "Frame done, but not scanning");
		}
		/* Image corruption caused by misplaced frame->segment = 0
		 * fixed by carlosf@conectiva.com.br
		 */
	} else {
		/* Frame start */
		PDEBUG(4, "Frame start, framenum = %d", ov->curframe);

		/* Check to see if it's a snapshot frame */
		/* FIXME?? Should the snapshot reset go here? Performance? */
		if (in[8] & 0x02) {
			frame->snapshot = 1;
			PDEBUG(3, "snapshot detected");
		}

		frame->scanstate = STATE_LINES;
		frame->bytes_recvd = 0;
		frame->compressed = in[8] & 0x40;
	}

check_middle:
	/* Are we in a frame? */
	if (frame->scanstate != STATE_LINES) {
		PDEBUG(5, "Not in a frame; packet skipped");
		return;
	}

	/* If frame start, skip header */
	if (frame->bytes_recvd == 0)
		offset = 9;
	else
		offset = 0;

	num = n - offset - 1;

	/* Dump all data exactly as received */
	if (dumppix == 2) {
		frame->bytes_recvd += n - 1;
		if (frame->bytes_recvd <= max_raw)
			memcpy(frame->rawdata + frame->bytes_recvd - (n - 1),
				in, n - 1);
		else
			PDEBUG(3, "Raw data buffer overrun!! (%d)",
				frame->bytes_recvd - max_raw);
	} else if (!frame->compressed && !remove_zeros) {
		frame->bytes_recvd += num;
		if (frame->bytes_recvd <= max_raw)
			memcpy(frame->rawdata + frame->bytes_recvd - num,
				in + offset, num);
		else
			PDEBUG(3, "Raw data buffer overrun!! (%d)",
				frame->bytes_recvd - max_raw);
	} else { /* Remove all-zero FIFO lines (aligned 32-byte blocks) */
		int b, read = 0, allzero, copied = 0;
		if (offset) {
			frame->bytes_recvd += 32 - offset;	// Bytes out
			memcpy(frame->rawdata,	in + offset, 32 - offset);
			read += 32;
		}

		while (read < n - 1) {
			allzero = 1;
			for (b = 0; b < 32; b++) {
				if (in[read + b]) {
					allzero = 0;
					break;
				}
			}

			if (allzero) {
				/* Don't copy it */
			} else {
				if (frame->bytes_recvd + copied + 32 <= max_raw)
				{
					memcpy(frame->rawdata
						+ frame->bytes_recvd + copied,
						in + read, 32);
					copied += 32;
				} else {
					PDEBUG(3, "Raw data buffer overrun!!");
				}
			}
			read += 32;
		}

		frame->bytes_recvd += copied;
	}
}

static inline void
ov518_move_data(struct usb_ov511 *ov, unsigned char *in, int n)
{
	int max_raw = MAX_RAW_DATA_SIZE(ov->maxwidth, ov->maxheight);
	struct ov511_frame *frame = &ov->frame[ov->curframe];
	struct timeval *ts;

	/* Don't copy the packet number byte */
	if (ov->packet_numbering)
		--n;

	/* A false positive here is likely, until OVT gives me
	 * the definitive SOF/EOF format */
	if ((!(in[0] | in[1] | in[2] | in[3] | in[5])) && in[6]) {
		if (printph) {
			info("ph: %2x %2x %2x %2x %2x %2x %2x %2x", in[0],
			     in[1], in[2], in[3], in[4], in[5], in[6], in[7]);
		}

		if (frame->scanstate == STATE_LINES) {
			PDEBUG(4, "Detected frame end/start");
			goto eof;
		} else { //scanstate == STATE_SCANNING
			/* Frame start */
			PDEBUG(4, "Frame start, framenum = %d", ov->curframe);
			goto sof;
		}
	} else {
		goto check_middle;
	}

eof:
	ts = (struct timeval *)(frame->data
	      + MAX_FRAME_SIZE(ov->maxwidth, ov->maxheight));
	do_gettimeofday(ts);

	PDEBUG(4, "Frame end, curframe = %d, hw=%d, vw=%d, recvd=%d",
		ov->curframe,
		(int)(in[9]), (int)(in[10]), frame->bytes_recvd);

	// FIXME: Since we don't know the header formats yet,
	// there is no way to know what the actual image size is
	frame->rawwidth = frame->width;
	frame->rawheight = frame->height;

	/* Validate the header data */
	RESTRICT_TO_RANGE(frame->rawwidth, ov->minwidth, ov->maxwidth);
	RESTRICT_TO_RANGE(frame->rawheight, ov->minheight, ov->maxheight);

	/* Don't allow byte count to exceed buffer size */
	RESTRICT_TO_RANGE(frame->bytes_recvd, 8, max_raw);

	if (frame->scanstate == STATE_LINES) {
    		int nextf;

		frame->grabstate = FRAME_DONE;
		wake_up_interruptible(&frame->wq);

		/* If next frame is ready or grabbing,
		 * point to it */
		nextf = (ov->curframe + 1) % OV511_NUMFRAMES;
		if (ov->frame[nextf].grabstate == FRAME_READY
		    || ov->frame[nextf].grabstate == FRAME_GRABBING) {
			ov->curframe = nextf;
			ov->frame[nextf].scanstate = STATE_SCANNING;
			frame = &ov->frame[nextf];
		} else {
			if (ov->frame[nextf].grabstate == FRAME_DONE) {
				PDEBUG(4, "No empty frames left");
			} else {
				PDEBUG(4, "Frame not ready? state = %d",
				       ov->frame[nextf].grabstate);
			}

			ov->curframe = -1;
			PDEBUG(4, "SOF dropped (no active frame)");
			return;  /* Nowhere to store this frame */
		}
	}
sof:
	PDEBUG(4, "Starting capture on frame %d", frame->framenum);

// Snapshot not reverse-engineered yet.
#if 0
	/* Check to see if it's a snapshot frame */
	/* FIXME?? Should the snapshot reset go here? Performance? */
	if (in[8] & 0x02) {
		frame->snapshot = 1;
		PDEBUG(3, "snapshot detected");
	}
#endif
	frame->scanstate = STATE_LINES;
	frame->bytes_recvd = 0;
	frame->compressed = 1;

check_middle:
	/* Are we in a frame? */
	if (frame->scanstate != STATE_LINES) {
		PDEBUG(4, "scanstate: no SOF yet");
		return;
	}

	/* Dump all data exactly as received */
	if (dumppix == 2) {
		frame->bytes_recvd += n;
		if (frame->bytes_recvd <= max_raw)
			memcpy(frame->rawdata + frame->bytes_recvd - n, in, n);
		else
			PDEBUG(3, "Raw data buffer overrun!! (%d)",
				frame->bytes_recvd - max_raw);
	} else {
		/* All incoming data are divided into 8-byte segments. If the
		 * segment contains all zero bytes, it must be skipped. These
		 * zero-segments allow the OV518 to mainain a constant data rate
		 * regardless of the effectiveness of the compression. Segments
		 * are aligned relative to the beginning of each isochronous
		 * packet. The first segment in each image is a header (the
		 * decompressor skips it later).
		 */

		int b, read = 0, allzero, copied = 0;

		while (read < n) {
			allzero = 1;
			for (b = 0; b < 8; b++) {
				if (in[read + b]) {
					allzero = 0;
					break;
				}
			}

			if (allzero) {
			/* Don't copy it */
			} else {
				if (frame->bytes_recvd + copied + 8 <= max_raw)
				{
					memcpy(frame->rawdata
						+ frame->bytes_recvd + copied,
						in + read, 8);
					copied += 8;
				} else {
					PDEBUG(3, "Raw data buffer overrun!!");
				}
			}
			read += 8;
		}
		frame->bytes_recvd += copied;
	}
}

static inline void
ov519_move_data(struct usb_ov511 *ov, unsigned char *in, int n)
{
	int max_raw = MAX_RAW_DATA_SIZE(ov->maxwidth, ov->maxheight);
	struct ov511_frame *frame = &ov->frame[ov->curframe];
	struct timeval *ts;

	/* Don't copy the packet number byte */
//	if (ov->packet_numbering)
//		--n;
/* Header of ov519 is 16 bytes:
 * Byte     Value      Description
 *	0		0xff	magic
 *	1		0xff	magic
 *	2		0xff	magic
 *	3		0xXX	0x50 = SOF, 0x51 = EOF
 *	9		0xXX	0x01 initial frame without data, 0x00 standard frame with image
 *	14		Lo		in EOF: length of image data / 8
 *	15		Hi
 */

	// Start Of Frame
	if ((in[0]==0xff) && (in[1]==0xff) && (in[2]==0xff)) {

		if (printph) {
			info("ph: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x",
				in[0], in[1], in[2],  in[3],  in[4],  in[5],  in[6],  in[7],
				in[8], in[9], in[10], in[11], in[12], in[13], in[14], in[15]);
		}

		if (in[3]==0x50) {
			PDEBUG(4, "Start Of Frame, framenum = %d", ov->curframe);
			goto sof;
		} else if (in[3]==0x51) {
			PDEBUG(4, "End Of Frame");
			goto eof;
		} else {
			goto check_middle;
		}
	} else {
		goto check_middle;
	}

eof:
	ts = (struct timeval *)(frame->data
	      + MAX_FRAME_SIZE(ov->maxwidth, ov->maxheight));
	do_gettimeofday(ts);
	PDEBUG(4, "Frame end, curframe=%d, length=%d, recvd=%d",
		ov->curframe, (int)((in[14]) + ((int)(in[15])<<8))<<3, frame->bytes_recvd - (jpeginfo?2:0));


	if (in[9]) {
		PDEBUG(1, "initial frame");
		frame->scanstate = STATE_SCANNING;
		return;
	}

	// FIXME: Since we don't know the header formats yet,
	// there is no way to know what the actual image size is
	frame->rawwidth = frame->width;
	frame->rawheight = frame->height;

	/* Validate the header data */
	RESTRICT_TO_RANGE(frame->rawwidth, ov->minwidth, ov->maxwidth);
	RESTRICT_TO_RANGE(frame->rawheight, ov->minheight, ov->maxheight);

	/* Don't allow byte count to exceed buffer size */
	//RESTRICT_TO_RANGE(frame->bytes_recvd, 8, max_raw);

	if (frame->scanstate == STATE_LINES) {
		int nextf;

		if (((int)((in[14]) + ((int)(in[15])<<8))<<3) != (frame->bytes_recvd - (jpeginfo?2:0))) {
			info("Data length in header and number of received bytes differ");
			frame->scanstate = STATE_SCANNING;
			return;
		}

	if (jpeginfo) {
		frame->data[0] = in[14];
		frame->data[1] = in[15];
	}
		frame->grabstate = FRAME_DONE;
		wake_up_interruptible(&frame->wq);

		/* If next frame is ready or grabbing,
		 * point to it */
		nextf = (ov->curframe + 1) % OV511_NUMFRAMES;
		if (ov->frame[nextf].grabstate == FRAME_READY
		    || ov->frame[nextf].grabstate == FRAME_GRABBING) {
			ov->curframe = nextf;
			ov->frame[nextf].scanstate = STATE_SCANNING;
		} else {
			ov->curframe = -1;
		}
	} else {
		info("EOF without SOF"); // This happens if there was no active frame when SOF arrived
	}
	return;

sof:
	PDEBUG(4, "Starting capture on frame %d", frame->framenum);

	// Skip SOF Header:
	in += 16;
	n  -= 16;

	frame->scanstate = STATE_LINES;
	if (jpeginfo) {
		frame->bytes_recvd = 2; // Space for length bytes. Will be written at EOF
		frame->data[0] = 0;
		frame->data[1] = 0;
	} else {
		frame->bytes_recvd = 0;
	}
	frame->compressed = 1;

check_middle:
	/* Are we in a frame? */
	if (frame->scanstate != STATE_LINES) {
		PDEBUG(4, "scanstate: no SOF yet");
		return;
	}

	/* Dump all data exactly as received. It is standard JPEG */
	frame->bytes_recvd += n;
	if (frame->bytes_recvd <= max_raw) {
		memcpy(frame->data + frame->bytes_recvd - n, in, n);
	} else {
		PDEBUG(3, "Raw data buffer overrun!! (%d)", frame->bytes_recvd - max_raw);
	}
}

static void
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 51)
ov51x_isoc_irq(struct urb *urb, struct pt_regs *regs)
#else
ov51x_isoc_irq(struct urb *urb)
#endif
{
	int i;
	struct usb_ov511 *ov;
	struct ov511_sbuf *sbuf;

	if (!urb->context) {
		PDEBUG(4, "no context");
		return;
	}

	sbuf = urb->context;
	ov = sbuf->ov;

	if (!ov || !ov->dev || !ov->user) {
		PDEBUG(4, "no device, or not open");
		return;
	}

	if (!ov->streaming) {
		PDEBUG(4, "hmmm... not streaming, but got interrupt");
		return;
	}

        if (urb->status == -ENOENT || urb->status == -ECONNRESET) {
                PDEBUG(4, "URB unlinked");
                return;
        }

	if (urb->status != -EINPROGRESS && urb->status != 0) {
		err("ERROR: urb->status=%d: %s", urb->status,
		    symbolic(urb_errlist, urb->status));
	}

	/* Copy the data received into our frame buffer */
	PDEBUG(5, "sbuf[%d]: Moving %d packets", sbuf->n,
	       urb->number_of_packets);
	for (i = 0; i < urb->number_of_packets; i++) {
		/* Warning: Don't call *_move_data() if no frame active! */
		if (ov->curframe >= 0) {
			int n = urb->iso_frame_desc[i].actual_length;
			int st = urb->iso_frame_desc[i].status;
			unsigned char *cdata;

			urb->iso_frame_desc[i].actual_length = 0;
			urb->iso_frame_desc[i].status = 0;

			cdata = urb->transfer_buffer
				+ urb->iso_frame_desc[i].offset;

			if (!n) {
				PDEBUG(4, "Zero-length packet");
				continue;
			}

			if (st)
				PDEBUG(2, "data error: [%d] len=%d, status=%d",
				       i, n, st);

			switch (ov->bclass) {
				case BCL_OV511:
					ov511_move_data(ov, cdata, n);
					break;
				case BCL_OV518:
					ov518_move_data(ov, cdata, n);
					break;
				case BCL_OV519:
					ov519_move_data(ov, cdata, n);
					break;
				default:
					err("Unknown bridge device (%d)", ov->bridge);
			}
		} else if (waitqueue_active(&ov->wq)) {
			wake_up_interruptible(&ov->wq);
		}
	}

	/* Resubmit this URB */
	urb->dev = ov->dev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 4)
	if ((i = usb_submit_urb(urb, GFP_ATOMIC)) != 0)
#else
	if ((i = usb_submit_urb(urb)) != 0)
#endif
		err("usb_submit_urb() ret %d", i);

	return;
}

/****************************************************************************
 *
 * Stream initialization and termination
 *
 ***************************************************************************/

static int
ov51x_init_isoc(struct usb_ov511 *ov)
{
	struct urb *urb;
	int fx, err, n, size;

	PDEBUG(3, "*** Initializing capture ***");

	ov->curframe = -1;

	switch (ov->bridge) {
		case BRG_OV511:
			if (cams == 1)				size = 993;
			else if (cams == 2)			size = 513;
			else if (cams == 3 || cams == 4)	size = 257;
			else {
				err("\"cams\" parameter too high!");
				return -1;
			}
			break;
		case BRG_OV511PLUS:
			if (cams == 1)				size = 961;
			else if (cams == 2)			size = 513;
			else if (cams == 3 || cams == 4)	size = 257;
			else if (cams >= 5 && cams <= 8)	size = 129;
			else if (cams >= 9 && cams <= 31)	size = 33;
			else {
				err("\"cams\" parameter too high!");
				return -1;
			}
			break;
		case BRG_OV518:
		case BRG_OV518PLUS:
			if (cams == 1)				size = 896;
			else if (cams == 2)			size = 512;
			else if (cams == 3 || cams == 4)	size = 256;
			else if (cams >= 5 && cams <= 8)	size = 128;
			else {
				err("\"cams\" parameter too high!");
				return -1;
			}
			break;
		case BRG_OV519:
			if (cams == 1)				size = 896;
			else if (cams == 2)			size = 512;
			else {
				err("\"cams\" parameter too high!");
				return -1;
			}
			break;
		default:
			err("invalid bridge type");
			return -1;
	}

	// FIXME: OV518+ is hardcoded to 15 FPS (alternate 5) for now
	if (ov->bridge == BRG_OV518PLUS) {
		if (packetsize == -1) {
			ov518_set_packet_size(ov, 640);
		} else {
			info("Forcing packet size to %d", packetsize);
			ov518_set_packet_size(ov, packetsize);
		}
	} else if (ov->bridge == BRG_OV518) {
		if (packetsize == -1) {
			ov518_set_packet_size(ov, 896);
		} else {
			info("Forcing packet size to %d", packetsize);
			ov518_set_packet_size(ov, packetsize);
		}
	} else if (ov->bridge == BRG_OV519) {
		if (packetsize == -1) {
			ov519_set_packet_size(ov, size);
		} else {
			info("Forcing packet size to %d", packetsize);
			ov519_set_packet_size(ov, packetsize);
		}
	} else {
		if (packetsize == -1) {
			ov511_set_packet_size(ov, size);
		} else {
			info("Forcing packet size to %d", packetsize);
			ov511_set_packet_size(ov, packetsize);
		}
	}

	for (n = 0; n < OV511_NUMSBUF; n++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 5)
		urb = usb_alloc_urb(FRAMES_PER_DESC, GFP_KERNEL);
#else
		urb = usb_alloc_urb(FRAMES_PER_DESC);
#endif
		if (!urb) {
			err("init isoc: usb_alloc_urb ret. NULL");
			return -ENOMEM;
		}
		ov->sbuf[n].urb = urb;
		urb->dev = ov->dev;
		urb->context = &ov->sbuf[n];
		urb->pipe = usb_rcvisocpipe(ov->dev, OV511_ENDPOINT_ADDRESS);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 45)
		urb->transfer_flags = URB_ISO_ASAP;
#else
		urb->transfer_flags = USB_ISO_ASAP;
#endif
		urb->transfer_buffer = ov->sbuf[n].data;
		urb->complete = ov51x_isoc_irq;
		urb->number_of_packets = FRAMES_PER_DESC;
		urb->transfer_buffer_length = ov->packet_size * FRAMES_PER_DESC;
		urb->interval = 1;
		for (fx = 0; fx < FRAMES_PER_DESC; fx++) {
			urb->iso_frame_desc[fx].offset = ov->packet_size * fx;
			urb->iso_frame_desc[fx].length = ov->packet_size;
		}
	}

	ov->streaming = 1;

	for (n = 0; n < OV511_NUMSBUF; n++) {
		ov->sbuf[n].urb->dev = ov->dev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 4)
		err = usb_submit_urb(ov->sbuf[n].urb, GFP_KERNEL);
#else
		err = usb_submit_urb(ov->sbuf[n].urb);
#endif
		if (err) {
			err("init isoc: usb_submit_urb(%d) ret %d", n, err);
			return err;
		}
	}

	return 0;
}

static void
ov51x_unlink_isoc(struct usb_ov511 *ov)
{
	int n;

	/* Unschedule all of the iso td's */
	for (n = OV511_NUMSBUF - 1; n >= 0; n--) {
		if (ov->sbuf[n].urb) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 10)
			usb_unlink_urb(ov->sbuf[n].urb);
#else
			usb_kill_urb(ov->sbuf[n].urb);
#endif
			usb_free_urb(ov->sbuf[n].urb);
			ov->sbuf[n].urb = NULL;
		}
	}
}

static void
ov51x_stop_isoc(struct usb_ov511 *ov)
{
	if (!ov->streaming || !ov->dev)
		return;

	PDEBUG(3, "*** Stopping capture ***");

	switch (ov->bclass) {
		case BCL_OV511:
			ov511_set_packet_size(ov, 0);
			break;
		case BCL_OV518:
			ov518_set_packet_size(ov, 0);
			break;
		case BCL_OV519:
			ov519_set_packet_size(ov, 0);
			break;
	}
	ov->streaming = 0;

	ov51x_unlink_isoc(ov);
}

static int
ov51x_new_frame(struct usb_ov511 *ov, int framenum)
{
	struct ov511_frame *frame;
	int newnum;

	PDEBUG(4, "ov->curframe = %d, framenum = %d", ov->curframe, framenum);

	if (!ov->dev)
		return -1;

	/* If we're not grabbing a frame right now and the other frame is */
	/* ready to be grabbed into, then use it instead */
	if (ov->curframe == -1) {
		newnum = (framenum - 1 + OV511_NUMFRAMES) % OV511_NUMFRAMES;
		if (ov->frame[newnum].grabstate == FRAME_READY)
			framenum = newnum;
	} else
		return 0;

	frame = &ov->frame[framenum];

	PDEBUG(4, "framenum = %d, width = %d, height = %d", framenum,
	       frame->width, frame->height);

	frame->grabstate = FRAME_GRABBING;
	frame->scanstate = STATE_SCANNING;
	frame->snapshot = 0;

	ov->curframe = framenum;

	/* Make sure it's not too big */
	if (frame->width > ov->maxwidth)
		frame->width = ov->maxwidth;

	frame->width &= ~7L;		/* Multiple of 8 */

	if (frame->height > ov->maxheight)
		frame->height = ov->maxheight;

	frame->height &= ~3L;		/* Multiple of 4 */

	return 0;
}

/****************************************************************************
 *
 * Buffer management
 *
 ***************************************************************************/

/*
 * - You must acquire buf_lock before entering this function.
 * - Because this code will free any non-null pointer, you must be sure to null
 *   them if you explicitly free them somewhere else!
 */
static void
ov51x_do_dealloc(struct usb_ov511 *ov)
{
	int i;
	PDEBUG(4, "entered");

	if (ov->fbuf) {
		rvfree(ov->fbuf, OV511_NUMFRAMES
		       * MAX_DATA_SIZE(ov->maxwidth, ov->maxheight));
		ov->fbuf = NULL;
	}

	vfree(ov->rawfbuf);
	ov->rawfbuf = NULL;

	vfree(ov->tempfbuf);
	ov->tempfbuf = NULL;

	for (i = 0; i < OV511_NUMSBUF; i++) {
		kfree(ov->sbuf[i].data);
		ov->sbuf[i].data = NULL;
	}

	for (i = 0; i < OV511_NUMFRAMES; i++) {
		ov->frame[i].data = NULL;
		ov->frame[i].rawdata = NULL;
		ov->frame[i].tempdata = NULL;
		if (ov->frame[i].compbuf) {
			free_page((unsigned long) ov->frame[i].compbuf);
			ov->frame[i].compbuf = NULL;
		}
	}

	PDEBUG(4, "buffer memory deallocated");
	ov->buf_state = BUF_NOT_ALLOCATED;
	PDEBUG(4, "leaving");
}

static int
ov51x_alloc(struct usb_ov511 *ov)
{
	int i;
	const int w = ov->maxwidth;
	const int h = ov->maxheight;
	const int data_bufsize = OV511_NUMFRAMES * MAX_DATA_SIZE(w, h);
	const int raw_bufsize = OV511_NUMFRAMES * MAX_RAW_DATA_SIZE(w, h);

	PDEBUG(4, "entered");
	down(&ov->buf_lock);

	if (ov->buf_state == BUF_ALLOCATED)
		goto out;

	ov->fbuf = rvmalloc(data_bufsize);
	if (!ov->fbuf)
		goto error;

	ov->rawfbuf = vmalloc(raw_bufsize);
	if (!ov->rawfbuf)
		goto error;

	memset(ov->rawfbuf, 0, raw_bufsize);

	ov->tempfbuf = vmalloc(raw_bufsize);
	if (!ov->tempfbuf)
		goto error;

	memset(ov->tempfbuf, 0, raw_bufsize);

	for (i = 0; i < OV511_NUMSBUF; i++) {
		ov->sbuf[i].data = kmalloc(FRAMES_PER_DESC *
			MAX_FRAME_SIZE_PER_DESC, GFP_KERNEL);
		if (!ov->sbuf[i].data)
			goto error;

		PDEBUG(4, "sbuf[%d] @ %p", i, ov->sbuf[i].data);
	}

	for (i = 0; i < OV511_NUMFRAMES; i++) {
		ov->frame[i].data = ov->fbuf + i * MAX_DATA_SIZE(w, h);
		ov->frame[i].rawdata = ov->rawfbuf
		 + i * MAX_RAW_DATA_SIZE(w, h);
		ov->frame[i].tempdata = ov->tempfbuf
		 + i * MAX_RAW_DATA_SIZE(w, h);

		ov->frame[i].compbuf =
		 (unsigned char *) __get_free_page(GFP_KERNEL);
		if (!ov->frame[i].compbuf)
			goto error;

		PDEBUG(4, "frame[%d] @ %p", i, ov->frame[i].data);
	}

	ov->buf_state = BUF_ALLOCATED;
out:
	up(&ov->buf_lock);
	PDEBUG(4, "leaving");
	return 0;
error:
	ov51x_do_dealloc(ov);
	up(&ov->buf_lock);
	PDEBUG(4, "errored");
	return -ENOMEM;
}

static void
ov51x_dealloc(struct usb_ov511 *ov)
{
	PDEBUG(4, "entered");
	down(&ov->buf_lock);
	ov51x_do_dealloc(ov);
	up(&ov->buf_lock);
	PDEBUG(4, "leaving");
}

/****************************************************************************
 *
 * V4L 1 API
 *
 ***************************************************************************/

#ifdef OV511_OLD_V4L
static int
ov51x_v4l1_open(struct video_device *vdev, int flags)
{
#else
static int
ov51x_v4l1_open(struct inode *inode, struct file *file)
{
	struct video_device *vdev = video_devdata(file);
#endif
	struct usb_ov511 *ov = video_get_drvdata(vdev);
	int err, i;

/* 2.2.x needs explicit module-locking */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 17)
	MOD_INC_USE_COUNT;
#endif

	PDEBUG(4, "opening");

	down(&ov->lock);

	err = -EBUSY;
	if (ov->user)
		goto out;

	ov->sub_flag = 0;

	/* In case app doesn't set them... */
	err = ov51x_set_default_params(ov);
	if (err < 0)
		goto out;

	/* Make sure frames are reset */
	for (i = 0; i < OV511_NUMFRAMES; i++) {
		ov->frame[i].grabstate = FRAME_UNUSED;
		ov->frame[i].bytes_read = 0;
	}

	/* If compression is on, make sure now that a
	 * decompressor can be loaded */
	if (ov->compress && !ov->decomp_ops) {
		err = request_decompressor(ov);
		if (err && !dumppix)
			goto out;
	}

	err = ov51x_alloc(ov);
	if (err < 0)
		goto out;

	err = ov51x_init_isoc(ov);
	if (err) {
		ov51x_dealloc(ov);
		goto out;
	}

	ov->user++;
// If using _NEW_ V4L...
#if !defined(OV511_OLD_V4L)
	file->private_data = vdev;
#endif

	if (ov->led_policy == LED_AUTO)
		ov51x_led_control(ov, 1);

out:
	up(&ov->lock);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 17)
	if (err)
		MOD_DEC_USE_COUNT;
#endif

	return err;
}

#ifdef OV511_OLD_V4L
static void
ov51x_v4l1_close(struct video_device *vdev)
{
#else
static int
ov51x_v4l1_close(struct inode *inode, struct file *file)
{
	struct video_device *vdev = file->private_data;
#endif
	struct usb_ov511 *ov = video_get_drvdata(vdev);

	PDEBUG(4, "ov511_close");

	down(&ov->lock);

	ov->user--;
	ov51x_stop_isoc(ov);

	release_decompressor(ov);

	if (ov->led_policy == LED_AUTO)
		ov51x_led_control(ov, 0);

	if (ov->dev)
		ov51x_dealloc(ov);

	up(&ov->lock);

	/* Device unplugged while open. Only a minimum of unregistration is done
	 * here; the disconnect callback already did the rest. */
	if (!ov->dev) {
		down(&ov->cbuf_lock);
		kfree(ov->cbuf);
		ov->cbuf = NULL;
		up(&ov->cbuf_lock);

		ov51x_dealloc(ov);
#ifdef OV511_OLD_V4L
		video_unregister_device(&ov->vdev);
#endif
		kfree(ov);
		ov = NULL;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 17)
	MOD_DEC_USE_COUNT;
#endif

#ifdef OV511_OLD_V4L
	return;
#else
	file->private_data = NULL;
	return 0;
#endif
}

/* Do not call this function directly! */
static int
#ifdef OV511_OLD_V4L
ov51x_v4l1_ioctl_internal(struct usb_ov511 *ov, unsigned int cmd,
			  void *arg)
{
#else
ov51x_v4l1_ioctl_internal(struct inode *inode, struct file *file,
			  unsigned int cmd, void *arg)
{
	struct video_device *vdev = file->private_data;
	struct usb_ov511 *ov = video_get_drvdata(vdev);
#endif
	PDEBUG(5, "IOCtl: 0x%X", cmd);

	if (!ov->dev)
		return -EIO;

	switch (cmd) {
	case VIDIOCGCAP:
	{
		struct video_capability *b = arg;

		PDEBUG(4, "VIDIOCGCAP");

		memset(b, 0, sizeof(struct video_capability));
		sprintf(b->name, "%s USB Camera",
			symbolic(brglist, ov->bridge));
		b->type = VID_TYPE_CAPTURE | VID_TYPE_SUBCAPTURE;
		b->channels = ov->num_inputs;
		b->audios = 0;
		b->maxwidth = ov->maxwidth;
		b->maxheight = ov->maxheight;
		b->minwidth = ov->minwidth;
		b->minheight = ov->minheight;

		return 0;
	}
	case VIDIOCGCHAN:
	{
		struct video_channel *v = arg;

		PDEBUG(4, "VIDIOCGCHAN");

		if ((unsigned)(v->channel) >= ov->num_inputs) {
			err("Invalid channel (%d)", v->channel);
			return -EINVAL;
		}

		v->norm = ov->norm;
		v->type = VIDEO_TYPE_CAMERA;
		v->flags = 0;
//		v->flags |= (ov->has_decoder) ? VIDEO_VC_NORM : 0;
		v->tuners = 0;
		decoder_get_input_name(ov, v->channel, v->name);

		return 0;
	}
	case VIDIOCSCHAN:
	{
		struct video_channel *v = arg;
		int err;

		PDEBUG(4, "VIDIOCSCHAN");

		/* Make sure it's not a camera */
		if (!ov->has_decoder) {
			if (v->channel == 0)
				return 0;
			else
				return -EINVAL;
		}

		if (v->norm != VIDEO_MODE_PAL &&
		    v->norm != VIDEO_MODE_NTSC &&
		    v->norm != VIDEO_MODE_SECAM &&
		    v->norm != VIDEO_MODE_AUTO) {
			err("Invalid norm (%d)", v->norm);
			return -EINVAL;
		}

		if ((unsigned)(v->channel) >= ov->num_inputs) {
			err("Invalid channel (%d)", v->channel);
			return -EINVAL;
		}

		err = decoder_set_input(ov, v->channel);
		if (err)
			return err;

		err = decoder_set_norm(ov, v->norm);
		if (err)
			return err;

		return 0;
	}
	case VIDIOCGPICT:
	{
		struct video_picture *p = arg;

		PDEBUG(4, "VIDIOCGPICT");

		memset(p, 0, sizeof(struct video_picture));
		if (sensor_get_picture(ov, p))
			return -EIO;

		/* Can we get these from frame[0]? -claudio? */
		p->depth = ov->frame[0].depth;
		p->palette = ov->frame[0].format;

		return 0;
	}
	case VIDIOCSPICT:
	{
		struct video_picture *p = arg;
		int i, rc;

		PDEBUG(4, "VIDIOCSPICT");

		if (!get_depth(p->palette))
			return -EINVAL;

		if (sensor_set_picture(ov, p))
			return -EIO;

		if (force_palette && p->palette != force_palette) {
			info("Palette rejected (%s)",
			     symbolic(v4l1_plist, p->palette));
			return -EINVAL;
		}

		// FIXME: Format should be independent of frames
		if (p->palette != ov->frame[0].format) {
			PDEBUG(4, "Detected format change");

			rc = ov51x_wait_frames_inactive(ov);
			if (rc)
				return rc;

			mode_init_regs(ov, ov->frame[0].width,
				ov->frame[0].height, p->palette, ov->sub_flag);
		}

		PDEBUG(4, "Setting depth=%d, palette=%s",
		       p->depth, symbolic(v4l1_plist, p->palette));

		for (i = 0; i < OV511_NUMFRAMES; i++) {
			ov->frame[i].depth = p->depth;
			ov->frame[i].format = p->palette;
		}

		return 0;
	}
	case VIDIOCGCAPTURE:
	{
		int *vf = arg;

		PDEBUG(4, "VIDIOCGCAPTURE");

		ov->sub_flag = *vf;
		return 0;
	}
	case VIDIOCSCAPTURE:
	{
		struct video_capture *vc = arg;

		PDEBUG(4, "VIDIOCSCAPTURE");

		if (vc->flags)
			return -EINVAL;
		if (vc->decimation)
			return -EINVAL;

		vc->x &= ~3L;
		vc->y &= ~1L;
		vc->y &= ~31L;

		if (vc->width == 0)
			vc->width = 32;

		vc->height /= 16;
		vc->height *= 16;
		if (vc->height == 0)
			vc->height = 16;

		ov->subx = vc->x;
		ov->suby = vc->y;
		ov->subw = vc->width;
		ov->subh = vc->height;

		return 0;
	}
	case VIDIOCSWIN:
	{
		struct video_window *vw = arg;
		int i, rc;

		PDEBUG(4, "VIDIOCSWIN: %dx%d", vw->width, vw->height);

#if 0
		if (vw->flags)
			return -EINVAL;
		if (vw->clipcount)
			return -EINVAL;
		if (vw->height != ov->maxheight)
			return -EINVAL;
		if (vw->width != ov->maxwidth)
			return -EINVAL;
#endif

		rc = ov51x_wait_frames_inactive(ov);
		if (rc)
			return rc;

		rc = mode_init_regs(ov, vw->width, vw->height,
			ov->frame[0].format, ov->sub_flag);
		if (rc < 0)
			return rc;

		for (i = 0; i < OV511_NUMFRAMES; i++) {
			ov->frame[i].width = vw->width;
			ov->frame[i].height = vw->height;
		}

		return 0;
	}
	case VIDIOCGWIN:
	{
		struct video_window *vw = arg;

		memset(vw, 0, sizeof(struct video_window));
		vw->x = 0;		/* FIXME */
		vw->y = 0;
		vw->width = ov->frame[0].width;
		vw->height = ov->frame[0].height;
		vw->flags = 30;

		PDEBUG(4, "VIDIOCGWIN: %dx%d", vw->width, vw->height);

		return 0;
	}
	case VIDIOCGMBUF:
	{
		struct video_mbuf *vm = arg;
		int i;

		PDEBUG(4, "VIDIOCGMBUF");

		memset(vm, 0, sizeof(struct video_mbuf));
		vm->size = OV511_NUMFRAMES
			   * MAX_DATA_SIZE(ov->maxwidth, ov->maxheight);
		vm->frames = OV511_NUMFRAMES;

		vm->offsets[0] = 0;
		for (i = 1; i < OV511_NUMFRAMES; i++) {
			vm->offsets[i] = vm->offsets[i-1]
			   + MAX_DATA_SIZE(ov->maxwidth, ov->maxheight);
		}

		return 0;
	}
	case VIDIOCMCAPTURE:
	{
		struct video_mmap *vm = arg;
		int rc, depth;
		unsigned int f = vm->frame;

		PDEBUG(4, "VIDIOCMCAPTURE: frame: %d, %dx%d, %s", f, vm->width,
			vm->height, symbolic(v4l1_plist, vm->format));

		depth = get_depth(vm->format);
		if (!depth) {
			PDEBUG(2, "VIDIOCMCAPTURE: invalid format (%s)",
			       symbolic(v4l1_plist, vm->format));
			return -EINVAL;
		}

		if (f >= OV511_NUMFRAMES) {
			err("VIDIOCMCAPTURE: invalid frame (%d)", f);
			return -EINVAL;
		}

		if (vm->width > ov->maxwidth
		    || vm->height > ov->maxheight) {
			err("VIDIOCMCAPTURE: requested dimensions too big");
			return -EINVAL;
		}

		if (ov->frame[f].grabstate == FRAME_GRABBING) {
			PDEBUG(4, "VIDIOCMCAPTURE: already grabbing");
			return -EBUSY;
		}

		if (force_palette && (vm->format != force_palette)) {
			PDEBUG(2, "palette rejected (%s)",
			       symbolic(v4l1_plist, vm->format));
			return -EINVAL;
		}

		if ((ov->frame[f].width != vm->width) ||
		    (ov->frame[f].height != vm->height) ||
		    (ov->frame[f].format != vm->format) ||
		    (ov->frame[f].sub_flag != ov->sub_flag) ||
		    (ov->frame[f].depth != depth)) {
			PDEBUG(4, "VIDIOCMCAPTURE: change in image parameters");

			rc = ov51x_wait_frames_inactive(ov);
			if (rc)
				return rc;

			rc = mode_init_regs(ov, vm->width, vm->height,
				vm->format, ov->sub_flag);
#if 0
			if (rc < 0) {
				PDEBUG(1, "Got error while initializing regs ");
				return ret;
			}
#endif
			ov->frame[f].width = vm->width;
			ov->frame[f].height = vm->height;
			ov->frame[f].format = vm->format;
			ov->frame[f].sub_flag = ov->sub_flag;
			ov->frame[f].depth = depth;
		}

		/* Mark it as ready */
		ov->frame[f].grabstate = FRAME_READY;

		PDEBUG(4, "VIDIOCMCAPTURE: renewing frame %d", f);

		return ov51x_new_frame(ov, f);
	}
	case VIDIOCSYNC:
	{
		unsigned int fnum = *((unsigned int *) arg);
		struct ov511_frame *frame;
		int rc;

		if (fnum >= OV511_NUMFRAMES) {
			err("VIDIOCSYNC: invalid frame (%d)", fnum);
			return -EINVAL;
		}

		frame = &ov->frame[fnum];

		PDEBUG(4, "syncing to frame %d, grabstate = %d", fnum,
		       frame->grabstate);

		switch (frame->grabstate) {
		case FRAME_UNUSED:
			return -EINVAL;
		case FRAME_READY:
		case FRAME_GRABBING:
		case FRAME_ERROR:
redo:
			if (!ov->dev)
				return -EIO;

			rc = wait_event_interruptible(frame->wq,
			    (frame->grabstate == FRAME_DONE)
			    || (frame->grabstate == FRAME_ERROR));

			if (rc)
				return rc;

			if (frame->grabstate == FRAME_ERROR) {
				if ((rc = ov51x_new_frame(ov, fnum)) < 0)
					return rc;
				goto redo;
			}
			/* Fall through */
		case FRAME_DONE:
			if (ov->snap_enabled && !frame->snapshot) {
				if ((rc = ov51x_new_frame(ov, fnum)) < 0)
					return rc;
				goto redo;
			}

			frame->grabstate = FRAME_UNUSED;

			/* Reset the hardware snapshot button */
			/* FIXME - Is this the best place for this? */
			if ((ov->snap_enabled) && (frame->snapshot)) {
				frame->snapshot = 0;
				ov51x_clear_snapshot(ov);
			}

			/* Decompression, format conversion, etc... */
			if (ov->bclass != BCL_OV519) {
				ov51x_postprocess(ov, frame);
			}

			break;
		} /* end switch */

		return 0;
	}
	case VIDIOCGFBUF:
	{
		struct video_buffer *vb = arg;

		PDEBUG(4, "VIDIOCGFBUF");

		memset(vb, 0, sizeof(struct video_buffer));

		return 0;
	}
	case VIDIOCGUNIT:
	{
		struct video_unit *vu = arg;

		PDEBUG(4, "VIDIOCGUNIT");

		memset(vu, 0, sizeof(struct video_unit));

		vu->video = ov->vdev->minor;
		vu->vbi = VIDEO_NO_UNIT;
		vu->radio = VIDEO_NO_UNIT;
		vu->audio = VIDEO_NO_UNIT;
		vu->teletext = VIDEO_NO_UNIT;

		return 0;
	}
	case OV511IOC_WI2C:
	{
		struct ov511_i2c_struct *w = arg;

		return i2c_w_slave(ov, w->slave, w->reg, w->value, w->mask);
	}
	case OV511IOC_RI2C:
	{
		struct ov511_i2c_struct *r = arg;
		int rc;

		rc = i2c_r_slave(ov, r->slave, r->reg);
		if (rc < 0)
			return rc;

		r->value = rc;
		return 0;
	}
	default:
		PDEBUG(3, "Unsupported IOCtl: 0x%X", cmd);
		return -ENOIOCTLCMD;
	} /* end switch */

	return 0;
}

#ifdef OV511_OLD_V4L
/* This is implemented as video_generic_ioctl() in the new V4L's videodev.c */
static int
ov51x_v4l1_generic_ioctl(struct video_device *vdev, unsigned int cmd, void *arg)
{
	char	sbuf[128];
	void    *mbuf = NULL;
	void	*parg = NULL;
	int	err  = -EINVAL;

	/*  Copy arguments into temp kernel buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_NONE:
		parg = arg;
		break;
	case _IOC_READ: /* some v4l ioctls are marked wrong ... */
	case _IOC_WRITE:
	case (_IOC_WRITE | _IOC_READ):
		if (_IOC_SIZE(cmd) <= sizeof(sbuf)) {
			parg = sbuf;
		} else {
			/* too big to allocate from stack */
			mbuf = kmalloc(_IOC_SIZE(cmd), GFP_KERNEL);
			if (NULL == mbuf)
				return -ENOMEM;
			parg = mbuf;
		}

		err = -EFAULT;
		if (copy_from_user(parg, arg, _IOC_SIZE(cmd)))
			goto out;
		break;
	}

	err = ov51x_v4l1_ioctl_internal(vdev->priv, cmd, parg);
	if (err == -ENOIOCTLCMD)
		err = -EINVAL;
	if (err < 0)
		goto out;

	/*  Copy results into user buffer  */
	switch (_IOC_DIR(cmd))
	{
	case _IOC_READ:
	case (_IOC_WRITE | _IOC_READ):
		if (copy_to_user(arg, parg, _IOC_SIZE(cmd)))
			err = -EFAULT;
		break;
	}

out:
	if (mbuf)
		kfree(mbuf);
	return err;
}

static int
ov51x_v4l1_ioctl(struct video_device *vdev, unsigned int cmd, void *arg)
{
	struct usb_ov511 *ov = vdev->priv;
	int rc;

	if (down_interruptible(&ov->lock))
		return -EINTR;

	rc = ov51x_v4l1_generic_ioctl(vdev, cmd, arg);

	up(&ov->lock);
	return rc;
}

#else	/* If new V4L API */

static int
ov51x_v4l1_ioctl(struct inode *inode, struct file *file,
		 unsigned int cmd, unsigned long arg)
{
	struct video_device *vdev = file->private_data;
	struct usb_ov511 *ov = video_get_drvdata(vdev);
	int rc;

	if (down_interruptible(&ov->lock))
		return -EINTR;

	rc = video_usercopy(inode, file, cmd, arg, ov51x_v4l1_ioctl_internal);

	up(&ov->lock);
	return rc;
}
#endif	/* OV511_OLD_V4L */

#ifdef OV511_OLD_V4L
static long
ov51x_v4l1_read(struct video_device *vdev, char *buf, unsigned long count,
		int noblock)
{
#else
static ssize_t
ov51x_v4l1_read(struct file *file, char *buf, size_t cnt, loff_t *ppos)
{
	struct video_device *vdev = file->private_data;
	int noblock = file->f_flags&O_NONBLOCK;
	unsigned long count = cnt;
#endif
	struct usb_ov511 *ov = video_get_drvdata(vdev);
	int i, rc = 0, frmx = -1;
	struct ov511_frame *frame;

	if (down_interruptible(&ov->lock))
		return -EINTR;

	PDEBUG(4, "%ld bytes, noblock=%d", count, noblock);

	if (!vdev || !buf) {
		rc = -EFAULT;
		goto error;
	}

	if (!ov->dev) {
		rc = -EIO;
		goto error;
	}

// FIXME: Only supports two frames
	/* See if a frame is completed, then use it. */
	if (ov->frame[0].grabstate >= FRAME_DONE)	/* _DONE or _ERROR */
		frmx = 0;
	else if (ov->frame[1].grabstate >= FRAME_DONE)/* _DONE or _ERROR */
		frmx = 1;

	/* If nonblocking we return immediately */
	if (noblock && (frmx == -1)) {
		rc = -EAGAIN;
		goto error;
	}

	/* If no FRAME_DONE, look for a FRAME_GRABBING state. */
	/* See if a frame is in process (grabbing), then use it. */
	if (frmx == -1) {
		if (ov->frame[0].grabstate == FRAME_GRABBING)
			frmx = 0;
		else if (ov->frame[1].grabstate == FRAME_GRABBING)
			frmx = 1;
	}

	/* If no frame is active, start one. */
	if (frmx == -1) {
		if ((rc = ov51x_new_frame(ov, frmx = 0))) {
			err("read: ov51x_new_frame error");
			goto error;
		}
	}

	frame = &ov->frame[frmx];

restart:
	if (!ov->dev) {
		rc = -EIO;
		goto error;
	}

	/* Wait while we're grabbing the image */
	PDEBUG(4, "Waiting image grabbing");
	rc = wait_event_interruptible(frame->wq,
		(frame->grabstate == FRAME_DONE)
		|| (frame->grabstate == FRAME_ERROR));

	if (rc)
		goto error;

	PDEBUG(4, "Got image, frame->grabstate = %d", frame->grabstate);
	PDEBUG(4, "bytes_recvd = %d", frame->bytes_recvd);

	if (frame->grabstate == FRAME_ERROR) {
		frame->bytes_read = 0;
		err("** ick! ** Errored frame %d", ov->curframe);
		if (ov51x_new_frame(ov, frmx)) {
			err("read: ov51x_new_frame error");
			goto error;
		}
		goto restart;
	}


	/* Repeat until we get a snapshot frame */
	if (ov->snap_enabled)
		PDEBUG(4, "Waiting snapshot frame");
	if (ov->snap_enabled && !frame->snapshot) {
		frame->bytes_read = 0;
		if ((rc = ov51x_new_frame(ov, frmx))) {
			err("read: ov51x_new_frame error");
			goto error;
		}
		goto restart;
	}

	/* Clear the snapshot */
	if (ov->snap_enabled && frame->snapshot) {
		frame->snapshot = 0;
		ov51x_clear_snapshot(ov);
	}

	/* Decompression, format conversion, etc... */
	if (ov->bclass != BCL_OV519) {
		ov51x_postprocess(ov, frame);
	}

	PDEBUG(4, "frmx=%d, bytes_read=%ld, length=%ld", frmx,
		frame->bytes_read,
		get_frame_length(ov, frame));

	/* copy bytes to user space; we allow for partials reads */
	if ((count + frame->bytes_read) > get_frame_length(ov, (struct ov511_frame *)frame)) {
		count = frame->bytes_recvd - frame->bytes_read;
		PDEBUG(4, "set count to %d", (int)count);
	}

	/* FIXME - count hardwired to be one frame... */
	//count = get_frame_length(ov, frame);

	PDEBUG(4, "Copy to user space: %ld bytes", count);
	if ((i = copy_to_user(buf, frame->data + frame->bytes_read, count))) {
		PDEBUG(4, "Copy failed! %d bytes not copied", i);
		rc = -EFAULT;
		goto error;
	}

	frame->bytes_read += count;
	PDEBUG(4, "{copy} count used=%ld, new bytes_read=%ld",
		count, frame->bytes_read);

	/* If all data have been read... */
	if (frame->bytes_read >= get_frame_length(ov, frame)) {
		frame->bytes_read = 0;

// FIXME: Only supports two frames
		/* Mark it as available to be used again. */
		ov->frame[frmx].grabstate = FRAME_UNUSED;
		if ((rc = ov51x_new_frame(ov, !frmx))) {
			err("ov51x_new_frame returned error");
			goto error;
		}
	}

	PDEBUG(4, "read finished, returning %ld (sweet)", count);

	up(&ov->lock);
	return count;

error:
	up(&ov->lock);
	return rc;
}

static int
#ifdef OV511_OLD_V4L
  #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 3)
ov51x_v4l1_mmap(struct vm_area_struct *vma, struct video_device *vdev,
		const char *adr, unsigned long size)
  #else
ov51x_v4l1_mmap(struct video_device *vdev, const char *adr, unsigned long size)
  #endif	/* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 3) */
{
	unsigned long start = (unsigned long)adr;

#else	/* New V4L API */

ov51x_v4l1_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct video_device *vdev = file->private_data;
	unsigned long start = vma->vm_start;
	unsigned long size  = vma->vm_end - vma->vm_start;
#endif	/* OV511_OLD_V4L */

	struct usb_ov511 *ov = video_get_drvdata(vdev);
	unsigned long page, pos;

	if (ov->dev == NULL)
		return -EIO;

	PDEBUG(4, "mmap: %ld (%lX) bytes", size, size);

	if (size > (((OV511_NUMFRAMES
	              * MAX_DATA_SIZE(ov->maxwidth, ov->maxheight)
	              + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))))
		return -EINVAL;

	if (down_interruptible(&ov->lock))
		return -EINTR;

	pos = (unsigned long)ov->fbuf;
	while (size > 0) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 10)
		page = vmalloc_to_pfn((void *)pos);
		if (remap_pfn_range(vma, start, page, PAGE_SIZE, PAGE_SHARED)) {
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 3) || defined(RH9_REMAP)
		page = kvirt_to_pa(pos);
		if (remap_page_range(vma, start, page, PAGE_SIZE,
				     PAGE_SHARED)) {
#else
		page = kvirt_to_pa(pos);
		if (remap_page_range(start, page, PAGE_SIZE, PAGE_SHARED)) {
#endif
			up(&ov->lock);
			return -EAGAIN;
		}
		start += PAGE_SIZE;
		pos += PAGE_SIZE;
		if (size > PAGE_SIZE)
			size -= PAGE_SIZE;
		else
			size = 0;
	}

	up(&ov->lock);
	return 0;
}

#ifdef OV511_OLD_V4L
static struct video_device vdev_template = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 17)
	.owner =	THIS_MODULE,
#endif
	.name =		"OV511 USB Camera",
	.type =		VID_TYPE_CAPTURE,
	.hardware =	VID_HARDWARE_OV511,
	.open =		ov51x_v4l1_open,
	.close =	ov51x_v4l1_close,
	.read =		ov51x_v4l1_read,
	.ioctl =	ov51x_v4l1_ioctl,
	.mmap =		ov51x_v4l1_mmap,
};

#else	/* New V4L API */

static struct file_operations ov511_fops = {
	.owner =	THIS_MODULE,
	.open =		ov51x_v4l1_open,
	.release =	ov51x_v4l1_close,
	.read =		ov51x_v4l1_read,
	.mmap =		ov51x_v4l1_mmap,
	.ioctl =	ov51x_v4l1_ioctl,
	.llseek =	no_llseek,
};

static struct video_device vdev_template = {
	.owner =	THIS_MODULE,
	.name =		"OV51x USB Camera",
	.type =		VID_TYPE_CAPTURE,
	.hardware =	VID_HARDWARE_OV511,
	.fops =		&ov511_fops,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	.release =	video_device_release,
#endif
	.minor =	-1,
};
#endif	/* OV511_OLD_V4L */

#if defined(CONFIG_VIDEO_PROC_FS)
static int
ov51x_control_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		    unsigned long ularg)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 5)
	struct proc_dir_entry *pde = PDE(inode);
#else
	struct proc_dir_entry *pde = inode->u.generic_ip;
#endif
	struct usb_ov511 *ov;
	void *arg = (void *) ularg;
	int rc;

	if (!pde)
		return -ENOENT;

	ov = pde->data;
	if (!ov)
		return -ENODEV;

	if (!ov->dev)
		return -EIO;

	/* Should we pass through standard V4L IOCTLs? */

	switch (cmd) {
	case OV511IOC_GINTVER:
	{
		return OV511_INTERFACE_VER;
	}
	case OV511IOC_GUSHORT:
	{
		struct ov511_ushort_opt opt;

		if (copy_from_user(&opt, arg, sizeof(opt)))
			return -EFAULT;

		switch (opt.optnum) {
		case OV511_USOPT_BRIGHT:
			rc = sensor_get_brightness(ov, &(opt.val));
			if (rc)	return rc;
			break;
		case OV511_USOPT_SAT:
			rc = sensor_get_saturation(ov, &(opt.val));
			if (rc)	return rc;
			break;
		case OV511_USOPT_HUE:
			rc = sensor_get_hue(ov, &(opt.val));
			if (rc)	return rc;
			break;
		case OV511_USOPT_CONTRAST:
			rc = sensor_get_contrast(ov, &(opt.val));
			if (rc)	return rc;
			break;
		default:
			err("Invalid get short option number");
			return -EINVAL;
		}

		if (copy_to_user(arg, &opt, sizeof(opt)))
			return -EFAULT;

		return 0;
	}
	case OV511IOC_SUSHORT:
	{
		struct ov511_ushort_opt opt;

		if (copy_from_user(&opt, arg, sizeof(opt)))
			return -EFAULT;

		switch (opt.optnum) {
		case OV511_USOPT_BRIGHT:
			rc = sensor_set_brightness(ov, opt.val);
			if (rc)	return rc;
			break;
		case OV511_USOPT_SAT:
			rc = sensor_set_saturation(ov, opt.val);
			if (rc)	return rc;
			break;
		case OV511_USOPT_HUE:
			rc = sensor_set_hue(ov, opt.val);
			if (rc)	return rc;
			break;
		case OV511_USOPT_CONTRAST:
			rc = sensor_set_contrast(ov, opt.val);
			if (rc)	return rc;
			break;
		default:
			err("Invalid set short option number");
			return -EINVAL;
		}

		return 0;
	}
	case OV511IOC_GUINT:
	{
		struct ov511_uint_opt opt;

		if (copy_from_user(&opt, arg, sizeof(opt)))
			return -EFAULT;

		switch (opt.optnum) {
		case OV511_UIOPT_POWER_FREQ:
			opt.val = ov->lightfreq;
			break;
		case OV511_UIOPT_BFILTER:
			opt.val = ov->bandfilt;
			break;
		case OV511_UIOPT_LED:
			opt.val = ov->led_policy;
			break;
		case OV511_UIOPT_LED2:
			opt.val = ov->led2_policy;
			break;
		case OV511_UIOPT_DEBUG:
			opt.val = debug;
			break;
		case OV511_UIOPT_COMPRESS:
			opt.val = ov->compress;
			break;
		default:
			err("Invalid get int option number");
			return -EINVAL;
		}

		if (copy_to_user(arg, &opt, sizeof(opt)))
			return -EFAULT;

		return 0;
	}
	case OV511IOC_SUINT:
	{
		struct ov511_uint_opt opt;

		if (copy_from_user(&opt, arg, sizeof(opt)))
			return -EFAULT;

		switch (opt.optnum) {
		case OV511_UIOPT_POWER_FREQ:
			rc = sensor_set_light_freq(ov, opt.val);
			if (rc)	return rc;
			break;
		case OV511_UIOPT_BFILTER:
			rc = sensor_set_banding_filter(ov, opt.val);
			if (rc)	return rc;
			break;
		case OV511_UIOPT_LED:
			if (opt.val <= 2) {
				ov->led_policy = opt.val;
				if (ov->led_policy == LED_OFF)
					ov51x_led_control(ov, 0);
				else if (ov->led_policy == LED_ON)
					ov51x_led_control(ov, 1);
			} else {
				return -EINVAL;
			}
			break;
		case OV511_UIOPT_LED2:
			if (opt.val <= 2) {
				ov->led2_policy = opt.val;
				if (ov->led2_policy == LED_OFF)
					ov51x_led_control(ov, 0);
				else if (ov->led2_policy == LED_ON)
					ov51x_led_control(ov, 1);
			} else {
				return -EINVAL;
			}
			break;
		case OV511_UIOPT_DEBUG:
			if (opt.val <= 5)
				debug = opt.val;
			else
				return -EINVAL;
			break;
		case OV511_UIOPT_COMPRESS:
			ov->compress = opt.val;
			if (ov->compress) {
				if (ov->bclass == BCL_OV511)
					ov511_init_compression(ov);
				else if (ov->bclass == BCL_OV518)
					ov518_init_compression(ov);
				else if (ov->bclass == BCL_OV519)
					ov519_init_compression(ov);
			}
			break;
		default:
			err("Invalid get int option number");
			return -EINVAL;
		}

		return 0;
	}
	case OV511IOC_WI2C:
	{
		struct ov511_i2c_struct w;

		if (copy_from_user(&w, arg, sizeof(w)))
			return -EFAULT;

		return i2c_w_slave(ov, w.slave, w.reg, w.value,	w.mask);
	}
	case OV511IOC_RI2C:
	{
		struct ov511_i2c_struct r;

		if (copy_from_user(&r, arg, sizeof(r)))
			return -EFAULT;

		rc = i2c_r_slave(ov, r.slave, r.reg);
		if (rc < 0)
			return rc;

		r.value = rc;

		if (copy_to_user(arg, &r, sizeof(r)))
			return -EFAULT;

		return 0;
	}
	default:
		return -EINVAL;
	}

	return 0;
}
#endif

/****************************************************************************
 *
 * OV511 and sensor configuration
 *
 ***************************************************************************/

/* This initializes the OV8110, OV8610 sensor. The OV8110 uses
 * the same register settings as the OV8610, since they are very similar.
 */
static int
ov8xx0_configure(struct usb_ov511 *ov)
{
	int rc;

	static struct ov511_regvals regvals_norm_8610[] = {
		{ OV511_I2C_BUS, 0x12, 0x80 },
		{ OV511_I2C_BUS, 0x00, 0x00 },
		{ OV511_I2C_BUS, 0x01, 0x80 },
		{ OV511_I2C_BUS, 0x02, 0x80 },
		{ OV511_I2C_BUS, 0x03, 0xc0 },
		{ OV511_I2C_BUS, 0x04, 0x30 },
		{ OV511_I2C_BUS, 0x05, 0x30 }, /* was 0x10, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x06, 0x70 }, /* was 0x80, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x0a, 0x86 },
		{ OV511_I2C_BUS, 0x0b, 0xb0 },
		{ OV511_I2C_BUS, 0x0c, 0x20 },
		{ OV511_I2C_BUS, 0x0d, 0x20 },
		{ OV511_I2C_BUS, 0x11, 0x01 },
		{ OV511_I2C_BUS, 0x12, 0x25 },
		{ OV511_I2C_BUS, 0x13, 0x01 },
		{ OV511_I2C_BUS, 0x14, 0x04 },
		{ OV511_I2C_BUS, 0x15, 0x01 }, /* Lin and Win think different about UV order */
		{ OV511_I2C_BUS, 0x16, 0x03 },
		{ OV511_I2C_BUS, 0x17, 0x38 }, /* was 0x2f, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x18, 0xea }, /* was 0xcf, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x19, 0x02 }, /* was 0x06, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x1a, 0xf5 },
		{ OV511_I2C_BUS, 0x1b, 0x00 },
		{ OV511_I2C_BUS, 0x20, 0xd0 }, /* was 0x90, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x23, 0xc0 }, /* was 0x00, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x24, 0x30 }, /* was 0x1d, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x25, 0x50 }, /* was 0x57, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x26, 0xa2 },
		{ OV511_I2C_BUS, 0x27, 0xea },
		{ OV511_I2C_BUS, 0x28, 0x00 },
		{ OV511_I2C_BUS, 0x29, 0x00 },
		{ OV511_I2C_BUS, 0x2a, 0x80 },
		{ OV511_I2C_BUS, 0x2b, 0xc8 }, /* was 0xcc, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x2c, 0xac },
		{ OV511_I2C_BUS, 0x2d, 0x45 }, /* was 0xd5, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x2e, 0x80 },
		{ OV511_I2C_BUS, 0x2f, 0x14 }, /* was 0x01, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x4c, 0x00 },
		{ OV511_I2C_BUS, 0x4d, 0x30 }, /* was 0x10, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x60, 0x02 }, /* was 0x01, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x61, 0x00 }, /* was 0x09, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x62, 0x5f }, /* was 0xd7, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x63, 0xff },
		{ OV511_I2C_BUS, 0x64, 0x53 }, /* new windrv 090403 says 0x57, maybe thats wrong */
		{ OV511_I2C_BUS, 0x65, 0x00 },
		{ OV511_I2C_BUS, 0x66, 0x55 },
		{ OV511_I2C_BUS, 0x67, 0xb0 },
		{ OV511_I2C_BUS, 0x68, 0xc0 }, /* was 0xaf, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x69, 0x02 },
		{ OV511_I2C_BUS, 0x6a, 0x22 },
		{ OV511_I2C_BUS, 0x6b, 0x00 },
		{ OV511_I2C_BUS, 0x6c, 0x99 }, /* was 0x80, old windrv says 0x00, but deleting bit7 colors the first images red */
		{ OV511_I2C_BUS, 0x6d, 0x11 }, /* was 0x00, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x6e, 0x11 }, /* was 0x00, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x6f, 0x01 },
		{ OV511_I2C_BUS, 0x70, 0x8b },
		{ OV511_I2C_BUS, 0x71, 0x00 },
		{ OV511_I2C_BUS, 0x72, 0x14 },
		{ OV511_I2C_BUS, 0x73, 0x54 },
		{ OV511_I2C_BUS, 0x74, 0x00 },//0x60? /* was 0x00, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x75, 0x0e },
		{ OV511_I2C_BUS, 0x76, 0x02 }, /* was 0x02, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x77, 0xff },
		{ OV511_I2C_BUS, 0x78, 0x80 },
		{ OV511_I2C_BUS, 0x79, 0x80 },
		{ OV511_I2C_BUS, 0x7a, 0x80 },
		{ OV511_I2C_BUS, 0x7b, 0x10 }, /* was 0x13, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x7c, 0x00 },
		{ OV511_I2C_BUS, 0x7d, 0x08 }, /* was 0x09, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x7e, 0x08 }, /* was 0xc0, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x7f, 0xfb },
		{ OV511_I2C_BUS, 0x80, 0x28 },
		{ OV511_I2C_BUS, 0x81, 0x00 },
		{ OV511_I2C_BUS, 0x82, 0x23 },
		{ OV511_I2C_BUS, 0x83, 0x0b },
		{ OV511_I2C_BUS, 0x84, 0x00 },
		{ OV511_I2C_BUS, 0x85, 0x62 }, /* was 0x61, new from windrv 090403 */
		{ OV511_I2C_BUS, 0x86, 0xc9 },
		{ OV511_I2C_BUS, 0x87, 0x00 },
		{ OV511_I2C_BUS, 0x88, 0x00 },
		{ OV511_I2C_BUS, 0x89, 0x01 },
		{ OV511_I2C_BUS, 0x12, 0x20 },
		{ OV511_I2C_BUS, 0x12, 0x25 }, /* was 0x24, new from windrv 090403 */
		{ OV511_DONE_BUS, 0x0, 0x00 },
	};

	PDEBUG(4, "starting configuration");

	if (init_ov_sensor(ov) < 0) {
		err("Failed to read sensor ID. You might not have an");
		err("OV8600/10, or it might not be responding. Report");
		err("this to <joerg@heckenbach-aw.de");
		err("This is only a warning. You can attempt to use");
		err("your camera anyway");
	} else {
		PDEBUG(1, "OV86x0 initialized");
	}

	/* Detect sensor (sub)type */
	rc = i2c_r(ov, OV7610_REG_COM_I);

	if (rc < 0) {
		err("Error detecting sensor type");
		return -1;
	} else if ((rc & 3) == 1) {
		info("Sensor is an OV8610");
		ov->sensor = SEN_OV8610;
	} else {
		err("Unknown image sensor version: %d", rc & 3);
		return -1;
	}

	PDEBUG(4, "Writing 8610 registers");
	if (write_regvals(ov, regvals_norm_8610))
		return -1;

	/* Set sensor-specific vars */
	ov->maxwidth = 800;
	ov->maxheight = 600;
	ov->minwidth = 64;
	ov->minheight = 48;

	// FIXME: These do not match the actual settings yet
	ov->brightness = 0x80 << 8;
	ov->contrast = 0x80 << 8;
	ov->colour = 0x80 << 8;
	ov->hue = 0x80 << 8;

	return 0;
}

/* This initializes the OV7610, OV7620, or OV76BE sensor. The OV76BE uses
 * the same register settings as the OV7610, since they are very similar.
 */
static int
ov7xx0_configure(struct usb_ov511 *ov)
{
	int rc, high, low;

	/* Lawrence Glaister <lg@jfm.bc.ca> reports:
	 *
	 * Register 0x0f in the 7610 has the following effects:
	 *
	 * 0x85 (AEC method 1): Best overall, good contrast range
	 * 0x45 (AEC method 2): Very overexposed
	 * 0xa5 (spec sheet default): Ok, but the black level is
	 *	shifted resulting in loss of contrast
	 * 0x05 (old driver setting): very overexposed, too much
	 *	contrast
	 */
	static struct ov511_regvals regvals_norm_7610[] = {
		{ OV511_I2C_BUS, 0x10, 0xff },
		{ OV511_I2C_BUS, 0x16, 0x06 },
		{ OV511_I2C_BUS, 0x28, 0x24 },
		{ OV511_I2C_BUS, 0x2b, 0xac },
		{ OV511_I2C_BUS, 0x12, 0x00 },
		{ OV511_I2C_BUS, 0x38, 0x81 },
		{ OV511_I2C_BUS, 0x28, 0x24 },	/* 0c */
		{ OV511_I2C_BUS, 0x0f, 0x85 },	/* lg's setting */
		{ OV511_I2C_BUS, 0x15, 0x01 },
		{ OV511_I2C_BUS, 0x20, 0x1c },
		{ OV511_I2C_BUS, 0x23, 0x2a },
		{ OV511_I2C_BUS, 0x24, 0x10 },
		{ OV511_I2C_BUS, 0x25, 0x8a },
		{ OV511_I2C_BUS, 0x26, 0xa2 },
		{ OV511_I2C_BUS, 0x27, 0xc2 },
		{ OV511_I2C_BUS, 0x2a, 0x04 },
		{ OV511_I2C_BUS, 0x2c, 0xfe },
		{ OV511_I2C_BUS, 0x2d, 0x93 },
		{ OV511_I2C_BUS, 0x30, 0x71 },
		{ OV511_I2C_BUS, 0x31, 0x60 },
		{ OV511_I2C_BUS, 0x32, 0x26 },
		{ OV511_I2C_BUS, 0x33, 0x20 },
		{ OV511_I2C_BUS, 0x34, 0x48 },
		{ OV511_I2C_BUS, 0x12, 0x24 },
		{ OV511_I2C_BUS, 0x11, 0x01 },
		{ OV511_I2C_BUS, 0x0c, 0x24 },
		{ OV511_I2C_BUS, 0x0d, 0x24 },
		{ OV511_DONE_BUS, 0x0, 0x00 },
	};

	static struct ov511_regvals regvals_norm_7620[] = {
		{ OV511_I2C_BUS, 0x00, 0x00 },
		{ OV511_I2C_BUS, 0x01, 0x80 },
		{ OV511_I2C_BUS, 0x02, 0x80 },
		{ OV511_I2C_BUS, 0x03, 0xc0 },
		{ OV511_I2C_BUS, 0x06, 0x60 },
		{ OV511_I2C_BUS, 0x07, 0x00 },
		{ OV511_I2C_BUS, 0x0c, 0x24 },
		{ OV511_I2C_BUS, 0x0c, 0x24 },
		{ OV511_I2C_BUS, 0x0d, 0x24 },
		{ OV511_I2C_BUS, 0x11, 0x01 },
		{ OV511_I2C_BUS, 0x12, 0x24 },
		{ OV511_I2C_BUS, 0x13, 0x01 },
		{ OV511_I2C_BUS, 0x14, 0x84 },
		{ OV511_I2C_BUS, 0x15, 0x01 },
		{ OV511_I2C_BUS, 0x16, 0x03 },
		{ OV511_I2C_BUS, 0x17, 0x2f },
		{ OV511_I2C_BUS, 0x18, 0xcf },
		{ OV511_I2C_BUS, 0x19, 0x06 },
		{ OV511_I2C_BUS, 0x1a, 0xf5 },
		{ OV511_I2C_BUS, 0x1b, 0x00 },
		{ OV511_I2C_BUS, 0x20, 0x18 },
		{ OV511_I2C_BUS, 0x21, 0x80 },
		{ OV511_I2C_BUS, 0x22, 0x80 },
		{ OV511_I2C_BUS, 0x23, 0x00 },
		{ OV511_I2C_BUS, 0x26, 0xa2 },
		{ OV511_I2C_BUS, 0x27, 0xea },
		{ OV511_I2C_BUS, 0x28, 0x20 },
		{ OV511_I2C_BUS, 0x29, 0x00 },
		{ OV511_I2C_BUS, 0x2a, 0x10 },
		{ OV511_I2C_BUS, 0x2b, 0x00 },
		{ OV511_I2C_BUS, 0x2c, 0x88 },
		{ OV511_I2C_BUS, 0x2d, 0x91 },
		{ OV511_I2C_BUS, 0x2e, 0x80 },
		{ OV511_I2C_BUS, 0x2f, 0x44 },
		{ OV511_I2C_BUS, 0x60, 0x27 },
		{ OV511_I2C_BUS, 0x61, 0x02 },
		{ OV511_I2C_BUS, 0x62, 0x5f },
		{ OV511_I2C_BUS, 0x63, 0xd5 },
		{ OV511_I2C_BUS, 0x64, 0x57 },
		{ OV511_I2C_BUS, 0x65, 0x83 },
		{ OV511_I2C_BUS, 0x66, 0x55 },
		{ OV511_I2C_BUS, 0x67, 0x92 },
		{ OV511_I2C_BUS, 0x68, 0xcf },
		{ OV511_I2C_BUS, 0x69, 0x76 },
		{ OV511_I2C_BUS, 0x6a, 0x22 },
		{ OV511_I2C_BUS, 0x6b, 0x00 },
		{ OV511_I2C_BUS, 0x6c, 0x02 },
		{ OV511_I2C_BUS, 0x6d, 0x44 },
		{ OV511_I2C_BUS, 0x6e, 0x80 },
		{ OV511_I2C_BUS, 0x6f, 0x1d },
		{ OV511_I2C_BUS, 0x70, 0x8b },
		{ OV511_I2C_BUS, 0x71, 0x00 },
		{ OV511_I2C_BUS, 0x72, 0x14 },
		{ OV511_I2C_BUS, 0x73, 0x54 },
		{ OV511_I2C_BUS, 0x74, 0x00 },
		{ OV511_I2C_BUS, 0x75, 0x8e },
		{ OV511_I2C_BUS, 0x76, 0x00 },
		{ OV511_I2C_BUS, 0x77, 0xff },
		{ OV511_I2C_BUS, 0x78, 0x80 },
		{ OV511_I2C_BUS, 0x79, 0x80 },
		{ OV511_I2C_BUS, 0x7a, 0x80 },
		{ OV511_I2C_BUS, 0x7b, 0xe2 },
		{ OV511_I2C_BUS, 0x7c, 0x00 },
		{ OV511_DONE_BUS, 0x0, 0x00 },
	};

	/* 7640 and 7648. The defaults should be OK for most registers. */
	static struct ov511_regvals regvals_norm_7640[] = {
		{ OV511_I2C_BUS, 0x12, 0x80 },
		{ OV511_I2C_BUS, 0x12, 0x14 },
		{ OV511_DONE_BUS, 0x0, 0x00 },
	};

	PDEBUG(4, "starting configuration");

	if (init_ov_sensor(ov) < 0) {
		err("Failed to read sensor ID. You might not have an");
		err("OV76xx, or it might not be responding. Report");
		err("this to " EMAIL);
		err("This is only a warning. You can attempt to use");
		err("your camera anyway");
	} else {
		PDEBUG(1, "OV7xx0 initialized");
	}

	/* Detect sensor (sub)type */
	rc = i2c_r(ov, OV7610_REG_COM_I);

	if (rc < 0) {
		err("Error detecting sensor type");
		return -1;
	} else if ((rc & 3) == 3) {
		info("Sensor is an OV7610");
		ov->sensor = SEN_OV7610;
	} else if ((rc & 3) == 1) {
		/* I don't know what's different about the 76BE yet. */
		if (i2c_r(ov, 0x15) & 1)
			info("Sensor is an OV7620AE");
		else
			info("Sensor is an OV76BE");

		/* OV511+ will return all zero isoc data unless we
		 * configure the sensor as a 7620. Someone needs to
		 * find the exact reg. setting that causes this. */
		if (ov->bridge == BRG_OV511PLUS)
			ov->sensor = SEN_OV7620;
		else
			ov->sensor = SEN_OV76BE;
	} else if ((rc & 3) == 0) {
		/* try to read product id registers */
		high = i2c_r(ov, 0x0a);
		if (high < 0) {
			err("Error detecting camera chip PID");
			return high;
		}
		low = i2c_r(ov, 0x0b);
		if (low < 0) {
			err("Error detecting camera chip VER");
			return low;
		}
		if (high == 0x76) {
			if (low == 0x30) {
				info("Sensor is an OV7630/OV7635");
				ov->sensor = SEN_OV7630;
			}
			else if (low == 0x40) {
				info("Sensor is an OV7645");
				ov->sensor = SEN_OV7640; // FIXME
			}
			else if (low == 0x45) {
				info("Sensor is an OV7645B");
				ov->sensor = SEN_OV7640; // FIXME
			}
			else if (low == 0x48) {
				info("Sensor is an OV7648");
				ov->sensor = SEN_OV7640; // FIXME
			}
			else {
				err("Unknown sensor: 0x76%X", low);
				return -1;
			}
		} else {
			info("Sensor is an OV7620");
			ov->sensor = SEN_OV7620;
		}
	} else {
		err("Unknown image sensor version: %d", rc & 3);
		return -1;
	}

	if (ov->sensor == SEN_OV7620) {
		PDEBUG(4, "Writing 7620 registers");
		if (write_regvals(ov, regvals_norm_7620))
			return -1;
	} else if (ov->sensor == SEN_OV7630) {
		PDEBUG(4, "7630 is not supported by this driver version");
		return -1;
	} else if (ov->sensor == SEN_OV7640) {
		PDEBUG(4, "Writing 7640 registers");
		if (write_regvals(ov, regvals_norm_7640))
			return -1;
	} else {
		PDEBUG(4, "Writing 7610 registers");
		if (write_regvals(ov, regvals_norm_7610))
			return -1;
	}

	/* Set sensor-specific vars */
	ov->maxwidth = 640;
	ov->maxheight = 480;
	ov->minwidth = 64;
	ov->minheight = 48;

	// FIXME: These do not match the actual settings yet
	ov->brightness = 0x80 << 8;
	ov->contrast = 0x80 << 8;
	ov->colour = 0x80 << 8;
	ov->hue = 0x80 << 8;

	return 0;
}

/* This initializes the OV6620, OV6630, OV6630AE, or OV6630AF sensor. */
static int
ov6xx0_configure(struct usb_ov511 *ov)
{
	int rc;

	static struct ov511_regvals regvals_norm_6x20[] = {
		{ OV511_I2C_BUS, 0x12, 0x80 }, /* reset */
		{ OV511_I2C_BUS, 0x11, 0x01 },
		{ OV511_I2C_BUS, 0x03, 0x60 },
		{ OV511_I2C_BUS, 0x05, 0x7f }, /* For when autoadjust is off */
		{ OV511_I2C_BUS, 0x07, 0xa8 },
		/* The ratio of 0x0c and 0x0d  controls the white point */
		{ OV511_I2C_BUS, 0x0c, 0x24 },
		{ OV511_I2C_BUS, 0x0d, 0x24 },
		{ OV511_I2C_BUS, 0x0f, 0x15 }, /* COMS */
		{ OV511_I2C_BUS, 0x10, 0x75 }, /* AEC Exposure time */
		{ OV511_I2C_BUS, 0x12, 0x24 }, /* Enable AGC */
		{ OV511_I2C_BUS, 0x14, 0x04 },
		/* 0x16: 0x06 helps frame stability with moving objects */
		{ OV511_I2C_BUS, 0x16, 0x06 },
//		{ OV511_I2C_BUS, 0x20, 0x30 }, /* Aperture correction enable */
		{ OV511_I2C_BUS, 0x26, 0xb2 }, /* BLC enable */
		/* 0x28: 0x05 Selects RGB format if RGB on */
		{ OV511_I2C_BUS, 0x28, 0x05 },
		{ OV511_I2C_BUS, 0x2a, 0x04 }, /* Disable framerate adjust */
//		{ OV511_I2C_BUS, 0x2b, 0xac }, /* Framerate; Set 2a[7] first */
		{ OV511_I2C_BUS, 0x2d, 0x99 },
		{ OV511_I2C_BUS, 0x33, 0xa0 }, /* Color Processing Parameter */
		{ OV511_I2C_BUS, 0x34, 0xd2 }, /* Max A/D range */
		{ OV511_I2C_BUS, 0x38, 0x8b },
		{ OV511_I2C_BUS, 0x39, 0x40 },

		{ OV511_I2C_BUS, 0x3c, 0x39 }, /* Enable AEC mode changing */
		{ OV511_I2C_BUS, 0x3c, 0x3c }, /* Change AEC mode */
		{ OV511_I2C_BUS, 0x3c, 0x24 }, /* Disable AEC mode changing */

		{ OV511_I2C_BUS, 0x3d, 0x80 },
		/* These next two registers (0x4a, 0x4b) are undocumented. They
		 * control the color balance */
		{ OV511_I2C_BUS, 0x4a, 0x80 },
		{ OV511_I2C_BUS, 0x4b, 0x80 },
		{ OV511_I2C_BUS, 0x4d, 0xd2 }, /* This reduces noise a bit */
		{ OV511_I2C_BUS, 0x4e, 0xc1 },
		{ OV511_I2C_BUS, 0x4f, 0x04 },
// Do 50-53 have any effect?
// Toggle 0x12[2] off and on here?
		{ OV511_DONE_BUS, 0x0, 0x00 },	/* END MARKER */
	};

	static struct ov511_regvals regvals_norm_6x30[] = {
		{ OV511_I2C_BUS, 0x12, 0x80 }, /* Reset */
		{ OV511_I2C_BUS, 0x00, 0x1f }, /* Gain */
		{ OV511_I2C_BUS, 0x01, 0x99 }, /* Blue gain */
		{ OV511_I2C_BUS, 0x02, 0x7c }, /* Red gain */
		{ OV511_I2C_BUS, 0x03, 0xc0 }, /* Saturation */
		{ OV511_I2C_BUS, 0x05, 0x0a }, /* Contrast */
		{ OV511_I2C_BUS, 0x06, 0x95 }, /* Brightness */
		{ OV511_I2C_BUS, 0x07, 0x2d }, /* Sharpness */
		{ OV511_I2C_BUS, 0x0c, 0x20 },
		{ OV511_I2C_BUS, 0x0d, 0x20 },
		{ OV511_I2C_BUS, 0x0e, 0x20 },
		{ OV511_I2C_BUS, 0x0f, 0x05 },
		{ OV511_I2C_BUS, 0x10, 0x9a },
		{ OV511_I2C_BUS, 0x11, 0x00 }, /* Pixel clock = fastest */
		{ OV511_I2C_BUS, 0x12, 0x24 }, /* Enable AGC and AWB */
		{ OV511_I2C_BUS, 0x13, 0x21 },
		{ OV511_I2C_BUS, 0x14, 0x80 },
		{ OV511_I2C_BUS, 0x15, 0x01 },
		{ OV511_I2C_BUS, 0x16, 0x03 },
		{ OV511_I2C_BUS, 0x17, 0x38 },
		{ OV511_I2C_BUS, 0x18, 0xea },
		{ OV511_I2C_BUS, 0x19, 0x04 },
		{ OV511_I2C_BUS, 0x1a, 0x93 },
		{ OV511_I2C_BUS, 0x1b, 0x00 },
		{ OV511_I2C_BUS, 0x1e, 0xc4 },
		{ OV511_I2C_BUS, 0x1f, 0x04 },
		{ OV511_I2C_BUS, 0x20, 0x20 },
		{ OV511_I2C_BUS, 0x21, 0x10 },
		{ OV511_I2C_BUS, 0x22, 0x88 },
		{ OV511_I2C_BUS, 0x23, 0xc0 }, /* Crystal circuit power level */
		{ OV511_I2C_BUS, 0x25, 0x9a }, /* Increase AEC black ratio */
		{ OV511_I2C_BUS, 0x26, 0xb2 }, /* BLC enable */
		{ OV511_I2C_BUS, 0x27, 0xa2 },
		{ OV511_I2C_BUS, 0x28, 0x00 },
		{ OV511_I2C_BUS, 0x29, 0x00 },
		{ OV511_I2C_BUS, 0x2a, 0x84 }, /* 60 Hz power */
		{ OV511_I2C_BUS, 0x2b, 0xa8 }, /* 60 Hz power */
		{ OV511_I2C_BUS, 0x2c, 0xa0 },
		{ OV511_I2C_BUS, 0x2d, 0x95 }, /* Enable auto-brightness */
		{ OV511_I2C_BUS, 0x2e, 0x88 },
		{ OV511_I2C_BUS, 0x33, 0x26 },
		{ OV511_I2C_BUS, 0x34, 0x03 },
		{ OV511_I2C_BUS, 0x36, 0x8f },
		{ OV511_I2C_BUS, 0x37, 0x80 },
		{ OV511_I2C_BUS, 0x38, 0x83 },
		{ OV511_I2C_BUS, 0x39, 0x80 },
		{ OV511_I2C_BUS, 0x3a, 0x0f },
		{ OV511_I2C_BUS, 0x3b, 0x3c },
		{ OV511_I2C_BUS, 0x3c, 0x1a },
		{ OV511_I2C_BUS, 0x3d, 0x80 },
		{ OV511_I2C_BUS, 0x3e, 0x80 },
		{ OV511_I2C_BUS, 0x3f, 0x0e },
		{ OV511_I2C_BUS, 0x40, 0x00 }, /* White bal */
		{ OV511_I2C_BUS, 0x41, 0x00 }, /* White bal */
		{ OV511_I2C_BUS, 0x42, 0x80 },
		{ OV511_I2C_BUS, 0x43, 0x3f }, /* White bal */
		{ OV511_I2C_BUS, 0x44, 0x80 },
		{ OV511_I2C_BUS, 0x45, 0x20 },
		{ OV511_I2C_BUS, 0x46, 0x20 },
		{ OV511_I2C_BUS, 0x47, 0x80 },
		{ OV511_I2C_BUS, 0x48, 0x7f },
		{ OV511_I2C_BUS, 0x49, 0x00 },
		{ OV511_I2C_BUS, 0x4a, 0x00 },
		{ OV511_I2C_BUS, 0x4b, 0x80 },
		{ OV511_I2C_BUS, 0x4c, 0xd0 },
		{ OV511_I2C_BUS, 0x4d, 0x10 }, /* U = 0.563u, V = 0.714v */
		{ OV511_I2C_BUS, 0x4e, 0x40 },
		{ OV511_I2C_BUS, 0x4f, 0x07 }, /* UV avg., col. killer: max */
		{ OV511_I2C_BUS, 0x50, 0xff },
		{ OV511_I2C_BUS, 0x54, 0x23 }, /* Max AGC gain: 18dB */
		{ OV511_I2C_BUS, 0x55, 0xff },
		{ OV511_I2C_BUS, 0x56, 0x12 },
		{ OV511_I2C_BUS, 0x57, 0x81 },
		{ OV511_I2C_BUS, 0x58, 0x75 },
		{ OV511_I2C_BUS, 0x59, 0x01 }, /* AGC dark current comp.: +1 */
		{ OV511_I2C_BUS, 0x5a, 0x2c },
		{ OV511_I2C_BUS, 0x5b, 0x0f }, /* AWB chrominance levels */
		{ OV511_I2C_BUS, 0x5c, 0x10 },
		{ OV511_I2C_BUS, 0x3d, 0x80 },
		{ OV511_I2C_BUS, 0x27, 0xa6 },
		{ OV511_I2C_BUS, 0x12, 0x20 }, /* Toggle AWB */
		{ OV511_I2C_BUS, 0x12, 0x24 },
		{ OV511_DONE_BUS, 0x0, 0x00 },	/* END MARKER */
	};

	PDEBUG(4, "starting sensor configuration");

	if (init_ov_sensor(ov) < 0) {
		err("Failed to read sensor ID. You might not have an OV6xx0,");
		err("or it may be not responding. Report this to " EMAIL);
		return -1;
	} else {
		PDEBUG(1, "OV6xx0 sensor detected");
	}

	/* Detect sensor (sub)type */
	rc = i2c_r(ov, OV7610_REG_COM_I);

	if (rc < 0) {
		err("Error detecting sensor type");
		return -1;
	}

	/* Ugh. The first two bits are the version bits, but the entire register
	 * value must be used. I guess OVT underestimated how many variants
	 * they would make. */
	if (rc == 0x00) {
		ov->sensor = SEN_OV6630;
		info("WARNING: Sensor is an OV66308. Your camera may have");
		info("been misdetected in previous driver versions. Please");
		info("report this to Mark.");
	} else if (rc == 0x01) {
		ov->sensor = SEN_OV6620;
		info("Sensor is an OV6620");
	} else if (rc == 0x02) {
		ov->sensor = SEN_OV6630;
		info("Sensor is an OV66308AE");
	} else if (rc == 0x03) {
		ov->sensor = SEN_OV6630;
		info("Sensor is an OV66308AF");
	} else if (rc == 0x90) {
		ov->sensor = SEN_OV6630;
		info("WARNING: Sensor is an OV66307. Your camera may have");
		info("been misdetected in previous driver versions. Please");
		info("report this to Mark.");
	} else {
		err("FATAL: Unknown sensor version: 0x%02x", rc);
		return -1;
	}

	/* Set sensor-specific vars */
	ov->maxwidth = 352;
	ov->maxheight = 288;
	ov->minwidth = 64;
	ov->minheight = 48;

	// FIXME: These do not match the actual settings yet
	ov->brightness = 0x80 << 8;
	ov->contrast = 0x80 << 8;
	ov->colour = 0x80 << 8;
	ov->hue = 0x80 << 8;

	if (ov->sensor == SEN_OV6620) {
		PDEBUG(4, "Writing 6x20 registers");
		if (write_regvals(ov, regvals_norm_6x20))
			return -1;
	} else {
		PDEBUG(4, "Writing 6x30 registers");
		if (write_regvals(ov, regvals_norm_6x30))
			return -1;
	}

	return 0;
}

/* This initializes the KS0127 and KS0127B video decoders. */
static int 
ks0127_configure(struct usb_ov511 *ov)
{
	int rc;

	/* Detect decoder subtype */
	rc = i2c_r(ov, 0x00);
	if (rc < 0) {
		err("Error detecting sensor type");
		return -1;
	} else if (rc & 0x08) {
		rc = i2c_r(ov, 0x3d);
		if (rc < 0) {
			err("Error detecting sensor type");
			return -1;
		} else if ((rc & 0x0f) == 0) {
			info("Sensor is a KS0127");
		} else if ((rc & 0x0f) == 9) {
			info("Sensor is a KS0127B Rev. A");
		}
	} else {
		info("Sensor is a KS0122");
	}

	/* This device is not supported yet. Bail out now... */
	err("This sensor is not supported yet.");
	return -1;
}

/* This initializes the SAA7111A video decoder. */
static int
saa7111a_configure(struct usb_ov511 *ov)
{
	int rc;

	/* Since there is no register reset command, all registers must be
	 * written, otherwise gives erratic results */
	static struct ov511_regvals regvals_norm_SAA7111A[] = {
		{ OV511_I2C_BUS, 0x06, 0xce },
		{ OV511_I2C_BUS, 0x07, 0x00 },
		{ OV511_I2C_BUS, 0x10, 0x44 }, /* YUV422, 240/286 lines */
		{ OV511_I2C_BUS, 0x0e, 0x01 }, /* NTSC M or PAL BGHI */
		{ OV511_I2C_BUS, 0x00, 0x00 },
		{ OV511_I2C_BUS, 0x01, 0x00 },
		{ OV511_I2C_BUS, 0x03, 0x23 },
		{ OV511_I2C_BUS, 0x04, 0x00 },
		{ OV511_I2C_BUS, 0x05, 0x00 },
		{ OV511_I2C_BUS, 0x08, 0xc8 }, /* Auto field freq */
		{ OV511_I2C_BUS, 0x09, 0x01 }, /* Chrom. trap off, APER=0.25 */
		{ OV511_I2C_BUS, 0x0a, 0x80 }, /* BRIG=128 */
		{ OV511_I2C_BUS, 0x0b, 0x40 }, /* CONT=1.0 */
		{ OV511_I2C_BUS, 0x0c, 0x40 }, /* SATN=1.0 */
		{ OV511_I2C_BUS, 0x0d, 0x00 }, /* HUE=0 */
		{ OV511_I2C_BUS, 0x0f, 0x00 },
		{ OV511_I2C_BUS, 0x11, 0x0c },
		{ OV511_I2C_BUS, 0x12, 0x00 },
		{ OV511_I2C_BUS, 0x13, 0x00 },
		{ OV511_I2C_BUS, 0x14, 0x00 },
		{ OV511_I2C_BUS, 0x15, 0x00 },
		{ OV511_I2C_BUS, 0x16, 0x00 },
		{ OV511_I2C_BUS, 0x17, 0x00 },
		{ OV511_I2C_BUS, 0x02, 0xc0 },	/* Composite input 0 */
		{ OV511_DONE_BUS, 0x0, 0x00 },
	};

	/* 640x480 not supported with PAL */
	if (ov->pal) {
		ov->maxwidth = 320;
		ov->maxheight = 240;		/* Even field only */
	} else {
		ov->maxwidth = 640;
		ov->maxheight = 480;		/* Even/Odd fields */
	}

	ov->minwidth = 320;
	ov->minheight = 240;		/* Even field only */

	ov->has_decoder = 1;
	ov->num_inputs = 8;
	ov->norm = VIDEO_MODE_AUTO;
	ov->stop_during_set = 0;	/* Decoder guarantees stable image */

	/* Decoder doesn't change these values, so we use these instead of
	 * acutally reading the registers (which doesn't work) */
	ov->brightness = 0x80 << 8;
	ov->contrast = 0x40 << 9;
	ov->colour = 0x40 << 9;
	ov->hue = 32768;

	PDEBUG(4, "Writing SAA7111A registers");
	if (write_regvals(ov, regvals_norm_SAA7111A))
		return -1;

	/* Detect version of decoder. This must be done after writing the
         * initial regs or the decoder will lock up. */
	rc = i2c_r(ov, 0x00);

	if (rc < 0) {
		err("Error detecting sensor version");
		return -1;
	} else {
		info("Sensor is an SAA7111A (version 0x%x)", rc);
		ov->sensor = SEN_SAA7111A;
	}

	/* Latch to negative edge of clock. Otherwise, we get incorrect
	 * colors and jitter in the digital signal. */
	if (ov->bclass == BCL_OV511)
		reg_w(ov, 0x11, 0x00);
	else
		warn("SAA7111A not yet supported with OV518/OV518+");

	return 0;
}

/* This initializes the OV511/OV511+ and the sensor */
static int 
ov511_configure(struct usb_ov511 *ov)
{
	static struct ov511_regvals regvals_init_511[] = {
		{ OV511_REG_BUS, R51x_SYS_RESET,	0x7f },
	 	{ OV511_REG_BUS, R51x_SYS_INIT,		0x01 },
	 	{ OV511_REG_BUS, R51x_SYS_RESET,	0x7f },
		{ OV511_REG_BUS, R51x_SYS_INIT,		0x01 },
		{ OV511_REG_BUS, R51x_SYS_RESET,	0x3f },
		{ OV511_REG_BUS, R51x_SYS_INIT,		0x01 },
		{ OV511_REG_BUS, R51x_SYS_RESET,	0x3d },
		{ OV511_DONE_BUS, 0x0, 0x00},
	};

	static struct ov511_regvals regvals_norm_511[] = {
		{ OV511_REG_BUS, R511_DRAM_FLOW_CTL, 	0x01 },
		{ OV511_REG_BUS, R51x_SYS_SNAP,		0x00 },
		{ OV511_REG_BUS, R51x_SYS_SNAP,		0x02 },
		{ OV511_REG_BUS, R51x_SYS_SNAP,		0x00 },
		{ OV511_REG_BUS, R511_FIFO_OPTS,	0x1f },
		{ OV511_REG_BUS, R511_COMP_EN,		0x00 },
		{ OV511_REG_BUS, R511_COMP_LUT_EN,	0x03 },
		{ OV511_DONE_BUS, 0x0, 0x00 },
	};

	static struct ov511_regvals regvals_norm_511_plus[] = {
		{ OV511_REG_BUS, R511_DRAM_FLOW_CTL,	0xff },
		{ OV511_REG_BUS, R51x_SYS_SNAP,		0x00 },
		{ OV511_REG_BUS, R51x_SYS_SNAP,		0x02 },
		{ OV511_REG_BUS, R51x_SYS_SNAP,		0x00 },
		{ OV511_REG_BUS, R511_FIFO_OPTS,	0xff },
		{ OV511_REG_BUS, R511_COMP_EN,		0x00 },
		{ OV511_REG_BUS, R511_COMP_LUT_EN,	0x03 },
		{ OV511_DONE_BUS, 0x0, 0x00 },
	};

	PDEBUG(4, "");

	ov->customid = reg_r(ov, R511_SYS_CUST_ID);
	if (ov->customid < 0) {
		err("Unable to read camera bridge registers");
		goto error;
	}

	PDEBUG (1, "CustomID = %d", ov->customid);
	ov->desc = symbolic(camlist, ov->customid);
	info("model: %s", ov->desc);

	if (0 == strcmp(ov->desc, NOT_DEFINED_STR)) {
		err("Camera type (%d) not recognized", ov->customid);
		err("Please notify " EMAIL " of the name,");
		err("manufacturer, model, and this number of your camera.");
		err("Also include the output of the detection process.");
	} 

	if (ov->customid == 70)		/* USB Life TV (PAL/SECAM) */
		ov->pal = 1;

	if (write_regvals(ov, regvals_init_511)) goto error;

	if (ov->led_policy == LED_OFF || ov->led_policy == LED_AUTO)
		ov51x_led_control(ov, 0);

	/* The OV511+ has undocumented bits in the flow control register.
	 * Setting it to 0xff fixes the corruption with moving objects. */
	if (ov->bridge == BRG_OV511) {
		if (write_regvals(ov, regvals_norm_511)) goto error;
	} else if (ov->bridge == BRG_OV511PLUS) {
		if (write_regvals(ov, regvals_norm_511_plus)) goto error;
	} else {
		err("Invalid bridge");
	}

	if (ov511_init_compression(ov)) goto error;

	ov->packet_numbering = 1;
	ov511_set_packet_size(ov, 0);

	ov->snap_enabled = snapshot;

	/* Test for 7xx0 */
	PDEBUG(3, "Testing for 0V7xx0");
	ov->primary_i2c_slave = OV7xx0_SID;
	if (ov51x_set_slave_ids(ov, OV7xx0_SID) < 0)
		goto error;

	if (i2c_w(ov, 0x12, 0x80) < 0) {
		/* Test for 6xx0 */
		PDEBUG(3, "Testing for 0V6xx0");
		ov->primary_i2c_slave = OV6xx0_SID;
		if (ov51x_set_slave_ids(ov, OV6xx0_SID) < 0)
			goto error;

		if (i2c_w(ov, 0x12, 0x80) < 0) {
			/* Test for 8xx0 */
			PDEBUG(3, "Testing for 0V8xx0");
			ov->primary_i2c_slave = OV8xx0_SID;
			if (ov51x_set_slave_ids(ov, OV8xx0_SID) < 0)
				goto error;

			if (i2c_w(ov, 0x12, 0x80) < 0) {
				/* Test for SAA7111A */
				PDEBUG(3, "Testing for SAA7111A");
				ov->primary_i2c_slave = SAA7111A_SID;
				if (ov51x_set_slave_ids(ov, SAA7111A_SID) < 0)
					goto error;

				if (i2c_w(ov, 0x0d, 0x00) < 0) {
					/* Test for KS0127 */
					PDEBUG(3, "Testing for KS0127");
					ov->primary_i2c_slave = KS0127_SID;
					if (ov51x_set_slave_ids(ov, KS0127_SID) < 0)
						goto error;

					if (i2c_w(ov, 0x10, 0x00) < 0) {
						err("Can't determine sensor slave IDs");
		 				goto error;
					} else {
						if (ks0127_configure(ov) < 0) {
							err("Failed to configure KS0127");
	 						goto error;
						}
					}
				} else {
					if (saa7111a_configure(ov) < 0) {
						err("Failed to configure SAA7111A");
	 					goto error;
					}
				}
			} else {
				if (ov8xx0_configure(ov) < 0) {
					err("Failed to configure OV8xx0 sensor");
					goto error;
				}
			}
		} else {
			if (ov6xx0_configure(ov) < 0) {
				err("Failed to configure OV6xx0");
 				goto error;
			}
		}
	} else {
		if (ov7xx0_configure(ov) < 0) {
			err("Failed to configure OV7xx0");
	 		goto error;
		}
	}

	return 0;

error:
	err("OV511 Config failed");

	return -EBUSY;
}

/* This initializes the OV518/OV518+ and the sensor */
static int
ov518_configure(struct usb_ov511 *ov)
{
	/* For 518 and 518+ */
	static struct ov511_regvals regvals_init_518[] = {
		{ OV511_REG_BUS, R51x_SYS_RESET,	0x40 },
	 	{ OV511_REG_BUS, R51x_SYS_INIT,		0xe1 },
	 	{ OV511_REG_BUS, R51x_SYS_RESET,	0x3e },
		{ OV511_REG_BUS, R51x_SYS_INIT,		0xe1 },
		{ OV511_REG_BUS, R51x_SYS_RESET,	0x00 },
		{ OV511_REG_BUS, R51x_SYS_INIT,		0xe1 },
		{ OV511_REG_BUS, 0x46,			0x00 },
		{ OV511_REG_BUS, 0x5d,			0x03 },
		{ OV511_DONE_BUS, 0x0, 0x00},
	};

	static struct ov511_regvals regvals_norm_518[] = {
		{ OV511_REG_BUS, R51x_SYS_SNAP,		0x02 }, /* Reset */
		{ OV511_REG_BUS, R51x_SYS_SNAP,		0x01 }, /* Enable */
		{ OV511_REG_BUS, 0x31, 			0x0f },
		{ OV511_REG_BUS, 0x5d,			0x03 },
		{ OV511_REG_BUS, 0x24,			0x9f },
		{ OV511_REG_BUS, 0x25,			0x90 },
		{ OV511_REG_BUS, 0x20,			0x00 },
		{ OV511_REG_BUS, 0x51,			0x04 },
		{ OV511_REG_BUS, 0x71,			0x19 },
		{ OV511_REG_BUS, 0x2f,			0x80 },
		{ OV511_DONE_BUS, 0x0, 0x00 },
	};

	static struct ov511_regvals regvals_norm_518_plus[] = {
		{ OV511_REG_BUS, R51x_SYS_SNAP,		0x02 }, /* Reset */
		{ OV511_REG_BUS, R51x_SYS_SNAP,		0x01 }, /* Enable */
		{ OV511_REG_BUS, 0x31, 			0x0f },
		{ OV511_REG_BUS, 0x5d,			0x03 },
		{ OV511_REG_BUS, 0x24,			0x9f },
		{ OV511_REG_BUS, 0x25,			0x90 },
		{ OV511_REG_BUS, 0x20,			0x60 },
		{ OV511_REG_BUS, 0x51,			0x02 },
		{ OV511_REG_BUS, 0x71,			0x19 },
		{ OV511_REG_BUS, 0x40,			0xff },
		{ OV511_REG_BUS, 0x41,			0x42 },
		{ OV511_REG_BUS, 0x46,			0x00 },
		{ OV511_REG_BUS, 0x33,			0x04 },
		{ OV511_REG_BUS, 0x21,			0x19 },
		{ OV511_REG_BUS, 0x3f,			0x10 },
		{ OV511_REG_BUS, 0x2f,			0x80 },
		{ OV511_DONE_BUS, 0x0, 0x00 },
	};

	PDEBUG(4, "");

	/* First 5 bits of custom ID reg are a revision ID on OV518 */
	info("Device revision %d", 0x1F & reg_r(ov, R511_SYS_CUST_ID));

	/* Give it the default description */
	ov->desc = symbolic(camlist, 0);

	if (write_regvals(ov, regvals_init_518)) goto error;

	/* Set LED GPIO pin to output mode */
	if (reg_w_mask(ov, 0x57, 0x00, 0x02) < 0) goto error;

	/* LED is off by default with OV518; have to explicitly turn it on */
	if (ov->led_policy == LED_OFF || ov->led_policy == LED_AUTO)
		ov51x_led_control(ov, 0);
	else
		ov51x_led_control(ov, 1);

	/* Don't require compression if dumppix is enabled; otherwise it's
	 * required. OV518 has no uncompressed mode, to save RAM. */
	if (!dumppix && !ov->compress) {
		ov->compress = 1;
		warn("Compression required with OV518...enabling");
	}

	if (ov->bridge == BRG_OV518) {
		if (write_regvals(ov, regvals_norm_518)) goto error;
	} else if (ov->bridge == BRG_OV518PLUS) {
		if (write_regvals(ov, regvals_norm_518_plus)) goto error;
	} else {
		err("Invalid bridge");
	}

	if (ov518_init_compression(ov)) goto error;

	if (ov->bridge == BRG_OV518)
	{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 7)
		struct usb_interface *ifp;
		struct usb_host_interface *alt;
		__u16 mxps = 0;

		ifp = usb_ifnum_to_if(ov->dev, 0);
		if (ifp) {
			alt = usb_altnum_to_altsetting(ifp, 7);
			if (alt)
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 11)
				mxps = le16_to_cpu(alt->endpoint[0].desc.wMaxPacketSize);
#  else
				mxps = alt->endpoint[0].desc.wMaxPacketSize;
#  endif
		}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
		struct usb_interface *ifp = ov->dev->config[0].interface[0];
		__u16 mxps = ifp->altsetting[7].endpoint[0].desc.wMaxPacketSize;
#else
		struct usb_interface *ifp = &ov->dev->config[0].interface[0];
		__u16 mxps = ifp->altsetting[7].endpoint[0].wMaxPacketSize;
#endif
		/* Some OV518s have packet numbering by default, some don't */
		if (mxps == 897)
			ov->packet_numbering = 1;
		else
			ov->packet_numbering = 0;
	} else {
		/* OV518+ has packet numbering turned on by default */
		ov->packet_numbering = 1;
	}

	ov518_set_packet_size(ov, 0);

	ov->snap_enabled = snapshot;

	/* Test for 76xx */
	ov->primary_i2c_slave = OV7xx0_SID;
	if (ov51x_set_slave_ids(ov, OV7xx0_SID) < 0)
		goto error;

	/* The OV518 must be more aggressive about sensor detection since
	 * I2C write will never fail if the sensor is not present. We have
	 * to try to initialize the sensor to detect its presence */

	if (init_ov_sensor(ov) < 0) {
		/* Test for 6xx0 */
		ov->primary_i2c_slave = OV6xx0_SID;
		if (ov51x_set_slave_ids(ov, OV6xx0_SID) < 0)
			goto error;

		if (init_ov_sensor(ov) < 0) {
			/* Test for 8xx0 */
			ov->primary_i2c_slave = OV8xx0_SID;
			if (ov51x_set_slave_ids(ov, OV8xx0_SID) < 0)
				goto error;

			if (init_ov_sensor(ov) < 0) {
				err("Can't determine sensor slave IDs");
 				goto error;
			} else {
				if (ov8xx0_configure(ov) < 0) {
					err("Failed to configure OV8xx0 sensor");
					goto error;
				}
			}
		} else {
			if (ov6xx0_configure(ov) < 0) {
				err("Failed to configure OV6xx0");
 				goto error;
			}
		}
	} else {
		if (ov7xx0_configure(ov) < 0) {
			err("Failed to configure OV7xx0");
	 		goto error;
		}
	}

	ov->maxwidth = 352;
	ov->maxheight = 288;

	// The OV518 cannot go as low as the sensor can
	ov->minwidth = 160;
	ov->minheight = 120;

	return 0;

error:
	err("OV518 Config failed");

	return -EBUSY;
}

/* This initializes the OV518/OV518+ and the sensor */
static int
ov519_configure(struct usb_ov511 *ov)
{

	static struct ov511_regvals regvals_init_519[] = {
		{ OV511_REG_BUS, 0x5a,	0x6d }, /* EnableSystem */
		/* windows reads 0x53 at this point*/
		{ OV511_REG_BUS, 0x53,	0x9b },
		{ OV511_REG_BUS, 0x54,	0x0f }, // set bit2 to enable jpeg
		{ OV511_REG_BUS, 0x5d,	0x03 },
		{ OV511_REG_BUS, 0x49,	0x01 },
		{ OV511_REG_BUS, 0x48,	0x00 },

		/* Set LED pin to output mode. Bit 4 must be cleared or sensor
		 * detection will fail. This deserves further investigation. */
		{ OV511_REG_BUS, OV519_GPIO_IO_CTRL0,	0xee },

		{ OV511_REG_BUS, 0x51,	0x0f },	/* SetUsbInit */
		{ OV511_REG_BUS, 0x51,	0x00 },
		{ OV511_REG_BUS, 0x22,	0x00 },
		/* windows reads 0x55 at this point*/
		{ OV511_DONE_BUS, 0x0, 0x00},
	};

	PDEBUG(4, "");

	/* Give it the default description */
	ov->desc = symbolic(camlist, 0);

	if (write_regvals(ov, regvals_init_519))
		goto error;

	if (ov519_init_compression(ov))
		goto error;

	if (ov->imp == IMP_EYETOY) {
		/* LED is annoyingly bright. Only turn it on if requested to. */
		if (ov->led2_policy == LED_ON)
			ov51x_led_control(ov, 1);
		else
			ov51x_led_control(ov, 0);
	} else {
		/* LED might be off by default */
		if (ov->led_policy == LED_ON)
			ov51x_led_control(ov, 1);
		else
			ov51x_led_control(ov, 0);
	}

	/* Don't require compression if dumppix is enabled; otherwise it's
	 * required. OV519 probably has no uncompressed mode, to save RAM. */
	if (!dumppix && !ov->compress)
		ov->compress = 1;

	ov->packet_numbering = 0;

	ov519_set_packet_size(ov, 0);

	ov->snap_enabled = snapshot;

	/* Test for 76xx */
	ov->primary_i2c_slave = OV7xx0_SID;
	if (ov51x_set_slave_ids(ov, OV7xx0_SID) < 0)
		goto error;

	/* The OV519 must be more aggressive about sensor detection since
	 * I2C write will never fail if the sensor is not present. We have
	 * to try to initialize the sensor to detect its presence */

	if (init_ov_sensor(ov) < 0) {
		/* Test for 6xx0 */
		ov->primary_i2c_slave = OV6xx0_SID;
		if (ov51x_set_slave_ids(ov, OV6xx0_SID) < 0)
			goto error;

		if (init_ov_sensor(ov) < 0) {
			/* Test for 8xx0 */
			ov->primary_i2c_slave = OV8xx0_SID;
			if (ov51x_set_slave_ids(ov, OV8xx0_SID) < 0)
				goto error;

			if (init_ov_sensor(ov) < 0) {
				err("Can't determine sensor slave IDs");
 				goto error;
			} else {
				if (ov8xx0_configure(ov) < 0) {
					err("Failed to configure OV8xx0 sensor");
					goto error;
				}
			}
		} else {
			if (ov6xx0_configure(ov) < 0) {
				err("Failed to configure OV6xx0");
 				goto error;
			}
		}
	} else {
		if (ov7xx0_configure(ov) < 0) {
			err("Failed to configure OV7xx0");
	 		goto error;
		}
	}

	return 0;

error:
	err("OV519 Config failed");

	return -EBUSY;
}

/****************************************************************************
 *
 *  USB routines
 *
 ***************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 36)
static int
ov51x_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
static void *
ov51x_probe(struct usb_device *dev, unsigned int ifnum,
	    const struct usb_device_id *id)
{
#else
static void *
ov51x_probe(struct usb_device *dev, unsigned int ifnum)
{
#endif
	struct usb_interface_descriptor *idesc;
	struct usb_ov511 *ov;
	int i;
	u16 vendor, product;

	PDEBUG(1, "probing for device...");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 36)
	/* We don't handle multi-config cameras */
	if (dev->descriptor.bNumConfigurations != 1)
		return -ENODEV;
#else
	/* We don't handle multi-config cameras */
	if (dev->descriptor.bNumConfigurations != 1)
		return NULL;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 7)
	idesc = &intf->cur_altsetting->desc;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 36)
	idesc = &intf->altsetting[0].desc;
#else
	idesc = &dev->actconfig->interface[ifnum].altsetting[0];
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 11)
	vendor = le16_to_cpu(dev->descriptor.idVendor);
	product = le16_to_cpu(dev->descriptor.idProduct);
#else
	vendor = dev->descriptor.idVendor;
	product = dev->descriptor.idProduct;
#endif

/* 2.2.x compatibility */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
	/* Is it an OV511/OV511+? */
	if (vendor != VEND_OMNIVISION
	 && vendor != VEND_MATTEL
	 && vendor != VEND_SONY)
		return NULL;

	if (vendor == VEND_OMNIVISION
	 && product != PROD_OV511
	 && product != PROD_OV518
	 && product != PROD_OV511PLUS
	 && product != PROD_OV518PLUS
	 && product != PROD_OV519
	 && product != PROD_OV1519
	 && product != PROD_OV2519
	 && product != PROD_OV3519
	 && product != PROD_OV4519
	 && product != PROD_OV5519
	 && product != PROD_OV6519
	 && product != PROD_OV7519
	 && product != PROD_OV8519
	 && product != PROD_OV9519
	 && product != PROD_OVA519
	 && product != PROD_OVB519
	 && product != PROD_OVC519
	 && product != PROD_OVD519
	 && product != PROD_OVE519
	 && product != PROD_OVF519
	 && product != PROD_OV530)
		return NULL;

	if (vendor == VEND_MATTEL
	 && product != PROD_ME2CAM)
		return NULL;

	if (vendor == VEND_SONY
	 && product != PROD_EYETOY4
	 && product != PROD_EYETOY5)
		return NULL;

	if (vendor == VEND_MICROSOFT
	 && product != PROD_XBOX_CAM)
		return NULL;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 36)
	if (idesc->bInterfaceClass != 0xFF)
		return -ENODEV;
	if (idesc->bInterfaceSubClass != 0x00)
		return -ENODEV;
#else
	if (idesc->bInterfaceClass != 0xFF)
		return NULL;
	if (idesc->bInterfaceSubClass != 0x00)
		return NULL;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 20)
	/* Since code below may sleep, we use this as a lock */
	MOD_INC_USE_COUNT;
#endif

	if ((ov = kmalloc(sizeof(*ov), GFP_KERNEL)) == NULL) {
		err("couldn't kmalloc ov struct");
		goto error_out;
	}

	memset(ov, 0, sizeof(*ov));

	ov->dev = dev;
	ov->iface = idesc->bInterfaceNumber;
	ov->led_policy = led;
	ov->led2_policy = led2;
	ov->compress = compress;
	ov->lightfreq = lightfreq;
	ov->num_inputs = 1;	   /* Video decoder init functs. change this */
	ov->stop_during_set = !fastset;
	ov->backlight = backlight;
	ov->mirror = mirror;
	ov->auto_brt = autobright;
	ov->auto_gain = autogain;
	ov->auto_exp = autoexp;
	ov->imp = IMP_GENERIC;

	switch (product) {
	case PROD_OV511:
		ov->bridge = BRG_OV511;
		ov->bclass = BCL_OV511;
		break;
	case PROD_OV511PLUS:
		ov->bridge = BRG_OV511PLUS;
		ov->bclass = BCL_OV511;
		break;
	case PROD_OV518:
		ov->bridge = BRG_OV518;
		ov->bclass = BCL_OV518;
		break;
	case PROD_OV518PLUS:
		ov->bridge = BRG_OV518PLUS;
		ov->bclass = BCL_OV518;
		break;
	case PROD_OV519:
	case PROD_OV4519:
	case PROD_OV8519:
	case PROD_OV530:
	case PROD_XBOX_CAM:
		ov->bridge = BRG_OV519;
		ov->bclass = BCL_OV519;
		break;
	case PROD_OV1519:
	case PROD_OV2519:
	case PROD_OV3519:
	case PROD_OV5519:
	case PROD_OV6519:
	case PROD_OV7519:
	case PROD_OV9519:
	case PROD_OVA519:
	case PROD_OVB519:
	case PROD_OVC519:
	case PROD_OVD519:
	case PROD_OVE519:
	case PROD_OVF519:
		info("Device has Product ID that hasn't been seen yet. It");
		info("will probably work anyway, but please send");
		info("/proc/bus/usb/devices, your dmesg log, and any info you");
		info("have about this device to mark@alpha.dyndns.org");
		ov->bridge = BRG_OV519;
		ov->bclass = BCL_OV519;
		break;
	case PROD_EYETOY4:
	case PROD_EYETOY5:
	/* These two should work, but they are untested */
//	case PROD_EYETOY6:
//	case PROD_EYETOY7:
		ov->bridge = BRG_OV519;
		ov->bclass = BCL_OV519;
		ov->imp = IMP_EYETOY;
		break;
	case PROD_ME2CAM:
		if (vendor != VEND_MATTEL)
			goto error;
		ov->bridge = BRG_OV511PLUS;
		ov->bclass = BCL_OV511;
		break;
	default:
		err("Unknown product ID 0x%04x", product);
		goto error;
	}

	info("USB %s video device found", symbolic(brglist, ov->bridge));

#ifdef OV511_ALLOW_CONVERSION
	/* Workaround for some applications that want data in RGB
	 * instead of BGR. */
	if (force_rgb)
		info("data format set to RGB");
#endif

	init_waitqueue_head(&ov->wq);

	init_MUTEX(&ov->lock);	/* to 1 == available */
	init_MUTEX(&ov->buf_lock);
	init_MUTEX(&ov->param_lock);
	init_MUTEX(&ov->i2c_lock);
	init_MUTEX(&ov->cbuf_lock);

	ov->buf_state = BUF_NOT_ALLOCATED;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 20)
	if (usb_make_path(dev, ov->usb_path, OV511_USB_PATH_LEN) < 0) {
		err("usb_make_path error");
		goto error;
	}
#endif

	/* Allocate control transfer buffer. */
	/* Must be kmalloc()'ed, for DMA compatibility */
	ov->cbuf = kmalloc(OV511_CBUF_SIZE, GFP_KERNEL);
	if (!ov->cbuf)
		goto error;

	switch (ov->bclass) {
		case BCL_OV511:
			if (ov511_configure(ov) < 0)
				goto error;
			break;
		case BCL_OV518:
			if (ov518_configure(ov) < 0)
				goto error;
			break;
		case BCL_OV519:
			if (ov519_configure(ov) < 0)
				goto error;
			break;
		default:
			goto error;
	}

	for (i = 0; i < OV511_NUMFRAMES; i++) {
		ov->frame[i].framenum = i;
		init_waitqueue_head(&ov->frame[i].wq);
	}

	for (i = 0; i < OV511_NUMSBUF; i++) {
		ov->sbuf[i].ov = ov;
		spin_lock_init(&ov->sbuf[i].lock);
		ov->sbuf[i].n = i;
	}

	/* Unnecessary? (This is done on open(). Need to make sure variables
	 * are properly initialized without this before removing it, though). */
	if (ov51x_set_default_params(ov) < 0)
		goto error;

#ifdef OV511_DEBUG
	if (dump_bridge) {
		if (ov->bclass == BCL_OV511)
			ov511_dump_regs(ov);
		else
			ov518_dump_regs(ov);
	}
#endif

	ov->vdev = video_device_alloc();
	if (!ov->vdev)
		goto error;

	memcpy(ov->vdev, &vdev_template, sizeof(*ov->vdev));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	ov->vdev->dev = &dev->dev;
#endif
	video_set_drvdata(ov->vdev, ov);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 5)
	for (i = 0; i < OV511_MAX_UNIT_VIDEO; i++) {
		/* Minor 0 cannot be specified; assume user wants autodetect */
		if (unit_video[i] == 0)
			break;

		if (video_register_device(ov->vdev, VFL_TYPE_GRABBER,
			unit_video[i]) >= 0) {
			break;
		}
	}

	/* Use the next available one */
	if ((ov->vdev->minor == -1) &&
	    video_register_device(ov->vdev, VFL_TYPE_GRABBER, -1) < 0) {
#else
	if (video_register_device(ov->vdev, VFL_TYPE_GRABBER) < 0) {
#endif
		err("video_register_device failed");
		goto error;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 20)
	info("Device at %s registered to minor %d", ov->usb_path,
	     ov->vdev->minor);
#else
	info("Device %d on bus %d registered to minor %d", dev->devnum,
	     dev->bus->busnum, ov->vdev->minor);
#endif

	create_proc_ov511_cam(ov);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 20)
	MOD_DEC_USE_COUNT;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 36)
	usb_set_intfdata(intf, ov);
	return 0;
#else
	return ov;
#endif

error:
	destroy_proc_ov511_cam(ov);

	if (ov->vdev) {
		if (-1 == ov->vdev->minor)
			video_device_release(ov->vdev);
		else
			video_unregister_device(ov->vdev);
		ov->vdev = NULL;
	}

	if (ov->cbuf) {
		down(&ov->cbuf_lock);
		kfree(ov->cbuf);
		ov->cbuf = NULL;
		up(&ov->cbuf_lock);
	}

	kfree(ov);
	ov = NULL;

error_out:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 20)
	MOD_DEC_USE_COUNT;
#endif
	err("Camera initialization failed");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 36)
	return -EIO;
#else
	return NULL;
#endif
}

static void
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
ov51x_disconnect(struct usb_interface *intf)
{
	struct usb_ov511 *ov = usb_get_intfdata(intf);
#else
ov51x_disconnect(struct usb_device *dev, void *ptr)
{
	struct usb_ov511 *ov = (struct usb_ov511 *) ptr;
#endif
	int n;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 20)
	MOD_INC_USE_COUNT;
#endif

	PDEBUG(3, "");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	usb_set_intfdata(intf, NULL);
#endif
	if (!ov)
		return;

#ifdef OV511_OLD_V4L
	/* We don't want people trying to open up the device */
	if (!ov->user)
		video_unregister_device(ov->vdev);
	else
		PDEBUG(3, "Device open...deferring video_unregister_device");
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	if (ov->vdev)
		video_unregister_device(ov->vdev);
#else
	video_unregister_device(ov->vdev);
	if (ov->user)
		PDEBUG(3, "Device open...deferring video_unregister_device");
#endif

	for (n = 0; n < OV511_NUMFRAMES; n++)
		ov->frame[n].grabstate = FRAME_ERROR;

	ov->curframe = -1;

	/* This will cause the process to request another frame */
	for (n = 0; n < OV511_NUMFRAMES; n++)
		wake_up_interruptible(&ov->frame[n].wq);

	wake_up_interruptible(&ov->wq);

	ov->streaming = 0;
	ov51x_unlink_isoc(ov);

        destroy_proc_ov511_cam(ov);

	ov->dev = NULL;

	/* Free the memory */
	if (ov && !ov->user) {
		down(&ov->cbuf_lock);
		kfree(ov->cbuf);
		ov->cbuf = NULL;
		up(&ov->cbuf_lock);

		ov51x_dealloc(ov);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
		if (ov->vdev)
			video_device_release(ov->vdev);
#endif
		kfree(ov);
		ov = NULL;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 20)
	MOD_DEC_USE_COUNT;
#endif
	PDEBUG(3, "Disconnect complete");
}

static struct usb_driver ov511_driver = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 20)
	.owner =	THIS_MODULE,
#endif
	.name =		"ov51x",
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
	.id_table =	device_table,
#endif
	.probe =	ov51x_probe,
	.disconnect =	ov51x_disconnect
};



module_init(usb_ov511_init);
module_exit(usb_ov511_exit);
#endif
#endif
