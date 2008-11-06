
#include <tamtypes.h>
#include <stdio.h>
#include <kernel.h>


void __putc(u8 c) {
	while (*((u32*)0x1000f130) & 0x8000) { __asm__ ("nop\nnop\nnop\n"); }

	*((u8*)0x1000f180) = c;
}

void __puts(u8 *s) {
	while (*s != 0) {
		__putc(*s++);
	}
}

int  __printf(const char *format, ...) {
	char buf[4096];
	va_list args;
	int ret;

	va_start(args, format);
	ret = vsnprintf(buf, 4096, format, args);
	va_end(args);

	__puts(buf);
	return ret;
}

