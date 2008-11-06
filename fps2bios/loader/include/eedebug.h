#ifndef __EEDEBUG_H__
#define __EEDEBUG_H__

#include <tamtypes.h>
#include <kernel.h>


void __putc(u8 c);
void __puts(u8 *s);
int  __printf(const char *format, ...);

#endif /* __EEDEBUG_H__ */
