/*
 * QEMU USB emulation
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

int usb_generic_handle_packet(USBDevice *s, int pid, 
                              uint8_t devaddr, uint8_t devep,
                              uint8_t *data, int len)
{
    int l, ret = 0;

    switch(pid) {
    case USB_MSG_ATTACH:
        s->state = USB_STATE_ATTACHED;
        break;
    case USB_MSG_DETACH:
        s->state = USB_STATE_NOTATTACHED;
        break;
    case USB_MSG_RESET:
        s->remote_wakeup = 0;
        s->addr = 0;
        s->state = USB_STATE_DEFAULT;
        s->handle_reset(s);
        break;
    case USB_TOKEN_SETUP:
        if (s->state < USB_STATE_DEFAULT || devaddr != s->addr)
            return USB_RET_NODEV;
        if (len != 8)
            goto fail;
        memcpy(s->setup_buf, data, 8);
        s->setup_len = (s->setup_buf[7] << 8) | s->setup_buf[6];
        s->setup_index = 0;
        if (s->setup_buf[0] & USB_DIR_IN) {
            ret = s->handle_control(s, 
                                    (s->setup_buf[0] << 8) | s->setup_buf[1],
                                    (s->setup_buf[3] << 8) | s->setup_buf[2],
                                    (s->setup_buf[5] << 8) | s->setup_buf[4],
                                    s->setup_len,
                                    s->data_buf);
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
        break;
    case USB_TOKEN_IN:
        if (s->state < USB_STATE_DEFAULT || devaddr != s->addr)
            return USB_RET_NODEV;
        switch(devep) {
        case 0:
            switch(s->setup_state) {
            case SETUP_STATE_ACK:
                if (!(s->setup_buf[0] & USB_DIR_IN)) {
                    s->setup_state = SETUP_STATE_IDLE;
                    ret = s->handle_control(s, 
                                      (s->setup_buf[0] << 8) | s->setup_buf[1],
                                      (s->setup_buf[3] << 8) | s->setup_buf[2],
                                      (s->setup_buf[5] << 8) | s->setup_buf[4],
                                      s->setup_len,
                                      s->data_buf);
                    if (ret > 0)
                        ret = 0;
                } else {
                    /* return 0 byte */
                }
                break;
            case SETUP_STATE_DATA:
                if (s->setup_buf[0] & USB_DIR_IN) {
                    l = s->setup_len - s->setup_index;
                    if (l > len)
                        l = len;
                    memcpy(data, s->data_buf + s->setup_index, l);
                    s->setup_index += l;
                    if (s->setup_index >= s->setup_len)
                        s->setup_state = SETUP_STATE_ACK;
                    ret = l;
                } else {
                    s->setup_state = SETUP_STATE_IDLE;
                    goto fail;
                }
                break;
            default:
                goto fail;
            }
            break;
        default:
            ret = s->handle_data(s, pid, devep, data, len);
            break;
        }
        break;
    case USB_TOKEN_OUT:
        if (s->state < USB_STATE_DEFAULT || devaddr != s->addr)
            return USB_RET_NODEV;
        switch(devep) {
        case 0:
            switch(s->setup_state) {
            case SETUP_STATE_ACK:
                if (s->setup_buf[0] & USB_DIR_IN) {
                    s->setup_state = SETUP_STATE_IDLE;
                    /* transfer OK */
                } else {
                    /* ignore additionnal output */
                }
                break;
            case SETUP_STATE_DATA:
                if (!(s->setup_buf[0] & USB_DIR_IN)) {
                    l = s->setup_len - s->setup_index;
                    if (l > len)
                        l = len;
                    memcpy(s->data_buf + s->setup_index, data, l);
                    s->setup_index += l;
                    if (s->setup_index >= s->setup_len)
                        s->setup_state = SETUP_STATE_ACK;
                    ret = l;
                } else {
                    s->setup_state = SETUP_STATE_IDLE;
                    goto fail;
                }
                break;
            default:
                goto fail;
            }
            break;
        default:
            ret = s->handle_data(s, pid, devep, data, len);
            break;
        }
        break;
    default:
    fail:
        ret = USB_RET_STALL;
        break;
    }
    return ret;
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
