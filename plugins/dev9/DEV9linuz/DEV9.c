#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "DEV9.h"
#include "socks.h"
#include "Config.h"

#ifdef __WIN32__
#pragma warning(disable:4244)

HINSTANCE hInst=NULL;
#endif

//#define HDD_48BIT

const u8 eeprom[] = {
	0x00, // MAC ADDR: 00:0A:E6:71:55:34
/*	0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x10, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x10,
	0x10, 0x10, 0x10, 0x00, 0x00, 0x10, 0x10, 0x00,
	0x00, 0x00, 0x10, 0x10, 0x00, 0x10, 0x00, 0x00,
	0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10,
	0x10, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x10, 0x10, 0x10, 0x00, 0x10, 0x10,*/
	0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

char ata_cmds[256][256];
void (*ata_cmdf[256])(int irq);

#define INIT_ATA_CMD(num, name, func) \
	strcpy(ata_cmds[num], name); \
	ata_cmdf[num] = func;

void ata_unknown(int irq);
void ata_nop(int irq);
void ata_recalibrate(int irq);
void ata_readsec(int irq);
void ata_readdma(int irq);
void ata_readdma_ext(int irq);
void ata_writedma(int irq);
void ata_writedma_ext(int irq);
void ata_specify(int irq);
void ata_flushcache(int irq);
void ata_identify(int irq);
void ata_pidentify(int irq);
void ata_setidle(int irq);
void ata_setfeatures(int irq);
void ata_smart(int irq);
void ata_specify(int irq);

int Log = 1;

const unsigned char version  = PS2E_DEV9_VERSION;
const unsigned char revision = 0;
const unsigned char build    = 2;    // increase that with each version

static char *libraryName     = "DEV9linuz Driver";

u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_DEV9;
}

char* CALLBACK PS2EgetLibName() {
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type) {
	return (version<<16) | (revision<<8) | build;
}

void __Log(char *fmt, ...) {
	va_list list;

//	if (!Log) return;

	va_start(list, fmt);
	vfprintf(dev9Log, fmt, list);
	va_end(list);
}

#ifdef __WIN32__
void _set_sparse(void *handle) {
	DWORD bytes=0;
	printf("%d\n", DeviceIoControl(handle, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &bytes, NULL));
	printf("%d\n", GetLastError());
}

static void *_openfile(const char *filename, int flags) {
	void *ret;
//	printf("_openfile %s: %d;%d\n", filename, flags & O_RDONLY, flags & O_RDWR);
	if (flags & O_RDWR) {
		ret = CreateFile(filename, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (ret == INVALID_HANDLE_VALUE) {
			ret = CreateFile(filename, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_NEW, 0, NULL);
			if (ret == INVALID_HANDLE_VALUE) return NULL;
		}
	} else
	if (flags & O_WRONLY) {
		ret = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
		if (ret == INVALID_HANDLE_VALUE) return NULL;
	} else {
		ret = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (ret == INVALID_HANDLE_VALUE) return NULL;
	}
	_set_sparse(ret);
	return ret;
}

static u64 _tellfile(void *handle) {
	u64 ofs;
	u32 *_ofs = (u32*)&ofs;
	_ofs[1] = 0;
	_ofs[0] = SetFilePointer(handle, 0, &_ofs[1], FILE_CURRENT);
	DEV9_LOG("_tellfile %p, %x_%x\n", handle, _ofs[1], _ofs[0]);
	return ofs;
}

static int _seekfile(void *handle, u64 offset) {
	u64 ofs = (u64)offset;
	u32 *_ofs = (u32*)&ofs;
	DEV9_LOG("_seekfile %p, %x_%x\n", handle, _ofs[1], _ofs[0]);
	SetFilePointer(handle, _ofs[0], &_ofs[1], FILE_BEGIN);
	return 0;
}
/*
static void dump(u8 *ptr, int size) {
	int i;
	DEV9_LOG("dump: ");
	for (i=0; i<size; i++) {
		if ((i%8) == 0) {
			DEV9_LOG("\n");
		}
		DEV9_LOG("%2.2x, ", *ptr++);
	}
	DEV9_LOG("\n");
}
*/
static int _readfile(void *handle, u8 *dst, int size) {
	u32 ret;

//	DEV9_LOG("_readfile size=%d\n", size);
//	printf("_readfile %p %d, %d\n", handle, offset, size);
	ReadFile(handle, dst, size, &ret, NULL);
//	dump(dst, size);
	
//	printf("_readfile ret %d; %d\n", ret, GetLastError());
	return ret;
}

static int _writefile(void *handle, u8 *src, int size) {
	u32 ret;

//	DEV9_LOG("_writefile size=%d\n", size);
//	printf("_writefile %p, %d\n", handle, size);
//	dump(src, size);
	WriteFile(handle, src, size, &ret, NULL);
//	printf("_writefile ret %d\n", ret);
	return ret;
}

static void _closefile(void *handle) {
	CloseHandle(handle);
}

#else

void *_openfile(const char *filename, int flags) {
	if (flags & O_RDONLY)
		 return fopen(filename, "rb");
	else return fopen(filename, "wb");
}

int _readfile(void *handle, u8 *dest, u64 offset, int size) {
	fseek(handle, offset, SEEK_SET);
	return fread(dest, 1, size, f);
}

void _closefile(void *handle) {
	fclose(handle);
}

#endif


s32 CALLBACK DEV9init() {
	int i;

#ifdef DEV9_LOG
	dev9Log = fopen("dev9Log.txt", "w");
	setvbuf(dev9Log, NULL,  _IONBF, 0);
	DEV9_LOG("DEV9init\n");
#endif

	memset(&dev9, 0, sizeof(dev9));
#ifdef DEV9_LOG
	DEV9_LOG("DEV9init2\n");
#endif

	ata_hwport = (ata_hwport_t*)&dev9.dev9R[0x40];
#ifdef DEV9_LOG
	DEV9_LOG("DEV9init3\n");
#endif

	for (i=0; i<256; i++) {
		INIT_ATA_CMD(i, "unknown", ata_unknown);
	}
	INIT_ATA_CMD(0x00, "nop", ata_nop);
	INIT_ATA_CMD(0x10, "recalibrate", ata_recalibrate);
	INIT_ATA_CMD(0x20, "read sector", ata_readsec);
	INIT_ATA_CMD(0x25, "readdma_ext", ata_readdma_ext);
	INIT_ATA_CMD(0x35, "writedma_ext", ata_writedma_ext);
	INIT_ATA_CMD(0x91, "specify", ata_specify);
	INIT_ATA_CMD(0x97, "set idle", ata_setidle);
	INIT_ATA_CMD(0xA1, "pidentify", ata_pidentify);
	INIT_ATA_CMD(0xB0, "smart", ata_smart);
	INIT_ATA_CMD(0xC8, "readdma", ata_readdma);
	INIT_ATA_CMD(0xCA, "writedma", ata_writedma);
	INIT_ATA_CMD(0xE3, "set idle", ata_setidle);
	INIT_ATA_CMD(0xE7, "flush cache", ata_flushcache);
	INIT_ATA_CMD(0xEA, "flush cache ext", ata_flushcache);
	INIT_ATA_CMD(0xEC, "identify", ata_identify);
	INIT_ATA_CMD(0xEF, "set features", ata_setfeatures);

	FLASHinit();
	DVRinit();
	
#ifdef DEV9_LOG
	DEV9_LOG("DEV9init ok\n");
#endif

	return 0;
}

void CALLBACK DEV9shutdown() {
#ifdef DEV9_LOG
	fclose(dev9Log);
#endif
}

s32 CALLBACK DEV9open(void *pDsp) {
#ifdef DEV9_LOG
	DEV9_LOG("DEV9open\n");
#endif
	LoadConf();
#ifdef DEV9_LOG
	DEV9_LOG("open r+: %s\n", config.Hdd);
#endif
	config.HddSize = 8*1024;

	hdd = _openfile(config.Hdd, O_RDWR);
#ifdef DEV9_LOG
	DEV9_LOG("_openfile %p\n", hdd);
#endif
	if (hdd == NULL) {
		SysMessage("Could not open '%s'", config.Hdd);
		return -1;
	}
	return _DEV9open();
}

void CALLBACK DEV9close() {
	_DEV9close();
	_closefile(hdd);
}

int CALLBACK _DEV9irqHandler(void) {
	if (dev9.irqcause & 0x1) { // ata_cmd complete
		ata_hwport->r_status&= ~ATA_STAT_BUSY;
		ata_cmdf[dev9.atacmd](1);
	}
	dev9Ru16(SPD_R_INTR_STAT)|= dev9.irqcause;
#ifdef DEV9_LOG
	DEV9_LOG("_DEV9irqHandler %x, %x\n", dev9Ru16(SPD_R_INTR_STAT), dev9Ru16(SPD_R_INTR_MASK));
#endif
	if (dev9Ru16(SPD_R_INTR_STAT) & dev9Ru16(SPD_R_INTR_MASK)) return 1;
	return 0;
}

DEV9handler CALLBACK DEV9irqHandler(void) {
	return (DEV9handler)_DEV9irqHandler;
}

void _DEV9irq(int cause, int cycles) {
#ifdef DEV9_LOG
	DEV9_LOG("_DEV9irq %x, %x\n", cause, dev9Ru16(SPD_R_INTR_MASK));
#endif

	dev9.irqcause|= cause;
	DEV9irq(cycles);
}


u8   CALLBACK DEV9read8(u32 addr) {
	u8 hard;

	switch (addr) {
		case SPD_R_PIO_DATA:
			hard = dev9.eeprom[dev9.eeprompos++];
			break;

		case DEV9_R_REV:
			hard = 0x30; // expansion bay
			break;

		case ATA_R_CONTROL:
		case ATA_R_STATUS:
		case ATA_R_NSECTOR:
			return DEV9read16(addr);

		default:
			if ((addr >= FLASH_REGBASE) && (addr < (FLASH_REGBASE + FLASH_REGSIZE))) {
				return (u8)FLASHread32(addr, 1);
			}
			if ((addr >= DVR_REGBASE) && (addr < (DVR_REGBASE + DVR_REGSIZE))) {
				return (u8)DVRread32(addr, 1);
			}

			hard = dev9Ru8(addr); 
#ifdef DEV9_LOG
			DEV9_LOG("*Unknown 8bit read at address %lx value %x\n", addr, hard);
#endif
			return hard;
	}
	
#ifdef DEV9_LOG
	DEV9_LOG("*Known 8bit read at address %lx value %x\n", addr, hard);
#endif
	return hard;
}

u16  CALLBACK DEV9read16(u32 addr) {
	u16 hard;

	switch (addr) {
#ifdef DEV9_LOG
		case SMAP_R_EMAC3_MODE0_L:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_MODE0_L 16bit read %x\n", hard);
			return hard;

		case SMAP_R_EMAC3_MODE0_H:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_MODE0_H 16bit read %x\n", hard);
			return hard;

		case SMAP_R_EMAC3_MODE1_L:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_MODE1_L 16bit read %x\n", hard);
			return hard;

		case SMAP_R_EMAC3_MODE1_H:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_MODE1_H 16bit read %x\n", hard);
			return hard;

		case SMAP_R_EMAC3_RxMODE_L:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_RxMODE_L 16bit read %x\n", hard);
			return hard;

		case SMAP_R_EMAC3_RxMODE_H:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_RxMODE_H 16bit read %x\n", hard);
			return hard;

		case SMAP_R_EMAC3_INTR_STAT_L:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_INTR_STAT_L 16bit read %x\n", hard);
			return hard;

		case SMAP_R_EMAC3_INTR_STAT_H:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_INTR_STAT_H 16bit read %x\n", hard);
			return hard;

		case SMAP_R_EMAC3_INTR_ENABLE_L:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_INTR_ENABLE_L 16bit read %x\n", hard);
			return hard;

		case SMAP_R_EMAC3_INTR_ENABLE_H:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_INTR_ENABLE_H 16bit read %x\n", hard);
			return hard;

		case SMAP_R_EMAC3_TxMODE0_L:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_TxMODE0_L 16bit read %x\n", hard);
			return hard;

		case SMAP_R_EMAC3_TxMODE0_H:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_TxMODE0_H 16bit read %x\n", hard);
			return hard;

		case SMAP_R_EMAC3_TxMODE1_L:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_TxMODE1_L 16bit read %x\n", hard);
			return hard;

		case SMAP_R_EMAC3_TxMODE1_H:
			hard = dev9Ru16(addr);
			DEV9_LOG("SMAP_R_EMAC3_TxMODE1_H 16bit read %x\n", hard);
			return hard;
#endif

/*		case SPD_R_INTR_STAT:
			return 0xff;
*/
		case DEV9_R_REV:
			hard = 0x0030; // expansion bay
			break;

		case SPD_R_REV_1:
			hard = 0x0011;
#ifdef DEV9_LOG
			DEV9_LOG("STD_R_REV_1 16bit read %x\n", hard);
#endif
			return hard;

		case SPD_R_REV_3:
			// bit 0: smap
			// bit 1: hdd
			// bit 5: flash
			hard = 0;
			if (config.hddEnable) {
				hard|= 0x2;
			}
			if (config.ethEnable) {
				hard|= 0x1;
			}
			hard|= 0x10 | 0x20;//dvr | flash
			break;

		case SPD_R_0e:
			hard = 0x0002;
			break;

		case ATA_R_CONTROL:
			hard = 0x0040; // we're always ready :D
#ifdef DEV9_LOG
			DEV9_LOG("ATA_R_CONTROL 16bit read %x\n", hard);
#endif
			return hard;

		case ATA_R_DATA:
			hard = dev9.atabuf[dev9.atacount++]; dev9.atasize--;
			if (dev9.atasize <= 0) {
				ata_hwport->r_status&= ~ATA_STAT_DRQ;
			}
#ifdef DEV9_LOG
			DEV9_LOG("ATA_R_DATA 16bit read %x\n", hard);
#endif
			return hard;

		case ATA_R_STATUS:
			if (dev9Ru16(ATA_R_SELECT) & 0x10) {
				hard = 0x0001;
			} else {
				hard = ata_hwport->r_status | 0x0040; // we're always ready :D
			}
#ifdef DEV9_LOG
			DEV9_LOG("ATA_R_STATUS 16bit read %x\n", hard);
#endif
			return hard;

		case ATA_R_NSECTOR:
			if ((dev9Ru16(ATA_R_SELECT) >> 4) != 0) {
				hard = 0x0000;
			} else {
				hard = 0x0001;
			}
#ifdef DEV9_LOG
			DEV9_LOG("ATA_R_NSECTOR 16bit read %x\n", hard);
#endif
			return hard;

		default:
			if (addr >= SMAP_BD_TX_BASE && addr < (SMAP_BD_TX_BASE + SMAP_BD_SIZE)) {
				switch (addr & 0x7) {
					case 0: // ctrl_stat
						hard = dev9Ru16(addr);
#ifdef DEV9_LOG
						DEV9_LOG("TX_CTRL_STAT[%d]: read %x\n", (addr - SMAP_BD_TX_BASE) / 8, hard);
#endif
						return hard;
					case 2: // unknown
						hard = dev9Ru16(addr);
#ifdef DEV9_LOG
						DEV9_LOG("TX_UNKNOWN[%d]: read %x\n", (addr - SMAP_BD_TX_BASE) / 8, hard);
#endif
						return hard;
					case 4: // length
						hard = dev9Ru16(addr);
#ifdef DEV9_LOG
						DEV9_LOG("TX_LENGTH[%d]: read %x\n", (addr - SMAP_BD_TX_BASE) / 8, hard);
#endif
						return hard;
					case 6: // pointer
						hard = dev9Ru16(addr);
#ifdef DEV9_LOG
						DEV9_LOG("TX_POINTER[%d]: read %x\n", (addr - SMAP_BD_TX_BASE) / 8, hard);
#endif
						return hard;
				}
			}
			if (addr >= SMAP_BD_RX_BASE && addr < (SMAP_BD_RX_BASE + SMAP_BD_SIZE)) {
				switch (addr & 0x7) {
					case 0: // ctrl_stat
						hard = dev9Ru16(addr);
#ifdef DEV9_LOG
						DEV9_LOG("RX_CTRL_STAT[%d]: read %x\n", (addr - SMAP_BD_RX_BASE) / 8, hard);
#endif
						return hard;
					case 2: // unknown
						hard = dev9Ru16(addr);
#ifdef DEV9_LOG
						DEV9_LOG("RX_UNKNOWN[%d]: read %x\n", (addr - SMAP_BD_RX_BASE) / 8, hard);
#endif
						return hard;
					case 4: // length
						hard = dev9Ru16(addr);
#ifdef DEV9_LOG
						DEV9_LOG("RX_LENGTH[%d]: read %x\n", (addr - SMAP_BD_RX_BASE) / 8, hard);
#endif
						return hard;
					case 6: // pointer
						hard = dev9Ru16(addr);
#ifdef DEV9_LOG
						DEV9_LOG("RX_POINTER[%d]: read %x\n", (addr - SMAP_BD_RX_BASE) / 8, hard);
#endif
						return hard;
				}
			}

			if ((addr >= FLASH_REGBASE) && (addr < (FLASH_REGBASE + FLASH_REGSIZE))) {
				return (u16)FLASHread32(addr, 2);
			}
			if ((addr >= DVR_REGBASE) && (addr < (DVR_REGBASE + DVR_REGSIZE))) {
				return (u16)DVRread32(addr, 2);
			}

			hard = dev9Ru16(addr); 
#ifdef DEV9_LOG
			DEV9_LOG("*Unknown 16bit read at address %lx value %x\n", addr, hard);
#endif
			return hard;
	}
	
#ifdef DEV9_LOG
	DEV9_LOG("*Known 16bit read at address %lx value %x\n", addr, hard);
#endif
	return hard;
}

u32  CALLBACK DEV9read32(u32 addr) {
	u32 hard;

	switch (addr) {
		case SMAP_R_RXFIFO_DATA:
			hard = *(u32*)&dev9.buffer[dev9Ru32(SMAP_R_RXFIFO_RD_PTR)];
			dev9Ru32(SMAP_R_RXFIFO_RD_PTR)+= 4;
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_RXFIFO_DATA 32bit read %x\n", hard);
#endif
			return hard;

		default:
			if ((addr >= FLASH_REGBASE) && (addr < (FLASH_REGBASE + FLASH_REGSIZE))) {
				return (u32)FLASHread32(addr, 4);
			}
			if ((addr >= DVR_REGBASE) && (addr < (DVR_REGBASE + DVR_REGSIZE))) {
				return (u32)DVRread32(addr, 4);
			}

			hard = dev9Ru32(addr); 
#ifdef DEV9_LOG
			DEV9_LOG("*Unkwnown 32bit read at address %lx\n", addr);
#endif
			return hard;
	}

#ifdef DEV9_LOG
	DEV9_LOG("*Known 32bit read at address %lx: %lx\n", addr, hard);
#endif
	return hard;
}

void CALLBACK DEV9write8(u32 addr,  u8 value) {
	switch (addr) {
		case 0x10000020:
			dev9Ru16(SPD_R_INTR_STAT) = 0xff;
			break;

		case ATA_R_STATUS:
			DEV9write16(addr, value);
			return;

		case SMAP_R_RXFIFO_FRAME_DEC:
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_RXFIFO_FRAME_DEC 8bit write %x\n", value);
#endif
//			dev9Ru8(addr) = 0; //  blah :P
			dev9Ru8(addr) = value;
			return;

		case SPD_R_PIO_DIR:
#ifdef DEV9_LOG
			DEV9_LOG("SPD_R_PIO_DIR 8bit write %x\n", value);
#endif
			if (value == 0xe1) dev9.eeprompos = 0;
			if (value == 0xe0) dev9.eeprompos = 0;
			return;

		case SPD_R_PIO_DATA:
#ifdef DEV9_LOG
			DEV9_LOG("SPD_R_PIO_DATA 8bit write %x\n", value);
#endif
			dev9Ru8(addr) = value;
			return;

		case SMAP_R_TXFIFO_CTRL:
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_TXFIFO_CTRL 8bit write %x\n", value);
#endif
			value&= ~SMAP_TXFIFO_RESET;
			return;

		case SMAP_R_RXFIFO_CTRL:
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_RXFIFO_CTRL 8bit write %x\n", value);
#endif
			value&= ~SMAP_RXFIFO_RESET;
			return;

#ifdef DEV9_LOG
		case ATA_R_DATA:
			DEV9_LOG("ATA_R_DATA 8bit write %x\n", value);
			dev9Ru8(addr) = value;
			return;

		case ATA_R_ERROR:
			DEV9_LOG("ATA_R_ERROR 8bit write %x\n", value);
			dev9Ru8(addr) = value;
			return;

		case ATA_R_NSECTOR:
			DEV9_LOG("ATA_R_NSECTOR 8bit write %x\n", value);
			dev9Ru8(addr) = value;
			return;

		case ATA_R_SECTOR:
			DEV9_LOG("ATA_R_SECTOR 8bit write %x\n", value);
			dev9Ru8(addr) = value;
			return;

		case ATA_R_LCYL:
			DEV9_LOG("ATA_R_LCYC 8bit write %x\n", value);
			dev9Ru8(addr) = value;
			return;

		case ATA_R_HCYL:
			DEV9_LOG("ATA_R_HCYC 8bit write %x\n", value);
			dev9Ru8(addr) = value;
			return;

		case ATA_R_SELECT:
			DEV9_LOG("ATA_R_SELECT 8bit write %x\n", value);
			dev9Ru8(addr) = value;
			return;

		case ATA_R_CONTROL:
			DEV9_LOG("ATA_R_CONTROL 8bit write %x\n", value);
			dev9Ru8(addr) = value;
			return;
#endif

		default:
			if ((addr >= FLASH_REGBASE) && (addr < (FLASH_REGBASE + FLASH_REGSIZE))) {
				FLASHwrite32(addr, (u32)value, 1);
				return;
			}
			if ((addr >= DVR_REGBASE) && (addr < (DVR_REGBASE + DVR_REGSIZE))) {
				DVRwrite32(addr, (u32)value, 1);
				dev9Ru8(addr) = value;
				return;
			}

			dev9Ru8(addr) = value;
#ifdef DEV9_LOG
			DEV9_LOG("*Unknown 8bit write at address %lx value %x\n", addr, value);
#endif
			return;
	}
	dev9Ru8(addr) = value;
#ifdef DEV9_LOG
	DEV9_LOG("*Known 8bit write at address %lx value %x\n", addr, value);
#endif
}

u32 _ata_getlba() {
	u32 lba;

	if (ata_hwport->r_select & 0x40) {
		lba = (ata_hwport->r_sector & 0xff);
		lba|= (ata_hwport->r_lcyl   & 0xff) <<  8;
		lba|= (ata_hwport->r_hcyl   & 0xff) << 16;
#ifdef HDD_48BIT
		lba|= ((ata_hwport->r_sector  >> 8) & 0xff) << 24;
#else
		lba|= (ata_hwport->r_select & 0x0f) << 24;
#endif
	} else {
#ifdef DEV9_LOG
		DEV9_LOG("fixme: lba not set\n");
#endif
		return -1;
	}

	return lba;
}

int _ata_seek() {
	u32 lba;

	lba = _ata_getlba();
#ifdef DEV9_LOG
	DEV9_LOG("_ata_seek: lba=%x\n", lba);
#endif
	if (lba == -1) return -1;
	if (_seekfile(hdd, (u64)lba*512) != 0) {
#ifdef DEV9_LOG
		DEV9_LOG("fseek error: %s\n", strerror(errno));
#endif
		return -1;
	}

	return 0;
}


void ata_unknown(int irq) {
#ifdef DEV9_LOG
	DEV9_LOG("ata_unknown\n");
#endif
	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);

	dev9Ru16(ATA_R_STATUS)|= ATA_STAT_ERR;
	dev9Ru16(ATA_R_ERROR) = 0x01;
}

void ata_nop(int irq) {
	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);
}

void ata_recalibrate(int irq) {
	if (irq) { ata_hwport->r_status&= ~ATA_STAT_BUSY; return; }

	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);

	ata_hwport->r_status|= ATA_STAT_BUSY;
	_DEV9irq(0x1, 0x1000);
}

void ata_readsec(int irq) {
	if (irq) {
		if (_seekfile(hdd, dev9.atasector*512) != 0) {
#ifdef DEV9_LOG
			DEV9_LOG("fseek error: %s\n", strerror(errno));
#endif
			dev9Ru16(ATA_R_STATUS)|= ATA_STAT_ERR;
			dev9Ru16(ATA_R_ERROR) = 0x01;
			return;
		}
		dev9.atasize = 512;
		dev9.atacount = 0;
#ifdef DEV9_LOG
		DEV9_LOG("read 512 bytes from %x\n", ftell(hdd) / 512);
#endif
		if (_readfile(hdd, dev9.atabuf, 512) == 0) {
			memset(dev9.atabuf, 0, 512);
//			printf("fread error: %s\n", strerror(errno)); return;
		}

		ata_hwport->r_status|= ATA_STAT_DRQ;
		dev9.atasector++;
		dev9.atansector--;
		if (dev9.atansector > 0) {
			_DEV9irq(0x1, 0x1000);
		}

		return;
	}

	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);

	ata_hwport->r_status|= ATA_STAT_BUSY;
	dev9.atansector = ata_hwport->r_nsector == 0 ? 256 : ata_hwport->r_nsector;
	dev9.atasector = _ata_getlba();
	if (dev9.atasector == -1) {
		dev9Ru16(ATA_R_STATUS)|= ATA_STAT_ERR;
		dev9Ru16(ATA_R_ERROR) = 0x01;
		return;
	}

	_DEV9irq(0x1, 0x1000);
}

void ata_specify(int irq) {
	if (irq) { ata_hwport->r_status&= ~ATA_STAT_BUSY; return; }

	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);

	ata_hwport->r_status|= ATA_STAT_BUSY;
	_DEV9irq(0x1, 0x1000);
}

void ata_flushcache(int irq) {
	if (irq) { ata_hwport->r_status&= ~ATA_STAT_BUSY; return; }

	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);

	ata_hwport->r_status|= ATA_STAT_BUSY;
	_DEV9irq(0x1, 0x1000);
}

void ata_readdma(int irq) {
	if (irq) { ata_hwport->r_status&= ~ATA_STAT_BUSY; return; }

	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);

	ata_hwport->r_status|= ATA_STAT_BUSY;
	_DEV9irq(0x1, 0x1000);
}

void ata_readdma_ext(int irq) {
	if (irq) { ata_hwport->r_status&= ~ATA_STAT_BUSY; return; }

	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);

	ata_hwport->r_status|= ATA_STAT_BUSY;
	_DEV9irq(0x1, 0x1000);
}

void ata_writedma(int irq) {
	if (irq) { ata_hwport->r_status&= ~ATA_STAT_BUSY; return; }

	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);

	ata_hwport->r_status|= ATA_STAT_BUSY;
	_DEV9irq(0x1, 0x1000);
}

void ata_writedma_ext(int irq) {
	if (irq) { ata_hwport->r_status&= ~ATA_STAT_BUSY; return; }

	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);

	ata_hwport->r_status|= ATA_STAT_BUSY;
	_DEV9irq(0x1, 0x1000);
}

void ata_pidentify(int irq) {
	if (irq) { ata_hwport->r_status&= ~ATA_STAT_BUSY; return; }

	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);

	ata_hwport->r_status|= ATA_STAT_BUSY;
	_DEV9irq(0x1, 0x1000);
}

void ata_identify(int irq) {
	struct hd_driveid *id = (struct hd_driveid *)dev9.atabuf;

	if (irq) { ata_hwport->r_status&= ~ATA_STAT_BUSY; return; }

	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);

	memset(id, 0, sizeof(struct hd_driveid));
	id->lba_capacity = config.HddSize * 1024*2;
	id->cyls = (id->lba_capacity/16)/63;
	id->heads = 16;
	id->sectors = 63;

#ifdef HDD_48BIT
	id->command_set_2|= 1<<10; // 48bit
	id->lba_capacity_2 = (u64)config.HddSize * 1024*2;
#endif
#ifdef DEV9_LOG
	DEV9_LOG("id->lba_capacity=%x\n", id->lba_capacity);
	DEV9_LOG("id->lba_capacity2=%x\n", id->lba_capacity_2);
#endif

	id->capability = 0x3; // DMA | LBA

	id->model[1] = 'D';
	id->model[0] = 'E';
	id->model[3] = 'V';
	id->model[2] = '9';
	id->model[5] = ' ';
	id->model[4] = 'H';
	id->model[7] = 'D';
	id->model[6] = 'D';
	dev9.atasize = 256; dev9.atacount = 0;
	ata_hwport->r_status|= ATA_STAT_DRQ | ATA_STAT_BUSY;
	_DEV9irq(0x1, 0x1000);
}

void ata_setidle(int irq) {
	if (irq) { ata_hwport->r_status&= ~ATA_STAT_BUSY; return; }

	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);

	ata_hwport->r_status|= ATA_STAT_BUSY;
	_DEV9irq(0x1, 0x1000);
}

void ata_setfeatures(int irq) {
	if (irq) { ata_hwport->r_status&= ~ATA_STAT_BUSY; return; }

	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);

	ata_hwport->r_status|= ATA_STAT_BUSY;
	_DEV9irq(0x1, 0x1000);
}

void ata_smart(int irq) {
#ifdef DEV9_LOG
	DEV9_LOG("ata_smart: irq=%x r_error=%x\n", irq, ata_hwport->r_error);
#endif
	if (irq) { ata_hwport->r_status&= ~ATA_STAT_BUSY; return; }

	switch (ata_hwport->r_error) { // no IO
		case SMART_ENABLE:
			_DEV9irq(0x1, 0x1000);
			break;

		default:
#ifdef DEV9_LOG
		DEV9_LOG("ata_smart: unknown cmd %x\n", ata_hwport->r_error);
#endif
	}

	ata_hwport->r_error  = 0;
	ata_hwport->r_status&= ~(ATA_STAT_DRQ | ATA_STAT_ERR);
}

void ata_cmd(u8 value) {
#ifdef DEV9_LOG
	DEV9_LOG("ata_cmd: %s (%2.2x)\n", ata_cmds[value], value);
#endif

	dev9.atacmd = value;
	ata_cmdf[dev9.atacmd](0);
}

void CALLBACK DEV9write16(u32 addr, u16 value) {
	switch (addr) {
		case ATA_R_STATUS:
#ifdef DEV9_LOG
			DEV9_LOG("ATA_R_STATUS 16bit write %x\n", value);
#endif
			ata_cmd(value);
			return;

		case SMAP_R_INTR_CLR:
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_INTR_CLR 16bit write %x\n", value);
#endif
			dev9Ru16(SPD_R_INTR_STAT)&= ~value;
			return;

		case SMAP_R_TXFIFO_WR_PTR:
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_TXFIFO_WR_PTR 16bit write %x\n", value);
#endif
			dev9Ru16(addr) = value;
			return;
		case SMAP_R_EMAC3_MODE0_L:
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_EMAC3_MODE0_L 16bit write %x\n", value);
#endif
			dev9Ru16(addr) = (value & (~SMAP_E3_SOFT_RESET)) | SMAP_E3_TXMAC_IDLE | SMAP_E3_RXMAC_IDLE;
			dev9Ru16(SMAP_R_EMAC3_STA_CTRL_H)|= SMAP_E3_PHY_OP_COMP;
			return;
		case SMAP_R_EMAC3_MODE0_H:
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_EMAC3_MODE0_H 16bit write %x\n", value);
#endif
			dev9Ru16(addr) = value;
			return;
		case SMAP_R_EMAC3_TxMODE0_L:
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_EMAC3_TxMODE0_L 16bit write %x\n", value);
#endif
			dev9Ru16(addr) = value & ~SMAP_E3_TX_GNP_0;
			return;
		case SMAP_R_EMAC3_TxMODE0_H:
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_EMAC3_TxMODE0_H 16bit write %x\n", value);
#endif
			dev9Ru16(addr) = value;
			return;
		case SMAP_R_EMAC3_TxMODE1_L:
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_EMAC3_TxMODE1_L 16bit write %x\n", value);
#endif
			dev9Ru16(addr) = value;
			return;
		case SMAP_R_EMAC3_TxMODE1_H:
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_EMAC3_TxMODE1_H 16bit write %x\n", value);
#endif
			dev9Ru16(addr) = value;
			return;
		case SMAP_R_EMAC3_STA_CTRL_H:
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_EMAC3_STA_CTRL_H 16bit write %x\n", value);
#endif
			value|= SMAP_E3_PHY_OP_COMP;
			if (value & (SMAP_E3_PHY_READ)) {
				int reg = value & (SMAP_E3_PHY_REG_ADDR_MSK);
				u16 val = dev9.phyregs[reg];
				switch (reg) {
					case SMAP_DsPHYTER_BMSR:
						val|= SMAP_PHY_BMSR_LINK | SMAP_PHY_BMSR_ANCP;
						break;
					case SMAP_DsPHYTER_PHYSTS:
						val|= SMAP_PHY_STS_LINK | SMAP_PHY_STS_ANCP;
						break;
				}
#ifdef DEV9_LOG
				DEV9_LOG("phy_read %d: %x\n", reg, val);
#endif
				dev9Ru16(SMAP_R_EMAC3_STA_CTRL_L) = val;
			} else 
			if (value & (SMAP_E3_PHY_WRITE)) {
				int reg = value & (SMAP_E3_PHY_REG_ADDR_MSK);
				u16 val = dev9Ru16(SMAP_R_EMAC3_STA_CTRL_L);
				switch (reg) {
					case SMAP_DsPHYTER_BMCR:
						val&= ~SMAP_PHY_BMCR_RST;
						val|= 0x1;
						break;
				}
#ifdef DEV9_LOG
				DEV9_LOG("phy_write %d: %x\n", reg, val);
#endif
				dev9.phyregs[reg] = val;
			}
			break;
		default:
			if (addr >= SMAP_BD_TX_BASE && addr < (SMAP_BD_TX_BASE + SMAP_BD_SIZE)) {
				switch (addr & 0x7) {
					case 0: // ctrl_stat
#ifdef DEV9_LOG
						DEV9_LOG("TX_CTRL_STAT[%d]: write %x\n", (addr - SMAP_BD_TX_BASE) / 8, value);
#endif
						if (value & SMAP_BD_TX_READY) {
							u32 base = (SMAP_REGBASE + dev9Ru16(addr + 6)) & 0xffff;

#ifdef DEV9_LOG
							DEV9_LOG("sockSendData: base %x, size %d\n", base, dev9Ru16(addr + 4));
#endif
							sockSendData(&dev9.dev9R[base], dev9Ru16(addr + 4));
							_DEV9irq(SMAP_INTR_TXEND, 0x1000);
							value&= ~SMAP_BD_TX_READY;
						}
						dev9Ru16(addr) = value;
						return;
					case 2: // unknown
#ifdef DEV9_LOG
						DEV9_LOG("TX_UNKNOWN[%d]: write %x\n", (addr - SMAP_BD_TX_BASE) / 8, value);
#endif
						dev9Ru16(addr) = value;
						return;
					case 4: // length
#ifdef DEV9_LOG
						DEV9_LOG("TX_LENGTH[%d]: write %x\n", (addr - SMAP_BD_TX_BASE) / 8, value);
#endif
						dev9Ru16(addr) = value;
						return;
					case 6: // pointer
#ifdef DEV9_LOG
						DEV9_LOG("TX_POINTER[%d]: write %x\n", (addr - SMAP_BD_TX_BASE) / 8, value);
#endif
						dev9Ru16(addr) = value;
						return;
				}
			}
			if (addr >= SMAP_BD_RX_BASE && addr < (SMAP_BD_RX_BASE + SMAP_BD_SIZE)) {
				switch (addr & 0x7) {
					case 0: // ctrl_stat
#ifdef DEV9_LOG
						DEV9_LOG("RX_CTRL_STAT[%d]: write %x\n", (addr - SMAP_BD_RX_BASE) / 8, value);
#endif
						dev9Ru16(addr) = value;
						return;
					case 2: // unknown
#ifdef DEV9_LOG
						DEV9_LOG("RX_UNKNOWN[%d]: write %x\n", (addr - SMAP_BD_RX_BASE) / 8, value);
#endif
						dev9Ru16(addr) = value;
						return;
					case 4: // length
#ifdef DEV9_LOG
						DEV9_LOG("RX_LENGTH[%d]: write %x\n", (addr - SMAP_BD_RX_BASE) / 8, value);
#endif
						dev9Ru16(addr) = value;
						return;
					case 6: // pointer
#ifdef DEV9_LOG
						DEV9_LOG("RX_POINTER[%d]: write %x\n", (addr - SMAP_BD_RX_BASE) / 8, value);
#endif
						dev9Ru16(addr) = value;
						return;
				}
			}

			if ((addr >= FLASH_REGBASE) && (addr < (FLASH_REGBASE + FLASH_REGSIZE))) {
				FLASHwrite32(addr, (u32)value, 2);
				return;
			}
			if ((addr >= DVR_REGBASE) && (addr < (DVR_REGBASE + DVR_REGSIZE))) {
				DVRwrite32(addr, (u32)value, 2);
				dev9Ru16(addr) = value;
				return;
			}

			dev9Ru16(addr) = value;
#ifdef DEV9_LOG
			DEV9_LOG("*Unknown 16bit write at address %lx value %x\n", addr, value);
#endif
			return;
	}
	dev9Ru16(addr) = value;
#ifdef DEV9_LOG
	DEV9_LOG("*Known 16bit write at address %lx value %x\n", addr, value);
#endif
}

void CALLBACK DEV9write32(u32 addr, u32 value) {
	switch (addr) {
		case SMAP_R_TXFIFO_DATA:
#ifdef DEV9_LOG
			DEV9_LOG("SMAP_R_TXFIFO_DATA 32bit write %x (ptr %x)\n", value, SMAP_TX_BASE + dev9Ru32(SMAP_R_TXFIFO_WR_PTR));
#endif
			dev9Ru32(SMAP_TX_BASE + dev9Ru32(SMAP_R_TXFIFO_WR_PTR)) = value;
			dev9Ru32(SMAP_R_TXFIFO_WR_PTR)+= 4;
			return;
		default:
			if ((addr >= FLASH_REGBASE) && (addr < (FLASH_REGBASE + FLASH_REGSIZE))) {
				FLASHwrite32(addr, (u32)value, 4);
				return;
			}
			if ((addr >= DVR_REGBASE) && (addr < (DVR_REGBASE + DVR_REGSIZE))) {
				DVRwrite32(addr, (u32)value, 4);
				dev9Ru32(addr) = value;
				return;
			}

			dev9Ru32(addr) = value;
#ifdef DEV9_LOG
			DEV9_LOG("*Unknown 32bit write at address %lx write %x\n", addr, value);
#endif
			return;
	}
	dev9Ru32(addr) = value;
#ifdef DEV9_LOG
	DEV9_LOG("*Known 32bit write at address %lx value %lx\n", addr, value);
#endif
}

void CALLBACK DEV9readDMA8Mem(u32 *pMem, int size) {
#ifdef DEV9_LOG
	DEV9_LOG("*DEV9readDMA8Mem: size %x\n", size);
#endif

	if (_ata_seek() == -1) return;
#ifdef DEV9_LOG
	DEV9_LOG("read %x bytes from %x\n", size, _tellfile(hdd) / 512);
#endif

	if (_readfile(hdd, pMem, size) == 0) {
#ifdef DEV9_LOG
		DEV9_LOG("fread error: %s\n", strerror(errno));
#endif
		memset(pMem, 0, size);
		return;
	}
}

void CALLBACK DEV9writeDMA8Mem(u32* pMem, int size) {
#ifdef DEV9_LOG
	DEV9_LOG("*DEV9writeDMA8Mem: size %x\n", size);
#endif

	if (_ata_seek() == -1) return;
#ifdef DEV9_LOG
	DEV9_LOG("write %x bytes from %x\n", size, _tellfile(hdd) / 512);
#endif

	if (_writefile(hdd, pMem, size) == 0) {
#ifdef DEV9_LOG
		DEV9_LOG("fwrite error: %s\n", strerror(errno));
#endif
		return;
	}
}

void CALLBACK DEV9irqCallback(void (*callback)(int cycles)) {
	DEV9irq = callback;
}


// extended funcs

s32  CALLBACK DEV9test() {
	return 0;
}

void DEV9thread() {
	smap_bd_t *pbd;
	int bytes;
	int rxbi = 0;

	if (config.ethEnable == 0) return;

//	if (sockOpen("d9n0") == -1) {
	if (sockOpen(config.Eth) == -1) {
		SysMessage("Can't open Device '%s'\n", config.Eth);
		return;
	}
	ThreadRun = 1;
	while (ThreadRun) {
#ifdef DEV9_LOG
		DEV9_LOG("sockRecvData\n");
#endif
		bytes = sockRecvData(dev9.buffer, sizeof(dev9.buffer));
#ifdef DEV9_LOG
		DEV9_LOG("sockRecvData: %d\n", bytes);
#endif
		if (bytes <= 0) continue;

		pbd = (smap_bd_t *)&dev9.dev9R[SMAP_BD_RX_BASE & 0xffff];
		pbd = &pbd[rxbi];
		if (!(pbd->ctrl_stat & SMAP_BD_RX_EMPTY)) {
#ifdef DEV9_LOG
			DEV9_LOG("Discarding %d bytes (RX%d not ready)\n", bytes, rxbi);
#endif
			printf("Discarding %d bytes (RX%d not ready)\n", bytes, rxbi);
			continue;
		}
#ifdef DEV9_LOG
		DEV9_LOG("DEV9received %d bytes, rxbi %d\n", bytes, rxbi);
#endif

		rxbi++;
		if (rxbi == (SMAP_BD_SIZE/8)) rxbi = 0;
		
		pbd->ctrl_stat&= ~SMAP_BD_RX_EMPTY;
		pbd->length = bytes;
//		dev9Ru8(SMAP_R_RXFIFO_FRAME_DEC) = 1; // guessing ;)
		_DEV9irq(SMAP_INTR_RXEND, 0);
	}
}

