/*
 * QEMU USB HID devices
 *
 * Copyright (c) 2005 Fabrice Bellard
 * Copyright (c) 2007 OpenMoko, Inc.  (andrew@openedhand.com)
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
#include "hw.h"
#include "console.h"
#include "usb.h"

/* HID interface requests */
#define GET_REPORT   0xa101
#define GET_IDLE     0xa102
#define GET_PROTOCOL 0xa103
#define SET_REPORT   0x2109
#define SET_IDLE     0x210a
#define SET_PROTOCOL 0x210b

/* HID descriptor types */
#define USB_DT_HID    0x21
#define USB_DT_REPORT 0x22
#define USB_DT_PHY    0x23

#define USB_MOUSE     1
#define USB_TABLET    2
#define USB_KEYBOARD  3

typedef struct USBMouseState {
    int dx, dy, dz, buttons_state;
    int x, y;
    int mouse_grabbed;
    QEMUPutMouseEntry *eh_entry;
} USBMouseState;

typedef struct USBKeyboardState {
    uint16_t modifiers;
    uint8_t leds;
    uint8_t key[16];
    int keys;
} USBKeyboardState;

typedef struct USBHIDState {
    USBDevice dev;
    union {
        USBMouseState ptr;
        USBKeyboardState kbd;
    };
    int kind;
    int protocol;
    uint8_t idle;
    int changed;
    void *datain_opaque;
    void (*datain)(void *);
} USBHIDState;

/* mostly the same values as the Bochs USB Mouse device */
static const uint8_t qemu_mouse_dev_descriptor[] = {
	0x12,       /*  u8 bLength; */
	0x01,       /*  u8 bDescriptorType; Device */
	0x00, 0x01, /*  u16 bcdUSB; v1.0 */

	0x00,	    /*  u8  bDeviceClass; */
	0x00,	    /*  u8  bDeviceSubClass; */
	0x00,       /*  u8  bDeviceProtocol; [ low/full speeds only ] */
	0x08,       /*  u8  bMaxPacketSize0; 8 Bytes */

	0x27, 0x06, /*  u16 idVendor; */
 	0x01, 0x00, /*  u16 idProduct; */
	0x00, 0x00, /*  u16 bcdDevice */

	0x03,       /*  u8  iManufacturer; */
	0x02,       /*  u8  iProduct; */
	0x01,       /*  u8  iSerialNumber; */
	0x01        /*  u8  bNumConfigurations; */
};

static const uint8_t qemu_mouse_config_descriptor[] = {
	/* one configuration */
	0x09,       /*  u8  bLength; */
	0x02,       /*  u8  bDescriptorType; Configuration */
	0x22, 0x00, /*  u16 wTotalLength; */
	0x01,       /*  u8  bNumInterfaces; (1) */
	0x01,       /*  u8  bConfigurationValue; */
	0x04,       /*  u8  iConfiguration; */
	0xa0,       /*  u8  bmAttributes;
				 Bit 7: must be set,
				     6: Self-powered,
				     5: Remote wakeup,
				     4..0: resvd */
	50,         /*  u8  MaxPower; */

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
	0x03,       /*  u8  if_bInterfaceClass; */
	0x01,       /*  u8  if_bInterfaceSubClass; */
	0x02,       /*  u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
	0x07,       /*  u8  if_iInterface; */

        /* HID descriptor */
        0x09,        /*  u8  bLength; */
        0x21,        /*  u8 bDescriptorType; */
        0x01, 0x00,  /*  u16 HID_class */
        0x00,        /*  u8 country_code */
        0x01,        /*  u8 num_descriptors */
        0x22,        /*  u8 type; Report */
        52, 0,       /*  u16 len */

	/* one endpoint (status change endpoint) */
	0x07,       /*  u8  ep_bLength; */
	0x05,       /*  u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x03,       /*  u8  ep_bmAttributes; Interrupt */
 	0x04, 0x00, /*  u16 ep_wMaxPacketSize; */
	0x0a,       /*  u8  ep_bInterval; (255ms -- usb 2.0 spec) */
};

static const uint8_t qemu_tablet_config_descriptor[] = {
	/* one configuration */
	0x09,       /*  u8  bLength; */
	0x02,       /*  u8  bDescriptorType; Configuration */
	0x22, 0x00, /*  u16 wTotalLength; */
	0x01,       /*  u8  bNumInterfaces; (1) */
	0x01,       /*  u8  bConfigurationValue; */
	0x05,       /*  u8  iConfiguration; */
	0xa0,       /*  u8  bmAttributes;
				 Bit 7: must be set,
				     6: Self-powered,
				     5: Remote wakeup,
				     4..0: resvd */
	50,         /*  u8  MaxPower; */

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
	0x03,       /*  u8  if_bInterfaceClass; */
	0x01,       /*  u8  if_bInterfaceSubClass; */
	0x02,       /*  u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
	0x07,       /*  u8  if_iInterface; */

        /* HID descriptor */
        0x09,        /*  u8  bLength; */
        0x21,        /*  u8 bDescriptorType; */
        0x01, 0x00,  /*  u16 HID_class */
        0x00,        /*  u8 country_code */
        0x01,        /*  u8 num_descriptors */
        0x22,        /*  u8 type; Report */
        74, 0,       /*  u16 len */

	/* one endpoint (status change endpoint) */
	0x07,       /*  u8  ep_bLength; */
	0x05,       /*  u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x03,       /*  u8  ep_bmAttributes; Interrupt */
 	0x08, 0x00, /*  u16 ep_wMaxPacketSize; */
	0x0a,       /*  u8  ep_bInterval; (255ms -- usb 2.0 spec) */
};

static const uint8_t qemu_keyboard_config_descriptor[] = {
    /* one configuration */
    0x09,		/*  u8  bLength; */
    USB_DT_CONFIG,	/*  u8  bDescriptorType; Configuration */
    0x22, 0x00,		/*  u16 wTotalLength; */
    0x01,		/*  u8  bNumInterfaces; (1) */
    0x01,		/*  u8  bConfigurationValue; */
    0x06,		/*  u8  iConfiguration; */
    0xa0,		/*  u8  bmAttributes;
				Bit 7: must be set,
				    6: Self-powered,
				    5: Remote wakeup,
				    4..0: resvd */
    0x32,		/*  u8  MaxPower; */

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
    0x09,		/*  u8  if_bLength; */
    USB_DT_INTERFACE,	/*  u8  if_bDescriptorType; Interface */
    0x00,		/*  u8  if_bInterfaceNumber; */
    0x00,		/*  u8  if_bAlternateSetting; */
    0x01,		/*  u8  if_bNumEndpoints; */
    0x03,		/*  u8  if_bInterfaceClass; HID */
    0x01,		/*  u8  if_bInterfaceSubClass; Boot */
    0x01,		/*  u8  if_bInterfaceProtocol; Keyboard */
    0x07,		/*  u8  if_iInterface; */

    /* HID descriptor */
    0x09,		/*  u8  bLength; */
    USB_DT_HID,		/*  u8  bDescriptorType; */
    0x11, 0x01,		/*  u16 HID_class */
    0x00,		/*  u8  country_code */
    0x01,		/*  u8  num_descriptors */
    USB_DT_REPORT,	/*  u8  type; Report */
    0x3f, 0x00,		/*  u16 len */

    /* one endpoint (status change endpoint) */
    0x07,		/*  u8  ep_bLength; */
    USB_DT_ENDPOINT,	/*  u8  ep_bDescriptorType; Endpoint */
    USB_DIR_IN | 0x01,	/*  u8  ep_bEndpointAddress; IN Endpoint 1 */
    0x03,		/*  u8  ep_bmAttributes; Interrupt */
    0x08, 0x00,		/*  u16 ep_wMaxPacketSize; */
    0x0a,		/*  u8  ep_bInterval; (255ms -- usb 2.0 spec) */
};

static const uint8_t qemu_mouse_hid_report_descriptor[] = {
    0x05, 0x01,		/* Usage Page (Generic Desktop) */
    0x09, 0x02,		/* Usage (Mouse) */
    0xa1, 0x01,		/* Collection (Application) */
    0x09, 0x01,		/*   Usage (Pointer) */
    0xa1, 0x00,		/*   Collection (Physical) */
    0x05, 0x09,		/*     Usage Page (Button) */
    0x19, 0x01,		/*     Usage Minimum (1) */
    0x29, 0x03,		/*     Usage Maximum (3) */
    0x15, 0x00,		/*     Logical Minimum (0) */
    0x25, 0x01,		/*     Logical Maximum (1) */
    0x95, 0x03,		/*     Report Count (3) */
    0x75, 0x01,		/*     Report Size (1) */
    0x81, 0x02,		/*     Input (Data, Variable, Absolute) */
    0x95, 0x01,		/*     Report Count (1) */
    0x75, 0x05,		/*     Report Size (5) */
    0x81, 0x01,		/*     Input (Constant) */
    0x05, 0x01,		/*     Usage Page (Generic Desktop) */
    0x09, 0x30,		/*     Usage (X) */
    0x09, 0x31,		/*     Usage (Y) */
    0x09, 0x38,		/*     Usage (Wheel) */
    0x15, 0x81,		/*     Logical Minimum (-0x7f) */
    0x25, 0x7f,		/*     Logical Maximum (0x7f) */
    0x75, 0x08,		/*     Report Size (8) */
    0x95, 0x03,		/*     Report Count (3) */
    0x81, 0x06,		/*     Input (Data, Variable, Relative) */
    0xc0,		/*   End Collection */
    0xc0,		/* End Collection */
};

static const uint8_t qemu_tablet_hid_report_descriptor[] = {
    0x05, 0x01,		/* Usage Page (Generic Desktop) */
    0x09, 0x01,		/* Usage (Pointer) */
    0xa1, 0x01,		/* Collection (Application) */
    0x09, 0x01,		/*   Usage (Pointer) */
    0xa1, 0x00,		/*   Collection (Physical) */
    0x05, 0x09,		/*     Usage Page (Button) */
    0x19, 0x01,		/*     Usage Minimum (1) */
    0x29, 0x03,		/*     Usage Maximum (3) */
    0x15, 0x00,		/*     Logical Minimum (0) */
    0x25, 0x01,		/*     Logical Maximum (1) */
    0x95, 0x03,		/*     Report Count (3) */
    0x75, 0x01,		/*     Report Size (1) */
    0x81, 0x02,		/*     Input (Data, Variable, Absolute) */
    0x95, 0x01,		/*     Report Count (1) */
    0x75, 0x05,		/*     Report Size (5) */
    0x81, 0x01,		/*     Input (Constant) */
    0x05, 0x01,		/*     Usage Page (Generic Desktop) */
    0x09, 0x30,		/*     Usage (X) */
    0x09, 0x31,		/*     Usage (Y) */
    0x15, 0x00,		/*     Logical Minimum (0) */
    0x26, 0xff, 0x7f,	/*     Logical Maximum (0x7fff) */
    0x35, 0x00,		/*     Physical Minimum (0) */
    0x46, 0xff, 0x7f,	/*     Physical Maximum (0x7fff) */
    0x75, 0x10,		/*     Report Size (16) */
    0x95, 0x02,		/*     Report Count (2) */
    0x81, 0x02,		/*     Input (Data, Variable, Absolute) */
    0x05, 0x01,		/*     Usage Page (Generic Desktop) */
    0x09, 0x38,		/*     Usage (Wheel) */
    0x15, 0x81,		/*     Logical Minimum (-0x7f) */
    0x25, 0x7f,		/*     Logical Maximum (0x7f) */
    0x35, 0x00,		/*     Physical Minimum (same as logical) */
    0x45, 0x00,		/*     Physical Maximum (same as logical) */
    0x75, 0x08,		/*     Report Size (8) */
    0x95, 0x01,		/*     Report Count (1) */
    0x81, 0x06,		/*     Input (Data, Variable, Relative) */
    0xc0,		/*   End Collection */
    0xc0,		/* End Collection */
};

static const uint8_t qemu_keyboard_hid_report_descriptor[] = {
    0x05, 0x01,		/* Usage Page (Generic Desktop) */
    0x09, 0x06,		/* Usage (Keyboard) */
    0xa1, 0x01,		/* Collection (Application) */
    0x75, 0x01,		/*   Report Size (1) */
    0x95, 0x08,		/*   Report Count (8) */
    0x05, 0x07,		/*   Usage Page (Key Codes) */
    0x19, 0xe0,		/*   Usage Minimum (224) */
    0x29, 0xe7,		/*   Usage Maximum (231) */
    0x15, 0x00,		/*   Logical Minimum (0) */
    0x25, 0x01,		/*   Logical Maximum (1) */
    0x81, 0x02,		/*   Input (Data, Variable, Absolute) */
    0x95, 0x01,		/*   Report Count (1) */
    0x75, 0x08,		/*   Report Size (8) */
    0x81, 0x01,		/*   Input (Constant) */
    0x95, 0x05,		/*   Report Count (5) */
    0x75, 0x01,		/*   Report Size (1) */
    0x05, 0x08,		/*   Usage Page (LEDs) */
    0x19, 0x01,		/*   Usage Minimum (1) */
    0x29, 0x05,		/*   Usage Maximum (5) */
    0x91, 0x02,		/*   Output (Data, Variable, Absolute) */
    0x95, 0x01,		/*   Report Count (1) */
    0x75, 0x03,		/*   Report Size (3) */
    0x91, 0x01,		/*   Output (Constant) */
    0x95, 0x06,		/*   Report Count (6) */
    0x75, 0x08,		/*   Report Size (8) */
    0x15, 0x00,		/*   Logical Minimum (0) */
    0x25, 0xff,		/*   Logical Maximum (255) */
    0x05, 0x07,		/*   Usage Page (Key Codes) */
    0x19, 0x00,		/*   Usage Minimum (0) */
    0x29, 0xff,		/*   Usage Maximum (255) */
    0x81, 0x00,		/*   Input (Data, Array) */
    0xc0,		/* End Collection */
};

#define USB_HID_USAGE_ERROR_ROLLOVER	0x01
#define USB_HID_USAGE_POSTFAIL		0x02
#define USB_HID_USAGE_ERROR_UNDEFINED	0x03

/* Indices are QEMU keycodes, values are from HID Usage Table.  Indices
 * above 0x80 are for keys that come after 0xe0 or 0xe1+0x1d or 0xe1+0x9d.  */
static const uint8_t usb_hid_usage_keys[0x100] = {
    0x00, 0x29, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x2d, 0x2e, 0x2a, 0x2b,
    0x14, 0x1a, 0x08, 0x15, 0x17, 0x1c, 0x18, 0x0c,
    0x12, 0x13, 0x2f, 0x30, 0x28, 0xe0, 0x04, 0x16,
    0x07, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x0f, 0x33,
    0x34, 0x35, 0xe1, 0x31, 0x1d, 0x1b, 0x06, 0x19,
    0x05, 0x11, 0x10, 0x36, 0x37, 0x38, 0xe5, 0x55,
    0xe2, 0x2c, 0x32, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e,
    0x3f, 0x40, 0x41, 0x42, 0x43, 0x53, 0x47, 0x5f,
    0x60, 0x61, 0x56, 0x5c, 0x5d, 0x5e, 0x57, 0x59,
    0x5a, 0x5b, 0x62, 0x63, 0x00, 0x00, 0x00, 0x44,
    0x45, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
    0xe8, 0xe9, 0x71, 0x72, 0x73, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x85, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xe3, 0xe7, 0x65,

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x58, 0xe4, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x00, 0x46,
    0xe6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x4a,
    0x52, 0x4b, 0x00, 0x50, 0x00, 0x4f, 0x00, 0x4d,
    0x51, 0x4e, 0x49, 0x4c, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static void usb_hid_changed(USBHIDState *hs)
{
    hs->changed = 1;

    if (hs->datain)
        hs->datain(hs->datain_opaque);
}

static void usb_mouse_event(void *opaque,
                            int dx1, int dy1, int dz1, int buttons_state)
{
    USBHIDState *hs = opaque;
    USBMouseState *s = &hs->ptr;

    s->dx += dx1;
    s->dy += dy1;
    s->dz += dz1;
    s->buttons_state = buttons_state;

    usb_hid_changed(hs);
}

static void usb_tablet_event(void *opaque,
			     int x, int y, int dz, int buttons_state)
{
    USBHIDState *hs = opaque;
    USBMouseState *s = &hs->ptr;

    s->x = x;
    s->y = y;
    s->dz += dz;
    s->buttons_state = buttons_state;

    usb_hid_changed(hs);
}

static void usb_keyboard_event(void *opaque, int keycode)
{
    USBHIDState *hs = opaque;
    USBKeyboardState *s = &hs->kbd;
    uint8_t hid_code, key;
    int i;

    key = keycode & 0x7f;
    hid_code = usb_hid_usage_keys[key | ((s->modifiers >> 1) & (1 << 7))];
    s->modifiers &= ~(1 << 8);

    switch (hid_code) {
    case 0x00:
        return;

    case 0xe0:
        if (s->modifiers & (1 << 9)) {
            s->modifiers ^= 3 << 8;
            return;
        }
    case 0xe1 ... 0xe7:
        if (keycode & (1 << 7)) {
            s->modifiers &= ~(1 << (hid_code & 0x0f));
            return;
        }
    case 0xe8 ... 0xef:
        s->modifiers |= 1 << (hid_code & 0x0f);
        return;
    }

    if (keycode & (1 << 7)) {
        for (i = s->keys - 1; i >= 0; i --)
            if (s->key[i] == hid_code) {
                s->key[i] = s->key[-- s->keys];
                s->key[s->keys] = 0x00;
                usb_hid_changed(hs);
                break;
            }
        if (i < 0)
            return;
    } else {
        for (i = s->keys - 1; i >= 0; i --)
            if (s->key[i] == hid_code)
                break;
        if (i < 0) {
            if (s->keys < sizeof(s->key))
                s->key[s->keys ++] = hid_code;
        } else
            return;
    }

    usb_hid_changed(hs);
}

static inline int int_clamp(int val, int vmin, int vmax)
{
    if (val < vmin)
        return vmin;
    else if (val > vmax)
        return vmax;
    else
        return val;
}

static int usb_mouse_poll(USBHIDState *hs, uint8_t *buf, int len)
{
    int dx, dy, dz, b, l;
    USBMouseState *s = &hs->ptr;

    if (!s->mouse_grabbed) {
	s->eh_entry = qemu_add_mouse_event_handler(usb_mouse_event, hs,
                                                  0, "QEMU USB Mouse");
	s->mouse_grabbed = 1;
    }

    dx = int_clamp(s->dx, -127, 127);
    dy = int_clamp(s->dy, -127, 127);
    dz = int_clamp(s->dz, -127, 127);

    s->dx -= dx;
    s->dy -= dy;
    s->dz -= dz;

    /* Appears we have to invert the wheel direction */
    dz = 0 - dz;

    b = 0;
    if (s->buttons_state & MOUSE_EVENT_LBUTTON)
        b |= 0x01;
    if (s->buttons_state & MOUSE_EVENT_RBUTTON)
        b |= 0x02;
    if (s->buttons_state & MOUSE_EVENT_MBUTTON)
        b |= 0x04;

    l = 0;
    if (len > l)
        buf[l ++] = b;
    if (len > l)
        buf[l ++] = dx;
    if (len > l)
        buf[l ++] = dy;
    if (len > l)
        buf[l ++] = dz;
    return l;
}

static int usb_tablet_poll(USBHIDState *hs, uint8_t *buf, int len)
{
    int dz, b, l;
    USBMouseState *s = &hs->ptr;

    if (!s->mouse_grabbed) {
	s->eh_entry = qemu_add_mouse_event_handler(usb_tablet_event, hs,
                                                  1, "QEMU USB Tablet");
	s->mouse_grabbed = 1;
    }

    dz = int_clamp(s->dz, -127, 127);
    s->dz -= dz;

    /* Appears we have to invert the wheel direction */
    dz = 0 - dz;
    b = 0;
    if (s->buttons_state & MOUSE_EVENT_LBUTTON)
        b |= 0x01;
    if (s->buttons_state & MOUSE_EVENT_RBUTTON)
        b |= 0x02;
    if (s->buttons_state & MOUSE_EVENT_MBUTTON)
        b |= 0x04;

    buf[0] = b;
    buf[1] = s->x & 0xff;
    buf[2] = s->x >> 8;
    buf[3] = s->y & 0xff;
    buf[4] = s->y >> 8;
    buf[5] = dz;
    l = 6;

    return l;
}

static int usb_keyboard_poll(USBKeyboardState *s, uint8_t *buf, int len)
{
    if (len < 2)
        return 0;

    buf[0] = s->modifiers & 0xff;
    buf[1] = 0;
    if (s->keys > 6)
        memset(buf + 2, USB_HID_USAGE_ERROR_ROLLOVER, MIN(8, len) - 2);
    else
        memcpy(buf + 2, s->key, MIN(8, len) - 2);

    return MIN(8, len);
}

static int usb_keyboard_write(USBKeyboardState *s, uint8_t *buf, int len)
{
    if (len > 0) {
        /* 0x01: Num Lock LED
         * 0x02: Caps Lock LED
         * 0x04: Scroll Lock LED
         * 0x08: Compose LED
         * 0x10: Kana LED */
        s->leds = buf[0];
    }
    return 0;
}

static void usb_mouse_handle_reset(USBDevice *dev)
{
    USBHIDState *s = (USBHIDState *)dev;

    s->ptr.dx = 0;
    s->ptr.dy = 0;
    s->ptr.dz = 0;
    s->ptr.x = 0;
    s->ptr.y = 0;
    s->ptr.buttons_state = 0;
    s->protocol = 1;
}

static void usb_keyboard_handle_reset(USBDevice *dev)
{
    USBHIDState *s = (USBHIDState *)dev;

    qemu_add_kbd_event_handler(usb_keyboard_event, s);
    s->protocol = 1;
}

static int usb_hid_handle_control(USBDevice *dev, int request, int value,
                                  int index, int length, uint8_t *data)
{
    USBHIDState *s = (USBHIDState *)dev;
    int ret = 0;

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
            memcpy(data, qemu_mouse_dev_descriptor,
                   sizeof(qemu_mouse_dev_descriptor));
            ret = sizeof(qemu_mouse_dev_descriptor);
            break;
        case USB_DT_CONFIG:
	    if (s->kind == USB_MOUSE) {
		memcpy(data, qemu_mouse_config_descriptor,
		       sizeof(qemu_mouse_config_descriptor));
		ret = sizeof(qemu_mouse_config_descriptor);
	    } else if (s->kind == USB_TABLET) {
		memcpy(data, qemu_tablet_config_descriptor,
		       sizeof(qemu_tablet_config_descriptor));
		ret = sizeof(qemu_tablet_config_descriptor);
            } else if (s->kind == USB_KEYBOARD) {
                memcpy(data, qemu_keyboard_config_descriptor,
                       sizeof(qemu_keyboard_config_descriptor));
                ret = sizeof(qemu_keyboard_config_descriptor);
            }
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
                ret = set_usb_string(data, "1");
                break;
            case 2:
                /* product description */
                ret = set_usb_string(data, s->dev.product_desc);
                break;
            case 3:
                /* vendor description */
                ret = set_usb_string(data, "QEMU " QEMU_VERSION);
                break;
            case 4:
                ret = set_usb_string(data, "HID Mouse");
                break;
            case 5:
                ret = set_usb_string(data, "HID Tablet");
                break;
            case 6:
                ret = set_usb_string(data, "HID Keyboard");
                break;
            case 7:
                ret = set_usb_string(data, "Endpoint1 Interrupt Pipe");
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
        switch(value >> 8) {
        case 0x22:
	    if (s->kind == USB_MOUSE) {
		memcpy(data, qemu_mouse_hid_report_descriptor,
		       sizeof(qemu_mouse_hid_report_descriptor));
		ret = sizeof(qemu_mouse_hid_report_descriptor);
	    } else if (s->kind == USB_TABLET) {
		memcpy(data, qemu_tablet_hid_report_descriptor,
		       sizeof(qemu_tablet_hid_report_descriptor));
		ret = sizeof(qemu_tablet_hid_report_descriptor);
            } else if (s->kind == USB_KEYBOARD) {
                memcpy(data, qemu_keyboard_hid_report_descriptor,
                       sizeof(qemu_keyboard_hid_report_descriptor));
                ret = sizeof(qemu_keyboard_hid_report_descriptor);
            }
            break;
        default:
            goto fail;
        }
        break;
    case GET_REPORT:
	if (s->kind == USB_MOUSE)
            ret = usb_mouse_poll(s, data, length);
	else if (s->kind == USB_TABLET)
            ret = usb_tablet_poll(s, data, length);
        else if (s->kind == USB_KEYBOARD)
            ret = usb_keyboard_poll(&s->kbd, data, length);
        break;
    case SET_REPORT:
        if (s->kind == USB_KEYBOARD)
            ret = usb_keyboard_write(&s->kbd, data, length);
        else
            goto fail;
        break;
    case GET_PROTOCOL:
        if (s->kind != USB_KEYBOARD)
            goto fail;
        ret = 1;
        data[0] = s->protocol;
        break;
    case SET_PROTOCOL:
        if (s->kind != USB_KEYBOARD)
            goto fail;
        ret = 0;
        s->protocol = value;
        break;
    case GET_IDLE:
        ret = 1;
        data[0] = s->idle;
        break;
    case SET_IDLE:
        s->idle = (uint8_t) (value >> 8);
        ret = 0;
        break;
    default:
    fail:
        ret = USB_RET_STALL;
        break;
    }
    return ret;
}

static int usb_hid_handle_data(USBDevice *dev, USBPacket *p)
{
    USBHIDState *s = (USBHIDState *)dev;
    int ret = 0;

    switch(p->pid) {
    case USB_TOKEN_IN:
        if (p->devep == 1) {
            /* TODO: Implement finite idle delays.  */
            if (!(s->changed || s->idle))
                return USB_RET_NAK;
            s->changed = 0;
            if (s->kind == USB_MOUSE)
                ret = usb_mouse_poll(s, p->data, p->len);
            else if (s->kind == USB_TABLET)
                ret = usb_tablet_poll(s, p->data, p->len);
            else if (s->kind == USB_KEYBOARD)
                ret = usb_keyboard_poll(&s->kbd, p->data, p->len);
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

static void usb_hid_handle_destroy(USBDevice *dev)
{
    USBHIDState *s = (USBHIDState *)dev;

    if (s->kind != USB_KEYBOARD)
        qemu_remove_mouse_event_handler(s->ptr.eh_entry);
    /* TODO: else */
}

static int usb_hid_initfn(USBDevice *dev, int kind)
{
    USBHIDState *s = DO_UPCAST(USBHIDState, dev, dev);
    s->dev.speed = USB_SPEED_FULL;
    s->kind = kind;
    /* Force poll routine to be run and grab input the first time.  */
    s->changed = 1;
    return 0;
}

static int usb_tablet_initfn(USBDevice *dev)
{
    return usb_hid_initfn(dev, USB_TABLET);
}

static int usb_mouse_initfn(USBDevice *dev)
{
    return usb_hid_initfn(dev, USB_MOUSE);
}

static int usb_keyboard_initfn(USBDevice *dev)
{
    return usb_hid_initfn(dev, USB_KEYBOARD);
}

void usb_hid_datain_cb(USBDevice *dev, void *opaque, void (*datain)(void *))
{
    USBHIDState *s = (USBHIDState *)dev;

    s->datain_opaque = opaque;
    s->datain = datain;
}

static struct USBDeviceInfo hid_info[] = {
    {
        .product_desc   = "QEMU USB Tablet",
        .qdev.name      = "usb-tablet",
        .usbdevice_name = "tablet",
        .qdev.size      = sizeof(USBHIDState),
        .init           = usb_tablet_initfn,
        .handle_packet  = usb_generic_handle_packet,
        .handle_reset   = usb_mouse_handle_reset,
        .handle_control = usb_hid_handle_control,
        .handle_data    = usb_hid_handle_data,
        .handle_destroy = usb_hid_handle_destroy,
    },{
        .product_desc   = "QEMU USB Mouse",
        .qdev.name      = "usb-mouse",
        .usbdevice_name = "mouse",
        .qdev.size      = sizeof(USBHIDState),
        .init           = usb_mouse_initfn,
        .handle_packet  = usb_generic_handle_packet,
        .handle_reset   = usb_mouse_handle_reset,
        .handle_control = usb_hid_handle_control,
        .handle_data    = usb_hid_handle_data,
        .handle_destroy = usb_hid_handle_destroy,
    },{
        .product_desc   = "QEMU USB Keyboard",
        .qdev.name      = "usb-kbd",
        .usbdevice_name = "keyboard",
        .qdev.size      = sizeof(USBHIDState),
        .init           = usb_keyboard_initfn,
        .handle_packet  = usb_generic_handle_packet,
        .handle_reset   = usb_keyboard_handle_reset,
        .handle_control = usb_hid_handle_control,
        .handle_data    = usb_hid_handle_data,
        .handle_destroy = usb_hid_handle_destroy,
    },{
        /* end of list */
    }
};

static void usb_hid_register_devices(void)
{
    usb_qdev_register_many(hid_info);
}
device_init(usb_hid_register_devices)
