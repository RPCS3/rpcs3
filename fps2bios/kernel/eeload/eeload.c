
#include <tamtypes.h>
#include <stdio.h>

#include "eeload.h"
#include "eeinit.h"
#include "eedebug.h"

void __attribute__((noreturn)) eeload_start() {
	void (*entry)();
	__puts("EELOAD start\n");

	__printf("about to SifInitRpc(0)\n");
	SifInitRpc(0);
    __printf("done rpc\n");

	entry = (void (*)())loadElfFile("INTRO");
	entry();

	entry = (void (*)())loadElfFile("LOADER");
	entry();

	for (;;);
}

void Kmemcpy(void *dest, const void *src, int n) {
	const u8 *s = (u8*)src;
	u8 *d = (u8*)dest;

	while (n) {
		*d++ = *s++; n--;
	}
}

