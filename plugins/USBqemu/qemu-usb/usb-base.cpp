/*
 * QEMU USB emulation
 *
 * Copyright (c) 2005 Fabrice Bellard
 *
 * 2008 Generic packet handler rewrite by Max Krasnyansky
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
//#include "usb.h"

void usb_attach(USBPort *port, USBDevice *dev)
{
    port->attach(port, dev);
}

/**********************/

/* generic USB device helpers (you are not forced to use them when
   writing your USB device driver, but they help handling the
   protocol)
*/

#define SETUP_STATE_IDLE 0
#define SETUP_STATE_DATA 1
#define SETUP_STATE_ACK  2

static int do_token_setup(USBDevice *s, USBPacket *p)
{
    int request, value, index;
    int ret = 0;

    if (p->len != 8)
        return USB_RET_STALL;
 
    memcpy(s->setup_buf, p->data, 8);
    s->setup_len   = (s->setup_buf[7] << 8) | s->setup_buf[6];
    s->setup_index = 0;

    request = (s->setup_buf[0] << 8) | s->setup_buf[1];
    value   = (s->setup_buf[3] << 8) | s->setup_buf[2];
    index   = (s->setup_buf[5] << 8) | s->setup_buf[4];
 
    if (s->setup_buf[0] & USB_DIR_IN) {
        ret = s->info->handle_control(s, request, value, index, 
                                      s->setup_len, s->data_buf);
        if (ret < 0)
            return ret;

        if (ret < s->setup_len)
            s->setup_len = ret;
        s->setup_state = SETUP_STATE_DATA;
    } else {
        if (s->setup_len == 0)
            s->setup_state = SETUP_STATE_ACK;
        else
            s->setup_state = SETUP_STATE_DATA;
    }

    return ret;
}

static int do_token_in(USBDevice *s, USBPacket *p)
{
    int request, value, index;
    int ret = 0;

    if (p->devep != 0)
        return s->info->handle_data(s, p);

    request = (s->setup_buf[0] << 8) | s->setup_buf[1];
    value   = (s->setup_buf[3] << 8) | s->setup_buf[2];
    index   = (s->setup_buf[5] << 8) | s->setup_buf[4];
 
    switch(s->setup_state) {
    case SETUP_STATE_ACK:
        if (!(s->setup_buf[0] & USB_DIR_IN)) {
            s->setup_state = SETUP_STATE_IDLE;
            ret = s->info->handle_control(s, request, value, index,
                                          s->setup_len, s->data_buf);
            if (ret > 0)
                return 0;
            return ret;
        }

        /* return 0 byte */
        return 0;

    case SETUP_STATE_DATA:
        if (s->setup_buf[0] & USB_DIR_IN) {
            int len = s->setup_len - s->setup_index;
            if (len > p->len)
                len = p->len;
            memcpy(p->data, s->data_buf + s->setup_index, len);
            s->setup_index += len;
            if (s->setup_index >= s->setup_len)
                s->setup_state = SETUP_STATE_ACK;
            return len;
        }

        s->setup_state = SETUP_STATE_IDLE;
        return USB_RET_STALL;

    default:
        return USB_RET_STALL;
    }
}

static int do_token_out(USBDevice *s, USBPacket *p)
{
    if (p->devep != 0)
        return s->info->handle_data(s, p);

    switch(s->setup_state) {
    case SETUP_STATE_ACK:
        if (s->setup_buf[0] & USB_DIR_IN) {
            s->setup_state = SETUP_STATE_IDLE;
            /* transfer OK */
        } else {
            /* ignore additional output */
        }
        return 0;

    case SETUP_STATE_DATA:
        if (!(s->setup_buf[0] & USB_DIR_IN)) {
            int len = s->setup_len - s->setup_index;
            if (len > p->len)
                len = p->len;
            memcpy(s->data_buf + s->setup_index, p->data, len);
            s->setup_index += len;
            if (s->setup_index >= s->setup_len)
                s->setup_state = SETUP_STATE_ACK;
            return len;
        }

        s->setup_state = SETUP_STATE_IDLE;
        return USB_RET_STALL;

    default:
        return USB_RET_STALL;
    }
}

/*
 * Generic packet handler.
 * Called by the HC (host controller).
 *
 * Returns length of the transaction or one of the USB_RET_XXX codes.
 */
int usb_generic_handle_packet(USBDevice *s, USBPacket *p)
{
    switch(p->pid) {
    case USB_MSG_ATTACH:
        s->state = USB_STATE_ATTACHED;
        return 0;

    case USB_MSG_DETACH:
        s->state = USB_STATE_NOTATTACHED;
        return 0;

    case USB_MSG_RESET:
        s->remote_wakeup = 0;
        s->addr = 0;
        s->state = USB_STATE_DEFAULT;
        s->info->handle_reset(s);
        return 0;
    }

    /* Rest of the PIDs must match our address */
    if (s->state < USB_STATE_DEFAULT || p->devaddr != s->addr)
        return USB_RET_NODEV;

    switch (p->pid) {
    case USB_TOKEN_SETUP:
        return do_token_setup(s, p);

    case USB_TOKEN_IN:
        return do_token_in(s, p);

    case USB_TOKEN_OUT:
        return do_token_out(s, p);
 
    default:
        return USB_RET_STALL;
    }
}

/* XXX: fix overflow */
int set_usb_string(uint8_t *buf, const char *str)
{
    int len, i;
    uint8_t *q;

    q = buf;
    len = strlen(str);
    *q++ = 2 * len + 2;
    *q++ = 3;
    for(i = 0; i < len; i++) {
        *q++ = str[i];
        *q++ = 0;
    }
    return q - buf;
}

/* Send an internal message to a USB device.  */
void usb_send_msg(USBDevice *dev, int msg)
{
    USBPacket p;
    memset(&p, 0, sizeof(p));
    p.pid = msg;
    dev->info->handle_packet(dev, &p);

    /* This _must_ be synchronous */
}
