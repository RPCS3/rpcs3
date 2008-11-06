#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <Winioctl.h>
#include <fcntl.h>
#include <windows.h>
#include <stdarg.h>

#include "smap.h"
#include "net.h"
#include "pcap.h"
#include "pcap_io.h"
#include "tap.h"

bool has_link=true;
/*
#define	SMAP_BASE			0xb0000000
#define	SMAP_REG8(Offset)		(*(u8 volatile*)(SMAP_BASE+(Offset)))
#define	SMAP_REG16(Offset)	(*(u16 volatile*)(SMAP_BASE+(Offset)))
#define	SMAP_REG32(Offset)	(*(u32 volatile*)(SMAP_BASE+(Offset)))

u32 EMAC3REG_READ(u32 u32Offset)
{
        u32	hi=SMAP_REG16(u32Offset);
        u32	lo=SMAP_REG16(u32Offset+2);
        return	(hi<<16)|lo;
}


void EMAC3REG_WRITE(u32 u32Offset,u32 u32V)
{
        SMAP_REG16(u32Offset)=((u32V>>16)&0xFFFF);
        SMAP_REG16(u32Offset+2)=(u32V&0xFFFF);
}
#define	SMAP_EMAC3_BASE	0x2000
#define	SMAP_EMAC3_STA_CTRL		(SMAP_EMAC3_BASE+0x5C)
void test()
{
	printf ("EMAC3R 0x%08X raw read 0x%08X\n",EMAC3REG_READ(SMAP_EMAC3_STA_CTRL),SMAP_REG32(SMAP_EMAC3_STA_CTRL));
}*/

//this can return a false positive, but its not problem since it may say it cant recv while it can (no harm done, just delay on packets)
bool rx_fifo_can_rx()
{
	//check if RX is on & stuff like that here
	
	//Check if there is space on RXBD
	if (dev9Ru8(SMAP_R_RXFIFO_FRAME_CNT)==64)
		return false;
	
	//Check if there is space on fifo
	int rd_ptr = dev9Ru32(SMAP_R_RXFIFO_RD_PTR);
	int space = sizeof(dev9.rxfifo) -
		((dev9.rxfifo_wr_ptr-rd_ptr)&16383);


	if(space==0)
		space = sizeof(dev9.rxfifo);

	if (space<1514)
		return false;

	//we can recv a packet !
	return true;
}
void rx_process(NetPacket* pk)
{
	if (!rx_fifo_can_rx())
	{
		emu_printf("ERROR : !rx_fifo_can_rx at rx_process\n");
		return;
	}
	smap_bd_t *pbd= ((smap_bd_t *)&dev9.dev9R[SMAP_BD_RX_BASE & 0xffff])+dev9.rxbdi;

	int bytes=(pk->size+3)&(~3);

	if (!(pbd->ctrl_stat & SMAP_BD_RX_EMPTY)) 
	{
		emu_printf("ERROR : Discarding %d bytes (RX%d not ready)\n", bytes, dev9.rxbdi);
		return;
	}

	int pstart=(dev9.rxfifo_wr_ptr)&16383;
	int i=0;
	while(i<bytes)
	{
		dev9_rxfifo_write(pk->buffer[i++]);
		dev9.rxfifo_wr_ptr&=16383;
	}

	//increase RXBD
	dev9.rxbdi++;
	dev9.rxbdi&=(SMAP_BD_SIZE/8)-1;

	//Fill the BD with info !
	pbd->length = pk->size;
	pbd->pointer = 0x4000 + pstart;
	pbd->ctrl_stat&= ~SMAP_BD_RX_EMPTY;

	//increase frame count
	u8* cntptr=&dev9Ru8(SMAP_R_RXFIFO_FRAME_CNT);

#ifdef WIN_X64
	*cntptr++; //no asm inline in x64
#else
	__asm 
	{ 
		//this is silly
		mov eax,[cntptr];
		lock inc byte ptr [eax] 
	}
#endif

	emu_printf("Got packet, %d bytes (%d fifo)\n", pk->size,bytes);
	_DEV9irq(SMAP_INTR_RXEND,0);//now ? or when the fifo is full ? i guess now atm
								//note that this _is_ wrong since the IOP interrupt system is not thread safe.. but nothing i can do about that
}

bool tx_p_first=false;
u32 wswap(u32 d)
{
	return (d>>16)|(d<<16);
}
void tx_process()
{
	//we loop based on count ? or just *use* it ?
	u32 cnt=dev9Ru8(SMAP_R_TXFIFO_FRAME_CNT);
	printf("tx_process : %d cnt frames !\n",cnt);

	if (!tx_p_first)
	{
		dev9Ru8(SMAP_R_TXFIFO_FRAME_CNT)=0;
		tx_p_first=true;
		//THIS IS A HACK.without that the stack wont init, i guess its missing e3/emac emulation ..
		printf("WARN : First packet interrupt hack ..\n");
		_DEV9irq(SMAP_INTR_RXEND|SMAP_INTR_TXEND|SMAP_INTR_TXDNV,100);
		return;
	}

	NetPacket pk;
	int fc=0;
	for (fc=0;fc<cnt;fc++)
	{
		smap_bd_t *pbd= ((smap_bd_t *)&dev9.dev9R[SMAP_BD_TX_BASE & 0xffff])+dev9.txbdi;

		if (! (pbd->ctrl_stat&SMAP_BD_TX_READY))
		{
			//emu_printf("ERROR : !pbd->ctrl_stat&SMAP_BD_TX_READY\n");
			break;
		}
		if (pbd->length&3)
		{
			emu_printf("WARN : pbd->length not alligned %d\n",pbd->length);
		}

		if(pbd->length>1514)
		{
			emu_printf("ERROR : Trying to send packet too big.\n");
		}
		else
		{
			u32 base=(pbd->pointer-0x1000)&16383;
			DEV9_LOG("Sending Packet from base %x, size %d\n", base, pbd->length);
			emu_printf("Sending Packet from base %x, size %d\n", base, pbd->length);
			
			pk.size=pbd->length;
			
			if (!(pbd->pointer>=0x1000))
			{
				emu_printf("ERROR: odd , !pbd->pointer>0x1000 | 0x%X %d\n", pbd->pointer, pbd->length);
			}
			//increase fifo pointer(s)
			//uh does that even exist on real h/w ?
			/*
			if(dev9.txfifo_rd_ptr+pbd->length >= 16383)
			{
				//warp around !
				//first part
				u32 was=16384-dev9.txfifo_rd_ptr;
				memcpy(pk.buffer,dev9.txfifo+dev9.txfifo_rd_ptr,was);
				//warp
				dev9.txfifo_rd_ptr+=pbd->length;
				dev9.txfifo_rd_ptr&=16383;
				if (pbd->length!=was+dev9.txfifo_rd_ptr)
				{
					emu_printf("ERROR ON TX FIFO HANDLING, %x\n", dev9.txfifo_rd_ptr);
				}
				//second part
				memcpy(pk.buffer+was,dev9.txfifo,pbd->length-was);
			}
			else
			{	//no warp or 'perfect' warp (reads end, resets to start
				memcpy(pk.buffer,dev9.txfifo+dev9.txfifo_rd_ptr,pbd->length);
				dev9.txfifo_rd_ptr+=pbd->length;
				if (dev9.txfifo_rd_ptr==16384)
					dev9.txfifo_rd_ptr=0;
			}
			
			

			if (dev9.txfifo_rd_ptr&(~16383))
			{
				emu_printf("ERROR ON TX FIFO HANDLING, %x\n", dev9.txfifo_rd_ptr);
			}
			*/

			if(base+pbd->length > 16384)
			{
				u32 was=16384-base;
				memcpy(pk.buffer,dev9.txfifo+base,was);
				memcpy(pk.buffer,dev9.txfifo,pbd->length-was);
				printf("Warped read, was=%d, sz=%d, sz-was=%d\n",was,pbd->length,pbd->length-was);
			}
			else
			{
				memcpy(pk.buffer,dev9.txfifo+base,pbd->length);
			}
			tx_put(&pk);
		}


		pbd->ctrl_stat&= ~SMAP_BD_TX_READY;

		//increase TXBD
		dev9.txbdi++;
		dev9.txbdi&=(SMAP_BD_SIZE/8)-1;

		//decrease frame count -- this is not thread safe
		dev9Ru8(SMAP_R_TXFIFO_FRAME_CNT)--;
	}

	printf("processed %d frames, %d count, cnt = %d\n",fc,dev9Ru8(SMAP_R_TXFIFO_FRAME_CNT),cnt);
	//if some error/early exit signal TXDNV
	if (fc!=cnt || cnt==0)
	{
		printf("WARN : (fc!=cnt || cnt==0) but packet send request was made oO..\n");
		_DEV9irq(SMAP_INTR_TXDNV,0);
	}
	//if we actualy send something send TXEND
	if(fc!=0)
		_DEV9irq(SMAP_INTR_TXEND,100);//now ? or when the fifo is empty ? i guess now atm
}


void emac3_write(u32 addr)
{
	u32 value=wswap(dev9Ru32(addr));
	switch(addr)
	{
	case SMAP_R_EMAC3_MODE0_L:
		DEV9_LOG("SMAP: SMAP_R_EMAC3_MODE0 write %x\n", value);
		value = (value & (~SMAP_E3_SOFT_RESET)) | SMAP_E3_TXMAC_IDLE | SMAP_E3_RXMAC_IDLE;
		dev9Ru16(SMAP_R_EMAC3_STA_CTRL_H)|= SMAP_E3_PHY_OP_COMP;
		break;
	case SMAP_R_EMAC3_TxMODE0_L:
		DEV9_LOG("SMAP: SMAP_R_EMAC3_TxMODE0_L write %x\n", value);
		emu_printf("SMAP: SMAP_R_EMAC3_TxMODE0_L write %x\n", value);
		//Process TX  here ?
		if (!value&SMAP_E3_TX_GNP_0)
			emu_printf("SMAP_R_EMAC3_TxMODE0_L: SMAP_E3_TX_GNP_0 not set\n");

		tx_process();
		value = value& ~SMAP_E3_TX_GNP_0;
		if (value)
			emu_printf("SMAP_R_EMAC3_TxMODE0_L: extra bits set !\n");
		break;

	case SMAP_R_EMAC3_TxMODE1_L:
		emu_printf("SMAP_R_EMAC3_TxMODE1_L 32bit write %x\n", value);
		break;


	case SMAP_R_EMAC3_STA_CTRL_L:
		DEV9_LOG("SMAP: SMAP_R_EMAC3_STA_CTRL write %x\n", value);
		{
			if (value & (SMAP_E3_PHY_READ)) 
			{
				value|= SMAP_E3_PHY_OP_COMP;
				int reg = value & (SMAP_E3_PHY_REG_ADDR_MSK);
				u16 val = dev9.phyregs[reg];
				switch (reg) 
				{
				case SMAP_DsPHYTER_BMSR:
					if (has_link)
						val|= SMAP_PHY_BMSR_LINK | SMAP_PHY_BMSR_ANCP;
					break;
				case SMAP_DsPHYTER_PHYSTS:
					if (has_link)
						val|= SMAP_PHY_STS_LINK |SMAP_PHY_STS_100M | SMAP_PHY_STS_FDX | SMAP_PHY_STS_ANCP;
					break;
				}
				DEV9_LOG("phy_read %d: %x\n", reg, val);
				value=(value&0xFFFF)|(val<<16);
			} 
			if (value & (SMAP_E3_PHY_WRITE)) 
			{
				value|= SMAP_E3_PHY_OP_COMP;
				int reg = value & (SMAP_E3_PHY_REG_ADDR_MSK);
				u16 val = value>>16;
				switch (reg) 
				{
				case SMAP_DsPHYTER_BMCR:
					val&= ~SMAP_PHY_BMCR_RST;
					val|= 0x1;
					break;
				}
				DEV9_LOG("phy_write %d: %x\n", reg, val);
				dev9.phyregs[reg] = val;
			}
		}
		break;
	default:
		DEV9_LOG("SMAP: emac3 write  %x=%x\n",addr, value);
	}
	dev9Ru32(addr)=wswap(value);
}
u8 CALLBACK smap_read8(u32 addr)
{
	switch(addr)
	{
	case SMAP_R_TXFIFO_FRAME_CNT:
		printf("SMAP_R_TXFIFO_FRAME_CNT read 8\n");
		break;
	case SMAP_R_RXFIFO_FRAME_CNT:
		printf("SMAP_R_RXFIFO_FRAME_CNT read 8\n");
		break;

	case SMAP_R_BD_MODE:
		return dev9.bd_swap;

	default:
		DEV9_LOG("SMAP : Unknown 8 bit read @ %X,v=%X\n",addr,dev9Ru8(addr));
		return dev9Ru8(addr);
	}

	DEV9_LOG("SMAP : error , 8 bit read @ %X,v=%X\n",addr,dev9Ru8(addr));
	return dev9Ru8(addr);
}
u16 CALLBACK smap_read16(u32 addr)
{
	if (addr >= SMAP_BD_TX_BASE && addr < (SMAP_BD_TX_BASE + SMAP_BD_SIZE)) 
	{
		int rv = dev9Ru16(addr);
		if(dev9.bd_swap)
				return (rv<<8)|(rv>>8);
		return rv;
		/*
		switch (addr & 0x7) 
		{
		case 0: // ctrl_stat
			hard = dev9Ru16(addr);
			//DEV9_LOG("TX_CTRL_STAT[%d]: read %x\n", (addr - SMAP_BD_TX_BASE) / 8, hard);
			if(dev9.bd_swap)
				return (hard<<8)|(hard>>8);
			return hard;
		case 2: // unknown
			hard = dev9Ru16(addr);
			//DEV9_LOG("TX_UNKNOWN[%d]: read %x\n", (addr - SMAP_BD_TX_BASE) / 8, hard);
			if(dev9.bd_swap)
				return (hard<<8)|(hard>>8);
			return hard;
		case 4: // length
			hard = dev9Ru16(addr);
			DEV9_LOG("TX_LENGTH[%d]: read %x\n", (addr - SMAP_BD_TX_BASE) / 8, hard);
			if(dev9.bd_swap)
				return (hard<<8)|(hard>>8);
			return hard;
		case 6: // pointer
			hard = dev9Ru16(addr);
			DEV9_LOG("TX_POINTER[%d]: read %x\n", (addr - SMAP_BD_TX_BASE) / 8, hard);
			if(dev9.bd_swap)
				return (hard<<8)|(hard>>8);
			return hard;
		}
		*/
	}
	else if (addr >= SMAP_BD_RX_BASE && addr < (SMAP_BD_RX_BASE + SMAP_BD_SIZE)) 
	{
		int rv = dev9Ru16(addr);
		if(dev9.bd_swap)
				return (rv<<8)|(rv>>8);
		return rv;
		/*
		switch (addr & 0x7) 
		{
		case 0: // ctrl_stat
			hard = dev9Ru16(addr);
			//DEV9_LOG("RX_CTRL_STAT[%d]: read %x\n", (addr - SMAP_BD_RX_BASE) / 8, hard);
			if(dev9.bd_swap)
				return (hard<<8)|(hard>>8);
			return hard;
		case 2: // unknown
			hard = dev9Ru16(addr);
			//DEV9_LOG("RX_UNKNOWN[%d]: read %x\n", (addr - SMAP_BD_RX_BASE) / 8, hard);
			if(dev9.bd_swap)
				return (hard<<8)|(hard>>8);
			return hard;
		case 4: // length
			hard = dev9Ru16(addr);
			DEV9_LOG("RX_LENGTH[%d]: read %x\n", (addr - SMAP_BD_RX_BASE) / 8, hard);
			if(dev9.bd_swap)
				return (hard<<8)|(hard>>8);
			return hard;
		case 6: // pointer
			hard = dev9Ru16(addr);
			DEV9_LOG("RX_POINTER[%d]: read %x\n", (addr - SMAP_BD_RX_BASE) / 8, hard);
			if(dev9.bd_swap)
				return (hard<<8)|(hard>>8);
			return hard;
		}
		*/
	}

	switch(addr)
	{
#ifdef DEV9_LOG_ENABLE
		case SMAP_R_TXFIFO_FRAME_CNT:
			printf("SMAP_R_TXFIFO_FRAME_CNT read 16\n");
			return dev9Ru16(addr);
		case SMAP_R_RXFIFO_FRAME_CNT:
			printf("SMAP_R_RXFIFO_FRAME_CNT read 16\n");
			return dev9Ru16(addr);
		case SMAP_R_EMAC3_MODE0_L:
			DEV9_LOG("SMAP_R_EMAC3_MODE0_L 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_MODE0_H:
			DEV9_LOG("SMAP_R_EMAC3_MODE0_H 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_MODE1_L:
			DEV9_LOG("SMAP_R_EMAC3_MODE1_L 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_MODE1_H:
			DEV9_LOG("SMAP_R_EMAC3_MODE1_H 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_RxMODE_L:
			DEV9_LOG("SMAP_R_EMAC3_RxMODE_L 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_RxMODE_H:
			DEV9_LOG("SMAP_R_EMAC3_RxMODE_H 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_INTR_STAT_L:
			DEV9_LOG("SMAP_R_EMAC3_INTR_STAT_L 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_INTR_STAT_H:
			DEV9_LOG("SMAP_R_EMAC3_INTR_STAT_H 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_INTR_ENABLE_L:
			DEV9_LOG("SMAP_R_EMAC3_INTR_ENABLE_L 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_INTR_ENABLE_H:
			DEV9_LOG("SMAP_R_EMAC3_INTR_ENABLE_H 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_TxMODE0_L:
			DEV9_LOG("SMAP_R_EMAC3_TxMODE0_L 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_TxMODE0_H:
			DEV9_LOG("SMAP_R_EMAC3_TxMODE0_H 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_TxMODE1_L:
			DEV9_LOG("SMAP_R_EMAC3_TxMODE1_L 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_TxMODE1_H:
			DEV9_LOG("SMAP_R_EMAC3_TxMODE1_H 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_STA_CTRL_L:
			DEV9_LOG("SMAP_R_EMAC3_STA_CTRL_L 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);

		case SMAP_R_EMAC3_STA_CTRL_H:
			DEV9_LOG("SMAP_R_EMAC3_STA_CTRL_H 16bit read %x\n", dev9Ru16(addr));
			return dev9Ru16(addr);
#endif
		default:
			DEV9_LOG("SMAP : Unknown 16 bit read @ %X,v=%X\n",addr,dev9Ru16(addr));
			return dev9Ru16(addr);
	}
	
	DEV9_LOG("SMAP : error , 16 bit read @ %X,v=%X\n",addr,dev9Ru16(addr));
	return dev9Ru16(addr);

}
u32 CALLBACK smap_read32(u32 addr)
{
	if (addr>=SMAP_EMAC3_REGBASE && addr<SMAP_EMAC3_REGEND)
	{
		u32 hi=smap_read16(addr);
		u32 lo=smap_read16(addr+2)<<16;
		return hi|lo;
	}
	switch(addr)
	{
	case SMAP_R_TXFIFO_FRAME_CNT:
		printf("SMAP_R_TXFIFO_FRAME_CNT read 32\n");
		return dev9Ru32(addr);
	case SMAP_R_RXFIFO_FRAME_CNT:
		printf("SMAP_R_RXFIFO_FRAME_CNT read 32\n");
		return dev9Ru32(addr);
	case SMAP_R_EMAC3_STA_CTRL_L:
		DEV9_LOG("SMAP_R_EMAC3_STA_CTRL_L 32bit read value %x\n", dev9Ru32(addr));
		return dev9Ru32(addr);

	case SMAP_R_RXFIFO_DATA:
		{
			int rd_ptr = dev9Ru32(SMAP_R_RXFIFO_RD_PTR)&16383;

			int rv = *((u32*)(dev9.rxfifo + rd_ptr));

			dev9Ru32(SMAP_R_RXFIFO_RD_PTR) = ((rd_ptr+4)&16383);

			if(dev9.bd_swap)
				rv=(rv<<24)|(rv>>24)|((rv>>8)&0xFF00)|((rv<<8)&0xFF0000);

			DEV9_LOG("SMAP_R_RXFIFO_DATA 32bit read %x\n", rv);
			return rv;
		}
	default:
		DEV9_LOG("SMAP : Unknown 32 bit read @ %X,v=%X\n",addr,dev9Ru32(addr));
		return dev9Ru32(addr);
	}
	
	DEV9_LOG("SMAP : error , 32 bit read @ %X,v=%X\n",addr,dev9Ru32(addr));
	return dev9Ru32(addr);
}
void CALLBACK smap_write8(u32 addr, u8 value)
{
	switch(addr)
	{
	case SMAP_R_TXFIFO_FRAME_INC:
		DEV9_LOG("SMAP_R_TXFIFO_FRAME_INC 8bit write %x\n", value);
		{
			dev9Ru8(SMAP_R_TXFIFO_FRAME_CNT)++;
		}
		return;

	case SMAP_R_RXFIFO_FRAME_DEC:
		DEV9_LOG("SMAP_R_RXFIFO_FRAME_DEC 8bit write %x\n", value);
		dev9Ru8(addr) = value;
		{
			u8* cntptr=&dev9Ru8(SMAP_R_RXFIFO_FRAME_CNT);
#ifdef WIN_X64
			*cntptr--; //no asm inline in x64
#else
			__asm 
			{ 
				//this is silly
				mov eax,[cntptr];
				lock dec byte ptr [eax] 
			}
#endif
		}
		return;

	case SMAP_R_TXFIFO_CTRL:
		DEV9_LOG("SMAP_R_TXFIFO_CTRL 8bit write %x\n", value);
		if(value&SMAP_TXFIFO_RESET)
		{
			dev9.txbdi=0;
			dev9.txfifo_rd_ptr=0;
			dev9Ru8(SMAP_R_TXFIFO_FRAME_CNT)=0;	//this actualy needs to be atomic (lock mov ...)
			dev9Ru32(SMAP_R_TXFIFO_WR_PTR)=0;
			dev9Ru32(SMAP_R_TXFIFO_SIZE)=16384;
		}
		value&= ~SMAP_TXFIFO_RESET;
		dev9Ru8(addr) = value;
		return;

	case SMAP_R_RXFIFO_CTRL:
		DEV9_LOG("SMAP_R_RXFIFO_CTRL 8bit write %x\n", value);
		if(value&SMAP_RXFIFO_RESET)
		{
			dev9.rxbdi=0;
			dev9.rxfifo_wr_ptr=0;
			dev9Ru8(SMAP_R_RXFIFO_FRAME_CNT)=0;
			dev9Ru32(SMAP_R_RXFIFO_RD_PTR)=0;
			dev9Ru32(SMAP_R_RXFIFO_SIZE)=16384;
		}
		value&= ~SMAP_RXFIFO_RESET;
		dev9Ru8(addr) = value;
		return;

	case SMAP_R_BD_MODE:
		if(value&SMAP_BD_SWAP)
		{
			DEV9_LOG("SMAP_R_BD_MODE: byteswapped.\n");
			emu_printf("BD Byteswapping enabled.\n");
			dev9.bd_swap=1;
		}
		else
		{
			DEV9_LOG("SMAP_R_BD_MODE: NOT byteswapped.\n");
			emu_printf("BD Byteswapping disabled.\n");
			dev9.bd_swap=0;
		}
		return;
	default :
		DEV9_LOG("SMAP : Unknown 8 bit write @ %X,v=%X\n",addr,value);
		dev9Ru8(addr) = value;
		return;
	}

	DEV9_LOG("SMAP : error , 8 bit write @ %X,v=%X\n",addr,value);
	dev9Ru8(addr) = value;
}
void CALLBACK smap_write16(u32 addr, u16 value)
{
	if (addr >= SMAP_BD_TX_BASE && addr < (SMAP_BD_TX_BASE + SMAP_BD_SIZE)) {
		if(dev9.bd_swap)
			value = (value>>8)|(value<<8);
		dev9Ru16(addr) = value;
		/*
		switch (addr & 0x7) 
		{
		case 0: // ctrl_stat
			DEV9_LOG("TX_CTRL_STAT[%d]: write %x\n", (addr - SMAP_BD_TX_BASE) / 8, value);
			//hacky
			dev9Ru16(addr) = value;
			return;
		case 2: // unknown
			//DEV9_LOG("TX_UNKNOWN[%d]: write %x\n", (addr - SMAP_BD_TX_BASE) / 8, value);
			dev9Ru16(addr) = value;
			return;
		case 4: // length
			DEV9_LOG("TX_LENGTH[%d]: write %x\n", (addr - SMAP_BD_TX_BASE) / 8, value);
			dev9Ru16(addr) = value;
			return;
		case 6: // pointer
			DEV9_LOG("TX_POINTER[%d]: write %x\n", (addr - SMAP_BD_TX_BASE) / 8, value);
			dev9Ru16(addr) = value;
			return;
		}
		*/
		return;
	}
	else if (addr >= SMAP_BD_RX_BASE && addr < (SMAP_BD_RX_BASE + SMAP_BD_SIZE)) 
	{
		int rx_index=(addr - SMAP_BD_RX_BASE)>>3;
		if(dev9.bd_swap)
			value = (value>>8)|(value<<8);
		dev9Ru16(addr) = value;
/*
		switch (addr & 0x7) 
		{
		case 0: // ctrl_stat
			DEV9_LOG("RX_CTRL_STAT[%d]: write %x\n", rx_index, value);
			dev9Ru16(addr) = value;
			if(value&0x8000)
			{
				DEV9_LOG(" * * PACKET READ COMPLETE:   rd_ptr=%d, wr_ptr=%d\n", dev9Ru32(SMAP_R_RXFIFO_RD_PTR), dev9.rxfifo_wr_ptr);
			}
			return;
		case 2: // unknown
			//DEV9_LOG("RX_UNKNOWN[%d]: write %x\n", rx_index, value);
			dev9Ru16(addr) = value;
			return;
		case 4: // length
			DEV9_LOG("RX_LENGTH[%d]: write %x\n", rx_index, value);
			dev9Ru16(addr) = value;
			return;
		case 6: // pointer
			DEV9_LOG("RX_POINTER[%d]: write %x\n", rx_index, value);
			dev9Ru16(addr) = value;
			return;
		}
		*/
		return;
	}

	switch(addr)
	{
	case SMAP_R_INTR_CLR:
		DEV9_LOG("SMAP: SMAP_R_INTR_CLR 16bit write %x\n", value);
		dev9.irqcause&= ~value;
		return;

	case SMAP_R_TXFIFO_WR_PTR:
		DEV9_LOG("SMAP: SMAP_R_TXFIFO_WR_PTR 16bit write %x\n", value);
		dev9Ru16(addr) = value;
		return;
#define EMAC3_L_WRITE(name) \
	case name: \
		DEV9_LOG("SMAP: " #name " 16 bit write %x\n", value); \
		dev9Ru16(addr) = value; \
		return;
	//handle L writes
	EMAC3_L_WRITE(SMAP_R_EMAC3_MODE0_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_MODE1_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_TxMODE0_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_TxMODE1_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_RxMODE_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_INTR_STAT_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_INTR_ENABLE_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_ADDR_HI_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_ADDR_LO_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_VLAN_TPID )
	EMAC3_L_WRITE( SMAP_R_EMAC3_PAUSE_TIMER_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_INDIVID_HASH1 )
	EMAC3_L_WRITE( SMAP_R_EMAC3_INDIVID_HASH2 )
	EMAC3_L_WRITE( SMAP_R_EMAC3_INDIVID_HASH3 )
	EMAC3_L_WRITE( SMAP_R_EMAC3_INDIVID_HASH4 )
	EMAC3_L_WRITE( SMAP_R_EMAC3_GROUP_HASH1 )
	EMAC3_L_WRITE( SMAP_R_EMAC3_GROUP_HASH2 )
	EMAC3_L_WRITE( SMAP_R_EMAC3_GROUP_HASH3 )
	EMAC3_L_WRITE( SMAP_R_EMAC3_GROUP_HASH4 )

	EMAC3_L_WRITE( SMAP_R_EMAC3_LAST_SA_HI )
	EMAC3_L_WRITE( SMAP_R_EMAC3_LAST_SA_LO )
	EMAC3_L_WRITE( SMAP_R_EMAC3_INTER_FRAME_GAP_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_STA_CTRL_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_TX_THRESHOLD_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_RX_WATERMARK_L )
	EMAC3_L_WRITE( SMAP_R_EMAC3_TX_OCTETS )
	EMAC3_L_WRITE( SMAP_R_EMAC3_RX_OCTETS )

#define EMAC3_H_WRITE(name) \
	case name: \
		DEV9_LOG("SMAP: " #name " 16 bit write %x\n", value); \
		dev9Ru16(addr) = value; \
		emac3_write(addr-2); \
		return;
	//handle H writes
	EMAC3_H_WRITE(SMAP_R_EMAC3_MODE0_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_MODE1_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_TxMODE0_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_TxMODE1_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_RxMODE_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_INTR_STAT_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_INTR_ENABLE_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_ADDR_HI_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_ADDR_LO_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_VLAN_TPID+2 )
	EMAC3_H_WRITE( SMAP_R_EMAC3_PAUSE_TIMER_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_INDIVID_HASH1+2 )
	EMAC3_H_WRITE( SMAP_R_EMAC3_INDIVID_HASH2+2 )
	EMAC3_H_WRITE( SMAP_R_EMAC3_INDIVID_HASH3+2 )
	EMAC3_H_WRITE( SMAP_R_EMAC3_INDIVID_HASH4+2 )
	EMAC3_H_WRITE( SMAP_R_EMAC3_GROUP_HASH1+2 )
	EMAC3_H_WRITE( SMAP_R_EMAC3_GROUP_HASH2+2 )
	EMAC3_H_WRITE( SMAP_R_EMAC3_GROUP_HASH3+2 )
	EMAC3_H_WRITE( SMAP_R_EMAC3_GROUP_HASH4+2 )

	EMAC3_H_WRITE( SMAP_R_EMAC3_LAST_SA_HI+2 )
	EMAC3_H_WRITE( SMAP_R_EMAC3_LAST_SA_LO+2 )
	EMAC3_H_WRITE( SMAP_R_EMAC3_INTER_FRAME_GAP_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_STA_CTRL_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_TX_THRESHOLD_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_RX_WATERMARK_H )
	EMAC3_H_WRITE( SMAP_R_EMAC3_TX_OCTETS+2 )
	EMAC3_H_WRITE( SMAP_R_EMAC3_RX_OCTETS+2 )
/*
	case SMAP_R_EMAC3_MODE0_L:
		DEV9_LOG("SMAP: SMAP_R_EMAC3_MODE0 write %x\n", value);
		dev9Ru16(addr) = value;
		return;
	case SMAP_R_EMAC3_TxMODE0_L:
		DEV9_LOG("SMAP: SMAP_R_EMAC3_TxMODE0_L 16bit write %x\n", value);
		dev9Ru16(addr) = value;
		return;
	case SMAP_R_EMAC3_TxMODE1_L:
		emu_printf("SMAP: SMAP_R_EMAC3_TxMODE1_L 16bit write %x\n", value);
		dev9Ru16(addr) = value;
		return;

	case SMAP_R_EMAC3_TxMODE0_H:
		emu_printf("SMAP: SMAP_R_EMAC3_TxMODE0_H 16bit write %x\n", value);
		dev9Ru16(addr) = value;
		return;
	
	case SMAP_R_EMAC3_TxMODE1_H:
		emu_printf("SMAP: SMAP_R_EMAC3_TxMODE1_H 16bit write %x\n", value);
		dev9Ru16(addr) = value;
		return;
	case SMAP_R_EMAC3_STA_CTRL_H:
		DEV9_LOG("SMAP: SMAP_R_EMAC3_STA_CTRL_H 16bit write %x\n", value);
		dev9Ru16(addr) = value;
		return;
		*/

	default :
		DEV9_LOG("SMAP : Unknown 16 bit write @ %X,v=%X\n",addr,value);
		dev9Ru16(addr) = value;
		return;
	}

	DEV9_LOG("SMAP : error , 16 bit write @ %X,v=%X\n",addr,value);
	dev9Ru16(addr) = value;
}
void CALLBACK smap_write32(u32 addr, u32 value)
{
	if (addr>=SMAP_EMAC3_REGBASE && addr<SMAP_EMAC3_REGEND)
	{
		smap_write16(addr,value&0xFFFF);
		smap_write16(addr+2,value>>16);
		return;
	}
	switch(addr)
	{
	case SMAP_R_TXFIFO_DATA:
		if(dev9.bd_swap)
			value=(value<<24)|(value>>24)|((value>>8)&0xFF00)|((value<<8)&0xFF0000);

		DEV9_LOG("SMAP_R_TXFIFO_DATA 32bit write %x\n", value);
		*((u32*)(dev9.txfifo+dev9Ru32(SMAP_R_TXFIFO_WR_PTR)))=value;
		dev9Ru32(SMAP_R_TXFIFO_WR_PTR) = (dev9Ru32(SMAP_R_TXFIFO_WR_PTR)+4)&16383;
		return;
	default :
		DEV9_LOG("SMAP : Unknown 32 bit write @ %X,v=%X\n",addr,value);
		dev9Ru32(addr) = value;
		return;
	}

	DEV9_LOG("SMAP : error , 32 bit write @ %X,v=%X\n",addr,value);
	dev9Ru32(addr) = value;
}
void CALLBACK smap_readDMA8Mem(u32 *pMem, int size)
{
	if(dev9Ru16(SMAP_R_RXFIFO_CTRL)&SMAP_RXFIFO_DMAEN)
	{
		dev9Ru32(SMAP_R_RXFIFO_RD_PTR)&=16383;
		size>>=1;
		DEV9_LOG(" * * SMAP DMA READ START: rd_ptr=%d, wr_ptr=%d\n", dev9Ru32(SMAP_R_RXFIFO_RD_PTR), dev9.rxfifo_wr_ptr);
		while(size>0)
		{
			*pMem = *((u32*)(dev9.rxfifo+dev9Ru32(SMAP_R_RXFIFO_RD_PTR)));
			pMem++;
			dev9Ru32(SMAP_R_RXFIFO_RD_PTR) = (dev9Ru32(SMAP_R_RXFIFO_RD_PTR)+4)&16383;
			
			size-=4;
		}
		DEV9_LOG(" * * SMAP DMA READ END:   rd_ptr=%d, wr_ptr=%d\n", dev9Ru32(SMAP_R_RXFIFO_RD_PTR), dev9.rxfifo_wr_ptr);

		dev9Ru16(SMAP_R_RXFIFO_CTRL) &= ~SMAP_RXFIFO_DMAEN;
	}
}
void CALLBACK smap_writeDMA8Mem(u32* pMem, int size)
{
	if(dev9Ru16(SMAP_R_TXFIFO_CTRL)&SMAP_TXFIFO_DMAEN)
	{
		dev9Ru32(SMAP_R_TXFIFO_WR_PTR)&=16383;
		size>>=1;
		DEV9_LOG(" * * SMAP DMA WRITE START: wr_ptr=%d, rd_ptr=%d\n", dev9Ru32(SMAP_R_TXFIFO_WR_PTR), dev9.txfifo_rd_ptr);
		while(size>0)
		{
			int value=*pMem;
			//	value=(value<<24)|(value>>24)|((value>>8)&0xFF00)|((value<<8)&0xFF0000);
			pMem++;

			*((u32*)(dev9.txfifo+dev9Ru32(SMAP_R_TXFIFO_WR_PTR)))=value;
			dev9Ru32(SMAP_R_TXFIFO_WR_PTR) = (dev9Ru32(SMAP_R_TXFIFO_WR_PTR)+4)&16383;
			size-=4;
		}
		DEV9_LOG(" * * SMAP DMA WRITE END:   wr_ptr=%d, rd_ptr=%d\n", dev9Ru32(SMAP_R_TXFIFO_WR_PTR), dev9.txfifo_rd_ptr);

		dev9Ru16(SMAP_R_TXFIFO_CTRL) &= ~SMAP_TXFIFO_DMAEN;

	}
}
