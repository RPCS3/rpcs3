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
#include "usb-desc.h"
#include "qemu-timer.h"

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

typedef struct USBPointerEvent {
    int32_t xdx, ydy; /* relative iff it's a mouse, otherwise absolute */
    int32_t dz, buttons_state;
} USBPointerEvent;

#define QUEUE_LENGTH    16 /* should be enough for a triple-click */
#define QUEUE_MASK      (QUEUE_LENGTH-1u)
#define QUEUE_INCR(v)   ((v)++, (v) &= QUEUE_MASK)

typedef struct USBMouseState {
    USBPointerEvent queue[QUEUE_LENGTH];
    int mouse_grabbed;
    QEMUPutMouseEntry *eh_entry;
} USBMouseState;

typedef struct USBKeyboardState {
    uint32_t keycodes[QUEUE_LENGTH];
    uint16_t modifiers;
    uint8_t leds;
    uint8_t key[16];
    int32_t keys;
} USBKeyboardState;

typedef struct USBHIDState {
    USBDevice dev;
    union {
        USBMouseState ptr;
        USBKeyboardState kbd;
    };
    uint32_t head; /* index into circular queue */
    uint32_t n;
    int kind;
    int32_t protocol;
    uint8_t idle;
    int64_t next_idle_clock;
    int changed;
    void *datain_opaque;
    void (*datain)(void *);
} USBHIDState;

enum {
    STR_MANUFACTURER = 1,
    STR_PRODUCT_MOUSE,
    STR_PRODUCT_TABLET,
    STR_PRODUCT_KEYBOARD,
    STR_SERIALNUMBER,
    STR_CONFIG_MOUSE,
    STR_CONFIG_TABLET,
    STR_CONFIG_KEYBOARD,
};

static const USBDescStrings desc_strings = {
    [STR_MANUFACTURER]     = "QEMU " QEMU_VERSION,
    [STR_PRODUCT_MOUSE]    = "QEMU USB Mouse",
    [STR_PRODUCT_TABLET]   = "QEMU USB Tablet",
    [STR_PRODUCT_KEYBOARD] = "QEMU USB Keyboard",
    [STR_SERIALNUMBER]     = "42", /* == remote wakeup works */
    [STR_CONFIG_MOUSE]     = "HID Mouse",
    [STR_CONFIG_TABLET]    = "HID Tablet",
    [STR_CONFIG_KEYBOARD]  = "HID Keyboard",
};

static const USBDescIface desc_iface_mouse = {
    .bInterfaceNumber              = 0,
    .bNumEndpoints                 = 1,
    .bInterfaceClass               = USB_CLASS_HID,
    .bInterfaceSubClass            = 0x01, /* boot */
    .bInterfaceProtocol            = 0x02,
    .ndesc                         = 1,
    .descs = (USBDescOther[]) {
        {
            /* HID descriptor */
            .data = (uint8_t[]) {
                0x09,          /*  u8  bLength */
                USB_DT_HID,    /*  u8  bDescriptorType */
                0x01, 0x00,    /*  u16 HID_class */
                0x00,          /*  u8  country_code */
                0x01,          /*  u8  num_descriptors */
                USB_DT_REPORT, /*  u8  type: Report */
                52, 0,         /*  u16 len */
            },
        },
    },
    .eps = (USBDescEndpoint[]) {
        {
            .bEndpointAddress      = USB_DIR_IN | 0x01,
            .bmAttributes          = USB_ENDPOINT_XFER_INT,
            .wMaxPacketSize        = 4,
            .bInterval             = 0x0a,
        },
    },
};

static const USBDescIface desc_iface_tablet = {
    .bInterfaceNumber              = 0,
    .bNumEndpoints                 = 1,
    .bInterfaceClass               = USB_CLASS_HID,
    .bInterfaceProtocol            = 0x02,
    .ndesc                         = 1,
    .descs = (USBDescOther[]) {
        {
            /* HID descriptor */
            .data = (uint8_t[]) {
                0x09,          /*  u8  bLength */
                USB_DT_HID,    /*  u8  bDescriptorType */
                0x01, 0x00,    /*  u16 HID_class */
                0x00,          /*  u8  country_code */
                0x01,          /*  u8  num_descriptors */
                USB_DT_REPORT, /*  u8  type: Report */
                74, 0,         /*  u16 len */
            },
        },
    },
    .eps = (USBDescEndpoint[]) {
        {
            .bEndpointAddress      = USB_DIR_IN | 0x01,
            .bmAttributes          = USB_ENDPOINT_XFER_INT,
            .wMaxPacketSize        = 8,
            .bInterval             = 0x0a,
        },
    },
};

static const USBDescIface desc_iface_keyboard = {
    .bInterfaceNumber              = 0,
    .bNumEndpoints                 = 1,
    .bInterfaceClass               = USB_CLASS_HID,
    .bInterfaceSubClass            = 0x01, /* boot */
    .bInterfaceProtocol            = 0x01, /* keyboard */
    .ndesc                         = 1,
    .descs = (USBDescOther[]) {
        {
            /* HID descriptor */
            .data = (uint8_t[]) {
                0x09,          /*  u8  bLength */
                USB_DT_HID,    /*  u8  bDescriptorType */
                0x11, 0x01,    /*  u16 HID_class */
                0x00,          /*  u8  country_code */
                0x01,          /*  u8  num_descriptors */
                USB_DT_REPORT, /*  u8  type: Report */
                0x3f, 0,       /*  u16 len */
            },
        },
    },
    .eps = (USBDescEndpoint[]) {
        {
            .bEndpointAddress      = USB_DIR_IN | 0x01,
            .bmAttributes          = USB_ENDPOINT_XFER_INT,
            .wMaxPacketSize        = 8,
            .bInterval             = 0x0a,
        },
    },
};

static const USBDescDevice desc_device_mouse = {
    .bcdUSB                        = 0x0100,
    .bMaxPacketSize0               = 8,
    .bNumConfigurations            = 1,
    .confs = (USBDescConfig[]) {
        {
            .bNumInterfaces        = 1,
            .bConfigurationValue   = 1,
            .iConfiguration        = STR_CONFIG_MOUSE,
            .bmAttributes          = 0xa0,
            .bMaxPower             = 50,
            .nif = 1,
            .ifs = &desc_iface_mouse,
        },
    },
};

static const USBDescDevice desc_device_tablet = {
    .bcdUSB                        = 0x0100,
    .bMaxPacketSize0               = 8,
    .bNumConfigurations            = 1,
    .confs = (USBDescConfig[]) {
        {
            .bNumInterfaces        = 1,
            .bConfigurationValue   = 1,
            .iConfiguration        = STR_CONFIG_TABLET,
            .bmAttributes          = 0xa0,
            .bMaxPower             = 50,
            .nif = 1,
            .ifs = &desc_iface_tablet,
        },
    },
};

static const USBDescDevice desc_device_keyboard = {
    .bcdUSB                        = 0x0100,
    .bMaxPacketSize0               = 8,
    .bNumConfigurations            = 1,
    .confs = (USBDescConfig[]) {
        {
            .bNumInterfaces        = 1,
            .bConfigurationValue   = 1,
            .iConfiguration        = STR_CONFIG_KEYBOARD,
            .bmAttributes          = 0xa0,
            .bMaxPower             = 50,
            .nif = 1,
            .ifs = &desc_iface_keyboard,
        },
    },
};

static const USBDesc desc_mouse = {
    .id = {
        .idVendor          = 0x0627,
        .idProduct         = 0x0001,
        .bcdDevice         = 0,
        .iManufacturer     = STR_MANUFACTURER,
        .iProduct          = STR_PRODUCT_MOUSE,
        .iSerialNumber     = STR_SERIALNUMBER,
    },
    .full = &desc_device_mouse,
    .str  = desc_strings,
};

static const USBDesc desc_tablet = {
    .id = {
        .idVendor          = 0x0627,
        .idProduct         = 0x0001,
        .bcdDevice         = 0,
        .iManufacturer     = STR_MANUFACTURER,
        .iProduct          = STR_PRODUCT_TABLET,
        .iSerialNumber     = STR_SERIALNUMBER,
    },
    .full = &desc_device_tablet,
    .str  = desc_strings,
};

static const USBDesc desc_keyboard = {
    .id = {
        .idVendor          = 0x0627,
        .idProduct         = 0x0001,
        .bcdDevice         = 0,
        .iManufacturer     = STR_MANUFACTURER,
        .iProduct          = STR_PRODUCT_KEYBOARD,
        .iSerialNumber     = STR_SERIALNUMBER,
    },
    .full = &desc_device_keyboard,
    .str  = desc_strings,
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
    0x00, 0x00, 0x00, 0xe3, 0xe7, 0x65, 0x00, 0x00,
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

    usb_wakeup(&hs->dev);
}

static void usb_pointer_event_clear(USBPointerEvent *e, int buttons) {
    e->xdx = e->ydy = e->dz = 0;
    e->buttons_state = buttons;
}

static void usb_pointer_event_combine(USBPointerEvent *e, int xyrel,
                                      int x1, int y1, int z1) {
    if (xyrel) {
        e->xdx += x1;
        e->ydy += y1;
    } else {
        e->xdx = x1;
        e->ydy = y1;
    }
    e->dz += z1;
}

static void usb_pointer_event(void *opaque,
                              int x1, int y1, int z1, int buttons_state)
{
    USBHIDState *hs = opaque;
    USBMouseState *s = &hs->ptr;
    unsigned use_slot = (hs->head + hs->n - 1) & QUEUE_MASK;
    unsigned previous_slot = (use_slot - 1) & QUEUE_MASK;

    /* We combine events where feasible to keep the queue small.  We shouldn't
     * combine anything with the first event of a particular button state, as
     * that would change the location of the button state change.  When the
     * queue is empty, a second event is needed because we don't know if
     * the first event changed the button state.  */
    if (hs->n == QUEUE_LENGTH) {
        /* Queue full.  Discard old button state, combine motion normally.  */
        s->queue[use_slot].buttons_state = buttons_state;
    } else if (hs->n < 2 ||
               s->queue[use_slot].buttons_state != buttons_state ||
               s->queue[previous_slot].buttons_state != s->queue[use_slot].buttons_state) {
        /* Cannot or should not combine, so add an empty item to the queue.  */
        QUEUE_INCR(use_slot);
        hs->n++;
        usb_pointer_event_clear(&s->queue[use_slot], buttons_state);
    }
    usb_pointer_event_combine(&s->queue[use_slot],
                              hs->kind == USB_MOUSE,
                              x1, y1, z1);
    usb_hid_changed(hs);
}

static void usb_keyboard_event(void *opaque, int keycode)
{
    USBHIDState *hs = opaque;
    USBKeyboardState *s = &hs->kbd;
    int slot;

    if (hs->n == QUEUE_LENGTH) {
        fprintf(stderr, "usb-kbd: warning: key event queue full\n");
        return;
    }
    slot = (hs->head + hs->n) & QUEUE_MASK; hs->n++;
    s->keycodes[slot] = keycode;
    usb_hid_changed(hs);
}

static void usb_keyboard_process_keycode(USBHIDState *hs)
{
    USBKeyboardState *s = &hs->kbd;
    uint8_t hid_code, key;
    int i, keycode, slot;

    if (hs->n == 0) {
        return;
    }
    slot = hs->head & QUEUE_MASK; QUEUE_INCR(hs->head); hs->n--;
    keycode = s->keycodes[slot];

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

static int usb_pointer_poll(USBHIDState *hs, uint8_t *buf, int len)
{
    int dx, dy, dz, b, l;
    int index;
    USBMouseState *s = &hs->ptr;
    USBPointerEvent *e;

    if (!s->mouse_grabbed) {
        qemu_activate_mouse_event_handler(s->eh_entry);
        s->mouse_grabbed = 1;
    }

    /* When the buffer is empty, return the last event.  Relative
       movements will all be zero.  */
    index = (hs->n ? hs->head : hs->head - 1);
    e = &s->queue[index & QUEUE_MASK];

    if (hs->kind == USB_MOUSE) {
        dx = int_clamp(e->xdx, -127, 127);
        dy = int_clamp(e->ydy, -127, 127);
        e->xdx -= dx;
        e->ydy -= dy;
    } else {
        dx = e->xdx;
        dy = e->ydy;
    }
    dz = int_clamp(e->dz, -127, 127);
    e->dz -= dz;

    b = 0;
    if (e->buttons_state & MOUSE_EVENT_LBUTTON)
        b |= 0x01;
    if (e->buttons_state & MOUSE_EVENT_RBUTTON)
        b |= 0x02;
    if (e->buttons_state & MOUSE_EVENT_MBUTTON)
        b |= 0x04;

    if (hs->n &&
        !e->dz &&
        (hs->kind == USB_TABLET || (!e->xdx && !e->ydy))) {
        /* that deals with this event */
        QUEUE_INCR(hs->head);
        hs->n--;
    }

    /* Appears we have to invert the wheel direction */
    dz = 0 - dz;
    l = 0;
    switch (hs->kind) {
    case USB_MOUSE:
        if (len > l)
            buf[l++] = b;
        if (len > l)
            buf[l++] = dx;
        if (len > l)
            buf[l++] = dy;
        if (len > l)
            buf[l++] = dz;
        break;

    case USB_TABLET:
        if (len > l)
            buf[l++] = b;
        if (len > l)
            buf[l++] = dx & 0xff;
        if (len > l)
            buf[l++] = dx >> 8;
        if (len > l)
            buf[l++] = dy & 0xff;
        if (len > l)
            buf[l++] = dy >> 8;
        if (len > l)
            buf[l++] = dz;
        break;

    default:
        abort();
    }

    return l;
}

static int usb_keyboard_poll(USBHIDState *hs, uint8_t *buf, int len)
{
    USBKeyboardState *s = &hs->kbd;
    if (len < 2)
        return 0;

    usb_keyboard_process_keycode(hs);

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
        int ledstate = 0;
        /* 0x01: Num Lock LED
         * 0x02: Caps Lock LED
         * 0x04: Scroll Lock LED
         * 0x08: Compose LED
         * 0x10: Kana LED */
        s->leds = buf[0];
        if (s->leds & 0x04)
            ledstate |= QEMU_SCROLL_LOCK_LED;
        if (s->leds & 0x01)
            ledstate |= QEMU_NUM_LOCK_LED;
        if (s->leds & 0x02)
            ledstate |= QEMU_CAPS_LOCK_LED;
        kbd_put_ledstate(ledstate);
    }
    return 0;
}

static void usb_mouse_handle_reset(USBDevice *dev)
{
    USBHIDState *s = (USBHIDState *)dev;

    memset(s->ptr.queue, 0, sizeof (s->ptr.queue));
    s->head = 0;
    s->n = 0;
    s->protocol = 1;
}

static void usb_keyboard_handle_reset(USBDevice *dev)
{
    USBHIDState *s = (USBHIDState *)dev;

    qemu_add_kbd_event_handler(usb_keyboard_event, s);
    memset(s->kbd.keycodes, 0, sizeof (s->kbd.keycodes));
    s->head = 0;
    s->n = 0;
    memset(s->kbd.key, 0, sizeof (s->kbd.key));
    s->kbd.keys = 0;
    s->protocol = 1;
}

static void usb_hid_set_next_idle(USBHIDState *s, int64_t curtime)
{
    s->next_idle_clock = curtime + (get_ticks_per_sec() * s->idle * 4) / 1000;
}

static int usb_hid_handle_control(USBDevice *dev, USBPacket *p,
               int request, int value, int index, int length, uint8_t *data)
{
    USBHIDState *s = (USBHIDState *)dev;
    int ret;

    ret = usb_desc_handle_control(dev, p, request, value, index, length, data);
    if (ret >= 0) {
        return ret;
    }

    ret = 0;
    switch(request) {
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
        if (s->kind == USB_MOUSE || s->kind == USB_TABLET) {
            ret = usb_pointer_poll(s, data, length);
        } else if (s->kind == USB_KEYBOARD) {
            ret = usb_keyboard_poll(s, data, length);
        }
        s->changed = s->n > 0;
        break;
    case SET_REPORT:
        if (s->kind == USB_KEYBOARD)
            ret = usb_keyboard_write(&s->kbd, data, length);
        else
            goto fail;
        break;
    case GET_PROTOCOL:
        if (s->kind != USB_KEYBOARD && s->kind != USB_MOUSE)
            goto fail;
        ret = 1;
        data[0] = s->protocol;
        break;
    case SET_PROTOCOL:
        if (s->kind != USB_KEYBOARD && s->kind != USB_MOUSE)
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
        usb_hid_set_next_idle(s, qemu_get_clock_ns(vm_clock));
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
            int64_t curtime = qemu_get_clock_ns(vm_clock);
            if (!s->changed && (!s->idle || s->next_idle_clock - curtime > 0))
                return USB_RET_NAK;
            usb_hid_set_next_idle(s, curtime);
            if (s->kind == USB_MOUSE || s->kind == USB_TABLET) {
                ret = usb_pointer_poll(s, p->data, p->len);
            }
            else if (s->kind == USB_KEYBOARD) {
                ret = usb_keyboard_poll(s, p->data, p->len);
            }
            s->changed = s->n > 0;
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

    switch(s->kind) {
    case USB_KEYBOARD:
        qemu_remove_kbd_event_handler();
        break;
    default:
        qemu_remove_mouse_event_handler(s->ptr.eh_entry);
    }
}

static int usb_hid_initfn(USBDevice *dev, int kind)
{
    USBHIDState *s = DO_UPCAST(USBHIDState, dev, dev);

    usb_desc_init(dev);
    s->kind = kind;

    if (s->kind == USB_MOUSE) {
        s->ptr.eh_entry = qemu_add_mouse_event_handler(usb_pointer_event, s,
                                                       0, "QEMU USB Mouse");
    } else if (s->kind == USB_TABLET) {
        s->ptr.eh_entry = qemu_add_mouse_event_handler(usb_pointer_event, s,
                                                       1, "QEMU USB Tablet");
    }

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

static int usb_hid_post_load(void *opaque, int version_id)
{
    USBHIDState *s = opaque;

    if (s->idle) {
        usb_hid_set_next_idle(s, qemu_get_clock_ns(vm_clock));
    }
    return 0;
}

static const VMStateDescription vmstate_usb_ptr_queue = {
    .name = "usb-ptr-queue",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField []) {
        VMSTATE_INT32(xdx, USBPointerEvent),
        VMSTATE_INT32(ydy, USBPointerEvent),
        VMSTATE_INT32(dz, USBPointerEvent),
        VMSTATE_INT32(buttons_state, USBPointerEvent),
        VMSTATE_END_OF_LIST()
    }
};
static const VMStateDescription vmstate_usb_ptr = {
    .name = "usb-ptr",
    .version_id = 1,
    .minimum_version_id = 1,
    .post_load = usb_hid_post_load,
    .fields = (VMStateField []) {
        VMSTATE_USB_DEVICE(dev, USBHIDState),
        VMSTATE_STRUCT_ARRAY(ptr.queue, USBHIDState, QUEUE_LENGTH, 0,
                             vmstate_usb_ptr_queue, USBPointerEvent),
        VMSTATE_UINT32(head, USBHIDState),
        VMSTATE_UINT32(n, USBHIDState),
        VMSTATE_INT32(protocol, USBHIDState),
        VMSTATE_UINT8(idle, USBHIDState),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription vmstate_usb_kbd = {
    .name = "usb-kbd",
    .version_id = 1,
    .minimum_version_id = 1,
    .post_load = usb_hid_post_load,
    .fields = (VMStateField []) {
        VMSTATE_USB_DEVICE(dev, USBHIDState),
        VMSTATE_UINT32_ARRAY(kbd.keycodes, USBHIDState, QUEUE_LENGTH),
        VMSTATE_UINT32(head, USBHIDState),
        VMSTATE_UINT32(n, USBHIDState),
        VMSTATE_UINT16(kbd.modifiers, USBHIDState),
        VMSTATE_UINT8(kbd.leds, USBHIDState),
        VMSTATE_UINT8_ARRAY(kbd.key, USBHIDState, 16),
        VMSTATE_INT32(kbd.keys, USBHIDState),
        VMSTATE_INT32(protocol, USBHIDState),
        VMSTATE_UINT8(idle, USBHIDState),
        VMSTATE_END_OF_LIST()
    }
};

static struct USBDeviceInfo hid_info[] = {
    {
        .product_desc   = "QEMU USB Tablet",
        .qdev.name      = "usb-tablet",
        .usbdevice_name = "tablet",
        .qdev.size      = sizeof(USBHIDState),
        .qdev.vmsd      = &vmstate_usb_ptr,
        .usb_desc       = &desc_tablet,
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
        .qdev.vmsd      = &vmstate_usb_ptr,
        .usb_desc       = &desc_mouse,
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
        .qdev.vmsd      = &vmstate_usb_kbd,
        .usb_desc       = &desc_keyboard,
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
