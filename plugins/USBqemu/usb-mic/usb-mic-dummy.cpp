/*
 * QEMU USB HID devices
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
#include "../qemu-usb/vl.h"

/* HID interface requests */
#define GET_REPORT   0xa101
#define GET_IDLE     0xa102
#define GET_PROTOCOL 0xa103
#define SET_IDLE     0x210a
#define SET_PROTOCOL 0x210b

#define USB_MOUSE  1
#define USB_TABLET 2

#include "type.h"

#include "usb.h"
#include "audio.h"
#include "usbcfg.h"
#include "usbdesc.h"

typedef struct SINGSTARMICState {
    USBDevice dev;
	//nothing yet
} SINGSTARMICState;

/* descriptor dumped from a real singstar MIC adapter */
static const uint8_t singstar_mic_dev_descriptor[] = {
    /* bLength             */ 0x12, //(18)
    /* bDescriptorType     */ 0x01, //(1)
    /* bcdUSB              */ WBVAL(0x0110), //(272)
    /* bDeviceClass        */ 0x00, //(0)
    /* bDeviceSubClass     */ 0x00, //(0)
    /* bDeviceProtocol     */ 0x00, //(0)
    /* bMaxPacketSize0     */ 0x08, //(8)
    /* idVendor            */ WBVAL(0x1415), //(5141)
    /* idProduct           */ WBVAL(0x0000), //(0)
    /* bcdDevice           */ WBVAL(0x0001), //(1)
    /* iManufacturer       */ 0x01, //(1)
    /* iProduct            */ 0x02, //(2)
    /* iSerialNumber       */ 0x00, //(0)
    /* bNumConfigurations  */ 0x01, //(1)

};

static const uint8_t singstar_mic_config_descriptor[] = {

/* Configuration 1 */
  0x09,   /* bLength */
  USB_CONFIGURATION_DESCRIPTOR_TYPE,    /* bDescriptorType */
  WBVAL(0x00b1),                        /* wTotalLength */
  0x02,                                 /* bNumInterfaces */
  0x01,                                 /* bConfigurationValue */
  0x00,                                 /* iConfiguration */
  USB_CONFIG_BUS_POWERED,               /* bmAttributes */
  USB_CONFIG_POWER_MA(90),              /* bMaxPower */

/* Interface 0, Alternate Setting 0, Audio Control */
  USB_INTERFACE_DESC_SIZE,              /* bLength */
  USB_INTERFACE_DESCRIPTOR_TYPE,        /* bDescriptorType */
  0x00,                                 /* bInterfaceNumber */
  0x00,                                 /* bAlternateSetting */
  0x00,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOCONTROL,          /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */

/* Audio Control Interface */
  AUDIO_CONTROL_INTERFACE_DESC_SZ(1),   /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_HEADER,                 /* bDescriptorSubtype */
  WBVAL(0x0100), /* 1.00 */             /* bcdADC */
  WBVAL(0x0028),                        /* wTotalLength */
  0x01,                                 /* bInCollection */
  0x01,                                 /* baInterfaceNr */

/* Audio Input Terminal */
  AUDIO_INPUT_TERMINAL_DESC_SIZE,       /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_INPUT_TERMINAL,         /* bDescriptorSubtype */
  0x01,                                 /* bTerminalID */
  WBVAL(AUDIO_TERMINAL_MICROPHONE),     /* wTerminalType */
  0x02,                                 /* bAssocTerminal */
  0x02,                                 /* bNrChannels */
  WBVAL(AUDIO_CHANNEL_L
       |AUDIO_CHANNEL_R),				/* wChannelConfig */
  0x00,                                 /* iChannelNames */
  0x00,                                 /* iTerminal */

/* Audio Output Terminal */
  AUDIO_OUTPUT_TERMINAL_DESC_SIZE,      /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_OUTPUT_TERMINAL,        /* bDescriptorSubtype */
  0x02,                                 /* bTerminalID */
  WBVAL(AUDIO_TERMINAL_USB_STREAMING),  /* wTerminalType */
  0x01,                                 /* bAssocTerminal */
  0x03,                                 /* bSourceID */
  0x00,                                 /* iTerminal */

/* Audio Feature Unit */
  AUDIO_FEATURE_UNIT_DESC_SZ(2,1),      /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_FEATURE_UNIT,           /* bDescriptorSubtype */
  0x03,                                 /* bUnitID */
  0x01,                                 /* bSourceID */
  0x01,                                 /* bControlSize */
  0x01,                                 /* bmaControls(0) */
  0x02,                                 /* bmaControls(1) */
  0x02,                                 /* bmaControls(2) */
  0x00,                                 /* iTerminal */

/* Interface 1, Alternate Setting 0, Audio Streaming - Zero Bandwith */
  USB_INTERFACE_DESC_SIZE,              /* bLength */
  USB_INTERFACE_DESCRIPTOR_TYPE,        /* bDescriptorType */
  0x01,                                 /* bInterfaceNumber */
  0x00,                                 /* bAlternateSetting */
  0x00,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */

/* Interface 1, Alternate Setting 1, Audio Streaming - Operational */
  USB_INTERFACE_DESC_SIZE,              /* bLength */
  USB_INTERFACE_DESCRIPTOR_TYPE,        /* bDescriptorType */
  0x01,                                 /* bInterfaceNumber */
  0x01,                                 /* bAlternateSetting */
  0x01,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */

/* Audio Streaming Interface */
  AUDIO_STREAMING_INTERFACE_DESC_SIZE,  /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_GENERAL,              /* bDescriptorSubtype */
  0x02,                                 /* bTerminalLink */
  0x01,                                 /* bDelay */
  WBVAL(AUDIO_FORMAT_PCM),              /* wFormatTag */

/* Audio Type I Format */
  AUDIO_FORMAT_TYPE_I_DESC_SZ(5),       /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype */
  AUDIO_FORMAT_TYPE_I,                  /* bFormatType */
  0x01,                                 /* bNrChannels */
  0x02,                                 /* bSubFrameSize */
  0x10,                                 /* bBitResolution */
  0x05,                                 /* bSamFreqType */
  B3VAL(8000),                          /* tSamFreq 1 */
  B3VAL(11025),                         /* tSamFreq 2 */
  B3VAL(22050),                         /* tSamFreq 3 */
  B3VAL(44100),                         /* tSamFreq 4 */
  B3VAL(48000),                         /* tSamFreq 5 */

/* Endpoint - Standard Descriptor */
  AUDIO_STANDARD_ENDPOINT_DESC_SIZE,    /* bLength */
  USB_ENDPOINT_DESCRIPTOR_TYPE,         /* bDescriptorType */
  USB_ENDPOINT_OUT(0x81),               /* bEndpointAddress */
  USB_ENDPOINT_TYPE_ISOCHRONOUS
  | USB_ENDPOINT_SYNC_ASYNCHRONOUS,     /* bmAttributes */
  WBVAL(0x0064),                        /* wMaxPacketSize */
  0x01,                                 /* bInterval */
  0x00,                                 /* bRefresh */
  0x00,                                 /* bSynchAddress */

/* Endpoint - Audio Streaming */
  AUDIO_STREAMING_ENDPOINT_DESC_SIZE,   /* bLength */
  AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType */
  AUDIO_ENDPOINT_GENERAL,               /* bDescriptor */
  0x01,                                 /* bmAttributes */
  0x00,                                 /* bLockDelayUnits */
  WBVAL(0x0000),                        /* wLockDelay */

/* Interface 1, Alternate Setting 2, Audio Streaming - ? */
  USB_INTERFACE_DESC_SIZE,              /* bLength */
  USB_INTERFACE_DESCRIPTOR_TYPE,        /* bDescriptorType */
  0x01,                                 /* bInterfaceNumber */
  0x02,                                 /* bAlternateSetting */
  0x01,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */

/* Audio Streaming Interface */
  AUDIO_STREAMING_INTERFACE_DESC_SIZE,  /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_GENERAL,              /* bDescriptorSubtype */
  0x02,                                 /* bTerminalLink */
  0x01,                                 /* bDelay */
  WBVAL(AUDIO_FORMAT_PCM),              /* wFormatTag */

/* Audio Type I Format */
  AUDIO_FORMAT_TYPE_I_DESC_SZ(5),       /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype */
  AUDIO_FORMAT_TYPE_I,                  /* bFormatType */
  0x02,                                 /* bNrChannels */
  0x02,                                 /* bSubFrameSize */
  0x10,                                 /* bBitResolution */
  0x05,                                 /* bSamFreqType */
  B3VAL(8000),                          /* tSamFreq 1 */
  B3VAL(11025),                         /* tSamFreq 2 */
  B3VAL(22050),                         /* tSamFreq 3 */
  B3VAL(44100),                         /* tSamFreq 4 */
  B3VAL(48000),                         /* tSamFreq 5 */

/* Endpoint - Standard Descriptor */
  AUDIO_STANDARD_ENDPOINT_DESC_SIZE,    /* bLength */
  USB_ENDPOINT_DESCRIPTOR_TYPE,         /* bDescriptorType */
  USB_ENDPOINT_OUT(0x81),               /* bEndpointAddress */
  USB_ENDPOINT_TYPE_ISOCHRONOUS
  | USB_ENDPOINT_SYNC_ASYNCHRONOUS,     /* bmAttributes */
  WBVAL(0x00c8),                        /* wMaxPacketSize */
  0x01,                                 /* bInterval */
  0x00,                                 /* bRefresh */
  0x00,                                 /* bSynchAddress */

/* Endpoint - Audio Streaming */
  AUDIO_STREAMING_ENDPOINT_DESC_SIZE,   /* bLength */
  AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType */
  AUDIO_ENDPOINT_GENERAL,               /* bDescriptor */
  0x01,                                 /* bmAttributes */
  0x00,                                 /* bLockDelayUnits */
  WBVAL(0x0000),                        /* wLockDelay */

/* Terminator */
  0                                     /* bLength */
};


static void singstar_mic_handle_reset(USBDevice *dev)
{
    /* XXX: do it */
	return;
}

static int singstar_mic_handle_control(USBDevice *dev, int request, int value,
                                  int index, int length, uint8_t *data)
{
    SINGSTARMICState *s = (SINGSTARMICState *)dev;
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
            memcpy(data, singstar_mic_dev_descriptor, 
                   sizeof(singstar_mic_dev_descriptor));
            ret = sizeof(singstar_mic_dev_descriptor);
            break;
        case USB_DT_CONFIG:
			memcpy(data, singstar_mic_config_descriptor, 
				sizeof(singstar_mic_config_descriptor));
			ret = sizeof(singstar_mic_config_descriptor);
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

static int singstar_mic_handle_data(USBDevice *dev, int pid, 
                               uint8_t devep, uint8_t *data, int len)
{
    SINGSTARMICState *s = (SINGSTARMICState *)dev;
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


static void singstar_mic_handle_destroy(USBDevice *dev)
{
    SINGSTARMICState *s = (SINGSTARMICState *)dev;

    free(s);
}

int singstar_mic_handle_packet(USBDevice *s, int pid, 
                              uint8_t devaddr, uint8_t devep,
                              uint8_t *data, int len)
{
	fprintf(stderr,"usb-singstar_mic: packet received with pid=%x, devaddr=%x, devep=%x and len=%x\n",pid,devaddr,devep,len);
	return usb_generic_handle_packet(s,pid,devaddr,devep,data,len);
}

USBDevice *singstar_mic_init()
{
    SINGSTARMICState *s;

    s = (SINGSTARMICState *)qemu_mallocz(sizeof(SINGSTARMICState));
    if (!s)
        return NULL;
    s->dev.speed = USB_SPEED_FULL;
    s->dev.handle_packet  = singstar_mic_handle_packet;
    s->dev.handle_reset   = singstar_mic_handle_reset;
    s->dev.handle_control = singstar_mic_handle_control;
    s->dev.handle_data    = singstar_mic_handle_data;
    s->dev.handle_destroy = singstar_mic_handle_destroy;

    strncpy(s->dev.devname, "EyeToy USB camera Namtai", sizeof(s->dev.devname));

    return (USBDevice *)s;

}
