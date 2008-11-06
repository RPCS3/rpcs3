#include <tamtypes.h>
#include "romdir.h"

static void Kputc(u8 c) {
	*((u8*)0x1f80380c) = c;
}

static void Kputs(u8 *s) {
	while (*s != 0) {
		Kputc(*s++);
	}
}

static void Kmemcpy(void *dest, const void *src, int n) {
	const u8 *s = (u8*)src;
	u8 *d = (u8*)dest;

	while (n) {
		*d++ = *s++; n--;
	}
}

static void _iopstart() {
	struct rominfo ri;
	u8 *str;

    // setup iop
    *(char*)0xbf802070 = 9;
    *(int*)0xfffe0130 = 0xcc4;
    *(int*)0xfffe0130 = 0xcc0;
    if( (*(int*)0xbf801450)  & 8 )
        *(int*)0xfffe0130 = 0x1e988;
    else
        *(int*)0xfffe0130 = 0x1edd8;

	romdirGetFile("ROMVER", &ri);
	str = (u8*)(0xbfc00000 + ri.fileOffset);
	Kputs("iopstart: fps2bios v");
	Kputc(str[1]); Kputc('.'); Kputc(str[3]); Kputc('\n');

    // note, IOPBOOT has to be linked at location ri.fileOffset=0x600!!
	romdirGetFile("IOPBOOT", &ri);

	Kputs("_iopstart: loading IOPBOOT to 0xbfc0a180\n");
	//Kmemcpy((void*)0x80000000, (void*)(0xbfc00000 + ri.fileOffset), ri.fileSize);

	__asm__ (
        "li $a0, 2\n" // 2Mb
        "move $a1, $0\n"
        "move $a2, $0\n"
        "move $a3, $0\n"
		"move $26, %0\n"
		"jr $26\n"
        "nop\n" : : "r"(0xbfc00000+ri.fileOffset));
	for (;;);
}

__asm__ (
	".global iopstart\n"
	"iopstart:\n"
	"li	$sp, 0x001fffc0\n" // (2<<20)-0x40
    "move $fp, $sp\n"
	"j     _iopstart\n"
    "nop\n");


