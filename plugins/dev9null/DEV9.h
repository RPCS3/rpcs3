#ifndef __DEV9_H__
#define __DEV9_H__

#include <stdio.h>

#ifdef __LINUX__
#include <gtk/gtk.h>
#else
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#endif

extern "C"
{
#define DEV9defs
#include "PS2Edefs.h"
}

/*#ifdef _MSC_VER
#define EXPORT_C_(type) extern "C" __declspec(dllexport) type CALLBACK
#else
#define EXPORT_C_(type) extern "C" type
#endif*/

#ifdef _MSC_VER
#define EXPORT_C_(type) extern "C" type CALLBACK
#else
#define EXPORT_C_(type) extern "C" type
#endif

extern const unsigned char version;
extern const unsigned char revision;
extern const unsigned char build;
extern const unsigned int minor;
extern const char *libraryName;

extern void (*DEV9irq)();
extern void SysMessage(const char *fmt, ...);

extern FILE *dev9Log;
void __Log(char *fmt, ...);


#endif
