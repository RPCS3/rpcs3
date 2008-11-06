

#ifdef _WIN32

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#endif

#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>
#include "PS2Etypes.h"
#include "DEV9.h"

#ifdef _WIN32

HINSTANCE hInst;

void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];
	va_start(list,fmt);
	vsprintf(tmp,fmt,list);
	va_end(list);
	MessageBox(0, tmp, "DEV9null Msg", 0);
}

#else

#include <X11/Xlib.h>

void SysMessage(char *fmt, ...) {
}

#endif

const unsigned char version  = PS2E_DEV9_VERSION;
const unsigned char revision = 0;
const unsigned char build    = 3;    // increase that with each version

static char *libraryName     = "DEV9null Driver";

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

s32 CALLBACK DEV9init() {

	return 0;
}

void CALLBACK DEV9shutdown() {
}

s32 CALLBACK DEV9open(void *pDsp) {

#ifdef _WIN32
#else
 Display* dsp = *(Display**)pDsp;
#endif
 return 0;
}

void CALLBACK DEV9close() {
	
}


u8   CALLBACK DEV9read8(u32 addr) {
  return 0;
}

u16  CALLBACK DEV9read16(u32 addr) {
 return 0;
}

u32  CALLBACK DEV9read32(u32 addr) {
  return 0;
}

void CALLBACK DEV9write8(u32 addr,  u8 value) {
}

void CALLBACK DEV9write16(u32 addr, u16 value) {

}

void CALLBACK DEV9write32(u32 addr, u32 value) {

}

void CALLBACK DEV9readDMA8Mem(u32 *pMem, int size) {

}

void CALLBACK DEV9writeDMA8Mem(u32* pMem, int size) {

}


void CALLBACK DEV9irqCallback(DEV9callback callback) {
	
}

DEV9handler CALLBACK DEV9irqHandler(void) {
	return NULL;
}


// extended funcs

s32  CALLBACK DEV9test() {
	return 0;
}

void CALLBACK DEV9configure()
{
	SysMessage("Nothing to Configure");
}
void CALLBACK DEV9about(){}

#ifdef _WIN32

BOOL APIENTRY DllMain(HANDLE hModule,                  // DLL INIT
                      DWORD  dwReason, 
                      LPVOID lpReserved) {
	hInst = (HINSTANCE)hModule;
	return TRUE;                                          // very quick :)
}

#endif
