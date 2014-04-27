/*
 * QEMU USB OHCI Emulation
 * Copyright (c) 2004 Gianni Tedesco
 * Copyright (c) 2006 CodeSourcery
 * Copyright (c) 2006 Openedhand Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * TODO:
 *  o Isochronous transfers
 *  o Allocate bandwidth in frames properly
 *  o Disable timers when nothing needs to be done, or remove timer usage
 *    all together.
 *  o Handle unrecoverable errors properly
 *  o BIOS work to boot from USB storage
*/

//typedef CPUReadMemoryFunc

#include "USBinternal.h"

s64 last_cycle = 0;
#define MIN_IRQ_INTERVAL 64 /* hack */

extern FILE* usbLog;

int dprintf(const char *fmt,...)
{
#ifdef DEBUG_OHCI
	int t;
	va_list list;

	va_start(list, fmt);
	t=vfprintf(stderr, fmt, list);
	va_end(list);

	if(usbLog)
	{
		va_start(list, fmt);
		vfprintf(usbLog, fmt, list);
		va_end(list);
	}

	return t;
#else
	return 0;
#endif
}

static int last_level = 0;

/* Update IRQ levels */
static inline void ohci_intr_update(OHCIState *ohci)
{
    int level = 0;

    if ((ohci->intr & OHCI_INTR_MIE) &&
        (ohci->intr_status & ohci->intr))
        level = 1;

	if(level && !last_level)
	{
		if( (get_clock() - last_cycle) > MIN_IRQ_INTERVAL)
		{
			USBirq(1);
			last_cycle = get_clock();
		}
	}
}

/* Set an interrupt */
static inline void ohci_set_interrupt(OHCIState *ohci, uint32_t intr)
{
    ohci->intr_status |= intr;
    ohci_intr_update(ohci);
}

/* Attach or detach a device on a root hub port.  */
static void ohci_attach(USBPort *port1)
{
    OHCIState *s = (OHCIState*)port1->opaque;
    OHCIPort *port = &s->rhport[port1->index];
    uint32_t old_state = port->ctrl;

    /* set connect status */
    port->ctrl |= OHCI_PORT_CCS | OHCI_PORT_CSC;

    /* update speed */
    if (port->port.dev->speed == USB_SPEED_LOW) {
        port->ctrl |= OHCI_PORT_LSDA;
    } else {
        port->ctrl &= ~OHCI_PORT_LSDA;
    }

    /* notify of remote-wakeup */
    if ((s->ctl & OHCI_CTL_HCFS) == OHCI_USB_SUSPEND) {
        ohci_set_interrupt(s, OHCI_INTR_RD);
    }

    dprintf("usb-ohci: Attached port %d\n", port1->index);

    if (old_state != port->ctrl) {
        ohci_set_interrupt(s, OHCI_INTR_RHSC);
    }
}

static void ohci_async_cancel_device(OHCIState *ohci, USBDevice *dev)
{
    if (ohci->async_td && ohci->usb_packet.owner == dev) {
        usb_cancel_packet(&ohci->usb_packet);
        ohci->async_td = 0;
    }
}

static void ohci_detach(USBPort *port1)
{
    OHCIState *s = (OHCIState*)port1->opaque;
    OHCIPort *port = &s->rhport[port1->index];
    uint32_t old_state = port->ctrl;

    ohci_async_cancel_device(s, port1->dev);

    /* set connect status */
    if (port->ctrl & OHCI_PORT_CCS) {
        port->ctrl &= ~OHCI_PORT_CCS;
        port->ctrl |= OHCI_PORT_CSC;
    }
    /* disable port */
    if (port->ctrl & OHCI_PORT_PES) {
        port->ctrl &= ~OHCI_PORT_PES;
        port->ctrl |= OHCI_PORT_PESC;
    }
    dprintf("usb-ohci: Detached port %d\n", port1->index);

    if (old_state != port->ctrl) {
        ohci_set_interrupt(s, OHCI_INTR_RHSC);
    }
}

static void ohci_wakeup(USBPort *port1)
{
    OHCIState *s = (OHCIState*)port1->opaque;
    OHCIPort *port = &s->rhport[port1->index];
    uint32_t intr = 0;
    if (port->ctrl & OHCI_PORT_PSS) {
        dprintf("usb-ohci: port %d: wakeup\n", port1->index);
        port->ctrl |= OHCI_PORT_PSSC;
        port->ctrl &= ~OHCI_PORT_PSS;
        intr = OHCI_INTR_RHSC;
    }
    /* Note that the controller can be suspended even if this port is not */
    if ((s->ctl & OHCI_CTL_HCFS) == OHCI_USB_SUSPEND) {
        dprintf("usb-ohci: remote-wakeup: SUSPEND->RESUME\n");
        /* This is the one state transition the controller can do by itself */
        s->ctl &= ~OHCI_CTL_HCFS;
        s->ctl |= OHCI_USB_RESUME;
        /* In suspend mode only ResumeDetected is possible, not RHSC:
         * see the OHCI spec 5.1.2.3.
         */
        intr = OHCI_INTR_RD;
    }
    ohci_set_interrupt(s, intr);
}

static void ohci_child_detach(USBPort *port1, USBDevice *child)
{
    OHCIState *s = (OHCIState*)port1->opaque;

    ohci_async_cancel_device(s, child);
}

/* Reset the controller */
static void ohci_reset(void *opaque)
{
    OHCIState *ohci = (OHCIState *)opaque;
    OHCIPort *port;
    int i;

    ohci_bus_stop(ohci);
    ohci->ctl = 0;
    ohci->old_ctl = 0;
    ohci->status = 0;
    ohci->intr_status = 0;
    ohci->intr = OHCI_INTR_MIE;

    ohci->hcca = 0;
    ohci->ctrl_head = ohci->ctrl_cur = 0;
    ohci->bulk_head = ohci->bulk_cur = 0;
    ohci->per_cur = 0;
    ohci->done = 0;
    ohci->done_count = 7;

    /* FSMPS is marked TBD in OCHI 1.0, what gives ffs?
     * I took the value linux sets ...
     */
    ohci->fsmps = 0x2778;
    ohci->fi = 0x2edf;
    ohci->fit = 0;
    ohci->frt = 0;
    ohci->frame_number = 0;
    ohci->pstart = 0;
    ohci->lst = OHCI_LS_THRESH;

    ohci->rhdesc_a = OHCI_RHA_NPS | ohci->num_ports;
    ohci->rhdesc_b = 0x0; /* Impl. specific */
    ohci->rhstatus = 0;

    for (i = 0; i < ohci->num_ports; i++)
      {
        port = &ohci->rhport[i];
        port->ctrl = 0;
        if (port->port.dev) {
            usb_attach(&port->port, port->port.dev);
        }
      }
    if (ohci->async_td) {
        usb_cancel_packet(&ohci->usb_packet);
        ohci->async_td = 0;
    }
    dprintf("usb-ohci: Reset %s\n", ohci->name);
}

/* Get an array of dwords from main memory */
static inline int get_dwords(OHCIState *ohci,
                             uint32_t addr, uint32_t *buf, int num)
{
    int i;

    addr += ohci->localmem_base;

    for (i = 0; i < num; i++, buf++, addr += sizeof(*buf)) {
        cpu_physical_memory_read(addr, (uint8_t*)buf, sizeof(*buf));
        *buf = (*buf);
    }

    return 1;
}

/* Put an array of dwords in to main memory */
static inline int put_dwords(OHCIState *ohci,
                             uint32_t addr, uint32_t *buf, int num)
{
    int i;

    addr += ohci->localmem_base;

    for (i = 0; i < num; i++, buf++, addr += sizeof(*buf)) {
        uint32_t tmp = (*buf);
        cpu_physical_memory_write(addr, (uint8_t*)&tmp, sizeof(tmp));
    }

    return 1;
}

/* Get an array of words from main memory */
static inline int get_words(OHCIState *ohci,
                            uint32_t addr, uint16_t *buf, int num)
{
    int i;

    addr += ohci->localmem_base;

    for (i = 0; i < num; i++, buf++, addr += sizeof(*buf)) {
        cpu_physical_memory_read(addr, (uint8_t*)buf, sizeof(*buf));
        *buf = (*buf);
    }

    return 1;
}

/* Put an array of words in to main memory */
static inline int put_words(OHCIState *ohci,
                            uint32_t addr, uint16_t *buf, int num)
{
    int i;

    addr += ohci->localmem_base;

    for (i = 0; i < num; i++, buf++, addr += sizeof(*buf)) {
        uint16_t tmp = (*buf);
        cpu_physical_memory_write(addr, (uint8_t*)&tmp, sizeof(tmp));
    }

    return 1;
}

static inline int ohci_read_ed(OHCIState *ohci,
                               uint32_t addr, struct ohci_ed *ed)
{
    return get_dwords(ohci, addr, (uint32_t *)ed, sizeof(*ed) >> 2);
}

static inline int ohci_read_td(OHCIState *ohci,
                               uint32_t addr, struct ohci_td *td)
{
    return get_dwords(ohci, addr, (uint32_t *)td, sizeof(*td) >> 2);
}

static inline int ohci_read_iso_td(OHCIState *ohci,
                                   uint32_t addr, struct ohci_iso_td *td)
{
    return (get_dwords(ohci, addr, (uint32_t *)td, 4) &&
            get_words(ohci, addr + 16, td->offset, 8));
}

static inline int ohci_read_hcca(OHCIState *ohci,
                                 uint32_t addr, struct ohci_hcca *hcca)
{
    cpu_physical_memory_read(addr + ohci->localmem_base, (uint8_t*)hcca, sizeof(*hcca));
    return 1;
}

static inline int ohci_put_ed(OHCIState *ohci,
                              uint32_t addr, struct ohci_ed *ed)
{
    return put_dwords(ohci, addr, (uint32_t *)ed, sizeof(*ed) >> 2);
}

static inline int ohci_put_td(OHCIState *ohci,
                              uint32_t addr, struct ohci_td *td)
{
    return put_dwords(ohci, addr, (uint32_t *)td, sizeof(*td) >> 2);
}

static inline int ohci_put_iso_td(OHCIState *ohci,
                                  uint32_t addr, struct ohci_iso_td *td)
{
    return (put_dwords(ohci, addr, (uint32_t *)td, 4) &&
            put_words(ohci, addr + 16, td->offset, 8));
}

static inline int ohci_put_hcca(OHCIState *ohci,
                                uint32_t addr, struct ohci_hcca *hcca)
{
    cpu_physical_memory_write(addr + ohci->localmem_base, (uint8_t*)hcca, sizeof(*hcca));
    return 1;
}

/* Read/Write the contents of a TD from/to main memory.  */
static void ohci_copy_td(OHCIState *ohci, struct ohci_td *td,
                         uint8_t *buf, int len, int write)
{
    uint32_t ptr;
    uint32_t n;

    ptr = td->cbp;
    n = 0x1000 - (ptr & 0xfff);
    if (n > len)
        n = len;
    cpu_physical_memory_rw(ptr + ohci->localmem_base, buf, n, write);
    if (n == len)
        return;
    ptr = td->be & ~0xfffu;
    buf += n;
    cpu_physical_memory_rw(ptr + ohci->localmem_base, buf, len - n, write);
}

/* Read/Write the contents of an ISO TD from/to main memory.  */
static void ohci_copy_iso_td(OHCIState *ohci,
                             uint32_t start_addr, uint32_t end_addr,
                             uint8_t *buf, int len, int write)
{
    uint32_t ptr;
    uint32_t n;

    ptr = start_addr;
    n = 0x1000 - (ptr & 0xfff);
    if (n > len)
        n = len;
    cpu_physical_memory_rw(ptr + ohci->localmem_base, buf, n, write);
    if (n == len)
        return;
    ptr = end_addr & ~0xfffu;
    buf += n;
    cpu_physical_memory_rw(ptr + ohci->localmem_base, buf, len - n, write);
}

static void ohci_process_lists(OHCIState *ohci, int completion);

static void ohci_async_complete_packet(USBPort *port, USBPacket *packet)
{
	OHCIState *ohci = (OHCIState*)packet->owner->opaque;
#ifdef DEBUG_PACKET
    dprintf("Async packet complete\n");
#endif
    ohci->async_complete = 1;
    ohci_process_lists(ohci, 1);
}

#define USUB(a, b) ((int16_t)((uint16_t)(a) - (uint16_t)(b)))

static int ohci_service_iso_td(OHCIState *ohci, struct ohci_ed *ed,
                               int completion)
{
    int dir;
    size_t len = 0;
#ifdef DEBUG_ISOCH
    const char *str = NULL;
#endif
    int pid;
    int ret;
    int i;
    USBDevice *dev;
    struct ohci_iso_td iso_td;
    uint32_t addr;
    uint16_t starting_frame;
    int16_t relative_frame_number;
    int frame_count;
    uint32_t start_offset, next_offset, end_offset = 0;
    uint32_t start_addr, end_addr;

    addr = ed->head & OHCI_DPTR_MASK;

    if (!ohci_read_iso_td(ohci, addr, &iso_td)) {
        printf("usb-ohci: ISO_TD read error at %x\n", addr);
        return 0;
    }

    starting_frame = OHCI_BM(iso_td.flags, TD_SF);
    frame_count = OHCI_BM(iso_td.flags, TD_FC);
    relative_frame_number = USUB(ohci->frame_number, starting_frame); 

#ifdef DEBUG_ISOCH
    printf("--- ISO_TD ED head 0x%.8x tailp 0x%.8x\n"
           "0x%.8x 0x%.8x 0x%.8x 0x%.8x\n"
           "0x%.8x 0x%.8x 0x%.8x 0x%.8x\n"
           "0x%.8x 0x%.8x 0x%.8x 0x%.8x\n"
           "frame_number 0x%.8x starting_frame 0x%.8x\n"
           "frame_count  0x%.8x relative %d\n"
           "di 0x%.8x cc 0x%.8x\n",
           ed->head & OHCI_DPTR_MASK, ed->tail & OHCI_DPTR_MASK,
           iso_td.flags, iso_td.bp, iso_td.next, iso_td.be,
           iso_td.offset[0], iso_td.offset[1], iso_td.offset[2], iso_td.offset[3],
           iso_td.offset[4], iso_td.offset[5], iso_td.offset[6], iso_td.offset[7],
           ohci->frame_number, starting_frame, 
           frame_count, relative_frame_number,         
           OHCI_BM(iso_td.flags, TD_DI), OHCI_BM(iso_td.flags, TD_CC));
#endif

    if (relative_frame_number < 0) {
        dprintf("usb-ohci: ISO_TD R=%d < 0\n", relative_frame_number);
        return 1;
    } else if (relative_frame_number > frame_count) {
        /* ISO TD expired - retire the TD to the Done Queue and continue with
           the next ISO TD of the same ED */
        dprintf("usb-ohci: ISO_TD R=%d > FC=%d\n", relative_frame_number, 
               frame_count);
        OHCI_SET_BM(iso_td.flags, TD_CC, OHCI_CC_DATAOVERRUN);
        ed->head &= ~OHCI_DPTR_MASK;
        ed->head |= (iso_td.next & OHCI_DPTR_MASK);
        iso_td.next = ohci->done;
        ohci->done = addr;
        i = OHCI_BM(iso_td.flags, TD_DI);
        if (i < ohci->done_count)
            ohci->done_count = i;
        ohci_put_iso_td(ohci, addr, &iso_td);
        return 0;
    }

    dir = OHCI_BM(ed->flags, ED_D);
    switch (dir) {
    case OHCI_TD_DIR_IN:
#ifdef DEBUG_ISOCH
        str = "in";
#endif
        pid = USB_TOKEN_IN;
        break;
    case OHCI_TD_DIR_OUT:
#ifdef DEBUG_ISOCH
        str = "out";
#endif
        pid = USB_TOKEN_OUT;
        break;
    case OHCI_TD_DIR_SETUP:
#ifdef DEBUG_ISOCH
        str = "setup";
#endif
        pid = USB_TOKEN_SETUP;
        break;
    default:
        printf("usb-ohci: Bad direction %d\n", dir);
        return 1;
    }

    if (!iso_td.bp || !iso_td.be) {
        printf("usb-ohci: ISO_TD bp 0x%.8x be 0x%.8x\n", iso_td.bp, iso_td.be);
        return 1;
    }

    start_offset = iso_td.offset[relative_frame_number];
    next_offset = iso_td.offset[relative_frame_number + 1];

    if (!(OHCI_BM(start_offset, TD_PSW_CC) & 0xe) || 
        ((relative_frame_number < frame_count) && 
         !(OHCI_BM(next_offset, TD_PSW_CC) & 0xe))) {
        printf("usb-ohci: ISO_TD cc != not accessed 0x%.8x 0x%.8x\n",
               start_offset, next_offset);
        return 1;
    }

    if ((relative_frame_number < frame_count) && (start_offset > next_offset)) {
        printf("usb-ohci: ISO_TD start_offset=0x%.8x > next_offset=0x%.8x\n",
                start_offset, next_offset);
        return 1;
    }

    if ((start_offset & 0x1000) == 0) {
        start_addr = (iso_td.bp & OHCI_PAGE_MASK) |
            (start_offset & OHCI_OFFSET_MASK);
    } else {
        start_addr = (iso_td.be & OHCI_PAGE_MASK) |
            (start_offset & OHCI_OFFSET_MASK);
    }

    if (relative_frame_number < frame_count) {
        end_offset = next_offset - 1;
        if ((end_offset & 0x1000) == 0) {
            end_addr = (iso_td.bp & OHCI_PAGE_MASK) |
                (end_offset & OHCI_OFFSET_MASK);
        } else {
            end_addr = (iso_td.be & OHCI_PAGE_MASK) |
                (end_offset & OHCI_OFFSET_MASK);
        }
    } else {
        /* Last packet in the ISO TD */
        end_addr = iso_td.be;
    }

    if ((start_addr & OHCI_PAGE_MASK) != (end_addr & OHCI_PAGE_MASK)) {
        len = (end_addr & OHCI_OFFSET_MASK) + 0x1001
            - (start_addr & OHCI_OFFSET_MASK);
    } else {
        len = end_addr - start_addr + 1;
    }

    if (len && dir != OHCI_TD_DIR_IN) {
        ohci_copy_iso_td(ohci, start_addr, end_addr, ohci->usb_buf, len, 0);
    }

    if (completion) {
        ret = ohci->usb_packet.len;
    } else {
        ret = USB_RET_NODEV;
        for (i = 0; i < ohci->num_ports; i++) {
            dev = ohci->rhport[i].port.dev;
            if ((ohci->rhport[i].ctrl & OHCI_PORT_PES) == 0)
                continue;
            ohci->usb_packet.pid = pid;
            ohci->usb_packet.devaddr = OHCI_BM(ed->flags, ED_FA);
            ohci->usb_packet.devep = OHCI_BM(ed->flags, ED_EN);
            ohci->usb_packet.data = ohci->usb_buf;
            ohci->usb_packet.len = len;
            ret = usb_handle_packet(dev, &ohci->usb_packet);
            if (ret != USB_RET_NODEV)
                break;
        }
    
        if (ret == USB_RET_ASYNC) {
            return 1;
        }
    }

#ifdef DEBUG_ISOCH
    printf("so 0x%.8x eo 0x%.8x\nsa 0x%.8x ea 0x%.8x\ndir %s len %zu ret %d\n",
           start_offset, end_offset, start_addr, end_addr, str, len, ret);
#endif

    /* Writeback */
    if (dir == OHCI_TD_DIR_IN && ret >= 0 && ret <= len) {
        /* IN transfer succeeded */
        ohci_copy_iso_td(ohci, start_addr, end_addr, ohci->usb_buf, ret, 1);
        OHCI_SET_BM(iso_td.offset[relative_frame_number], TD_PSW_CC,
                    OHCI_CC_NOERROR);
        OHCI_SET_BM(iso_td.offset[relative_frame_number], TD_PSW_SIZE, ret);
    } else if (dir == OHCI_TD_DIR_OUT && ret == len) {
        /* OUT transfer succeeded */
        OHCI_SET_BM(iso_td.offset[relative_frame_number], TD_PSW_CC,
                    OHCI_CC_NOERROR);
        OHCI_SET_BM(iso_td.offset[relative_frame_number], TD_PSW_SIZE, 0);
    } else {
        if (ret > (signed) len) {
            printf("usb-ohci: DataOverrun %d > %zu\n", ret, len);
            OHCI_SET_BM(iso_td.offset[relative_frame_number], TD_PSW_CC,
                        OHCI_CC_DATAOVERRUN);
            OHCI_SET_BM(iso_td.offset[relative_frame_number], TD_PSW_SIZE,
                        len);
        } else if (ret >= 0) {
            printf("usb-ohci: DataUnderrun %d\n", ret);
            OHCI_SET_BM(iso_td.offset[relative_frame_number], TD_PSW_CC,
                        OHCI_CC_DATAUNDERRUN);
        } else {
            switch (ret) {
            case USB_RET_NODEV:
                OHCI_SET_BM(iso_td.offset[relative_frame_number], TD_PSW_CC,
                            OHCI_CC_DEVICENOTRESPONDING);
                OHCI_SET_BM(iso_td.offset[relative_frame_number], TD_PSW_SIZE,
                            0);
                break;
            case USB_RET_NAK:
            case USB_RET_STALL:
                printf("usb-ohci: got NAK/STALL %d\n", ret);
                OHCI_SET_BM(iso_td.offset[relative_frame_number], TD_PSW_CC,
                            OHCI_CC_STALL);
                OHCI_SET_BM(iso_td.offset[relative_frame_number], TD_PSW_SIZE,
                            0);
                break;
            default:
                printf("usb-ohci: Bad device response %d\n", ret);
                OHCI_SET_BM(iso_td.offset[relative_frame_number], TD_PSW_CC,
                            OHCI_CC_UNDEXPETEDPID);
                break;
            }
        }
    }

    if (relative_frame_number == frame_count) {
        /* Last data packet of ISO TD - retire the TD to the Done Queue */
        OHCI_SET_BM(iso_td.flags, TD_CC, OHCI_CC_NOERROR);
        ed->head &= ~OHCI_DPTR_MASK;
        ed->head |= (iso_td.next & OHCI_DPTR_MASK);
        iso_td.next = ohci->done;
        ohci->done = addr;
        i = OHCI_BM(iso_td.flags, TD_DI);
        if (i < ohci->done_count)
            ohci->done_count = i;
    }
    ohci_put_iso_td(ohci, addr, &iso_td);
    return 1;
}

/* Service a transport descriptor.
   Returns nonzero to terminate processing of this endpoint.  */

static int ohci_service_td(OHCIState *ohci, struct ohci_ed *ed)
{
    int dir;
    size_t len = 0;
#ifdef DEBUG_PACKET
    const char *str = NULL;
#endif
    int pid;
    int ret;
    int i;
    USBDevice *dev;
    struct ohci_td td;
    uint32_t addr;
    int flag_r;
    int completion;

    addr = ed->head & OHCI_DPTR_MASK;
    /* See if this TD has already been submitted to the device.  */
    completion = (addr == ohci->async_td);
    if (completion && !ohci->async_complete) {
#ifdef DEBUG_PACKET
        dprintf("Skipping async TD\n");
#endif
        return 1;
    }
    if (!ohci_read_td(ohci, addr, &td)) {
        fprintf(stderr, "usb-ohci: TD read error at %x\n", addr);
        return 0;
    }

    dir = OHCI_BM(ed->flags, ED_D);
    switch (dir) {
    case OHCI_TD_DIR_OUT:
    case OHCI_TD_DIR_IN:
        /* Same value.  */
        break;
    default:
        dir = OHCI_BM(td.flags, TD_DP);
        break;
    }

    switch (dir) {
    case OHCI_TD_DIR_IN:
#ifdef DEBUG_PACKET
        str = "in";
#endif
        pid = USB_TOKEN_IN;
        break;
    case OHCI_TD_DIR_OUT:
#ifdef DEBUG_PACKET
        str = "out";
#endif
        pid = USB_TOKEN_OUT;
        break;
    case OHCI_TD_DIR_SETUP:
#ifdef DEBUG_PACKET
        str = "setup";
#endif
        pid = USB_TOKEN_SETUP;
        break;
    default:
        fprintf(stderr, "usb-ohci: Bad direction\n");
        return 1;
    }
    if (td.cbp && td.be) {
        if ((td.cbp & 0xfffff000) != (td.be & 0xfffff000)) {
            len = (td.be & 0xfff) + 0x1001 - (td.cbp & 0xfff);
        } else {
            len = (td.be - td.cbp) + 1;
        }

        if (len && dir != OHCI_TD_DIR_IN && !completion) {
            ohci_copy_td(ohci, &td, ohci->usb_buf, len, 0);
        }
    }

    flag_r = (td.flags & OHCI_TD_R) != 0;
#ifdef DEBUG_PACKET
    dprintf(" TD @ 0x%.8x %" PRId64 " bytes %s r=%d cbp=0x%.8x be=0x%.8x\n",
            addr, (int64_t)len, str, flag_r, td.cbp, td.be);

    if (len > 0 && dir != OHCI_TD_DIR_IN) {
        dprintf("  data:");
        for (i = 0; i < len; i++)
            printf(" %.2x", ohci->usb_buf[i]);
        dprintf("\n");
    }
#endif
    if (completion) {
        ret = ohci->usb_packet.len;
        ohci->async_td = 0;
        ohci->async_complete = 0;
    } else {
        ret = USB_RET_NODEV;
        for (i = 0; i < ohci->num_ports; i++) {
            dev = ohci->rhport[i].port.dev;
            if ((ohci->rhport[i].ctrl & OHCI_PORT_PES) == 0)
                continue;

            if (ohci->async_td) {
                /* ??? The hardware should allow one active packet per
                   endpoint.  We only allow one active packet per controller.
                   This should be sufficient as long as devices respond in a
                   timely manner.
                 */
#ifdef DEBUG_PACKET
                dprintf("Too many pending packets\n");
#endif
                return 1;
            }
            ohci->usb_packet.pid = pid;
            ohci->usb_packet.devaddr = OHCI_BM(ed->flags, ED_FA);
            ohci->usb_packet.devep = OHCI_BM(ed->flags, ED_EN);
            ohci->usb_packet.data = ohci->usb_buf;
            ohci->usb_packet.len = len;
            ret = usb_handle_packet(dev, &ohci->usb_packet);
            if (ret != USB_RET_NODEV)
                break;
        }
#ifdef DEBUG_PACKET
        dprintf("ret=%d\n", ret);
#endif
        if (ret == USB_RET_ASYNC) {
            ohci->async_td = addr;
            return 1;
        }
    }
    if (ret >= 0) {
        if (dir == OHCI_TD_DIR_IN) {
            ohci_copy_td(ohci, &td, ohci->usb_buf, ret, 1);
#ifdef DEBUG_PACKET
            dprintf("  data:");
            for (i = 0; i < ret; i++)
                printf(" %.2x", ohci->usb_buf[i]);
            dprintf("\n");
#endif
        } else {
            ret = len;
        }
    }

    /* Writeback */
    if (ret == len || (dir == OHCI_TD_DIR_IN && ret >= 0 && flag_r)) {
        /* Transmission succeeded.  */
        if (ret == len) {
            td.cbp = 0;
        } else {
            td.cbp += ret;
            if ((td.cbp & 0xfff) + ret > 0xfff) {
                td.cbp &= 0xfff;
                td.cbp |= td.be & ~0xfff;
            }
        }
        td.flags |= OHCI_TD_T1;
        td.flags ^= OHCI_TD_T0;
        OHCI_SET_BM(td.flags, TD_CC, OHCI_CC_NOERROR);
        OHCI_SET_BM(td.flags, TD_EC, 0);

        ed->head &= ~OHCI_ED_C;
        if (td.flags & OHCI_TD_T0)
            ed->head |= OHCI_ED_C;
    } else {
        if (ret >= 0) {
            dprintf("usb-ohci: Underrun\n");
            OHCI_SET_BM(td.flags, TD_CC, OHCI_CC_DATAUNDERRUN);
        } else {
            switch (ret) {
            case USB_RET_NODEV:
                OHCI_SET_BM(td.flags, TD_CC, OHCI_CC_DEVICENOTRESPONDING);
            case USB_RET_NAK:
                dprintf("usb-ohci: got NAK\n");
                return 1;
            case USB_RET_STALL:
                dprintf("usb-ohci: got STALL\n");
                OHCI_SET_BM(td.flags, TD_CC, OHCI_CC_STALL);
                break;
            case USB_RET_BABBLE:
                dprintf("usb-ohci: got BABBLE\n");
                OHCI_SET_BM(td.flags, TD_CC, OHCI_CC_DATAOVERRUN);
                break;
            default:
                fprintf(stderr, "usb-ohci: Bad device response %d\n", ret);
                OHCI_SET_BM(td.flags, TD_CC, OHCI_CC_UNDEXPETEDPID);
                OHCI_SET_BM(td.flags, TD_EC, 3);
                break;
            }
        }
        ed->head |= OHCI_ED_H;
    }

    /* Retire this TD */
    ed->head &= ~OHCI_DPTR_MASK;
    ed->head |= td.next & OHCI_DPTR_MASK;
    td.next = ohci->done;
    ohci->done = addr;
    i = OHCI_BM(td.flags, TD_DI);
    if (i < ohci->done_count)
        ohci->done_count = i;
    ohci_put_td(ohci, addr, &td);
    return OHCI_BM(td.flags, TD_CC) != OHCI_CC_NOERROR;
}

/* Service an endpoint list.  Returns nonzero if active TD were found.  */
static int ohci_service_ed_list(OHCIState *ohci, uint32_t head, int completion)
{
    struct ohci_ed ed;
    uint32_t next_ed;
    uint32_t cur;
    int active;

    active = 0;

    if (head == 0)
        return 0;

    for (cur = head; cur; cur = next_ed) {
        if (!ohci_read_ed(ohci, cur, &ed)) {
            fprintf(stderr, "usb-ohci: ED read error at %x\n", cur);
            return 0;
        }

        next_ed = ed.next & OHCI_DPTR_MASK;

        if ((ed.head & OHCI_ED_H) || (ed.flags & OHCI_ED_K)) {
            uint32_t addr;
            /* Cancel pending packets for ED that have been paused.  */
            addr = ed.head & OHCI_DPTR_MASK;
            if (ohci->async_td && addr == ohci->async_td) {
                usb_cancel_packet(&ohci->usb_packet);
                ohci->async_td = 0;
            }
            continue;
        }

        while ((ed.head & OHCI_DPTR_MASK) != ed.tail) {
#ifdef DEBUG_PACKET
            dprintf("ED @ 0x%.8x fa=%u en=%u d=%u s=%u k=%u f=%u mps=%u "
                    "h=%u c=%u\n  head=0x%.8x tailp=0x%.8x next=0x%.8x\n", cur,
                    OHCI_BM(ed.flags, ED_FA), OHCI_BM(ed.flags, ED_EN),
                    OHCI_BM(ed.flags, ED_D), (ed.flags & OHCI_ED_S)!= 0,
                    (ed.flags & OHCI_ED_K) != 0, (ed.flags & OHCI_ED_F) != 0,
                    OHCI_BM(ed.flags, ED_MPS), (ed.head & OHCI_ED_H) != 0,
                    (ed.head & OHCI_ED_C) != 0, ed.head & OHCI_DPTR_MASK,
                    ed.tail & OHCI_DPTR_MASK, ed.next & OHCI_DPTR_MASK);
#endif
            active = 1;

            if ((ed.flags & OHCI_ED_F) == 0) {
                if (ohci_service_td(ohci, &ed))
                    break;
            } else {
                /* Handle isochronous endpoints */
                if (ohci_service_iso_td(ohci, &ed, completion))
                    break;
            }
        }

        ohci_put_ed(ohci, cur, &ed);
    }

    return active;
}

/* Generate a SOF event, and set a timer for EOF */
static void ohci_sof(OHCIState *ohci)
{
    ohci->sof_time = get_clock();
    ohci->eof_timer += usb_frame_time;
    //ohci->sof_time = qemu_get_clock_ns(vm_clock);
    //qemu_mod_timer(ohci->eof_timer, ohci->sof_time + usb_frame_time);
    ohci_set_interrupt(ohci, OHCI_INTR_SF);
}

/* Process Control and Bulk lists.  */
static void ohci_process_lists(OHCIState *ohci, int completion)
{
    if ((ohci->ctl & OHCI_CTL_CLE) && (ohci->status & OHCI_STATUS_CLF)) {
        if (ohci->ctrl_cur && ohci->ctrl_cur != ohci->ctrl_head) {
            dprintf("usb-ohci: head %x, cur %x\n",
                    ohci->ctrl_head, ohci->ctrl_cur);
        }
        if (!ohci_service_ed_list(ohci, ohci->ctrl_head, completion)) {
            ohci->ctrl_cur = 0;
            ohci->status &= ~OHCI_STATUS_CLF;
        }
    }

    if ((ohci->ctl & OHCI_CTL_BLE) && (ohci->status & OHCI_STATUS_BLF)) {
        if (!ohci_service_ed_list(ohci, ohci->bulk_head, completion)) {
            ohci->bulk_cur = 0;
            ohci->status &= ~OHCI_STATUS_BLF;
        }
    }
}

/* Do frame processing on frame boundary */
void ohci_frame_boundary(void *opaque)
{
    OHCIState *ohci = (OHCIState*)opaque;
    struct ohci_hcca hcca;

    ohci_read_hcca(ohci, ohci->hcca, &hcca);

    /* Process all the lists at the end of the frame */
    if (ohci->ctl & OHCI_CTL_PLE) {
        int n;

        n = ohci->frame_number & 0x1f;
        ohci_service_ed_list(ohci, (hcca.intr[n]), 0);
    }

    /* Cancel all pending packets if either of the lists has been disabled.  */
    if (ohci->async_td &&
        ohci->old_ctl & (~ohci->ctl) & (OHCI_CTL_BLE | OHCI_CTL_CLE)) {
        usb_cancel_packet(&ohci->usb_packet);
        ohci->async_td = 0;
    }
    ohci->old_ctl = ohci->ctl;
    ohci_process_lists(ohci, 0);

    /* Frame boundary, so do EOF stuf here */
    ohci->frt = ohci->fit;

    /* Increment frame number and take care of endianness. */
    ohci->frame_number = (ohci->frame_number + 1) & 0xffff;
    hcca.frame = (ohci->frame_number);

    if (ohci->done_count == 0 && !(ohci->intr_status & OHCI_INTR_WD)) {
        if (!ohci->done)
            abort();
        if (ohci->intr & ohci->intr_status)
            ohci->done |= 1;
        hcca.done = (ohci->done);
        ohci->done = 0;
        ohci->done_count = 7;
        ohci_set_interrupt(ohci, OHCI_INTR_WD);
    }

    if (ohci->done_count != 7 && ohci->done_count != 0)
        ohci->done_count--;

    /* Do SOF stuff here */
    ohci_sof(ohci);

    /* Writeback HCCA */
    ohci_put_hcca(ohci, ohci->hcca, &hcca);
}

/* Start sending SOF tokens across the USB bus, lists are processed in
 * next frame
 */
static int ohci_bus_start(OHCIState *ohci)
{
    ohci->eof_timer = 0;

    dprintf("usb-ohci: %s: USB Operational\n", ohci->name);

    ohci_sof(ohci);

    return 1;
}

/* Stop sending SOF tokens on the bus */
static void ohci_bus_stop(OHCIState *ohci)
{
    if (ohci->eof_timer)
        ohci->eof_timer=0;
}

/* Sets a flag in a port status register but only set it if the port is
 * connected, if not set ConnectStatusChange flag. If flag is enabled
 * return 1.
 */
static int ohci_port_set_if_connected(OHCIState *ohci, int i, uint32_t val)
{
    int ret = 1;

    /* writing a 0 has no effect */
    if (val == 0)
        return 0;

    /* If CurrentConnectStatus is cleared we set
     * ConnectStatusChange
     */
    if (!(ohci->rhport[i].ctrl & OHCI_PORT_CCS)) {
        ohci->rhport[i].ctrl |= OHCI_PORT_CSC;
        if (ohci->rhstatus & OHCI_RHS_DRWE) {
            /* TODO: CSC is a wakeup event */
        }
        return 0;
    }

    if (ohci->rhport[i].ctrl & val)
        ret = 0;

    /* set the bit */
    ohci->rhport[i].ctrl |= val;

    return ret;
}

/* Set the frame interval - frame interval toggle is manipulated by the hcd only */
static void ohci_set_frame_interval(OHCIState *ohci, uint16_t val)
{
    val &= OHCI_FMI_FI;

    if (val != ohci->fi) {
        dprintf("usb-ohci: %s: FrameInterval = 0x%x (%u)\n",
            ohci->name, ohci->fi, ohci->fi);
    }

    ohci->fi = val;
}

static void ohci_port_power(OHCIState *ohci, int i, int p)
{
    if (p) {
        ohci->rhport[i].ctrl |= OHCI_PORT_PPS;
    } else {
        ohci->rhport[i].ctrl &= ~(OHCI_PORT_PPS|
                    OHCI_PORT_CCS|
                    OHCI_PORT_PSS|
                    OHCI_PORT_PRS);
    }
}

/* Set HcControlRegister */
static void ohci_set_ctl(OHCIState *ohci, uint32_t val)
{
    uint32_t old_state;
    uint32_t new_state;

    old_state = ohci->ctl & OHCI_CTL_HCFS;
    ohci->ctl = val;
    new_state = ohci->ctl & OHCI_CTL_HCFS;

    /* no state change */
    if (old_state == new_state)
        return;

    switch (new_state) {
    case OHCI_USB_OPERATIONAL:
        ohci_bus_start(ohci);
        break;
    case OHCI_USB_SUSPEND:
        ohci_bus_stop(ohci);
        dprintf("usb-ohci: %s: USB Suspended\n", ohci->name);
        break;
    case OHCI_USB_RESUME:
        dprintf("usb-ohci: %s: USB Resume\n", ohci->name);
        break;
    case OHCI_USB_RESET:
        ohci_reset(ohci);
        dprintf("usb-ohci: %s: USB Reset\n", ohci->name);
        break;
    }
}

static uint32_t ohci_get_frame_remaining(OHCIState *ohci)
{
    uint16_t fr;
    int64_t tks;

    if ((ohci->ctl & OHCI_CTL_HCFS) != OHCI_USB_OPERATIONAL)
        return (ohci->frt << 31);

    /* Being in USB operational state guarnatees sof_time was
     * set already.
     */
    tks = get_clock() - ohci->sof_time;

    /* avoid muldiv if possible */
    if (tks >= usb_frame_time)
        return (ohci->frt << 31);

    tks = muldiv64(1, tks, usb_bit_time);
    fr = (uint16_t)(ohci->fi - tks);

    return (ohci->frt << 31) | fr;
}


/* Set root hub status */
static void ohci_set_hub_status(OHCIState *ohci, uint32_t val)
{
    uint32_t old_state;

    old_state = ohci->rhstatus;

    /* write 1 to clear OCIC */
    if (val & OHCI_RHS_OCIC)
        ohci->rhstatus &= ~OHCI_RHS_OCIC;

    if (val & OHCI_RHS_LPS) {
        int i;

        for (i = 0; i < ohci->num_ports; i++)
            ohci_port_power(ohci, i, 0);
        dprintf("usb-ohci: powered down all ports\n");
    }

    if (val & OHCI_RHS_LPSC) {
        int i;

        for (i = 0; i < ohci->num_ports; i++)
            ohci_port_power(ohci, i, 1);
        dprintf("usb-ohci: powered up all ports\n");
    }

    if (val & OHCI_RHS_DRWE)
        ohci->rhstatus |= OHCI_RHS_DRWE;

    if (val & OHCI_RHS_CRWE)
        ohci->rhstatus &= ~OHCI_RHS_DRWE;

    if (old_state != ohci->rhstatus)
        ohci_set_interrupt(ohci, OHCI_INTR_RHSC);
}

/* Set root hub port status */
static void ohci_port_set_status(OHCIState *ohci, int portnum, uint32_t val)
{
    uint32_t old_state;
    OHCIPort *port;

    port = &ohci->rhport[portnum];
    old_state = port->ctrl;

    /* Write to clear CSC, PESC, PSSC, OCIC, PRSC */
    if (val & OHCI_PORT_WTC)
        port->ctrl &= ~(val & OHCI_PORT_WTC);

    if (val & OHCI_PORT_CCS)
        port->ctrl &= ~OHCI_PORT_PES;

    ohci_port_set_if_connected(ohci, portnum, val & OHCI_PORT_PES);

    if (ohci_port_set_if_connected(ohci, portnum, val & OHCI_PORT_PSS)) {
        dprintf("usb-ohci: port %d: SUSPEND\n", portnum);
    }

    if (ohci_port_set_if_connected(ohci, portnum, val & OHCI_PORT_PRS)) {
        dprintf("usb-ohci: port %d: RESET\n", portnum);
        usb_send_msg(port->port.dev, USB_MSG_RESET);
        port->ctrl &= ~OHCI_PORT_PRS;
        /* ??? Should this also set OHCI_PORT_PESC.  */
        port->ctrl |= OHCI_PORT_PES | OHCI_PORT_PRSC;
    }

    /* Invert order here to ensure in ambiguous case, device is
     * powered up...
     */
    if (val & OHCI_PORT_LSDA)
        ohci_port_power(ohci, portnum, 0);
    if (val & OHCI_PORT_PPS)
        ohci_port_power(ohci, portnum, 1);

    if (old_state != port->ctrl)
        ohci_set_interrupt(ohci, OHCI_INTR_RHSC);

    return;
}

uint32_t ohci_mem_read(void *ptr, target_phys_addr_t addr)
{
    OHCIState *ohci = (OHCIState*)ptr;
    uint32_t retval;

    addr &= 0xff;

    /* Only aligned reads are allowed on OHCI */
    if (addr & 3) {
        fprintf(stderr, "usb-ohci: Mis-aligned read\n");
        return 0xffffffff;
    } else if (addr >= 0x54 && addr < 0x54 + ohci->num_ports * 4) {
        /* HcRhPortStatus */
        retval = ohci->rhport[(addr - 0x54) >> 2].ctrl | OHCI_PORT_PPS;
    } else {
        switch (addr >> 2) {
        case 0: /* HcRevision */
            retval = 0x10;
            break;

        case 1: /* HcControl */
            retval = ohci->ctl;
            break;

        case 2: /* HcCommandStatus */
            retval = ohci->status;
            break;

        case 3: /* HcInterruptStatus */
            retval = ohci->intr_status;
            break;

        case 4: /* HcInterruptEnable */
        case 5: /* HcInterruptDisable */
            retval = ohci->intr;
            break;

        case 6: /* HcHCCA */
            retval = ohci->hcca;
            break;

        case 7: /* HcPeriodCurrentED */
            retval = ohci->per_cur;
            break;

        case 8: /* HcControlHeadED */
            retval = ohci->ctrl_head;
            break;

        case 9: /* HcControlCurrentED */
            retval = ohci->ctrl_cur;
            break;

        case 10: /* HcBulkHeadED */
            retval = ohci->bulk_head;
            break;

        case 11: /* HcBulkCurrentED */
            retval = ohci->bulk_cur;
            break;

        case 12: /* HcDoneHead */
            retval = ohci->done;
            break;

        case 13: /* HcFmInterretval */
            retval = (ohci->fit << 31) | (ohci->fsmps << 16) | (ohci->fi);
            break;

        case 14: /* HcFmRemaining */
            retval = ohci_get_frame_remaining(ohci);
            break;

        case 15: /* HcFmNumber */
            retval = ohci->frame_number;
            break;

        case 16: /* HcPeriodicStart */
            retval = ohci->pstart;
            break;

        case 17: /* HcLSThreshold */
            retval = ohci->lst;
            break;

        case 18: /* HcRhDescriptorA */
            retval = ohci->rhdesc_a;
            break;

        case 19: /* HcRhDescriptorB */
            retval = ohci->rhdesc_b;
            break;

        case 20: /* HcRhStatus */
            retval = ohci->rhstatus;
            break;

        /* PXA27x specific registers */
        case 24: /* HcStatus */
            retval = ohci->hstatus & ohci->hmask;
            break;

        case 25: /* HcHReset */
            retval = ohci->hreset;
            break;

        case 26: /* HcHInterruptEnable */
            retval = ohci->hmask;
            break;

        case 27: /* HcHInterruptTest */
            retval = ohci->htest;
            break;

        default:
            fprintf(stderr, "ohci_read: Bad offset %x\n", (int)addr);
            retval = 0xffffffff;
        }
    }

    return retval;
}

void ohci_mem_write(void *ptr, target_phys_addr_t addr, uint32_t val)
{
    OHCIState *ohci = (OHCIState *)ptr;

    addr &= 0xff;

    /* Only aligned reads are allowed on OHCI */
    if (addr & 3) {
        fprintf(stderr, "usb-ohci: Mis-aligned write\n");
        return;
    }

    if (addr >= 0x54 && addr < 0x54 + ohci->num_ports * 4) {
        /* HcRhPortStatus */
        ohci_port_set_status(ohci, (addr - 0x54) >> 2, val);
        return;
    }

    switch (addr >> 2) {
    case 1: /* HcControl */
        ohci_set_ctl(ohci, val);
        break;

    case 2: /* HcCommandStatus */
        /* SOC is read-only */
        val = (val & ~OHCI_STATUS_SOC);

        /* Bits written as '0' remain unchanged in the register */
        ohci->status |= val;

        if (ohci->status & OHCI_STATUS_HCR)
            ohci_reset(ohci);
        break;

    case 3: /* HcInterruptStatus */
        ohci->intr_status &= ~val;
        ohci_intr_update(ohci);
        break;

    case 4: /* HcInterruptEnable */
        ohci->intr |= val;
        ohci_intr_update(ohci);
        break;

    case 5: /* HcInterruptDisable */
        ohci->intr &= ~val;
        ohci_intr_update(ohci);
        break;

    case 6: /* HcHCCA */
        ohci->hcca = val & OHCI_HCCA_MASK;
        break;

    case 7: /* HcPeriodCurrentED */
        /* Ignore writes to this read-only register, Linux does them */
        break;

    case 8: /* HcControlHeadED */
        ohci->ctrl_head = val & OHCI_EDPTR_MASK;
        break;

    case 9: /* HcControlCurrentED */
        ohci->ctrl_cur = val & OHCI_EDPTR_MASK;
        break;

    case 10: /* HcBulkHeadED */
        ohci->bulk_head = val & OHCI_EDPTR_MASK;
        break;

    case 11: /* HcBulkCurrentED */
        ohci->bulk_cur = val & OHCI_EDPTR_MASK;
        break;

    case 13: /* HcFmInterval */
        ohci->fsmps = (val & OHCI_FMI_FSMPS) >> 16;
        ohci->fit = (val & OHCI_FMI_FIT) >> 31;
        ohci_set_frame_interval(ohci, val);
        break;

    case 15: /* HcFmNumber */
        break;

    case 16: /* HcPeriodicStart */
        ohci->pstart = val & 0xffff;
        break;

    case 17: /* HcLSThreshold */
        ohci->lst = val & 0xffff;
        break;

    case 18: /* HcRhDescriptorA */
        ohci->rhdesc_a &= ~OHCI_RHA_RW_MASK;
        ohci->rhdesc_a |= val & OHCI_RHA_RW_MASK;
        break;

    case 19: /* HcRhDescriptorB */
        break;

    case 20: /* HcRhStatus */
        ohci_set_hub_status(ohci, val);
        break;

    /* PXA27x specific registers */
    case 24: /* HcStatus */
        ohci->hstatus &= ~(val & ohci->hmask);

    case 25: /* HcHReset */
        ohci->hreset = val & ~OHCI_HRESET_FSBIR;
        if (val & OHCI_HRESET_FSBIR)
            ohci_reset(ohci);
        break;

    case 26: /* HcHInterruptEnable */
        ohci->hmask = val;
        break;

    case 27: /* HcHInterruptTest */
        ohci->htest = val;
        break;

    default:
        fprintf(stderr, "ohci_write: Bad offset %x\n", (int)addr);
        break;
    }
}

static USBPortOps ohci_port_ops = {
    ohci_attach,
    ohci_detach,
    ohci_child_detach,
    ohci_wakeup,
    ohci_async_complete_packet,
};

static USBBusOps ohci_bus_ops = {
};

OHCIState *ohci_create(uint32_t base, int ports)
{
	OHCIState *ohci=(OHCIState*)malloc(sizeof(OHCIState));
    int i;

	memset(ohci,0,sizeof(OHCIState));

	//ohci->localmem_base=base;

	ohci->mem = base;
    //ohci->mem = cpu_register_io_memory(ohci_readfn, ohci_writefn, ohci);
    //ohci->localmem_base = localmem_base;
    ohci->name = "USBemu";

    //ohci->irq = irq;
    //ohci->type = type;
	

    if (usb_frame_time == 0) {
#ifdef OHCI_TIME_WARP
        usb_frame_time = get_ticks_per_sec();
        usb_bit_time = muldiv64(1, get_ticks_per_sec(), USB_HZ/1000);
#else
        usb_frame_time = muldiv64(1, get_ticks_per_sec(), 1000);
        if (get_ticks_per_sec() >= USB_HZ) {
            usb_bit_time = muldiv64(1, get_ticks_per_sec(), USB_HZ);
        } else {
            usb_bit_time = 1;
        }
#endif
        dprintf("usb-ohci: usb_bit_time=%" PRId64 " usb_frame_time=%" PRId64 "\n",
                usb_frame_time, usb_bit_time);
    }

    ohci->num_ports = ports;
    for (i = 0; i < ports; i++) {
		ohci->rhport[i].port.opaque = ohci;
		ohci->rhport[i].port.index = i;
		ohci->rhport[i].port.ops = &ohci_port_ops;
    }

    ohci->async_td = 0;

	return ohci;
}
