
#include <tamtypes.h>
//#include <stdio.h>

void Kputc(u8 c) {
	*((u8*)0x1f80380c) = c;
}

void Kputs(u8 *s) {
	while (*s != 0) {
		Kputc(*s++);
	}
}

void Kmemcpy(void *dest, const void *src, int n) {
	const u8 *s = (u8*)src;
	u8 *d = (u8*)dest;

	while (n) {
		*d++ = *s++; n--;
	}
}
/*
int  Kprintf(const char *format, ...) {
	char buf[4096];
	va_list args;
	int ret;

	va_start(args, format);
	ret = vsnprintf(buf, 4096, format, args);
	va_end(args);

	Kputs(buf);
	return ret;
}
*/
void _start() {
	Kputs("STDIO start\n");
}

