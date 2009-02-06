#ifndef __DEV9_H__
#define __DEV9_H__

#include <stdio.h>

#define DEV9defs

#include "PS2Edefs.h"


FILE *dev9Log;
void __Log(char *fmt, ...);
void (*DEV9irq)();
void SysMessage(char *fmt, ...);


#endif
