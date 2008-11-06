/*
 * QEMU IDE disk and CD-ROM Emulator
 * 
 * Copyright (c) 2003 Fabrice Bellard
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
#include "dev9.h"
#include "vl.h"

/* debug IDE devices */
//#define DEBUG_IDE
//#define DEBUG_IDE_ATAPI
//#define DEBUG_AIO

#define USE_DMA_CDROM
#define QEMU_VERSION " PCSX2"
#define snprintf _snprintf

#define le32_to_cpu(x) (x)
#define le16_to_cpu(x) (x)
#define cpu_to_le16(x) (x)
#define cpu_to_le32(x) (x)

/* Bits of HD_STATUS */
#define ERR_STAT		0x01
#define INDEX_STAT		0x02
#define ECC_STAT		0x04	/* Corrected error */
#define DRQ_STAT		0x08
#define SEEK_STAT		0x10
#define SRV_STAT		0x10
#define WRERR_STAT		0x20
#define READY_STAT		0x40
#define BUSY_STAT		0x80

/* Bits for HD_ERROR */
#define MARK_ERR		0x01	/* Bad address mark */
#define TRK0_ERR		0x02	/* couldn't find track 0 */
#define ABRT_ERR		0x04	/* Command aborted */
#define MCR_ERR			0x08	/* media change request */
#define ID_ERR			0x10	/* ID field not found */
#define MC_ERR			0x20	/* media changed */
#define ECC_ERR			0x40	/* Uncorrectable ECC error */
#define BBD_ERR			0x80	/* pre-EIDE meaning:  block marked bad */
#define ICRC_ERR		0x80	/* new meaning:  CRC error during transfer */

/* Bits of HD_NSECTOR */
#define CD			0x01
#define IO			0x02
#define REL			0x04
#define TAG_MASK		0xf8

#define IDE_CMD_RESET           0x04
#define IDE_CMD_DISABLE_IRQ     0x02

/* ATA/ATAPI Commands pre T13 Spec */
#define WIN_NOP				0x00
/*
 *	0x01->0x02 Reserved
 */
#define CFA_REQ_EXT_ERROR_CODE		0x03 /* CFA Request Extended Error Code */
/*
 *	0x04->0x07 Reserved
 */
#define WIN_SRST			0x08 /* ATAPI soft reset command */
#define WIN_DEVICE_RESET		0x08
/*
 *	0x09->0x0F Reserved
 */
#define WIN_RECAL			0x10
#define WIN_RESTORE			WIN_RECAL
/*
 *	0x10->0x1F Reserved
 */
#define WIN_READ			0x20 /* 28-Bit */
#define WIN_READ_ONCE			0x21 /* 28-Bit without retries */
#define WIN_READ_LONG			0x22 /* 28-Bit */
#define WIN_READ_LONG_ONCE		0x23 /* 28-Bit without retries */
#define WIN_READ_EXT			0x24 /* 48-Bit */
#define WIN_READDMA_EXT			0x25 /* 48-Bit */
#define WIN_READDMA_QUEUED_EXT		0x26 /* 48-Bit */
#define WIN_READ_NATIVE_MAX_EXT		0x27 /* 48-Bit */
/*
 *	0x28
 */
#define WIN_MULTREAD_EXT		0x29 /* 48-Bit */
/*
 *	0x2A->0x2F Reserved
 */
#define WIN_WRITE			0x30 /* 28-Bit */
#define WIN_WRITE_ONCE			0x31 /* 28-Bit without retries */
#define WIN_WRITE_LONG			0x32 /* 28-Bit */
#define WIN_WRITE_LONG_ONCE		0x33 /* 28-Bit without retries */
#define WIN_WRITE_EXT			0x34 /* 48-Bit */
#define WIN_WRITEDMA_EXT		0x35 /* 48-Bit */
#define WIN_WRITEDMA_QUEUED_EXT		0x36 /* 48-Bit */
#define WIN_SET_MAX_EXT			0x37 /* 48-Bit */
#define CFA_WRITE_SECT_WO_ERASE		0x38 /* CFA Write Sectors without erase */
#define WIN_MULTWRITE_EXT		0x39 /* 48-Bit */
/*
 *	0x3A->0x3B Reserved
 */
#define WIN_WRITE_VERIFY		0x3C /* 28-Bit */
/*
 *	0x3D->0x3F Reserved
 */
#define WIN_VERIFY			0x40 /* 28-Bit - Read Verify Sectors */
#define WIN_VERIFY_ONCE			0x41 /* 28-Bit - without retries */
#define WIN_VERIFY_EXT			0x42 /* 48-Bit */
/*
 *	0x43->0x4F Reserved
 */
#define WIN_FORMAT			0x50
/*
 *	0x51->0x5F Reserved
 */
#define WIN_INIT			0x60
/*
 *	0x61->0x5F Reserved
 */
#define WIN_SEEK			0x70 /* 0x70-0x7F Reserved */
#define CFA_TRANSLATE_SECTOR		0x87 /* CFA Translate Sector */
#define WIN_DIAGNOSE			0x90
#define WIN_SPECIFY			0x91 /* set drive geometry translation */
#define WIN_DOWNLOAD_MICROCODE		0x92
#define WIN_STANDBYNOW2			0x94
#define WIN_STANDBY2			0x96
#define WIN_SETIDLE2			0x97
#define WIN_CHECKPOWERMODE2		0x98
#define WIN_SLEEPNOW2			0x99
/*
 *	0x9A VENDOR
 */
#define WIN_PACKETCMD			0xA0 /* Send a packet command. */
#define WIN_PIDENTIFY			0xA1 /* identify ATAPI device	*/
#define WIN_QUEUED_SERVICE		0xA2
#define WIN_SMART			0xB0 /* self-monitoring and reporting */
#define CFA_ERASE_SECTORS       	0xC0
#define WIN_MULTREAD			0xC4 /* read sectors using multiple mode*/
#define WIN_MULTWRITE			0xC5 /* write sectors using multiple mode */
#define WIN_SETMULT			0xC6 /* enable/disable multiple mode */
#define WIN_READDMA_QUEUED		0xC7 /* read sectors using Queued DMA transfers */
#define WIN_READDMA			0xC8 /* read sectors using DMA transfers */
#define WIN_READDMA_ONCE		0xC9 /* 28-Bit - without retries */
#define WIN_WRITEDMA			0xCA /* write sectors using DMA transfers */
#define WIN_WRITEDMA_ONCE		0xCB /* 28-Bit - without retries */
#define WIN_WRITEDMA_QUEUED		0xCC /* write sectors using Queued DMA transfers */
#define CFA_WRITE_MULTI_WO_ERASE	0xCD /* CFA Write multiple without erase */
#define WIN_GETMEDIASTATUS		0xDA	
#define WIN_ACKMEDIACHANGE		0xDB /* ATA-1, ATA-2 vendor */
#define WIN_POSTBOOT			0xDC
#define WIN_PREBOOT			0xDD
#define WIN_DOORLOCK			0xDE /* lock door on removable drives */
#define WIN_DOORUNLOCK			0xDF /* unlock door on removable drives */
#define WIN_STANDBYNOW1			0xE0
#define WIN_IDLEIMMEDIATE		0xE1 /* force drive to become "ready" */
#define WIN_STANDBY             	0xE2 /* Set device in Standby Mode */
#define WIN_SETIDLE1			0xE3
#define WIN_READ_BUFFER			0xE4 /* force read only 1 sector */
#define WIN_CHECKPOWERMODE1		0xE5
#define WIN_SLEEPNOW1			0xE6
#define WIN_FLUSH_CACHE			0xE7
#define WIN_WRITE_BUFFER		0xE8 /* force write only 1 sector */
#define WIN_WRITE_SAME			0xE9 /* read ata-2 to use */
	/* SET_FEATURES 0x22 or 0xDD */
#define WIN_FLUSH_CACHE_EXT		0xEA /* 48-Bit */
#define WIN_IDENTIFY			0xEC /* ask drive to identify itself	*/
#define WIN_MEDIAEJECT			0xED
#define WIN_IDENTIFY_DMA		0xEE /* same as WIN_IDENTIFY, but DMA */
#define WIN_SETFEATURES			0xEF /* set special drive features */
#define EXABYTE_ENABLE_NEST		0xF0
#define WIN_SECURITY_SET_PASS		0xF1
#define WIN_SECURITY_UNLOCK		0xF2
#define WIN_SECURITY_ERASE_PREPARE	0xF3
#define WIN_SECURITY_ERASE_UNIT		0xF4
#define WIN_SECURITY_FREEZE_LOCK	0xF5
#define WIN_SECURITY_DISABLE		0xF6
#define WIN_READ_NATIVE_MAX		0xF8 /* return the native maximum address */
#define WIN_SET_MAX			0xF9
#define DISABLE_SEAGATE			0xFB

/* set to 1 set disable mult support */
#define MAX_MULT_SECTORS 16

/* ATAPI defines */

#define ATAPI_PACKET_SIZE 12

/* The generic packet command opcodes for CD/DVD Logical Units,
 * From Table 57 of the SFF8090 Ver. 3 (Mt. Fuji) draft standard. */
#define GPCMD_BLANK			    0xa1
#define GPCMD_CLOSE_TRACK		    0x5b
#define GPCMD_FLUSH_CACHE		    0x35
#define GPCMD_FORMAT_UNIT		    0x04
#define GPCMD_GET_CONFIGURATION		    0x46
#define GPCMD_GET_EVENT_STATUS_NOTIFICATION 0x4a
#define GPCMD_GET_PERFORMANCE		    0xac
#define GPCMD_INQUIRY			    0x12
#define GPCMD_LOAD_UNLOAD		    0xa6
#define GPCMD_MECHANISM_STATUS		    0xbd
#define GPCMD_MODE_SELECT_10		    0x55
#define GPCMD_MODE_SENSE_10		    0x5a
#define GPCMD_PAUSE_RESUME		    0x4b
#define GPCMD_PLAY_AUDIO_10		    0x45
#define GPCMD_PLAY_AUDIO_MSF		    0x47
#define GPCMD_PLAY_AUDIO_TI		    0x48
#define GPCMD_PLAY_CD			    0xbc
#define GPCMD_PREVENT_ALLOW_MEDIUM_REMOVAL  0x1e
#define GPCMD_READ_10			    0x28
#define GPCMD_READ_12			    0xa8
#define GPCMD_READ_CDVD_CAPACITY	    0x25
#define GPCMD_READ_CD			    0xbe
#define GPCMD_READ_CD_MSF		    0xb9
#define GPCMD_READ_DISC_INFO		    0x51
#define GPCMD_READ_DVD_STRUCTURE	    0xad
#define GPCMD_READ_FORMAT_CAPACITIES	    0x23
#define GPCMD_READ_HEADER		    0x44
#define GPCMD_READ_TRACK_RZONE_INFO	    0x52
#define GPCMD_READ_SUBCHANNEL		    0x42
#define GPCMD_READ_TOC_PMA_ATIP		    0x43
#define GPCMD_REPAIR_RZONE_TRACK	    0x58
#define GPCMD_REPORT_KEY		    0xa4
#define GPCMD_REQUEST_SENSE		    0x03
#define GPCMD_RESERVE_RZONE_TRACK	    0x53
#define GPCMD_SCAN			    0xba
#define GPCMD_SEEK			    0x2b
#define GPCMD_SEND_DVD_STRUCTURE	    0xad
#define GPCMD_SEND_EVENT		    0xa2
#define GPCMD_SEND_KEY			    0xa3
#define GPCMD_SEND_OPC			    0x54
#define GPCMD_SET_READ_AHEAD		    0xa7
#define GPCMD_SET_STREAMING		    0xb6
#define GPCMD_START_STOP_UNIT		    0x1b
#define GPCMD_STOP_PLAY_SCAN		    0x4e
#define GPCMD_TEST_UNIT_READY		    0x00
#define GPCMD_VERIFY_10			    0x2f
#define GPCMD_WRITE_10			    0x2a
#define GPCMD_WRITE_AND_VERIFY_10	    0x2e
/* This is listed as optional in ATAPI 2.6, but is (curiously) 
 * missing from Mt. Fuji, Table 57.  It _is_ mentioned in Mt. Fuji
 * Table 377 as an MMC command for SCSi devices though...  Most ATAPI
 * drives support it. */
#define GPCMD_SET_SPEED			    0xbb
/* This seems to be a SCSI specific CD-ROM opcode 
 * to play data at track/index */
#define GPCMD_PLAYAUDIO_TI		    0x48
/*
 * From MS Media Status Notification Support Specification. For
 * older drives only.
 */
#define GPCMD_GET_MEDIA_STATUS		    0xda

/* Mode page codes for mode sense/set */
#define GPMODE_R_W_ERROR_PAGE		0x01
#define GPMODE_WRITE_PARMS_PAGE		0x05
#define GPMODE_AUDIO_CTL_PAGE		0x0e
#define GPMODE_POWER_PAGE		0x1a
#define GPMODE_FAULT_FAIL_PAGE		0x1c
#define GPMODE_TO_PROTECT_PAGE		0x1d
#define GPMODE_CAPABILITIES_PAGE	0x2a
#define GPMODE_ALL_PAGES		0x3f
/* Not in Mt. Fuji, but in ATAPI 2.6 -- depricated now in favor
 * of MODE_SENSE_POWER_PAGE */
#define GPMODE_CDROM_PAGE		0x0d

#define ATAPI_INT_REASON_CD             0x01 /* 0 = data transfer */
#define ATAPI_INT_REASON_IO             0x02 /* 1 = transfer to the host */
#define ATAPI_INT_REASON_REL            0x04
#define ATAPI_INT_REASON_TAG            0xf8

/* same constants as bochs */
#define ASC_ILLEGAL_OPCODE                   0x20
#define ASC_LOGICAL_BLOCK_OOR                0x21
#define ASC_INV_FIELD_IN_CMD_PACKET          0x24
#define ASC_MEDIUM_NOT_PRESENT               0x3a
#define ASC_SAVING_PARAMETERS_NOT_SUPPORTED  0x39

#define SENSE_NONE            0
#define SENSE_NOT_READY       2
#define SENSE_ILLEGAL_REQUEST 5
#define SENSE_UNIT_ATTENTION  6

struct IDEState;

typedef void EndTransferFunc(struct IDEState *);

/* NOTE: IDEState represents in fact one drive */
typedef struct IDEState {
    /* ide config */
    int is_cdrom;
    int cylinders, heads, sectors;
    int64_t nb_sectors;
    int mult_sectors;
    int identify_set;
    uint16_t identify_data[256];
    SetIRQFunc *set_irq;
    void *irq_opaque;
    int irq;
    //* PCIDevice *pci_dev;
    struct BMDMAState *bmdma;
    int drive_serial;
    /* ide regs */
    uint8_t feature;
    uint8_t error;
    uint32_t nsector;
    uint8_t sector;
    uint8_t lcyl;
    uint8_t hcyl;
    /* other part of tf for lba48 support */
    uint8_t hob_feature;
    uint8_t hob_nsector;
    uint8_t hob_sector;
    uint8_t hob_lcyl;
    uint8_t hob_hcyl;

    uint8_t select;
    uint8_t status;

    /* 0x3f6 command, only meaningful for drive 0 */
    uint8_t cmd;
    /* set for lba48 access */
    uint8_t lba48;
    /* depends on bit 4 in select, only meaningful for drive 0 */
    struct IDEState *cur_drive; 
    BlockDriverState *bs;
    /* ATAPI specific */
    uint8_t sense_key;
    uint8_t asc;
    int packet_transfer_size;
    int elementary_transfer_size;
    int io_buffer_index;
    int lba;
    int cd_sector_size;
    int atapi_dma; /* true if dma is requested for the packet cmd */
    /* ATA DMA state */
    int io_buffer_size;
    /* PIO transfer handling */
    int req_nb_sectors; /* number of sectors per interrupt */
    EndTransferFunc *end_transfer_func;
    uint8_t *data_ptr;
    uint8_t *data_end;
    uint8_t io_buffer[MAX_MULT_SECTORS*512 + 4];
    QEMUTimer *sector_write_timer; /* only used for win2k instal hack */
    uint32_t irq_count; /* counts IRQs when using win2k install hack */
} IDEState;

#define BM_STATUS_DMAING 0x01
#define BM_STATUS_ERROR  0x02
#define BM_STATUS_INT    0x04

#define BM_CMD_START     0x01
#define BM_CMD_READ      0x08

#define IDE_TYPE_PIIX3   0
#define IDE_TYPE_CMD646  1

/* CMD646 specific */
#define MRDMODE		0x71
#define   MRDMODE_INTR_CH0	0x04
#define   MRDMODE_INTR_CH1	0x08
#define   MRDMODE_BLK_CH0	0x10
#define   MRDMODE_BLK_CH1	0x20
#define UDIDETCR0	0x73
#define UDIDETCR1	0x7B

typedef struct BMDMAState {
    uint8_t cmd;
    uint8_t status;
    uint32_t addr;
    
  //  struct PCIIDEState *pci_dev;
    /* current transfer state */
    uint32_t cur_addr;
    uint32_t cur_prd_last;
    uint32_t cur_prd_addr;
    uint32_t cur_prd_len;
    IDEState *ide_if;
    BlockDriverCompletionFunc *dma_cb;
    BlockDriverAIOCB *aiocb;
} BMDMAState;
struct BlockDriverState {
    int64_t total_sectors; /* if we are reading a disk image, give its
                              size in sectors */
    int read_only; /* if true, the media is read only */
    int removable; /* if true, the media can be removed */
    int locked;    /* if true, the media cannot temporarily be ejected */
    int encrypted; /* if true, the media is encrypted */
    /* event callback when inserting/removing */
    void (*change_cb)(void *opaque);
    void *change_opaque;

    BlockDriver *drv; /* NULL means no media */
    void *opaque;

    int boot_sector_enabled;
    uint8_t boot_sector_data[512];

    char filename[1024];
    char backing_file[1024]; /* if non zero, the image is a diff of
                                this file image */
    int is_temporary;
    int media_changed;

    /* async read/write emulation */

    void *sync_aiocb;
    
    /* NOTE: the following infos are only hints for real hardware
       drivers. They are not used by the block driver */
    int cyls, heads, secs, translation;
    int type;
    char device_name[32];
};
#if 0
typedef struct PCIIDEState {
    PCIDevice dev;
    IDEState ide_if[4];
    BMDMAState bmdma[2];
    int type; /* see IDE_TYPE_xxx */
} PCIIDEState;
#endif
static void ide_dma_start(IDEState *s, BlockDriverCompletionFunc *dma_cb);
static void ide_atapi_cmd_read_dma_cb(void *opaque, int ret);

void cpu_physical_memory_write(u32 addr,void* ptr,u32 sz);
void cpu_physical_memory_read(u32 addr,void* ptr,u32 sz);

static void padstr(char *str, const char *src, int len)
{
    int i, v;
    for(i = 0; i < len; i++) {
        if (*src)
            v = *src++;
        else
            v = ' ';
        *(char *)((long)str ^ 1) = v;
        str++;
    }
}

static void padstr8(uint8_t *buf, int buf_size, const char *src)
{
    int i;
    for(i = 0; i < buf_size; i++) {
        if (*src)
            buf[i] = *src++;
        else
            buf[i] = ' ';
    }
}

static void put_le16(uint16_t *p, unsigned int v)
{
    *p = (v);
}

static void ide_identify(IDEState *s)
{
    uint16_t *p;
    unsigned int oldsize;
    char buf[20];

    if (s->identify_set) {
	memcpy(s->io_buffer, s->identify_data, sizeof(s->identify_data));
	return;
    }

    memset(s->io_buffer, 0, 512);
    p = (uint16_t *)s->io_buffer;
    put_le16(p + 0, 0x0040);
    put_le16(p + 1, s->cylinders); 
    put_le16(p + 3, s->heads);
    put_le16(p + 4, 512 * s->sectors); /* XXX: retired, remove ? */
    put_le16(p + 5, 512); /* XXX: retired, remove ? */
    put_le16(p + 6, s->sectors); 
    snprintf(buf, sizeof(buf), "QM%05d", s->drive_serial);
    padstr((char *)(p + 10), buf, 20); /* serial number */
    put_le16(p + 20, 3); /* XXX: retired, remove ? */
    put_le16(p + 21, 512); /* cache size in sectors */
    put_le16(p + 22, 4); /* ecc bytes */
    padstr((char *)(p + 23), QEMU_VERSION, 8); /* firmware version */
    padstr((char *)(p + 27), "QEMU HARDDISK", 40); /* model */
#if MAX_MULT_SECTORS > 1    
    put_le16(p + 47, 0x8000 | MAX_MULT_SECTORS);
#endif
    put_le16(p + 48, 1); /* dword I/O */
    put_le16(p + 49, (1 << 11) | (1 << 9) | (1 << 8)); /* DMA and LBA supported */
    put_le16(p + 51, 0x200); /* PIO transfer cycle */
    put_le16(p + 52, 0x200); /* DMA transfer cycle */
    put_le16(p + 53, 1 | (1 << 1) | (1 << 2)); /* words 54-58,64-70,88 are valid */
    put_le16(p + 54, s->cylinders);
    put_le16(p + 55, s->heads);
    put_le16(p + 56, s->sectors);
    oldsize = s->cylinders * s->heads * s->sectors;
    put_le16(p + 57, oldsize);
    put_le16(p + 58, oldsize >> 16);
    if (s->mult_sectors)
        put_le16(p + 59, 0x100 | s->mult_sectors);
    put_le16(p + 60, s->nb_sectors);
    put_le16(p + 61, s->nb_sectors >> 16);
    put_le16(p + 63, 0x07); /* mdma0-2 supported */
    put_le16(p + 65, 120);
    put_le16(p + 66, 120);
    put_le16(p + 67, 120);
    put_le16(p + 68, 120);
    put_le16(p + 80, 0xf0); /* ata3 -> ata6 supported */
    put_le16(p + 81, 0x16); /* conforms to ata5 */
    put_le16(p + 82, (1 << 14));
    /* 13=flush_cache_ext,12=flush_cache,10=lba48 */
    put_le16(p + 83, (1 << 14) | (1 << 13) | (1 <<12) | (1 << 10));
    put_le16(p + 84, (1 << 14));
    put_le16(p + 85, (1 << 14));
    /* 13=flush_cache_ext,12=flush_cache,10=lba48 */
    put_le16(p + 86, (1 << 14) | (1 << 13) | (1 <<12) | (1 << 10));
    put_le16(p + 87, (1 << 14));
    put_le16(p + 88, 0x3f | (1 << 13)); /* udma5 set and supported */
    put_le16(p + 93, 1 | (1 << 14) | 0x2000);
    put_le16(p + 100, s->nb_sectors);
    put_le16(p + 101, s->nb_sectors >> 16);
    put_le16(p + 102, s->nb_sectors >> 32);
    put_le16(p + 103, s->nb_sectors >> 48);

    memcpy(s->identify_data, p, sizeof(s->identify_data));
    s->identify_set = 1;
}

static void ide_atapi_identify(IDEState *s)
{
    uint16_t *p;
    char buf[20];

    if (s->identify_set) {
	memcpy(s->io_buffer, s->identify_data, sizeof(s->identify_data));
	return;
    }

    memset(s->io_buffer, 0, 512);
    p = (uint16_t *)s->io_buffer;
    /* Removable CDROM, 50us response, 12 byte packets */
    put_le16(p + 0, (2 << 14) | (5 << 8) | (1 << 7) | (2 << 5) | (0 << 0));
    snprintf(buf, sizeof(buf), "QM%05d", s->drive_serial);
    padstr((char *)(p + 10), buf, 20); /* serial number */
    put_le16(p + 20, 3); /* buffer type */
    put_le16(p + 21, 512); /* cache size in sectors */
    put_le16(p + 22, 4); /* ecc bytes */
    padstr((char *)(p + 23), QEMU_VERSION, 8); /* firmware version */
    padstr((char *)(p + 27), "QEMU CD-ROM", 40); /* model */
    put_le16(p + 48, 1); /* dword I/O (XXX: should not be set on CDROM) */
#ifdef USE_DMA_CDROM
    put_le16(p + 49, 1 << 9 | 1 << 8); /* DMA and LBA supported */
    put_le16(p + 53, 7); /* words 64-70, 54-58, 88 valid */
    put_le16(p + 63, 7);  /* mdma0-2 supported */
    put_le16(p + 64, 0x3f); /* PIO modes supported */
#else
    put_le16(p + 49, 1 << 9); /* LBA supported, no DMA */
    put_le16(p + 53, 3); /* words 64-70, 54-58 valid */
    put_le16(p + 63, 0x103); /* DMA modes XXX: may be incorrect */
    put_le16(p + 64, 1); /* PIO modes */
#endif
    put_le16(p + 65, 0xb4); /* minimum DMA multiword tx cycle time */
    put_le16(p + 66, 0xb4); /* recommended DMA multiword tx cycle time */
    put_le16(p + 67, 0x12c); /* minimum PIO cycle time without flow control */
    put_le16(p + 68, 0xb4); /* minimum PIO cycle time with IORDY flow control */

    put_le16(p + 71, 30); /* in ns */
    put_le16(p + 72, 30); /* in ns */

    put_le16(p + 80, 0x1e); /* support up to ATA/ATAPI-4 */
#ifdef USE_DMA_CDROM
    put_le16(p + 88, 0x3f | (1 << 13)); /* udma5 set and supported */
#endif
    memcpy(s->identify_data, p, sizeof(s->identify_data));
    s->identify_set = 1;
}

static void ide_set_signature(IDEState *s)
{
    s->select &= 0xf0; /* clear head */
    /* put signature */
    s->nsector = 1;
    s->sector = 1;
    if (s->is_cdrom) {
        s->lcyl = 0x14;
        s->hcyl = 0xeb;
    } else if (s->bs) {
        s->lcyl = 0;
        s->hcyl = 0;
    } else {
        s->lcyl = 0xff;
        s->hcyl = 0xff;
    }
}

static inline void ide_abort_command(IDEState *s)
{
    s->status = READY_STAT | ERR_STAT;
    s->error = ABRT_ERR;
}

static inline void ide_set_irq(IDEState *s)
{
    BMDMAState *bm = s->bmdma;
    if (!(s->cmd & IDE_CMD_DISABLE_IRQ)) {
        if (bm) {
            bm->status |= BM_STATUS_INT;
        }
        s->set_irq(s->irq_opaque, s->irq, 1);
    }
}

/* prepare data transfer and tell what to do after */
static void ide_transfer_start(IDEState *s, uint8_t *buf, int size, 
                               EndTransferFunc *end_transfer_func)
{
    s->end_transfer_func = end_transfer_func;
    s->data_ptr = buf;
    s->data_end = buf + size;
    s->status |= DRQ_STAT;
}

static void ide_transfer_stop(IDEState *s)
{
    s->end_transfer_func = ide_transfer_stop;
    s->data_ptr = s->io_buffer;
    s->data_end = s->io_buffer;
    s->status &= ~DRQ_STAT;
}

static int64_t ide_get_sector(IDEState *s)
{
    int64_t sector_num;
    if (s->select & 0x40) {
        /* lba */
	if (!s->lba48) {
	    sector_num = ((s->select & 0x0f) << 24) | (s->hcyl << 16) |
		(s->lcyl << 8) | s->sector;
	} else {
	    sector_num = ((int64_t)s->hob_hcyl << 40) |
		((int64_t) s->hob_lcyl << 32) |
		((int64_t) s->hob_sector << 24) |
		((int64_t) s->hcyl << 16) |
		((int64_t) s->lcyl << 8) | s->sector;
	}
    } else {
        sector_num = ((s->hcyl << 8) | s->lcyl) * s->heads * s->sectors +
            (s->select & 0x0f) * s->sectors + (s->sector - 1);
    }
    return sector_num;
}

static void ide_set_sector(IDEState *s, int64_t sector_num)
{
    unsigned int cyl, r;
    if (s->select & 0x40) {
	if (!s->lba48) {
            s->select = (s->select & 0xf0) | (sector_num >> 24);
            s->hcyl = (sector_num >> 16);
            s->lcyl = (sector_num >> 8);
            s->sector = (sector_num);
	} else {
	    s->sector = sector_num;
	    s->lcyl = sector_num >> 8;
	    s->hcyl = sector_num >> 16;
	    s->hob_sector = sector_num >> 24;
	    s->hob_lcyl = sector_num >> 32;
	    s->hob_hcyl = sector_num >> 40;
	}
    } else {
        cyl = sector_num / (s->heads * s->sectors);
        r = sector_num % (s->heads * s->sectors);
        s->hcyl = cyl >> 8;
        s->lcyl = cyl;
        s->select = (s->select & 0xf0) | ((r / s->sectors) & 0x0f);
        s->sector = (r % s->sectors) + 1;
    }
}

static void ide_sector_read(IDEState *s)
{
    int64_t sector_num;
    int ret, n;

    s->status = READY_STAT | SEEK_STAT;
    s->error = 0; /* not needed by IDE spec, but needed by Windows */
    sector_num = ide_get_sector(s);
    n = s->nsector;
    if (n == 0) {
        /* no more sector to read from disk */
        ide_transfer_stop(s);
    } else {
#if defined(DEBUG_IDE)
      //  printf("read sector=%Ld\n", sector_num);
#endif
		printf("ATA: read sector=%Ld\n", sector_num);

        if (n > s->req_nb_sectors)
            n = s->req_nb_sectors;
        ret = bdrv_read(s->bs, sector_num, s->io_buffer, n);
        ide_transfer_start(s, s->io_buffer, 512 * n, ide_sector_read);
        ide_set_irq(s);
        ide_set_sector(s, sector_num + n);
        s->nsector -= n;
    }
}

/* return 0 if buffer completed */
static int dma_buf_rw(BMDMAState *bm, int is_write)
{
    IDEState *s = bm->ide_if;
    struct {
        uint32_t addr;
        uint32_t size;
    } prd;
    int l, len;

    for(;;) {
        l = s->io_buffer_size - s->io_buffer_index;
        if (l <= 0) 
            break;
        if (bm->cur_prd_len == 0) {
            /* end of table (with a fail safe of one page) */
            if (bm->cur_prd_last ||
                (bm->cur_addr - bm->addr) >= 4096)
                return 0;
			emu_printf("dma seems to have ended .. duno what the code i removed from here does.. chan dma ?\n");
			return 2;
			/*
			///i wonder what is this 
			//lets hope its not bad to remove it here xD
            cpu_physical_memory_read(bm->cur_addr, (uint8_t *)&prd, 8);
            bm->cur_addr += 8;
            prd.addr = le32_to_cpu(prd.addr);
            prd.size = le32_to_cpu(prd.size);
            len = prd.size & 0xfffe;
            if (len == 0)
                len = 0x10000;
            bm->cur_prd_len = len;
            bm->cur_prd_addr = prd.addr;
            bm->cur_prd_last = (prd.size & 0x80000000);*/
        }
        if (l > bm->cur_prd_len)
            l = bm->cur_prd_len;
        if (l > 0) {
            if (is_write) {
                cpu_physical_memory_write(bm->cur_prd_addr, 
                                          s->io_buffer + s->io_buffer_index, l);
            } else {
                cpu_physical_memory_read(bm->cur_prd_addr, 
                                          s->io_buffer + s->io_buffer_index, l);
            }
            bm->cur_prd_addr += l;
            bm->cur_prd_len -= l;
            s->io_buffer_index += l;
        }
    }
    return 1;
}

/* XXX: handle errors */
static void ide_read_dma_cb(void *opaque, int ret)
{
    BMDMAState* bm = (BMDMAState* )opaque;
    IDEState *s = (IDEState*)bm->ide_if;
    int n;
    int64_t sector_num;

    n = s->io_buffer_size >> 9;
    sector_num = ide_get_sector(s);
    if (n > 0) {
        sector_num += n;
        ide_set_sector(s, sector_num);
        s->nsector -= n;
		int dmrv=dma_buf_rw(bm, 1);
        if ( dmrv== 0)
            goto eot;
		else if (dmrv==2)
		{
			bm->status &= ~BM_STATUS_DMAING;
			bm->status |= BM_STATUS_INT;
			return;	//end of dma buffer (ha !)
		}
    }

    /* end of transfer ? */
    if (s->nsector == 0) {
        s->status = READY_STAT | SEEK_STAT;
        ide_set_irq(s);
    eot:
        bm->status &= ~BM_STATUS_DMAING;
        bm->status |= BM_STATUS_INT;
        bm->dma_cb = NULL;
        bm->ide_if = NULL;
        bm->aiocb = NULL;
        return;
    }

    /* launch next transfer */
    n = s->nsector;
    if (n > MAX_MULT_SECTORS)
        n = MAX_MULT_SECTORS;
    s->io_buffer_index = 0;
    s->io_buffer_size = n * 512;
#ifdef DEBUG_AIO
    printf("aio_read: sector_num=%lld n=%d\n", sector_num, n);
#endif
    bm->aiocb = bdrv_aio_read(s->bs, sector_num, s->io_buffer, n, 
                              ide_read_dma_cb, bm);
}

static void ide_sector_read_dma(IDEState *s)
{
    s->status = READY_STAT | SEEK_STAT | DRQ_STAT | BUSY_STAT;
    s->io_buffer_index = 0;
    s->io_buffer_size = 0;
    ide_dma_start(s, ide_read_dma_cb);
}

static void ide_sector_write_timer_cb(void *opaque)
{
    IDEState* s = (IDEState*)opaque;
    ide_set_irq(s);
}

static void ide_sector_write(IDEState *s)
{
    int64_t sector_num;
    int ret, n, n1;

    s->status = READY_STAT | SEEK_STAT;
    sector_num = ide_get_sector(s);
#if defined(DEBUG_IDE)
    printf("write sector=%Ld\n", sector_num);
#endif
	printf("ATA: write sector=%Ld\n", sector_num);
    n = s->nsector;
    if (n > s->req_nb_sectors)
        n = s->req_nb_sectors;
    ret = bdrv_write(s->bs, sector_num, s->io_buffer, n);
    s->nsector -= n;
    if (s->nsector == 0) {
        /* no more sector to write */
        ide_transfer_stop(s);
    } else {
        n1 = s->nsector;
        if (n1 > s->req_nb_sectors)
            n1 = s->req_nb_sectors;
        ide_transfer_start(s, s->io_buffer, 512 * n1, ide_sector_write);
    }
    ide_set_sector(s, sector_num + n);
    
#ifdef TARGET_I386
    if (win2k_install_hack && ((++s->irq_count % 16) == 0)) {
        /* It seems there is a bug in the Windows 2000 installer HDD
           IDE driver which fills the disk with empty logs when the
           IDE write IRQ comes too early. This hack tries to correct
           that at the expense of slower write performances. Use this
           option _only_ to install Windows 2000. You must disable it
           for normal use. */
        qemu_mod_timer(s->sector_write_timer, 
                       qemu_get_clock(vm_clock) + (ticks_per_sec / 1000));
    } else 
#endif
    {
        ide_set_irq(s);
    }
}

/* XXX: handle errors */
static void ide_write_dma_cb(void *opaque, int ret)
{
    BMDMAState* bm =(BMDMAState*)opaque;
    IDEState* s =(IDEState* ) bm->ide_if;
    int n;
    int64_t sector_num;

    n = s->io_buffer_size >> 9;
    sector_num = ide_get_sector(s);
    if (n > 0) {
        sector_num += n;
        ide_set_sector(s, sector_num);
        s->nsector -= n;
    }

    /* end of transfer ? */
    if (s->nsector == 0) {
        s->status = READY_STAT | SEEK_STAT;
        ide_set_irq(s);
    eot:
        bm->status &= ~BM_STATUS_DMAING;
        bm->status |= BM_STATUS_INT;
        bm->dma_cb = NULL;
        bm->ide_if = NULL;
        bm->aiocb = NULL;
        return;
    }

    /* launch next transfer */
    n = s->nsector;
    if (n > MAX_MULT_SECTORS)
        n = MAX_MULT_SECTORS;
    s->io_buffer_index = 0;
    s->io_buffer_size = n * 512;

	int dmrv=dma_buf_rw(bm, 0);
	if ( dmrv== 0)
		goto eot;
	else if (dmrv==2)
	{
		bm->status &= ~BM_STATUS_DMAING;
        bm->status |= BM_STATUS_INT;
		return;	//end of dma buffer (ha !)
	}
    
#ifdef DEBUG_AIO
    printf("aio_write: sector_num=%lld n=%d\n", sector_num, n);
#endif
    bm->aiocb = bdrv_aio_write(s->bs, sector_num, s->io_buffer, n, 
                               ide_write_dma_cb, bm);
}

static void ide_sector_write_dma(IDEState *s)
{
    s->status = READY_STAT | SEEK_STAT | DRQ_STAT | BUSY_STAT;
    s->io_buffer_index = 0;
    s->io_buffer_size = 0;
    ide_dma_start(s, ide_write_dma_cb);
}

static void ide_atapi_cmd_ok(IDEState *s)
{
    s->error = 0;
    s->status = READY_STAT;
    s->nsector = (s->nsector & ~7) | ATAPI_INT_REASON_IO | ATAPI_INT_REASON_CD;
    ide_set_irq(s);
}

static void ide_atapi_cmd_error(IDEState *s, int sense_key, int asc)
{
#ifdef DEBUG_IDE_ATAPI
    //printf("atapi_cmd_error: sense=0x%x asc=0x%x\n", sense_key, asc);
#endif
	printf("atapi_cmd_error: sense=0x%x asc=0x%x\n", sense_key, asc);
    s->error = sense_key << 4;
    s->status = READY_STAT | ERR_STAT;
    s->nsector = (s->nsector & ~7) | ATAPI_INT_REASON_IO | ATAPI_INT_REASON_CD;
    s->sense_key = sense_key;
    s->asc = asc;
    ide_set_irq(s);
}

static inline void cpu_to_ube16(uint8_t *buf, int val)
{
    buf[0] = val >> 8;
    buf[1] = val;
}

static inline void cpu_to_ube32(uint8_t *buf, unsigned int val)
{
    buf[0] = val >> 24;
    buf[1] = val >> 16;
    buf[2] = val >> 8;
    buf[3] = val;
}

static inline int ube16_to_cpu(const uint8_t *buf)
{
    return (buf[0] << 8) | buf[1];
}

static inline int ube32_to_cpu(const uint8_t *buf)
{
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static void lba_to_msf(uint8_t *buf, int lba)
{
    lba += 150;
    buf[0] = (lba / 75) / 60;
    buf[1] = (lba / 75) % 60;
    buf[2] = lba % 75;
}

static void cd_data_to_raw(uint8_t *buf, int lba)
{
    /* sync bytes */
    buf[0] = 0x00;
    memset(buf + 1, 0xff, 10);
    buf[11] = 0x00;
    buf += 12;
    /* MSF */
    lba_to_msf(buf, lba);
    buf[3] = 0x01; /* mode 1 data */
    buf += 4;
    /* data */
    buf += 2048;
    /* XXX: ECC not computed */
    memset(buf, 0, 288);
}

static int cd_read_sector(BlockDriverState *bs, int lba, uint8_t *buf, 
                           int sector_size)
{
    int ret;

    switch(sector_size) {
    case 2048:
        ret = bdrv_read(bs, (int64_t)lba << 2, buf, 4);
        break;
    case 2352:
        ret = bdrv_read(bs, (int64_t)lba << 2, buf + 16, 4);
        if (ret < 0)
            return ret;
        cd_data_to_raw(buf, lba);
        break;
    default:
        ret = -EIO;
        break;
    }
    return ret;
}

static void ide_atapi_io_error(IDEState *s, int ret)
{
    /* XXX: handle more errors */
    if (ret == -ENOMEDIUM) {
        ide_atapi_cmd_error(s, SENSE_NOT_READY, 
                            ASC_MEDIUM_NOT_PRESENT);
    } else {
        ide_atapi_cmd_error(s, SENSE_ILLEGAL_REQUEST, 
                            ASC_LOGICAL_BLOCK_OOR);
    }
}

/* The whole ATAPI transfer logic is handled in this function */
static void ide_atapi_cmd_reply_end(IDEState *s)
{
    int byte_count_limit, size, ret;
#ifdef DEBUG_IDE_ATAPI
    printf("reply: tx_size=%d elem_tx_size=%d index=%d\n", 
           s->packet_transfer_size,
           s->elementary_transfer_size,
           s->io_buffer_index);
#endif
    if (s->packet_transfer_size <= 0) {
        /* end of transfer */
        ide_transfer_stop(s);
        s->status = READY_STAT;
        s->nsector = (s->nsector & ~7) | ATAPI_INT_REASON_IO | ATAPI_INT_REASON_CD;
        ide_set_irq(s);
#ifdef DEBUG_IDE_ATAPI
        printf("status=0x%x\n", s->status);
#endif
    } else {
        /* see if a new sector must be read */
        if (s->lba != -1 && s->io_buffer_index >= s->cd_sector_size) {
            ret = cd_read_sector(s->bs, s->lba, s->io_buffer, s->cd_sector_size);
            if (ret < 0) {
                ide_transfer_stop(s);
                ide_atapi_io_error(s, ret);
                return;
            }
            s->lba++;
            s->io_buffer_index = 0;
        }
        if (s->elementary_transfer_size > 0) {
            /* there are some data left to transmit in this elementary
               transfer */
            size = s->cd_sector_size - s->io_buffer_index;
            if (size > s->elementary_transfer_size)
                size = s->elementary_transfer_size;
            ide_transfer_start(s, s->io_buffer + s->io_buffer_index, 
                               size, ide_atapi_cmd_reply_end);
            s->packet_transfer_size -= size;
            s->elementary_transfer_size -= size;
            s->io_buffer_index += size;
        } else {
            /* a new transfer is needed */
            s->nsector = (s->nsector & ~7) | ATAPI_INT_REASON_IO;
            byte_count_limit = s->lcyl | (s->hcyl << 8);
#ifdef DEBUG_IDE_ATAPI
            printf("byte_count_limit=%d\n", byte_count_limit);
#endif
            if (byte_count_limit == 0xffff)
                byte_count_limit--;
            size = s->packet_transfer_size;
            if (size > byte_count_limit) {
                /* byte count limit must be even if this case */
                if (byte_count_limit & 1)
                    byte_count_limit--;
                size = byte_count_limit;
            }
            s->lcyl = size;
            s->hcyl = size >> 8;
            s->elementary_transfer_size = size;
            /* we cannot transmit more than one sector at a time */
            if (s->lba != -1) {
                if (size > (s->cd_sector_size - s->io_buffer_index))
                    size = (s->cd_sector_size - s->io_buffer_index);
            }
            ide_transfer_start(s, s->io_buffer + s->io_buffer_index, 
                               size, ide_atapi_cmd_reply_end);
            s->packet_transfer_size -= size;
            s->elementary_transfer_size -= size;
            s->io_buffer_index += size;
            ide_set_irq(s);
#ifdef DEBUG_IDE_ATAPI
            printf("status=0x%x\n", s->status);
#endif
        }
    }
}

/* send a reply of 'size' bytes in s->io_buffer to an ATAPI command */
static void ide_atapi_cmd_reply(IDEState *s, int size, int max_size)
{
    if (size > max_size)
        size = max_size;
    s->lba = -1; /* no sector read */
    s->packet_transfer_size = size;
    s->io_buffer_size = size;    /* dma: send the reply data as one chunk */
    s->elementary_transfer_size = 0;
    s->io_buffer_index = 0;

    if (s->atapi_dma) {
    	s->status = READY_STAT | DRQ_STAT;
	ide_dma_start(s, ide_atapi_cmd_read_dma_cb);
    } else {
    	s->status = READY_STAT;
    	ide_atapi_cmd_reply_end(s);
    }
}

/* start a CD-CDROM read command */
static void ide_atapi_cmd_read_pio(IDEState *s, int lba, int nb_sectors,
                                   int sector_size)
{
    s->lba = lba;
    s->packet_transfer_size = nb_sectors * sector_size;
    s->elementary_transfer_size = 0;
    s->io_buffer_index = sector_size;
    s->cd_sector_size = sector_size;

    s->status = READY_STAT;
    ide_atapi_cmd_reply_end(s);
}

/* ATAPI DMA support */

/* XXX: handle read errors */
static void ide_atapi_cmd_read_dma_cb(void *opaque, int ret)
{
    BMDMAState* bm =(BMDMAState*) opaque;
    IDEState* s =(IDEState*) bm->ide_if;
    int data_offset, n;

    if (ret < 0) {
        ide_atapi_io_error(s, ret);
        goto eot;
    }

    if (s->io_buffer_size > 0) {
	/*
	 * For a cdrom read sector command (s->lba != -1),
	 * adjust the lba for the next s->io_buffer_size chunk
	 * and dma the current chunk.
	 * For a command != read (s->lba == -1), just transfer
	 * the reply data.
	 */
	if (s->lba != -1) {
	    if (s->cd_sector_size == 2352) {
		n = 1;
		cd_data_to_raw(s->io_buffer, s->lba);
	    } else {
		n = s->io_buffer_size >> 11;
	    }
	    s->lba += n;
	}
        s->packet_transfer_size -= s->io_buffer_size;
        if (dma_buf_rw(bm, 1) == 0)
            goto eot;
    }

    if (s->packet_transfer_size <= 0) {
        s->status = READY_STAT;
        s->nsector = (s->nsector & ~7) | ATAPI_INT_REASON_IO | ATAPI_INT_REASON_CD;
        ide_set_irq(s);
    eot:
        bm->status &= ~BM_STATUS_DMAING;
        bm->status |= BM_STATUS_INT;
        bm->dma_cb = NULL;
        bm->ide_if = NULL;
        bm->aiocb = NULL;
        return;
    }
    
    s->io_buffer_index = 0;
    if (s->cd_sector_size == 2352) {
        n = 1;
        s->io_buffer_size = s->cd_sector_size;
        data_offset = 16;
    } else {
        n = s->packet_transfer_size >> 11;
        if (n > (MAX_MULT_SECTORS / 4))
            n = (MAX_MULT_SECTORS / 4);
        s->io_buffer_size = n * 2048;
        data_offset = 0;
    }
#ifdef DEBUG_AIO
    printf("aio_read_cd: lba=%u n=%d\n", s->lba, n);
#endif
    bm->aiocb = bdrv_aio_read(s->bs, (int64_t)s->lba << 2, 
                              s->io_buffer + data_offset, n * 4, 
                              ide_atapi_cmd_read_dma_cb, bm);
    if (!bm->aiocb) {
        /* Note: media not present is the most likely case */
        ide_atapi_cmd_error(s, SENSE_NOT_READY, 
                            ASC_MEDIUM_NOT_PRESENT);
        goto eot;
    }
}

/* start a CD-CDROM read command with DMA */
/* XXX: test if DMA is available */
static void ide_atapi_cmd_read_dma(IDEState *s, int lba, int nb_sectors,
                                   int sector_size)
{
    s->lba = lba;
    s->packet_transfer_size = nb_sectors * sector_size;
    s->io_buffer_index = 0;
    s->io_buffer_size = 0;
    s->cd_sector_size = sector_size;

    /* XXX: check if BUSY_STAT should be set */
    s->status = READY_STAT | DRQ_STAT | BUSY_STAT;
    ide_dma_start(s, ide_atapi_cmd_read_dma_cb);
}

static void ide_atapi_cmd_read(IDEState *s, int lba, int nb_sectors, 
                               int sector_size)
{
#ifdef DEBUG_IDE_ATAPI
    printf("read %s: LBA=%d nb_sectors=%d\n", s->atapi_dma ? "dma" : "pio",
	lba, nb_sectors);
#endif
    if (s->atapi_dma) {
        ide_atapi_cmd_read_dma(s, lba, nb_sectors, sector_size);
    } else {
        ide_atapi_cmd_read_pio(s, lba, nb_sectors, sector_size);
    }
}

static void ide_atapi_cmd(IDEState *s)
{
    const uint8_t *packet;
    uint8_t *buf;
    int max_len;

    packet = s->io_buffer;
    buf = s->io_buffer;
#ifdef DEBUG_IDE_ATAPI
    {
        int i;
        printf("ATAPI limit=0x%x packet:", s->lcyl | (s->hcyl << 8));
        for(i = 0; i < ATAPI_PACKET_SIZE; i++) {
            printf(" %02x", packet[i]);
        }
        printf("\n");
    }
#endif
    switch(s->io_buffer[0]) {
    case GPCMD_TEST_UNIT_READY:
        if (bdrv_is_inserted(s->bs)) {
            ide_atapi_cmd_ok(s);
        } else {
            ide_atapi_cmd_error(s, SENSE_NOT_READY, 
                                ASC_MEDIUM_NOT_PRESENT);
        }
        break;
    case GPCMD_MODE_SENSE_10:
        {
            int action, code;
            max_len = ube16_to_cpu(packet + 7);
            action = packet[2] >> 6;
            code = packet[2] & 0x3f;
            switch(action) {
            case 0: /* current values */
                switch(code) {
                case 0x01: /* error recovery */
                    cpu_to_ube16(&buf[0], 16 + 6);
                    buf[2] = 0x70;
                    buf[3] = 0;
                    buf[4] = 0;
                    buf[5] = 0;
                    buf[6] = 0;
                    buf[7] = 0;

                    buf[8] = 0x01;
                    buf[9] = 0x06;
                    buf[10] = 0x00;
                    buf[11] = 0x05;
                    buf[12] = 0x00;
                    buf[13] = 0x00;
                    buf[14] = 0x00;
                    buf[15] = 0x00;
                    ide_atapi_cmd_reply(s, 16, max_len);
                    break;
                case 0x2a:
                    cpu_to_ube16(&buf[0], 28 + 6);
                    buf[2] = 0x70;
                    buf[3] = 0;
                    buf[4] = 0;
                    buf[5] = 0;
                    buf[6] = 0;
                    buf[7] = 0;

                    buf[8] = 0x2a;
                    buf[9] = 0x12;
                    buf[10] = 0x00;
                    buf[11] = 0x00;
                    
                    buf[12] = 0x70;
                    buf[13] = 3 << 5;
                    buf[14] = (1 << 0) | (1 << 3) | (1 << 5);
                    if (bdrv_is_locked(s->bs))
                        buf[6] |= 1 << 1;
                    buf[15] = 0x00;
                    cpu_to_ube16(&buf[16], 706);
                    buf[18] = 0;
                    buf[19] = 2;
                    cpu_to_ube16(&buf[20], 512);
                    cpu_to_ube16(&buf[22], 706);
                    buf[24] = 0;
                    buf[25] = 0;
                    buf[26] = 0;
                    buf[27] = 0;
                    ide_atapi_cmd_reply(s, 28, max_len);
                    break;
                default:
                    goto error_cmd;
                }
                break;
            case 1: /* changeable values */
                goto error_cmd;
            case 2: /* default values */
                goto error_cmd;
            default:
            case 3: /* saved values */
                ide_atapi_cmd_error(s, SENSE_ILLEGAL_REQUEST, 
                                    ASC_SAVING_PARAMETERS_NOT_SUPPORTED);
                break;
            }
        }
        break;
    case GPCMD_REQUEST_SENSE:
        max_len = packet[4];
        memset(buf, 0, 18);
        buf[0] = 0x70 | (1 << 7);
        buf[2] = s->sense_key;
        buf[7] = 10;
        buf[12] = s->asc;
        ide_atapi_cmd_reply(s, 18, max_len);
        break;
    case GPCMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
        if (bdrv_is_inserted(s->bs)) {
            bdrv_set_locked(s->bs, packet[4] & 1);
            ide_atapi_cmd_ok(s);
        } else {
            ide_atapi_cmd_error(s, SENSE_NOT_READY, 
                                ASC_MEDIUM_NOT_PRESENT);
        }
        break;
    case GPCMD_READ_10:
    case GPCMD_READ_12:
        {
            int nb_sectors, lba;

            if (packet[0] == GPCMD_READ_10)
                nb_sectors = ube16_to_cpu(packet + 7);
            else
                nb_sectors = ube32_to_cpu(packet + 6);
            lba = ube32_to_cpu(packet + 2);
            if (nb_sectors == 0) {
                ide_atapi_cmd_ok(s);
                break;
            }
            ide_atapi_cmd_read(s, lba, nb_sectors, 2048);
        }
        break;
    case GPCMD_READ_CD:
        {
            int nb_sectors, lba, transfer_request;

            nb_sectors = (packet[6] << 16) | (packet[7] << 8) | packet[8];
            lba = ube32_to_cpu(packet + 2);
            if (nb_sectors == 0) {
                ide_atapi_cmd_ok(s);
                break;
            }
            transfer_request = packet[9];
            switch(transfer_request & 0xf8) {
            case 0x00:
                /* nothing */
                ide_atapi_cmd_ok(s);
                break;
            case 0x10:
                /* normal read */
                ide_atapi_cmd_read(s, lba, nb_sectors, 2048);
                break;
            case 0xf8:
                /* read all data */
                ide_atapi_cmd_read(s, lba, nb_sectors, 2352);
                break;
            default:
                ide_atapi_cmd_error(s, SENSE_ILLEGAL_REQUEST, 
                                    ASC_INV_FIELD_IN_CMD_PACKET);
                break;
            }
        }
        break;
    case GPCMD_SEEK:
        {
            int lba;
            int64_t total_sectors;

            bdrv_get_geometry(s->bs, &total_sectors);
            total_sectors >>= 2;
            if (total_sectors <= 0) {
                ide_atapi_cmd_error(s, SENSE_NOT_READY, 
                                    ASC_MEDIUM_NOT_PRESENT);
                break;
            }
            lba = ube32_to_cpu(packet + 2);
            if (lba >= total_sectors) {
                ide_atapi_cmd_error(s, SENSE_ILLEGAL_REQUEST, 
                                    ASC_LOGICAL_BLOCK_OOR);
                break;
            }
            ide_atapi_cmd_ok(s);
        }
        break;
    case GPCMD_START_STOP_UNIT:
        {
            int start, eject;
            start = packet[4] & 1;
            eject = (packet[4] >> 1) & 1;
            
            if (eject && !start) {
                /* eject the disk */
                bdrv_eject(s->bs, 1);
            } else if (eject && start) {
                /* close the tray */
                bdrv_eject(s->bs, 0);
            }
            ide_atapi_cmd_ok(s);
        }
        break;
    case GPCMD_MECHANISM_STATUS:
        {
            max_len = ube16_to_cpu(packet + 8);
            cpu_to_ube16(buf, 0);
            /* no current LBA */
            buf[2] = 0;
            buf[3] = 0;
            buf[4] = 0;
            buf[5] = 1;
            cpu_to_ube16(buf + 6, 0);
            ide_atapi_cmd_reply(s, 8, max_len);
        }
        break;
    case GPCMD_READ_TOC_PMA_ATIP:
        {
            int format, msf, start_track, len;
            int64_t total_sectors;

            bdrv_get_geometry(s->bs, &total_sectors);
            total_sectors >>= 2;
            if (total_sectors <= 0) {
                ide_atapi_cmd_error(s, SENSE_NOT_READY, 
                                    ASC_MEDIUM_NOT_PRESENT);
                break;
            }
            max_len = ube16_to_cpu(packet + 7);
            format = packet[9] >> 6;
            msf = (packet[1] >> 1) & 1;
            start_track = packet[6];
            switch(format) {
            case 0:
				emu_printf("CDROM ?TOC?\n");
                len = -1;//cdrom_read_toc(total_sectors, buf, msf, start_track);
                if (len < 0)
                    goto error_cmd;
                ide_atapi_cmd_reply(s, len, max_len);
                break;
            case 1:
                /* multi session : only a single session defined */
                memset(buf, 0, 12);
                buf[1] = 0x0a;
                buf[2] = 0x01;
                buf[3] = 0x01;
                ide_atapi_cmd_reply(s, 12, max_len);
                break;
            case 2:
				emu_printf("CDROM ?TOC?\n");
                len = -1;//
               // len = cdrom_read_toc_raw(total_sectors, buf, msf, start_track);
                if (len < 0)
                    goto error_cmd;
                ide_atapi_cmd_reply(s, len, max_len);
                break;
            default:
            error_cmd:
                ide_atapi_cmd_error(s, SENSE_ILLEGAL_REQUEST, 
                                    ASC_INV_FIELD_IN_CMD_PACKET);
                break;
            }
        }
        break;
    case GPCMD_READ_CDVD_CAPACITY:
        {
            int64_t total_sectors;

            bdrv_get_geometry(s->bs, &total_sectors);
            total_sectors >>= 2;
            if (total_sectors <= 0) {
                ide_atapi_cmd_error(s, SENSE_NOT_READY, 
                                    ASC_MEDIUM_NOT_PRESENT);
                break;
            }
            /* NOTE: it is really the number of sectors minus 1 */
            cpu_to_ube32(buf, total_sectors - 1);
            cpu_to_ube32(buf + 4, 2048);
            ide_atapi_cmd_reply(s, 8, 8);
        }
        break;
    case GPCMD_INQUIRY:
        max_len = packet[4];
        buf[0] = 0x05; /* CD-ROM */
        buf[1] = 0x80; /* removable */
        buf[2] = 0x00; /* ISO */
        buf[3] = 0x21; /* ATAPI-2 (XXX: put ATAPI-4 ?) */
        buf[4] = 31; /* additionnal length */
        buf[5] = 0; /* reserved */
        buf[6] = 0; /* reserved */
        buf[7] = 0; /* reserved */
        padstr8(buf + 8, 8, "QEMU");
        padstr8(buf + 16, 16, "QEMU CD-ROM");
        padstr8(buf + 32, 4, QEMU_VERSION);
        ide_atapi_cmd_reply(s, 36, max_len);
        break;
    default:
        ide_atapi_cmd_error(s, SENSE_ILLEGAL_REQUEST, 
                            ASC_ILLEGAL_OPCODE);
        break;
    }
}

/* called when the inserted state of the media has changed */
static void cdrom_change_cb(void *opaque)
{
    IDEState* s = (IDEState*)opaque;
    int64_t nb_sectors;

    /* XXX: send interrupt too */
    bdrv_get_geometry(s->bs, &nb_sectors);
    s->nb_sectors = nb_sectors;
}

static void ide_cmd_lba48_transform(IDEState *s, int lba48)
{
    s->lba48 = lba48;

    /* handle the 'magic' 0 nsector count conversion here. to avoid
     * fiddling with the rest of the read logic, we just store the
     * full sector count in ->nsector and ignore ->hob_nsector from now
     */
    if (!s->lba48) {
	if (!s->nsector)
	    s->nsector = 256;
    } else {
	if (!s->nsector && !s->hob_nsector)
	    s->nsector = 65536;
	else {
	    int lo = s->nsector;
	    int hi = s->hob_nsector;

	    s->nsector = (hi << 8) | lo;
	}
    }
}

static void ide_clear_hob(IDEState *ide_if)
{
    /* any write clears HOB high bit of device control register */
    ide_if[0].select &= ~(1 << 7);
    ide_if[1].select &= ~(1 << 7);
}

static void ide_ioport_write(void *opaque, uint32_t addr, uint32_t val)
{
    IDEState *ide_if =(IDEState*)opaque;
    IDEState *s;
    int unit, n;
    int lba48 = 0;

#ifdef DEBUG_IDE
    printf("IDE: write addr=0x%x val=0x%02x\n", addr, val);
#endif

    addr &= 7;
    switch(addr) {
    case 0:
        break;
    case 1:
	ide_clear_hob(ide_if);
        /* NOTE: data is written to the two drives */
	ide_if[0].hob_feature = ide_if[0].feature;
	ide_if[1].hob_feature = ide_if[1].feature;
        ide_if[0].feature = val;
        ide_if[1].feature = val;
        break;
    case 2:
	ide_clear_hob(ide_if);
	ide_if[0].hob_nsector = ide_if[0].nsector;
	ide_if[1].hob_nsector = ide_if[1].nsector;
        ide_if[0].nsector = val;
        ide_if[1].nsector = val;
        break;
    case 3:
	ide_clear_hob(ide_if);
	ide_if[0].hob_sector = ide_if[0].sector;
	ide_if[1].hob_sector = ide_if[1].sector;
        ide_if[0].sector = val;
        ide_if[1].sector = val;
        break;
    case 4:
	ide_clear_hob(ide_if);
	ide_if[0].hob_lcyl = ide_if[0].lcyl;
	ide_if[1].hob_lcyl = ide_if[1].lcyl;
        ide_if[0].lcyl = val;
        ide_if[1].lcyl = val;
        break;
    case 5:
	ide_clear_hob(ide_if);
	ide_if[0].hob_hcyl = ide_if[0].hcyl;
	ide_if[1].hob_hcyl = ide_if[1].hcyl;
        ide_if[0].hcyl = val;
        ide_if[1].hcyl = val;
        break;
    case 6:
	/* FIXME: HOB readback uses bit 7 */
        ide_if[0].select = (val & ~0x10) | 0xa0;
        ide_if[1].select = (val | 0x10) | 0xa0;
        /* select drive */
        unit = (val >> 4) & 1;
        s = ide_if + unit;
        ide_if->cur_drive = s;
        break;
    default:
    case 7:
        /* command */
#if defined(DEBUG_IDE)
        printf("ide: CMD=%02x\n", val);
#endif
        s = ide_if->cur_drive;
        /* ignore commands to non existant slave */
        if (s != ide_if && !s->bs) 
            break;

        switch(val) {
        case WIN_IDENTIFY:
            if (s->bs && !s->is_cdrom) {
                ide_identify(s);
                s->status = READY_STAT | SEEK_STAT;
                ide_transfer_start(s, s->io_buffer, 512, ide_transfer_stop);
            } else {
                if (s->is_cdrom) {
                    ide_set_signature(s);
                }
                ide_abort_command(s);
            }
            ide_set_irq(s);
            break;
        case WIN_SPECIFY:
        case WIN_RECAL:
            s->error = 0;
            s->status = READY_STAT | SEEK_STAT;
            ide_set_irq(s);
            break;
        case WIN_SETMULT:
            if (s->nsector > MAX_MULT_SECTORS || 
                s->nsector == 0 ||
                (s->nsector & (s->nsector - 1)) != 0) {
                ide_abort_command(s);
            } else {
                s->mult_sectors = s->nsector;
                s->status = READY_STAT;
            }
            ide_set_irq(s);
            break;
        case WIN_VERIFY_EXT:
	    lba48 = 1;
        case WIN_VERIFY:
        case WIN_VERIFY_ONCE:
            /* do sector number check ? */
	    ide_cmd_lba48_transform(s, lba48);
            s->status = READY_STAT;
            ide_set_irq(s);
            break;
	case WIN_READ_EXT:
	    lba48 = 1;
        case WIN_READ:
        case WIN_READ_ONCE:
            if (!s->bs) 
                goto abort_cmd;
	    ide_cmd_lba48_transform(s, lba48);
            s->req_nb_sectors = 1;
            ide_sector_read(s);
            break;
	case WIN_WRITE_EXT:
	    lba48 = 1;
        case WIN_WRITE:
        case WIN_WRITE_ONCE:
	    ide_cmd_lba48_transform(s, lba48);
            s->error = 0;
            s->status = SEEK_STAT | READY_STAT;
            s->req_nb_sectors = 1;
            ide_transfer_start(s, s->io_buffer, 512, ide_sector_write);
            break;
	case WIN_MULTREAD_EXT:
	    lba48 = 1;
        case WIN_MULTREAD:
            if (!s->mult_sectors)
                goto abort_cmd;
	    ide_cmd_lba48_transform(s, lba48);
            s->req_nb_sectors = s->mult_sectors;
            ide_sector_read(s);
            break;
        case WIN_MULTWRITE_EXT:
	    lba48 = 1;
        case WIN_MULTWRITE:
            if (!s->mult_sectors)
                goto abort_cmd;
	    ide_cmd_lba48_transform(s, lba48);
            s->error = 0;
            s->status = SEEK_STAT | READY_STAT;
            s->req_nb_sectors = s->mult_sectors;
            n = s->nsector;
            if (n > s->req_nb_sectors)
                n = s->req_nb_sectors;
            ide_transfer_start(s, s->io_buffer, 512 * n, ide_sector_write);
            break;
	case WIN_READDMA_EXT:
	    lba48 = 1;
        case WIN_READDMA:
        case WIN_READDMA_ONCE:
            if (!s->bs) 
                goto abort_cmd;
	    ide_cmd_lba48_transform(s, lba48);
            ide_sector_read_dma(s);
            break;
	case WIN_WRITEDMA_EXT:
	    lba48 = 1;
        case WIN_WRITEDMA:
        case WIN_WRITEDMA_ONCE:
            if (!s->bs) 
                goto abort_cmd;
	    ide_cmd_lba48_transform(s, lba48);
            ide_sector_write_dma(s);
            break;
        case WIN_READ_NATIVE_MAX_EXT:
	    lba48 = 1;
        case WIN_READ_NATIVE_MAX:
	    ide_cmd_lba48_transform(s, lba48);
            ide_set_sector(s, s->nb_sectors - 1);
            s->status = READY_STAT;
            ide_set_irq(s);
            break;
        case WIN_CHECKPOWERMODE1:
            s->nsector = 0xff; /* device active or idle */
            s->status = READY_STAT;
            ide_set_irq(s);
            break;
        case WIN_SETFEATURES:
            if (!s->bs)
                goto abort_cmd;
            /* XXX: valid for CDROM ? */
            switch(s->feature) {
            case 0x02: /* write cache enable */
            case 0x82: /* write cache disable */
            case 0xaa: /* read look-ahead enable */
            case 0x55: /* read look-ahead disable */
                s->status = READY_STAT | SEEK_STAT;
                ide_set_irq(s);
                break;
            case 0x03: { /* set transfer mode */
		uint8_t val = s->nsector & 0x07;

		switch (s->nsector >> 3) {
		    case 0x00: /* pio default */
		    case 0x01: /* pio mode */
			put_le16(s->identify_data + 63,0x07);
			put_le16(s->identify_data + 88,0x3f);
			break;
		    case 0x04: /* mdma mode */
			put_le16(s->identify_data + 63,0x07 | (1 << (val + 8)));
			put_le16(s->identify_data + 88,0x3f);
			break;
		    case 0x08: /* udma mode */
			put_le16(s->identify_data + 63,0x07);
			put_le16(s->identify_data + 88,0x3f | (1 << (val + 8)));
			break;
		    default:
			goto abort_cmd;
		}
                s->status = READY_STAT | SEEK_STAT;
                ide_set_irq(s);
                break;
	    }
            default:
                goto abort_cmd;
            }
            break;
        case WIN_FLUSH_CACHE:
        case WIN_FLUSH_CACHE_EXT:
            if (s->bs)
                bdrv_flush(s->bs);
	    s->status = READY_STAT;
            ide_set_irq(s);
            break;
	case WIN_STANDBYNOW1:
        case WIN_IDLEIMMEDIATE:
	    s->status = READY_STAT;
            ide_set_irq(s);
            break;
            /* ATAPI commands */
        case WIN_PIDENTIFY:
            if (s->is_cdrom) {
                ide_atapi_identify(s);
                s->status = READY_STAT | SEEK_STAT;
                ide_transfer_start(s, s->io_buffer, 512, ide_transfer_stop);
            } else {
                ide_abort_command(s);
            }
            ide_set_irq(s);
            break;
        case WIN_DIAGNOSE:
            ide_set_signature(s);
            s->status = 0x00; /* NOTE: READY is _not_ set */
            s->error = 0x01;
            break;
        case WIN_SRST:
            if (!s->is_cdrom)
                goto abort_cmd;
            ide_set_signature(s);
            s->status = 0x00; /* NOTE: READY is _not_ set */
            s->error = 0x01;
            break;
        case WIN_PACKETCMD:
            if (!s->is_cdrom)
                goto abort_cmd;
            /* overlapping commands not supported */
            if (s->feature & 0x02)
                goto abort_cmd;
            s->atapi_dma = s->feature & 1;
            s->nsector = 1;
            ide_transfer_start(s, s->io_buffer, ATAPI_PACKET_SIZE, 
                               ide_atapi_cmd);
            break;
        default:
        abort_cmd:
            ide_abort_command(s);
            ide_set_irq(s);
            break;
        }
    }
}

static uint32_t ide_ioport_read(void *opaque, uint32_t addr1)
{
    IDEState *ide_if = (IDEState*)opaque;
    IDEState *s = (IDEState*)ide_if->cur_drive;
    uint32_t addr;
    int ret, hob;

    addr = addr1 & 7;
    /* FIXME: HOB readback uses bit 7, but it's always set right now */
    //hob = s->select & (1 << 7);
    hob = 0;
    switch(addr) {
    case 0:
        ret = 0xff;
        break;
    case 1:
        if (!ide_if[0].bs && !ide_if[1].bs)
            ret = 0;
        else if (!hob)
            ret = s->error;
	else
	    ret = s->hob_feature;
        break;
    case 2:
        if (!ide_if[0].bs && !ide_if[1].bs)
            ret = 0;
        else if (!hob)
            ret = s->nsector & 0xff;
	else
	    ret = s->hob_nsector;
        break;
    case 3:
        if (!ide_if[0].bs && !ide_if[1].bs)
            ret = 0;
        else if (!hob)
            ret = s->sector;
	else
	    ret = s->hob_sector;
        break;
    case 4:
        if (!ide_if[0].bs && !ide_if[1].bs)
            ret = 0;
        else if (!hob)
            ret = s->lcyl;
	else
	    ret = s->hob_lcyl;
        break;
    case 5:
        if (!ide_if[0].bs && !ide_if[1].bs)
            ret = 0;
        else if (!hob)
            ret = s->hcyl;
	else
	    ret = s->hob_hcyl;
        break;
    case 6:
        if (!ide_if[0].bs && !ide_if[1].bs)
            ret = 0;
        else
            ret = s->select;
        break;
    default:
    case 7:
        if ((!ide_if[0].bs && !ide_if[1].bs) ||
            (s != ide_if && !s->bs))
            ret = 0;
        else
            ret = s->status;
        s->set_irq(s->irq_opaque, s->irq, 0);
        break;
    }
#ifdef DEBUG_IDE
    printf("ide: read addr=0x%x val=%02x\n", addr1, ret);
#endif
    return ret;
}

static uint32_t ide_status_read(void *opaque, uint32_t addr)
{
    IDEState *ide_if =(IDEState*) opaque;
    IDEState *s = (IDEState*)ide_if->cur_drive;
    int ret;

    if ((!ide_if[0].bs && !ide_if[1].bs) ||
        (s != ide_if && !s->bs))
        ret = 0;
    else
        ret = s->status;
#ifdef DEBUG_IDE
    printf("ide: read status addr=0x%x val=%02x\n", addr, ret);
#endif
    return ret;
}

static void ide_cmd_write(void *opaque, uint32_t addr, uint32_t val)
{
    IDEState *ide_if = (IDEState*)opaque;
    IDEState *s;
    int i;

#ifdef DEBUG_IDE
    printf("ide: write control addr=0x%x val=%02x\n", addr, val);
#endif
    /* common for both drives */
    if (!(ide_if[0].cmd & IDE_CMD_RESET) &&
        (val & IDE_CMD_RESET)) {
        /* reset low to high */
        for(i = 0;i < 2; i++) {
            s = &ide_if[i];
            s->status = BUSY_STAT | SEEK_STAT;
            s->error = 0x01;
        }
    } else if ((ide_if[0].cmd & IDE_CMD_RESET) &&
               !(val & IDE_CMD_RESET)) {
        /* high to low */
        for(i = 0;i < 2; i++) {
            s = &ide_if[i];
            if (s->is_cdrom)
                s->status = 0x00; /* NOTE: READY is _not_ set */
            else
                s->status = READY_STAT | SEEK_STAT;
            ide_set_signature(s);
        }
    }

    ide_if[0].cmd = val;
    ide_if[1].cmd = val;
}

static void ide_data_writew(void *opaque, uint32_t addr, uint32_t val)
{
    IDEState *s = ((IDEState *)opaque)->cur_drive;
    uint8_t *p;

    p = s->data_ptr;
    *(uint16_t *)p = le16_to_cpu(val);
    p += 2;
    s->data_ptr = p;
    if (p >= s->data_end)
        s->end_transfer_func(s);
}

static uint32_t ide_data_readw(void *opaque, uint32_t addr)
{
    IDEState *s = ((IDEState *)opaque)->cur_drive;
    uint8_t *p;
    int ret;
    p = s->data_ptr;
    ret = cpu_to_le16(*(uint16_t *)p);
    p += 2;
    s->data_ptr = p;
    if (p >= s->data_end)
        s->end_transfer_func(s);
    return ret;
}

static void ide_data_writel(void *opaque, uint32_t addr, uint32_t val)
{
    IDEState *s = ((IDEState *)opaque)->cur_drive;
    uint8_t *p;

    p = s->data_ptr;
    *(uint32_t *)p = le32_to_cpu(val);
    p += 4;
    s->data_ptr = p;
    if (p >= s->data_end)
        s->end_transfer_func(s);
}

static uint32_t ide_data_readl(void *opaque, uint32_t addr)
{
    IDEState *s = ((IDEState *)opaque)->cur_drive;
    uint8_t *p;
    int ret;
    
    p = s->data_ptr;
    ret = cpu_to_le32(*(uint32_t *)p);
    p += 4;
    s->data_ptr = p;
    if (p >= s->data_end)
        s->end_transfer_func(s);
    return ret;
}

static void ide_dummy_transfer_stop(IDEState *s)
{
    s->data_ptr = s->io_buffer;
    s->data_end = s->io_buffer;
    s->io_buffer[0] = 0xff;
    s->io_buffer[1] = 0xff;
    s->io_buffer[2] = 0xff;
    s->io_buffer[3] = 0xff;
}

static void ide_reset(IDEState *s)
{
    s->mult_sectors = MAX_MULT_SECTORS;
    s->cur_drive = s;
    s->select = 0xa0;
    s->status = READY_STAT;
    ide_set_signature(s);
    /* init the transfer handler so that 0xffff is returned on data
       accesses */
    s->end_transfer_func = ide_dummy_transfer_stop;
    ide_dummy_transfer_stop(s);
}

#pragma pack(1)
struct partition {
	uint8_t boot_ind;		/* 0x80 - active */
	uint8_t head;		/* starting head */
	uint8_t sector;		/* starting sector */
	uint8_t cyl;		/* starting cylinder */
	uint8_t sys_ind;		/* What partition type */
	uint8_t end_head;		/* end head */
	uint8_t end_sector;	/* end sector */
	uint8_t end_cyl;		/* end cylinder */
	uint32_t start_sect;	/* starting sector counting from 0 */
	uint32_t nr_sects;		/* nr of sectors in partition */
} ;

/* try to guess the disk logical geometry from the MSDOS partition table. Return 0 if OK, -1 if could not guess */
static int guess_disk_lchs(IDEState *s, 
                           int *pcylinders, int *pheads, int *psectors)
{
    uint8_t buf[512];
    int ret, i, heads, sectors, cylinders;
    struct partition *p;
    uint32_t nr_sects;

    ret = bdrv_read(s->bs, 0, buf, 1);
    if (ret < 0)
        return -1;
    /* test msdos magic */
    if (buf[510] != 0x55 || buf[511] != 0xaa)
        return -1;
    for(i = 0; i < 4; i++) {
        p = ((struct partition *)(buf + 0x1be)) + i;
        nr_sects = le32_to_cpu(p->nr_sects);
        if (nr_sects && p->end_head) {
            /* We make the assumption that the partition terminates on
               a cylinder boundary */
            heads = p->end_head + 1;
            sectors = p->end_sector & 63;
            if (sectors == 0)
                continue;
            cylinders = s->nb_sectors / (heads * sectors);
            if (cylinders < 1 || cylinders > 16383)
                continue;
            *pheads = heads;
            *psectors = sectors;
            *pcylinders = cylinders;
#if 0
            printf("guessed geometry: LCHS=%d %d %d\n", 
                   cylinders, heads, sectors);
#endif
            return 0;
        }
    }
    return -1;
}

static void ide_init2(IDEState *ide_state,
                      BlockDriverState *hd0, BlockDriverState *hd1,
                      SetIRQFunc *set_irq, void *irq_opaque, int irq)
{
    IDEState *s;
    static int drive_serial = 1;
    int i, cylinders, heads, secs, translation, lba_detected = 0;
    int64_t nb_sectors;

    for(i = 0; i < 2; i++) {
        s = ide_state + i;
        if (i == 0)
            s->bs = hd0;
        else
			s->bs = hd1;
		if (s->bs) 
		{
			bdrv_get_geometry(s->bs, &nb_sectors);
			s->nb_sectors = nb_sectors;
			/* if a geometry hint is available, use it */
			bdrv_get_geometry_hint(s->bs, &cylinders, &heads, &secs);
			translation = bdrv_get_translation_hint(s->bs);
			if (cylinders != 0) {
				s->cylinders = cylinders;
				s->heads = heads;
				s->sectors = secs;
			} 
			else 
			{
				if (guess_disk_lchs(s, &cylinders, &heads, &secs) == 0) 
				{
					if (heads > 16) {
						/* if heads > 16, it means that a BIOS LBA
						translation was active, so the default
						hardware geometry is OK */
						lba_detected = 1;
						goto default_geometry;
					} 
					else
					{
						s->cylinders = cylinders;
						s->heads = heads;
						s->sectors = secs;
						/* disable any translation to be in sync with
						the logical geometry */
						if (translation == BIOS_ATA_TRANSLATION_AUTO) {
							bdrv_set_translation_hint(s->bs,
								BIOS_ATA_TRANSLATION_NONE);
						}
					}
				} 
				else 
				{
default_geometry:
					/* if no geometry, use a standard physical disk geometry */
					cylinders = nb_sectors / (16 * 63);
					if (cylinders > 16383)
						cylinders = 16383;
					else if (cylinders < 2)
						cylinders = 2;
					s->cylinders = cylinders;
					s->heads = 16;
					s->sectors = 63;
					if ((lba_detected == 1) && (translation == BIOS_ATA_TRANSLATION_AUTO)) 
					{
						if ((s->cylinders * s->heads) <= 131072) 
						{
							bdrv_set_translation_hint(s->bs,
								BIOS_ATA_TRANSLATION_LARGE);
						} 
						else 
						{
							bdrv_set_translation_hint(s->bs,
								BIOS_ATA_TRANSLATION_LBA);
						}
					}
				}
				bdrv_set_geometry_hint(s->bs, s->cylinders, s->heads, s->sectors);
			}
			if (bdrv_get_type_hint(s->bs) == BDRV_TYPE_CDROM) {
				s->is_cdrom = 1;
				bdrv_set_change_cb(s->bs, cdrom_change_cb, s);
			}
		}
		s->drive_serial = drive_serial++;
		s->set_irq = set_irq;
		s->irq_opaque = irq_opaque;
		s->irq = irq;
		s->sector_write_timer =0;//* qemu_new_timer(vm_clock, 
		//*              ide_sector_write_timer_cb, s);
		ide_reset(s);
    }
}
#if 0
static void ide_init_ioport(IDEState *ide_state, int iobase, int iobase2)
{
    register_ioport_write(iobase, 8, 1, ide_ioport_write, ide_state);
    register_ioport_read(iobase, 8, 1, ide_ioport_read, ide_state);
    if (iobase2) {
        register_ioport_read(iobase2, 1, 1, ide_status_read, ide_state);
        register_ioport_write(iobase2, 1, 1, ide_cmd_write, ide_state);
    }
    
    /* data ports */
    register_ioport_write(iobase, 2, 2, ide_data_writew, ide_state);
    register_ioport_read(iobase, 2, 2, ide_data_readw, ide_state);
    register_ioport_write(iobase, 4, 4, ide_data_writel, ide_state);
    register_ioport_read(iobase, 4, 4, ide_data_readl, ide_state);
}

/***********************************************************/
/* ISA IDE definitions */

void isa_ide_init(int iobase, int iobase2, int irq,
                  BlockDriverState *hd0, BlockDriverState *hd1)
{
    IDEState *ide_state;

    ide_state = qemu_mallocz(sizeof(IDEState) * 2);
    if (!ide_state)
        return;
    
    ide_init2(ide_state, hd0, hd1, pic_set_irq_new, isa_pic, irq);
    ide_init_ioport(ide_state, iobase, iobase2);
}

/***********************************************************/
/* PCI IDE definitions */

static void cmd646_update_irq(PCIIDEState *d);

static void ide_map(PCIDevice *pci_dev, int region_num, 
                    uint32_t addr, uint32_t size, int type)
{
    PCIIDEState *d = (PCIIDEState *)pci_dev;
    IDEState *ide_state;

    if (region_num <= 3) {
        ide_state = &d->ide_if[(region_num >> 1) * 2];
        if (region_num & 1) {
            register_ioport_read(addr + 2, 1, 1, ide_status_read, ide_state);
            register_ioport_write(addr + 2, 1, 1, ide_cmd_write, ide_state);
        } else {
            register_ioport_write(addr, 8, 1, ide_ioport_write, ide_state);
            register_ioport_read(addr, 8, 1, ide_ioport_read, ide_state);

            /* data ports */
            register_ioport_write(addr, 2, 2, ide_data_writew, ide_state);
            register_ioport_read(addr, 2, 2, ide_data_readw, ide_state);
            register_ioport_write(addr, 4, 4, ide_data_writel, ide_state);
            register_ioport_read(addr, 4, 4, ide_data_readl, ide_state);
        }
    }
}

#endif
static void ide_dma_start(IDEState *s, BlockDriverCompletionFunc *dma_cb)
{
    BMDMAState *bm = s->bmdma;
    if(!bm)
        return;
    bm->ide_if = s;
    bm->dma_cb = dma_cb;
    bm->cur_prd_last = 0;
    bm->cur_prd_addr = 0;
    bm->cur_prd_len = 0;
    if (bm->status & BM_STATUS_DMAING) {
        bm->dma_cb(bm, 0);
    }
}

#if 0
static void bmdma_cmd_writeb(void *opaque, uint32_t addr, uint32_t val)
{
    BMDMAState *bm =(BMDMAState*) opaque;
#ifdef DEBUG_IDE
    printf("%s: 0x%08x\n", __func__, val);
#endif
    if (!(val & BM_CMD_START)) {
        /* XXX: do it better */
        if (bm->status & BM_STATUS_DMAING) {
            bm->status &= ~BM_STATUS_DMAING;
            /* cancel DMA request */
            bm->ide_if = NULL;
            bm->dma_cb = NULL;
            if (bm->aiocb) {
#ifdef DEBUG_AIO
                printf("aio_cancel\n");
#endif
                bdrv_aio_cancel(bm->aiocb);
                bm->aiocb = NULL;
            }
        }
        bm->cmd = val & 0x09;
    } else {
        if (!(bm->status & BM_STATUS_DMAING)) {
            bm->status |= BM_STATUS_DMAING;
            /* start dma transfer if possible */
            if (bm->dma_cb)
                bm->dma_cb(bm, 0);
        }
        bm->cmd = val & 0x09;
    }
}

static uint32_t bmdma_readb(void *opaque, uint32_t addr)
{
    BMDMAState *bm =(BMDMAState*) opaque;
    PCIIDEState *pci_dev;
    uint32_t val;
    
    switch(addr & 3) {
    case 0: 
        val = bm->cmd;
        break;
    case 1:
        pci_dev = bm->pci_dev;
        if (pci_dev->type == IDE_TYPE_CMD646) {
            val = pci_dev->dev.config[MRDMODE];
        } else {
            val = 0xff;
        }
        break;
    case 2:
        val = bm->status;
        break;
    case 3:
        pci_dev = bm->pci_dev;
        if (pci_dev->type == IDE_TYPE_CMD646) {
            if (bm == &pci_dev->bmdma[0])
                val = pci_dev->dev.config[UDIDETCR0];
            else
                val = pci_dev->dev.config[UDIDETCR1];
        } else {
            val = 0xff;
        }
        break;
    default:
        val = 0xff;
        break;
    }
#ifdef DEBUG_IDE
    printf("bmdma: readb 0x%02x : 0x%02x\n", addr, val);
#endif
    return val;
}

static void bmdma_writeb(void *opaque, uint32_t addr, uint32_t val)
{
    BMDMAState *bm = opaque;
    PCIIDEState *pci_dev;
#ifdef DEBUG_IDE
    printf("bmdma: writeb 0x%02x : 0x%02x\n", addr, val);
#endif
    switch(addr & 3) {
    case 1:
        pci_dev = bm->pci_dev;
        if (pci_dev->type == IDE_TYPE_CMD646) {
            pci_dev->dev.config[MRDMODE] = 
                (pci_dev->dev.config[MRDMODE] & ~0x30) | (val & 0x30);
            cmd646_update_irq(pci_dev);
        }
        break;
    case 2:
        bm->status = (val & 0x60) | (bm->status & 1) | (bm->status & ~val & 0x06);
        break;
    case 3:
        pci_dev = bm->pci_dev;
        if (pci_dev->type == IDE_TYPE_CMD646) {
            if (bm == &pci_dev->bmdma[0])
                pci_dev->dev.config[UDIDETCR0] = val;
            else
                pci_dev->dev.config[UDIDETCR1] = val;
        }
        break;
    }
}

static uint32_t bmdma_addr_readl(void *opaque, uint32_t addr)
{
    BMDMAState *bm = opaque;
    uint32_t val;
    val = bm->addr;
#ifdef DEBUG_IDE
    printf("%s: 0x%08x\n", __func__, val);
#endif
    return val;
}

static void bmdma_addr_writel(void *opaque, uint32_t addr, uint32_t val)
{
    BMDMAState *bm = opaque;
#ifdef DEBUG_IDE
    printf("%s: 0x%08x\n", __func__, val);
#endif
    bm->addr = val & ~3;
    bm->cur_addr = bm->addr;
}

static void bmdma_map(PCIDevice *pci_dev, int region_num, 
                    uint32_t addr, uint32_t size, int type)
{
    PCIIDEState *d = (PCIIDEState *)pci_dev;
    int i;

    for(i = 0;i < 2; i++) {
        BMDMAState *bm = &d->bmdma[i];
        d->ide_if[2 * i].bmdma = bm;
        d->ide_if[2 * i + 1].bmdma = bm;
        bm->pci_dev = (PCIIDEState *)pci_dev;

        register_ioport_write(addr, 1, 1, bmdma_cmd_writeb, bm);

        register_ioport_write(addr + 1, 3, 1, bmdma_writeb, bm);
        register_ioport_read(addr, 4, 1, bmdma_readb, bm);

        register_ioport_write(addr + 4, 4, 4, bmdma_addr_writel, bm);
        register_ioport_read(addr + 4, 4, 4, bmdma_addr_readl, bm);
        addr += 8;
    }
}


/* XXX: call it also when the MRDMODE is changed from the PCI config
   registers */
static void cmd646_update_irq(PCIIDEState *d)
{
    int pci_level;
    pci_level = ((d->dev.config[MRDMODE] & MRDMODE_INTR_CH0) &&
                 !(d->dev.config[MRDMODE] & MRDMODE_BLK_CH0)) ||
        ((d->dev.config[MRDMODE] & MRDMODE_INTR_CH1) &&
         !(d->dev.config[MRDMODE] & MRDMODE_BLK_CH1));
    pci_set_irq((PCIDevice *)d, 0, pci_level);
}

/* the PCI irq level is the logical OR of the two channels */
static void cmd646_set_irq(void *opaque, int channel, int level)
{
    PCIIDEState *d = opaque;
    int irq_mask;

    irq_mask = MRDMODE_INTR_CH0 << channel;
    if (level)
        d->dev.config[MRDMODE] |= irq_mask;
    else
        d->dev.config[MRDMODE] &= ~irq_mask;
    cmd646_update_irq(d);
}

/* CMD646 PCI IDE controller */
void pci_cmd646_ide_init(PCIBus *bus, BlockDriverState **hd_table,
                         int secondary_ide_enabled)
{
    PCIIDEState *d;
    uint8_t *pci_conf;
    int i;

    d = (PCIIDEState *)pci_register_device(bus, "CMD646 IDE", 
                                           sizeof(PCIIDEState),
                                           -1, 
                                           NULL, NULL);
    d->type = IDE_TYPE_CMD646;
    pci_conf = d->dev.config;
    pci_conf[0x00] = 0x95; // CMD646
    pci_conf[0x01] = 0x10;
    pci_conf[0x02] = 0x46;
    pci_conf[0x03] = 0x06;

    pci_conf[0x08] = 0x07; // IDE controller revision
    pci_conf[0x09] = 0x8f; 

    pci_conf[0x0a] = 0x01; // class_sub = PCI_IDE
    pci_conf[0x0b] = 0x01; // class_base = PCI_mass_storage
    pci_conf[0x0e] = 0x00; // header_type
    
    if (secondary_ide_enabled) {
        /* XXX: if not enabled, really disable the seconday IDE controller */
        pci_conf[0x51] = 0x80; /* enable IDE1 */
    }

    pci_register_io_region((PCIDevice *)d, 0, 0x8, 
                           PCI_ADDRESS_SPACE_IO, ide_map);
    pci_register_io_region((PCIDevice *)d, 1, 0x4, 
                           PCI_ADDRESS_SPACE_IO, ide_map);
    pci_register_io_region((PCIDevice *)d, 2, 0x8, 
                           PCI_ADDRESS_SPACE_IO, ide_map);
    pci_register_io_region((PCIDevice *)d, 3, 0x4, 
                           PCI_ADDRESS_SPACE_IO, ide_map);
    pci_register_io_region((PCIDevice *)d, 4, 0x10, 
                           PCI_ADDRESS_SPACE_IO, bmdma_map);

    pci_conf[0x3d] = 0x01; // interrupt on pin 1
    
    for(i = 0; i < 4; i++)
        d->ide_if[i].pci_dev = (PCIDevice *)d;
    ide_init2(&d->ide_if[0], hd_table[0], hd_table[1],
              cmd646_set_irq, d, 0);
    ide_init2(&d->ide_if[2], hd_table[2], hd_table[3],
              cmd646_set_irq, d, 1);
}

static void pci_ide_save(QEMUFile* f, void *opaque)
{
    PCIIDEState *d = opaque;
    int i;

    pci_device_save(&d->dev, f);

    for(i = 0; i < 2; i++) {
        BMDMAState *bm = &d->bmdma[i];
        qemu_put_8s(f, &bm->cmd);
        qemu_put_8s(f, &bm->status);
        qemu_put_be32s(f, &bm->addr);
        /* XXX: if a transfer is pending, we do not save it yet */
    }

    /* per IDE interface data */
    for(i = 0; i < 2; i++) {
        IDEState *s = &d->ide_if[i * 2];
        uint8_t drive1_selected;
        qemu_put_8s(f, &s->cmd);
        drive1_selected = (s->cur_drive != s);
        qemu_put_8s(f, &drive1_selected);
    }

    /* per IDE drive data */
    for(i = 0; i < 4; i++) {
        IDEState *s = &d->ide_if[i];
        qemu_put_be32s(f, &s->mult_sectors);
        qemu_put_be32s(f, &s->identify_set);
        if (s->identify_set) {
            qemu_put_buffer(f, (const uint8_t *)s->identify_data, 512);
        }
        qemu_put_8s(f, &s->feature);
        qemu_put_8s(f, &s->error);
        qemu_put_be32s(f, &s->nsector);
        qemu_put_8s(f, &s->sector);
        qemu_put_8s(f, &s->lcyl);
        qemu_put_8s(f, &s->hcyl);
        qemu_put_8s(f, &s->hob_feature);
        qemu_put_8s(f, &s->hob_nsector);
        qemu_put_8s(f, &s->hob_sector);
        qemu_put_8s(f, &s->hob_lcyl);
        qemu_put_8s(f, &s->hob_hcyl);
        qemu_put_8s(f, &s->select);
        qemu_put_8s(f, &s->status);
        qemu_put_8s(f, &s->lba48);

        qemu_put_8s(f, &s->sense_key);
        qemu_put_8s(f, &s->asc);
        /* XXX: if a transfer is pending, we do not save it yet */
    }
}

static int pci_ide_load(QEMUFile* f, void *opaque, int version_id)
{
    PCIIDEState *d = opaque;
    int ret, i;

    if (version_id != 1)
        return -EINVAL;
    ret = pci_device_load(&d->dev, f);
    if (ret < 0)
        return ret;

    for(i = 0; i < 2; i++) {
        BMDMAState *bm = &d->bmdma[i];
        qemu_get_8s(f, &bm->cmd);
        qemu_get_8s(f, &bm->status);
        qemu_get_be32s(f, &bm->addr);
        /* XXX: if a transfer is pending, we do not save it yet */
    }

    /* per IDE interface data */
    for(i = 0; i < 2; i++) {
        IDEState *s = &d->ide_if[i * 2];
        uint8_t drive1_selected;
        qemu_get_8s(f, &s->cmd);
        qemu_get_8s(f, &drive1_selected);
        s->cur_drive = &d->ide_if[i * 2 + (drive1_selected != 0)];
    }

    /* per IDE drive data */
    for(i = 0; i < 4; i++) {
        IDEState *s = &d->ide_if[i];
        qemu_get_be32s(f, &s->mult_sectors);
        qemu_get_be32s(f, &s->identify_set);
        if (s->identify_set) {
            qemu_get_buffer(f, (uint8_t *)s->identify_data, 512);
        }
        qemu_get_8s(f, &s->feature);
        qemu_get_8s(f, &s->error);
        qemu_get_be32s(f, &s->nsector);
        qemu_get_8s(f, &s->sector);
        qemu_get_8s(f, &s->lcyl);
        qemu_get_8s(f, &s->hcyl);
        qemu_get_8s(f, &s->hob_feature);
        qemu_get_8s(f, &s->hob_nsector);
        qemu_get_8s(f, &s->hob_sector);
        qemu_get_8s(f, &s->hob_lcyl);
        qemu_get_8s(f, &s->hob_hcyl);
        qemu_get_8s(f, &s->select);
        qemu_get_8s(f, &s->status);
        qemu_get_8s(f, &s->lba48);

        qemu_get_8s(f, &s->sense_key);
        qemu_get_8s(f, &s->asc);
        /* XXX: if a transfer is pending, we do not save it yet */
    }
    return 0;
}

static void piix3_reset(PCIIDEState *d)
{
    uint8_t *pci_conf = d->dev.config;

    pci_conf[0x04] = 0x00;
    pci_conf[0x05] = 0x00;
    pci_conf[0x06] = 0x80; /* FBC */
    pci_conf[0x07] = 0x02; // PCI_status_devsel_medium
    pci_conf[0x20] = 0x01; /* BMIBA: 20-23h */
}

void pci_piix_ide_init(PCIBus *bus, BlockDriverState **hd_table, int devfn)
{
    PCIIDEState *d;
    uint8_t *pci_conf;
    
    /* register a function 1 of PIIX */
    d = (PCIIDEState *)pci_register_device(bus, "PIIX IDE", 
                                           sizeof(PCIIDEState),
                                           devfn,
                                           NULL, NULL);
    d->type = IDE_TYPE_PIIX3;

    pci_conf = d->dev.config;
    pci_conf[0x00] = 0x86; // Intel
    pci_conf[0x01] = 0x80;
    pci_conf[0x02] = 0x30;
    pci_conf[0x03] = 0x12;
    pci_conf[0x08] = 0x02; // Step A1
    pci_conf[0x09] = 0x80; // legacy ATA mode
    pci_conf[0x0a] = 0x01; // class_sub = PCI_IDE
    pci_conf[0x0b] = 0x01; // class_base = PCI_mass_storage
    pci_conf[0x0e] = 0x00; // header_type

    piix3_reset(d);

    pci_register_io_region((PCIDevice *)d, 4, 0x10, 
                           PCI_ADDRESS_SPACE_IO, bmdma_map);

    ide_init2(&d->ide_if[0], hd_table[0], hd_table[1],
              pic_set_irq_new, isa_pic, 14);
    ide_init2(&d->ide_if[2], hd_table[2], hd_table[3],
              pic_set_irq_new, isa_pic, 15);
    ide_init_ioport(&d->ide_if[0], 0x1f0, 0x3f6);
    ide_init_ioport(&d->ide_if[2], 0x170, 0x376);

    register_savevm("ide", 0, 1, pci_ide_save, pci_ide_load, d);
}

/* hd_table must contain 4 block drivers */
/* NOTE: for the PIIX3, the IRQs and IOports are hardcoded */
void pci_piix3_ide_init(PCIBus *bus, BlockDriverState **hd_table, int devfn)
{
    PCIIDEState *d;
    uint8_t *pci_conf;
    
    /* register a function 1 of PIIX3 */
    d = (PCIIDEState *)pci_register_device(bus, "PIIX3 IDE", 
                                           sizeof(PCIIDEState),
                                           devfn,
                                           NULL, NULL);
    d->type = IDE_TYPE_PIIX3;

    pci_conf = d->dev.config;
    pci_conf[0x00] = 0x86; // Intel
    pci_conf[0x01] = 0x80;
    pci_conf[0x02] = 0x10;
    pci_conf[0x03] = 0x70;
    pci_conf[0x09] = 0x80; // legacy ATA mode
    pci_conf[0x0a] = 0x01; // class_sub = PCI_IDE
    pci_conf[0x0b] = 0x01; // class_base = PCI_mass_storage
    pci_conf[0x0e] = 0x00; // header_type

    piix3_reset(d);

    pci_register_io_region((PCIDevice *)d, 4, 0x10, 
                           PCI_ADDRESS_SPACE_IO, bmdma_map);

    ide_init2(&d->ide_if[0], hd_table[0], hd_table[1],
              pic_set_irq_new, isa_pic, 14);
    ide_init2(&d->ide_if[2], hd_table[2], hd_table[3],
              pic_set_irq_new, isa_pic, 15);
    ide_init_ioport(&d->ide_if[0], 0x1f0, 0x3f6);
    ide_init_ioport(&d->ide_if[2], 0x170, 0x376);

    register_savevm("ide", 0, 1, pci_ide_save, pci_ide_load, d);
}

/***********************************************************/
/* MacIO based PowerPC IDE */

/* PowerMac IDE memory IO */
static void pmac_ide_writeb (void *opaque,
                             target_phys_addr_t addr, uint32_t val)
{
    addr = (addr & 0xFFF) >> 4; 
    switch (addr) {
    case 1 ... 7:
        ide_ioport_write(opaque, addr, val);
        break;
    case 8:
    case 22:
        ide_cmd_write(opaque, 0, val);
        break;
    default:
        break;
    }
}

static uint32_t pmac_ide_readb (void *opaque,target_phys_addr_t addr)
{
    uint8_t retval;

    addr = (addr & 0xFFF) >> 4;
    switch (addr) {
    case 1 ... 7:
        retval = ide_ioport_read(opaque, addr);
        break;
    case 8:
    case 22:
        retval = ide_status_read(opaque, 0);
        break;
    default:
        retval = 0xFF;
        break;
    }
    return retval;
}

static void pmac_ide_writew (void *opaque,
                             target_phys_addr_t addr, uint32_t val)
{
    addr = (addr & 0xFFF) >> 4; 
#ifdef TARGET_WORDS_BIGENDIAN
    val = bswap16(val);
#endif
    if (addr == 0) {
        ide_data_writew(opaque, 0, val);
    }
}

static uint32_t pmac_ide_readw (void *opaque,target_phys_addr_t addr)
{
    uint16_t retval;

    addr = (addr & 0xFFF) >> 4; 
    if (addr == 0) {
        retval = ide_data_readw(opaque, 0);
    } else {
        retval = 0xFFFF;
    }
#ifdef TARGET_WORDS_BIGENDIAN
    retval = bswap16(retval);
#endif
    return retval;
}

static void pmac_ide_writel (void *opaque,
                             target_phys_addr_t addr, uint32_t val)
{
    addr = (addr & 0xFFF) >> 4; 
#ifdef TARGET_WORDS_BIGENDIAN
    val = bswap32(val);
#endif
    if (addr == 0) {
        ide_data_writel(opaque, 0, val);
    }
}

static uint32_t pmac_ide_readl (void *opaque,target_phys_addr_t addr)
{
    uint32_t retval;

    addr = (addr & 0xFFF) >> 4; 
    if (addr == 0) {
        retval = ide_data_readl(opaque, 0);
    } else {
        retval = 0xFFFFFFFF;
    }
#ifdef TARGET_WORDS_BIGENDIAN
    retval = bswap32(retval);
#endif
    return retval;
}

static CPUWriteMemoryFunc *pmac_ide_write[] = {
    pmac_ide_writeb,
    pmac_ide_writew,
    pmac_ide_writel,
};

static CPUReadMemoryFunc *pmac_ide_read[] = {
    pmac_ide_readb,
    pmac_ide_readw,
    pmac_ide_readl,
};

/* hd_table must contain 4 block drivers */
/* PowerMac uses memory mapped registers, not I/O. Return the memory
   I/O index to access the ide. */
int pmac_ide_init (BlockDriverState **hd_table,
                   SetIRQFunc *set_irq, void *irq_opaque, int irq)
{
    IDEState *ide_if;
    int pmac_ide_memory;

    ide_if = qemu_mallocz(sizeof(IDEState) * 2);
    ide_init2(&ide_if[0], hd_table[0], hd_table[1],
              set_irq, irq_opaque, irq);
    
    pmac_ide_memory = cpu_register_io_memory(0, pmac_ide_read,
                                             pmac_ide_write, &ide_if[0]);
    return pmac_ide_memory;
}

#endif
#define ATA_R_DATA			(ATA_DEV9_HDD_BASE + 0x00)
#define ATA_R_ERROR			(ATA_DEV9_HDD_BASE + 0x02)
#define ATA_R_NSECTOR		(ATA_DEV9_HDD_BASE + 0x04)
#define ATA_R_SECTOR		(ATA_DEV9_HDD_BASE + 0x06)
#define ATA_R_LCYL			(ATA_DEV9_HDD_BASE + 0x08)
#define ATA_R_HCYL			(ATA_DEV9_HDD_BASE + 0x0a)
#define ATA_R_SELECT		(ATA_DEV9_HDD_BASE + 0x0c)
#define ATA_R_STATUS		(ATA_DEV9_HDD_BASE + 0x0e)
#define ATA_R_CONTROL		(ATA_DEV9_HDD_BASE + 0x1c)

//Feature -> error
//command -> status

//data : 1,2,4
//command : 1
IDEState ps2_hdd_data;
BMDMAState ps2_hdd_bmdma_data;
BlockDriverState ps2_hdd_bds_data;

#define ps2_hdd (&ps2_hdd_data)
void dev9_ata_irq(void *opaque, int irq_num, int level)
{
	emu_printf("ATA INTERRUPT level %d\n",level);
	if (level)
	{
		_DEV9irq(ATA_DEV9_INT,0);
	}
	else
	{
		dev9.irqcause&=~ATA_DEV9_INT;
		_DEV9irq(0,0);
	}
		
}
HANDLE HddFile;
void ata_init()
{
	HddFile = ::CreateFile("d:\\ps2_hdd.raw", GENERIC_READ|GENERIC_WRITE,
		0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD dwTemp;
	::DeviceIoControl(HddFile, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dwTemp, NULL);
	LARGE_INTEGER sp;
	
	//20 GBs
	/*
	//~7 GB
	sp.LowPart=0xEFFFFFFF;
	sp.HighPart=1;
	*/
	sp.QuadPart=(u64)20*1000*1000*1000;

	int64_t rcc=(sp.QuadPart/512/16/63)+1;

	ps2_hdd_bds_data.total_sectors=rcc*16*63;
	sp.QuadPart=ps2_hdd_bds_data.total_sectors*512;

	ps2_hdd_bds_data.secs=0;
	ps2_hdd_bds_data.heads=0;
	ps2_hdd_bds_data.cyls=0;

	emu_printf("Hdd size #.2fGB\n",(double)sp.QuadPart/1024/1024/1024);

	::SetFilePointerEx(HddFile, sp, 0, FILE_BEGIN);
	::SetEndOfFile(HddFile);


	ide_init2(ps2_hdd,&ps2_hdd_bds_data,0,dev9_ata_irq,0,0);
	ps2_hdd->bmdma=&ps2_hdd_bmdma_data;
}
void ata_term()
{
	::CloseHandle(HddFile);
}
#define LOG_IRS (sz!=2)
template<int sz>
u8 CALLBACK ata_read(u32 addr)
{
	//emu_printf("ata_read%d(0x%X)\n",sz*8,addr);
	switch(addr)
	{
	case ATA_R_DATA:
		if (sz==1)
		{
			emu_printf("ATA: Invalid ATA_R_DATA size: register cannot be readen using 8 bit\n");
		}
		else if (sz==2)
		{
			return ide_data_readw(ps2_hdd,0);
		}
		else
		{
			return ide_data_readl(ps2_hdd,0);
		}
		break;
	case ATA_R_ERROR:
		if (sz!=1 && LOG_IRS)
		{
			emu_printf("ATA: Invalid ATA_R_ERROR size: register cannot be read w/o using 8 bit\n");
		}
		//else
		{
			return ide_ioport_read(ps2_hdd,1);
		}
		break;
	case ATA_R_NSECTOR:
		if (sz!=1 && LOG_IRS)
		{
			emu_printf("ATA: Invalid ATA_R_NSECTOR size: register cannot be read w/o using 8 bit\n");
		}
		//else
		{
			return ide_ioport_read(ps2_hdd,2);
		}
		break;
	case ATA_R_SECTOR:
		if (sz!=1 && LOG_IRS)
		{
			emu_printf("ATA: Invalid ATA_R_SECTOR size: register cannot be read w/o using 8 bit\n");
		}
		//else
		{
			return ide_ioport_read(ps2_hdd,3);
		}
		break;
	case ATA_R_LCYL:
		if (sz!=1 && LOG_IRS)
		{
			emu_printf("ATA: Invalid ATA_R_LCYL size: register cannot be read w/o using 8 bit\n");
		}
		//else
		{
			return ide_ioport_read(ps2_hdd,4);
		}
		break;
	case ATA_R_HCYL:
		if (sz!=1 && LOG_IRS)
		{
			emu_printf("ATA: Invalid ATA_R_HCYL size: register cannot be read w/o using 8 bit\n");
		}
		//else
		{
			return ide_ioport_read(ps2_hdd,5);
		}
		break;
	case ATA_R_SELECT:
		if (sz!=1 && LOG_IRS)
		{
			emu_printf("ATA: Invalid ATA_R_SELECT size: register cannot be read w/o using 8 bit\n");
		}
		//else
		{
			return ide_ioport_read(ps2_hdd,6);
		}
		break;
		//command -> status
	case ATA_R_STATUS:
		if (sz!=1 && LOG_IRS)
		{
			emu_printf("ATA: Invalid ATA_R_STATUS size: register cannot be read w/o using 8 bit\n");
		}
		//else
		{
			return ide_ioport_read(ps2_hdd,7);
		}
		break;
	case ATA_R_CONTROL:
		//ide_cmd -> seems to be control
		if (sz!=1 && LOG_IRS)
		{
			emu_printf("ATA: Invalid ATA_R_STATUS size: register cannot be read w/o using 8 bit\n");
		}
		//else
		{
		return ide_status_read(ps2_hdd,0);
		}
		break;
	default:
		DEV9_LOG("ATA: Unknown %d bit read @ %X,v=%X\n",sz*8,addr,dev9Ru8(addr));
		if (sz==1)
		{
			return dev9Ru8(addr);
		}
		else if (sz==2)
		{
			return dev9Ru16(addr);
		}
		else
		{
			return dev9Ru32(addr);
		}
	}
	
	DEV9_LOG("ATA: Unknown %d bit read @ %X,v=%X\n",sz*8,addr,0xdeadb33f);
	return 0xdeadb33f;
}
#define LOG_IWS (value&~0xFF)
template<int sz>
void CALLBACK ata_write(u32 addr, u32 value)
{
	//emu_printf("ata_write%d(0x%X,0x%X)\n",sz*8,addr,value);
	switch(addr)
	{
	case ATA_R_DATA:
		if (sz==1)
		{
			emu_printf("ATA: Invalid ATA_R_DATA size: register cannot be writen using 8 bit\n");
		}
		else if (sz==2)
		{
			ide_data_writew(ps2_hdd,0,value);
		}
		else
		{
			ide_data_writel(ps2_hdd,0,value);
		}
		break;
	case ATA_R_ERROR:
		if (sz!=1 && LOG_IWS)
		{
			emu_printf("ATA: Invalid ATA_R_ERROR(feature) size: register cannot be writen w/o using 8 bit\n");
		}
		//else
		{
			ide_ioport_write(ps2_hdd,1,value);
		}
		break;
	case ATA_R_NSECTOR:
		if (sz!=1 && LOG_IWS)
		{
			emu_printf("ATA: Invalid ATA_R_NSECTOR size: register cannot be writen w/o using 8 bit\n");
		}
		//else
		{
			ide_ioport_write(ps2_hdd,2,value);
		}
		break;
	case ATA_R_SECTOR:
		if (sz!=1 && LOG_IWS)
		{
			emu_printf("ATA: Invalid ATA_R_SECTOR size: register cannot be writen w/o using 8 bit\n");
		}
		//else
		{
			ide_ioport_write(ps2_hdd,3,value);
		}
		break;
	case ATA_R_LCYL:
		if (sz!=1 && LOG_IWS)
		{
			emu_printf("ATA: Invalid ATA_R_LCYL size: register cannot be writen w/o using 8 bit\n");
		}
		//else
		{
			ide_ioport_write(ps2_hdd,4,value);
		}
		break;
	case ATA_R_HCYL:
		if (sz!=1 && LOG_IWS)
		{
			emu_printf("ATA: Invalid ATA_R_HCYL size: register cannot be writen w/o using 8 bit\n");
		}
		//else
		{
			ide_ioport_write(ps2_hdd,5,value);
		}
		break;
	case ATA_R_SELECT:
		if (sz!=1 && LOG_IWS)
		{
			emu_printf("ATA: Invalid ATA_R_SELECT size: register cannot be writen w/o using 8 bit\n");
		}
		//else
		{
			ide_ioport_write(ps2_hdd,6,value);
		}
		break;
		//command -> status
	case ATA_R_STATUS:
		if (sz!=1 && LOG_IWS)
		{
			emu_printf("ATA: Invalid ATA_R_STATUS(command) size: register cannot be writen w/o using 8 bit\n");
		}
		//else
		{
			ide_ioport_write(ps2_hdd,7,value);
		}
		break;
	case ATA_R_CONTROL:
		//ide_cmd -> seems to be control
		if (sz!=1 && LOG_IWS)
		{
			emu_printf("ATA: Invalid ATA_R_CONTROL size: register cannot be writen w/o using 8 bit\n");
		}
	//	else
		{
			ide_cmd_write(ps2_hdd,7,value);
		}
		break;
	default:
		DEV9_LOG("ATA: Unknown %d bit write @ %X,v=%X\n",addr,value);
		if (sz==1)
		{
			dev9Ru8(addr)=value;
		}
		else if (sz==2)
		{
			dev9Ru16(addr)=value;
		}
		else
		{
			dev9Ru32(addr)=value;
		}
	}
}

u8* pMem_dma;
void do_dma(int size)
{
	BMDMAState* bm=ps2_hdd->bmdma;
	bm->addr=0;
	bm->cur_prd_addr = 0;
	bm->cur_prd_len = size;

	if (!(bm->status & BM_STATUS_DMAING)) 
	{
		bm->status |= BM_STATUS_DMAING;
		/* start dma transfer if possible */
		if (bm->dma_cb)
			bm->dma_cb(bm, 0);
		else
			emu_printf("DMA ERROR 2 !!!!@#!@#!@#!#!@#!@#$####################################################^#^\n");
		//bm->status&=~BM_STATUS_DMAING;
	}
	else
	{
		emu_printf("DMA ERROR 1 !!!!@#!@#!@#!#!@#!@#$####################################################^#^\n");
	}

	_DEV9irq(ATA_DEV9_INT_DMA,0);
}
void CALLBACK ata_readDMA8Mem(u32 *pMem, int size)
{
	size>>=1;
	//#define SPD_R_IF_CTRL			(SPD_REGBASE + 0x64)
	//#define   SPD_IF_ATA_RESET	0x80
	//#define   SPD_IF_DMA_ENABLE	0x04
	pMem_dma=(u8*)pMem;
	if (dev9Ru8(SPD_R_IF_CTRL)&SPD_IF_DMA_ENABLE)
	{
		emu_printf("ATA: ata_readDMA8Mem(0x%X,%d) :D\n",pMem,size);
		do_dma(size);
		//bm->cmd = val & 0x09;
	}
	else
	{
		emu_printf("ATA: ata_readDMA8Mem & SPD_IF_DMA_ENABLE disabled\n");
	}
}
void CALLBACK ata_writeDMA8Mem(u32 *pMem, int size)
{
	size>>=1;
	pMem_dma=(u8*)pMem;
	if (dev9Ru8(SPD_R_IF_CTRL)&SPD_IF_DMA_ENABLE)
	{
		emu_printf("ATA: ata_writeDMA8Mem(0x%X,%d) :D\n",pMem,size);
		do_dma(size);
		//bm->cmd = val & 0x09;
	}
	else
	{
		emu_printf("ATA: ata_writeDMA8Mem & SPD_IF_DMA_ENABLE disabled\n");
	}
}
void _template_hack_()
{
	ata_read<1>(0);
	ata_read<2>(0);
	ata_read<4>(0);

	ata_write<1>(0,0);
	ata_write<2>(0,0);
	ata_write<4>(0,0);
}
#define printme emu_printf("Called stub:" __FUNCTION__ "\n");

//memory access
void __cdecl cpu_physical_memory_write(u32 addr,void* ptr,u32 sz)
{
	printf("cpu_physical_memory_write(0x%X,0x%X,%d)\n",addr,ptr,sz);
	//return;
	memcpy(pMem_dma+addr,ptr,sz);
}
void __cdecl cpu_physical_memory_read(u32 addr,void* ptr,u32 sz)
{
	printf("cpu_physical_memory_read(0x%X,0x%X,%d)\n",addr,ptr,sz);
	//return;
	memcpy(ptr,pMem_dma+addr,sz);
}

//IMAGE IO


//Async io emulation =)
BlockDriverAIOCB *bdrv_aio_read(BlockDriverState *bs,
        int64_t sector_num, uint8_t *buf, int nb_sectors,
        BlockDriverCompletionFunc *cb, void *opaque)
{
	//printme
    int ret;
    ret = bdrv_read(bs, sector_num, buf, nb_sectors);
    cb(opaque, ret);
	return 0;
}
BlockDriverAIOCB *bdrv_aio_write(BlockDriverState *bs,
        int64_t sector_num, const uint8_t *buf, int nb_sectors,
        BlockDriverCompletionFunc *cb, void *opaque)
{
    //printme
	int ret;
    ret = bdrv_write(bs, sector_num, buf, nb_sectors);
    cb(opaque, ret);
	return 0;
}

void bdrv_flush(BlockDriverState *bs)
{
	printme;
	//return;
	FlushFileBuffers(HddFile);
}
int bdrv_read(BlockDriverState *bs, int64_t sector_num, 
              uint8_t *buf, int nb_sectors)
{
	//printme;
	//return 0;
	if ((sector_num+nb_sectors)>bs->total_sectors)
	{
		emu_printf("ATA: ERROR , ((sector_num+nb_sectors)>bs->total_sectors)\n");
		return -1;
	}
	DWORD rv;
	LARGE_INTEGER sp;
	sp.QuadPart=sector_num*512;

	BOOL sfp=SetFilePointerEx(HddFile,sp,0,FILE_BEGIN);
	if (!sfp)
	{
		emu_printf("SetFilePointerEx file failed\n");
		return -1;
	}

	BOOL ss=ReadFile(HddFile,buf,nb_sectors*512,&rv,0);
	if (!ss)
	{
		emu_printf("ReadFile file failed - %d\n",GetLastError());
		return -1;
	}
	emu_printf("Readed %d bytes from %d:%d@file\n",rv,sp.HighPart,sp.LowPart);
	return 0;
}
int bdrv_write(BlockDriverState *bs, int64_t sector_num, 
               const uint8_t *buf, int nb_sectors)
{
	//printme;
	//return 0;
	if ((sector_num+nb_sectors)>bs->total_sectors)
	{
		emu_printf("ATA: ERROR , ((sector_num+nb_sectors)>bs->total_sectors)\n");
		return -1;
	}
	DWORD rv;
	LARGE_INTEGER sp;
	sp.QuadPart=sector_num*512;

	BOOL sfp=SetFilePointerEx(HddFile,sp,0,FILE_BEGIN);
	if (!sfp)
	{
		emu_printf("SetFilePointerEx file failed\n");
		return -1;
	}

	BOOL ss=WriteFile(HddFile,buf,nb_sectors*512,&rv,0);
	if (!ss)
	{
		emu_printf("WriteFile file failed - %d\n",GetLastError());
		return -1;
	}
	emu_printf("Writen %d bytes to %d:%d@file\n",rv,sp.HighPart,sp.LowPart);
	return 0;
}

void __cdecl bdrv_eject(struct BlockDriverState *,int)
{
	printme;
}
void bdrv_get_geometry(BlockDriverState *bs, int64_t *nb_sectors_ptr)
{
	printme;
	//return;
	*nb_sectors_ptr=bs->total_sectors;
}
void bdrv_set_locked(BlockDriverState *bs, int locked)
{
    printme;
	//return;
    bs->locked = locked;
}

int __cdecl bdrv_is_locked(struct BlockDriverState *bs)
{
	printme;
	return bs->locked;
}
int __cdecl bdrv_is_inserted(struct BlockDriverState *)
{
	printme;
	return 1;
}

void __cdecl bdrv_set_change_cb(struct BlockDriverState *,void (__cdecl*)(void *),void *)
{
	printme
}
int __cdecl bdrv_get_type_hint(struct BlockDriverState *)
{
	printme;
	return BDRV_TYPE_HD;
}
void bdrv_set_geometry_hint(BlockDriverState *bs, 
                            int cyls, int heads, int secs)
{
	printme;
	//return;
    bs->cyls = cyls;
    bs->heads = heads;
    bs->secs = secs;
}
void bdrv_set_translation_hint(BlockDriverState *bs, int translation)
{
	printme;
    bs->translation = translation;
}
int bdrv_get_translation_hint(BlockDriverState *bs)
{
	printme;
    //return 0;
	return bs->translation;
}

void bdrv_get_geometry_hint(BlockDriverState *bs, 
                            int *pcyls, int *pheads, int *psecs)
{
	printme;
	//return;
    *pcyls = bs->cyls;
    *pheads = bs->heads;
    *psecs = bs->secs;
}