#include <tamtypes.h>
#include "romdir.h"

static void Kputc(u8 c) {
	*((u8*)0x1000f180) = c;
}

static void Kputs(u8 *s) {
	while (*s != 0) {
		Kputc(*s++);
	}
}

static void Kmemcpy(void *dest, const void *src, int n) {
	const u8 *s = (u8*)src;
	u8 *d = (u8*)dest;

	while (n > 0) {
		*d++ = *s++; n--;
	}
}

void _eestart() {
	struct rominfo ri;
	u8 *str;

	romdirGetFile("ROMVER", &ri);
	str = (u8*)(0xbfc00000 + ri.fileOffset);
	Kputs("fps2bios v");
	Kputc(str[1]); Kputc('.'); Kputc(str[3]); Kputc('\n');

	romdirGetFile("EELOAD", &ri);

	Kputs("loading EELOAD to 0x80000000\n");

	Kmemcpy((void*)0x80000000, (void*)(0xbfc00000 + ri.fileOffset), ri.fileSize);

	__asm__ (
		"li  $26, 0x80001000\n"
		"jr  $26\n");
	for (;;);
}

__asm__ (
	".global eestart\n"
	"eestart:\n"
	"li	$sp, 0x80010000\n"
	"j     _eestart\n");


