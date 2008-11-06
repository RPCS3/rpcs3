#include <tamtypes.h>
void eestart() __attribute__ ((noreturn));
void iopstart() __attribute__ ((noreturn));

__asm__ (
	".org 0\n"
	".set noat\n"

	".global _start\n"
	"_start:\n"
	"mfc0  $at, $15\n"
	"sltiu $at, 0x59\n"
	"bne   $at, $0, __iopstart\n"
	"j     eestart\n"
    "nop\n"
	"__iopstart:\n"
	"j     iopstart\n"
    "nop\n");


/*
void _start() __attribute__ ((noreturn));
void _start() {
	register unsigned long PRid;

	__asm__ ("mfc0 %0, $15" : "=r"(PRid) : );
	if (PRid >= 0x59) eestart();
	else iopstart();
}*/

