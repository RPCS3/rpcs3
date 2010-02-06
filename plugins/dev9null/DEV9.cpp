/*  DEV9null
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

// Note: I was using MegaDev9, dev9ghzdrk, and dev9linuz  for reference on memory locations.
// The ones I included were just some of the more important ones, so you may want to look
// at the plugins I mentioned if trying to create your own dev9 plugin.

// Additionally, there is a lot of information in the ps2drv drivers by Marcus R. Brown, so 
// looking through its code would be a good starting point.

// Look under tags/plugins in svn for any older plugins that aren't included in pcsx2 any more.
// --arcum42


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string>
using namespace std;

#include "DEV9.h"

const unsigned char version  = PS2E_DEV9_VERSION;
const unsigned char revision = 0;
const unsigned char build = 5;    // increase that with each version

const char *libraryName = "DEV9null Driver";

// Our IRQ call.
void (*DEV9irq)(int);

__aligned16 s8 dev9regs[0x10000];

string s_strIniPath = "inis/";
PluginLog Dev9Log;
Config conf;

EXPORT_C_(u32) PS2EgetLibType() 
{
	return PS2E_LT_DEV9;
}

EXPORT_C_(char*) PS2EgetLibName() 
{
	return (char *)libraryName;
}

EXPORT_C_(u32) PS2EgetLibVersion2(u32 type) 
{
	return (version<<16) | (revision<<8) | build;
}

EXPORT_C_(s32) DEV9init() 
{		
	LoadConfig();
	setLoggingState();
	
	Dev9Log.Open("logs/dev9null.log");
	Dev9Log.WriteLn("dev9null plugin version %d,%d", revision, build);
	Dev9Log.WriteLn("Initializing dev9null");
	// Initialize anything that needs to be initialized.
	memset(dev9regs, 0, sizeof(dev9regs));
	return 0;
}

EXPORT_C_(void) DEV9shutdown() 
{
	Dev9Log.WriteLn("Shutting down Dev9null.");
	Dev9Log.Close();
}

EXPORT_C_(s32) DEV9open(void *pDsp)
{
	Dev9Log.WriteLn("Opening Dev9null.");
	// Get anything ready we need to. Opening and creating hard
	// drive files, for example.
	return 0;
}

EXPORT_C_(void) DEV9close() 
{
	Dev9Log.WriteLn("Closing Dev9null.");
	// Close files opened.
}

EXPORT_C_(u8) DEV9read8(u32 addr) 
{
    u8 value = 0;
    
    switch(addr)
    {
//        case 0x1F80146E:		// DEV9 hardware type (0x32 for an expansion bay)
        case 0x10000038: /*value = dev9Ru8(addr);*/ break; // We need to have at least one case to avoid warnings.
        default: 
            //value = dev9Ru8(addr); 
            Dev9Log.WriteLn("*Unknown 8 bit read at address %lx", addr);
            break;
    }
	return value;
}

EXPORT_C_(u16) DEV9read16(u32 addr) 
{
    u16 value = 0;
    
    switch(addr)
    {
		// Addresses you may want to catch here include:
//			case 0x1F80146E:		// DEV9 hardware type (0x32 for an expansion bay)
//			case 0x10000002:		// The Smart Chip revision. Should be 0x11
//			case 0x10000004:		// More type info: bit 0 - smap; bit 1 - hd; bit 5 - flash 
//			case 0x1000000E:		// Similar to the last; bit 1 should be set if a hd is hooked up.
//			case 0x10000028:			// intr_stat
//			case 0x10000038:			// hard drives seem to like reading and writing the max dma size per transfer here.
//			case 0x1000002A:			// intr_mask
//			case 0x10000040:			// pio_data
//			case 0x10000044:			// nsector
//			case 0x10000046:			// sector
//			case 0x10000048:			// lcyl
//			case 0x1000004A:			// hcyl
//			case 0x1000004C:			// select
//			case 0x1000004E:			// status
//			case 0x1000005C:			// status
//			case 0x10000064:			// if_ctrl
        case 0x10000038: /*value = dev9Ru16(addr);*/ break;
        default: 
            //value = dev9Ru16(addr); 
            Dev9Log.WriteLn("*Unknown 16 bit read at address %lx", addr);
            break;
    }
	
	return value;
}

EXPORT_C_(u32 ) DEV9read32(u32 addr) 
{
    u32 value = 0;
    
    switch(addr)
    {
        case 0x10000038: /*value = dev9Ru32(addr);*/ break;
        default: 
            //value = dev9Ru32(addr); 
            Dev9Log.WriteLn("*Unknown 32 bit read at address %lx", addr);
          break;
    }
	
	return value;
}

EXPORT_C_(void) DEV9write8(u32 addr,  u8 value) 
{
    switch(addr)
    {
        case 0x10000038: /*dev9Ru8(addr) = value;*/ break;
        default: 
			Dev9Log.WriteLn("*Unknown 8 bit write; address %lx = %x", addr, value);
			//dev9Ru8(addr) = value;
          break;
    }
}

EXPORT_C_(void) DEV9write16(u32 addr, u16 value) 
{
    switch(addr)
    {
    	// Remember that list on DEV9read16? You'll want to write to a
    	// lot of them, too.
        case 0x10000038: /*dev9Ru16(addr) = value;*/ break;
        default: 
			Dev9Log.WriteLn("*Unknown 16 bit write; address %lx = %x", addr, value);
            //dev9Ru16(addr) = value;
          break;
    }
}

EXPORT_C_(void) DEV9write32(u32 addr, u32 value) 
{
    switch(addr)
    {
        case 0x10000038: /*dev9Ru32(addr) = value;*/ break;
        default: 
			Dev9Log.WriteLn("*Unknown 32 bit write; address %lx = %x", addr, value);
            //dev9Ru32(addr) = value;
          break;
    }
}

//#ifdef ENABLE_NEW_IOPDMA_DEV9
EXPORT_C_(s32)  DEV9dmaRead(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed)
{
	// You'll want to but your own DMA8 reading code here.
	// Time to interact with your fake (or real) hardware.
	Dev9Log.WriteLn("Reading DMA8 Mem.");
	*bytesProcessed = bytesLeft;
	return 0;
}

EXPORT_C_(s32)  DEV9dmaWrite(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed)
{
	// See above.
	Dev9Log.WriteLn("Writing DMA8 Mem.");
	*bytesProcessed = bytesLeft;
	return 0;
}

EXPORT_C_(void) DEV9dmaInterrupt(s32 channel)
{
	// See above.
}
//#else
EXPORT_C_(void) DEV9readDMA8Mem(u32 *pMem, int size) 
{
	// You'll want to but your own DMA8 reading code here.
	// Time to interact with your fake (or real) hardware.
	Dev9Log.WriteLn("Reading DMA8 Mem.");
}

EXPORT_C_(void) DEV9writeDMA8Mem(u32* pMem, int size) 
{
	// See above.
	Dev9Log.WriteLn("Writing DMA8 Mem.");
}
//#endif

EXPORT_C_(void) DEV9irqCallback(DEV9callback callback) 
{
	// Setting our callback. You will call it with DEV9irq(cycles),
	// Where cycles is the number of cycles till the irq is triggered.
	DEV9irq = callback;
}

int _DEV9irqHandler(void)
{
	// And this gets called when the irq is triggered.
	return 0;
}

EXPORT_C_(DEV9handler) DEV9irqHandler(void) 
{
	// Pass it to pcsx2.
	return (DEV9handler)_DEV9irqHandler;
}

EXPORT_C_(void) DEV9setSettingsDir(const char* dir)
{
	// Grab the ini directory.
    s_strIniPath = (dir == NULL) ? "inis/" : dir;
}

// extended funcs

EXPORT_C_(s32) DEV9test() 
{
	return 0;
}

EXPORT_C_(s32) DEV9freeze(int mode, freezeData *data)
{
	// This should store or retrieve any information, for if emulation 
	// gets suspended, or for savestates.
	switch(mode)
	{
		case FREEZE_LOAD:
			// Load previously saved data.
			break;
		case FREEZE_SAVE:
			// Save data.
			break;
		case FREEZE_SIZE:
			// return the size of the data.
			break;
	}
	return 0;
}

/* For operating systems that need an entry point for a dll/library, here it is. Defined in PS2Eext.h. */
ENTRY_POINT;
