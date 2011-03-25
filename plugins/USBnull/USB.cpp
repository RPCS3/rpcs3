/*  USBnull
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

#include <stdlib.h>
#include <string>
using namespace std;

#include "USB.h"
string s_strIniPath="inis";
string s_strLogPath="logs";

const unsigned char version  = PS2E_USB_VERSION;
const unsigned char revision = 0;
const unsigned char build    = 7;    // increase that with each version

static char *libraryName     = "USBnull Driver";

USBcallback USBirq;
Config conf;
PluginLog USBLog;

s8 *usbregs, *ram;

void LogInit()
{
	const std::string LogFile(s_strLogPath + "/USBnull.log");
	setLoggingState();
	USBLog.Open(LogFile);
}

EXPORT_C_(void)  USBsetLogDir(const char* dir)
{
	// Get the path to the log directory.
	s_strLogPath = (dir==NULL) ? "logs" : dir;
	
	// Reload the log file after updated the path
	USBLog.Close();
	LogInit();
}

EXPORT_C_(u32) PS2EgetLibType()
{
	return PS2E_LT_USB;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	return libraryName;
}

EXPORT_C_(u32) PS2EgetLibVersion2(u32 type)
{
	return (version << 16) | (revision << 8) | build;
}

EXPORT_C_(s32) USBinit()
{
	LoadConfig();
	LogInit();
	USBLog.WriteLn("USBnull plugin version %d,%d", revision, build);
	USBLog.WriteLn("Initializing USBnull");

	// Initialize memory structures here.
	usbregs = (s8*)calloc(0x10000, 1);

	if (usbregs == NULL)
	{
		USBLog.Message("Error allocating memory");
		return -1;
	}

	return 0;
}

EXPORT_C_(void) USBshutdown()
{
	// Yes, we close things in the Shutdown routine, and
	// don't do anything in the close routine.
	USBLog.Close();
	
	free(usbregs);
	usbregs = NULL;
}

EXPORT_C_(s32) USBopen(void *pDsp)
{
	USBLog.WriteLn("Opening USBnull.");

	// Take care of anything else we need on opening, other then initialization.
	return 0;
}

EXPORT_C_(void) USBclose()
{
	USBLog.WriteLn("Closing USBnull.");
}

// Note: actually uncommenting the read/write functions I provided here
// caused uLauncher.elf to hang on startup, so careful when experimenting.
EXPORT_C_(u8) USBread8(u32 addr)
{
	u8 value = 0;

	switch(addr)
	{
		// Handle any appropriate addresses here.
		case 0x1f801600:
			USBLog.WriteLn("(USBnull) 8 bit read at address %lx", addr);
		break;

		default:
			//value = usbRu8(addr);
			USBLog.WriteLn("*(USBnull) 8 bit read at address %lx", addr);
			break;
	}
	return value;
}

EXPORT_C_(u16) USBread16(u32 addr)
{
	u16 value = 0;

	switch(addr)
	{
		// Handle any appropriate addresses here.
		case 0x1f801600:
			USBLog.WriteLn("(USBnull) 16 bit read at address %lx", addr);
		break;

		default:
			//value = usbRu16(addr);
			USBLog.WriteLn("(USBnull) 16 bit read at address %lx", addr);
	}
	return value;
}

EXPORT_C_(u32) USBread32(u32 addr)
{
	u32 value = 0;

	switch(addr)
	{
		// Handle any appropriate addresses here.
		case 0x1f801600:
			USBLog.WriteLn("(USBnull) 32 bit read at address %lx", addr);
		break;

		default:
			//value = usbRu32(addr);
			USBLog.WriteLn("(USBnull) 32 bit read at address %lx", addr);
	}
	return value;
}

EXPORT_C_(void) USBwrite8(u32 addr,  u8 value)
{
	switch(addr)
	{
		// Handle any appropriate addresses here.
		case 0x1f801600:
			USBLog.WriteLn("(USBnull) 8 bit write at address %lx value %x", addr, value);
		break;

		default:
			//usbRu8(addr) = value;
			USBLog.WriteLn("(USBnull) 8 bit write at address %lx value %x", addr, value);
	}
}

EXPORT_C_(void) USBwrite16(u32 addr, u16 value)
{
	switch(addr)
	{
		// Handle any appropriate addresses here.
		case 0x1f801600:
			USBLog.WriteLn("(USBnull) 16 bit write at address %lx value %x", addr, value);
		break;

		default:
			//usbRu16(addr) = value;
			USBLog.WriteLn("(USBnull) 16 bit write at address %lx value %x", addr, value);
	}
}

EXPORT_C_(void) USBwrite32(u32 addr, u32 value)
{
	switch(addr)
	{
		// Handle any appropriate addresses here.
		case 0x1f801600:
			USBLog.WriteLn("(USBnull) 16 bit write at address %lx value %x", addr, value);
		break;

		default:
			//usbRu32(addr) = value;
			USBLog.WriteLn("(USBnull) 32 bit write at address %lx value %x", addr, value);
	}
}

EXPORT_C_(void) USBirqCallback(USBcallback callback)
{
	// Register USBirq, so we can trigger an interrupt with it later.
	// It will be called as USBirq(cycles); where cycles is the number
	// of cycles before the irq is triggered.
	USBirq = callback;
}

EXPORT_C_(int) _USBirqHandler(void)
{
	// This is our USB irq handler, so if an interrupt gets triggered,
	// deal with it here.
	return 0;
}

EXPORT_C_(USBhandler) USBirqHandler(void)
{
	// Pass our handler to pcsx2.
	return (USBhandler)_USBirqHandler;
}

EXPORT_C_(void) USBsetRAM(void *mem)
{
	ram = (s8*)mem;
	USBLog.WriteLn("*Setting ram.");
}

EXPORT_C_(void) USBsetSettingsDir(const char* dir)
{
	// Get the path to the ini directory.
    s_strIniPath = (dir==NULL) ? "inis" : dir;
}

// extended funcs

EXPORT_C_(s32) USBfreeze(int mode, freezeData *data)
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

/*EXPORT_C_(void) USBasync(u32 cycles)
{
	// Optional function: Called in IopCounter.cpp.
}*/

EXPORT_C_(s32) USBtest()
{
	// 0 if the plugin works, non-0 if it doesn't.
	return 0;
}

/* For operating systems that need an entry point for a dll/library, here it is. Defined in PS2Eext.h. */
ENTRY_POINT;
