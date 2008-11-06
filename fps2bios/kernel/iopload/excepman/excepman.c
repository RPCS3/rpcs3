/*
 * The IOP exception manager module.
 */


#include <tamtypes.h>

#include "err.h"
#include "kexcepman.h"
#include "kloadcore.h"
#include "iopdebug.h"

#define _dprintf(fmt, args...) \
	__printf("excepman: " fmt, ## args)

int  _start();
int  excepman_reinit();
int  excepman_deinit();
void *GetExHandlersTable();
int RegisterExceptionHandler(int code, struct exHandler *handler);
int RegisterPriorityExceptionHandler(int code, int priority, struct exHandler *handler);
int RegisterDefaultExceptionHandler(struct exHandler *handler);
int ReleaseExceptionHandler(int code, struct exHandler *handler);
int ReleaseDefaultExceptionHandler(struct exHandler *handler);

struct export excepman_stub __attribute__((section(".text"))) ={
	0x41C00000,
	0,
	VER(1, 1),	// 1.1 => 0x101
	0,
	"excepman",
	(func)_start,	// entrypoint
	(func)excepman_reinit,
	(func)excepman_deinit,
	(func)GetExHandlersTable,
	(func)RegisterExceptionHandler,
	(func)RegisterPriorityExceptionHandler,
	(func)RegisterDefaultExceptionHandler,
	(func)ReleaseExceptionHandler,
	(func)ReleaseDefaultExceptionHandler,
	0
};

void* *extable;
struct exHandler *handlers[16];
void *defhandler; 

void iopException();

extern void defaultEH();

void defaultEHfunc()
{
	__printf("panic: excepman: %s\n", __FUNCTION__);
	__printf("EPC=0x%x, cause: %x\n", *(u32*)0x404, *(u32*)0x40C);
	__printf("EXECUTION HALTED\n");
	for (;;);
}

///////////////////////////////////////////////////////////////////////
void registerEH() {
	int i;

	_dprintf("%s\n", __FUNCTION__);

	// Set up the chain of exception handlers, making sure that each used one is terminated by the
	// default exception handler.
	for (i=0; i<16; i++) {
		if (handlers[i]) {
			struct exHandler *eh = handlers[i];
			while(eh->next){
				struct exHandler *p_handler = (struct exHandler *)(eh->info & ~3);
				struct exHandler *p_next_handler = (struct exHandler *)(((struct exHandler *)(eh->next))->info & ~3);
				p_handler->next = (void*)p_next_handler + 8;
				eh = eh->next;
			}
			{
			struct exHandler *p_handler = (struct exHandler *)(eh->info & ~3);
			p_handler->next = defhandler;
			}
		}
	}

	for (i=0; i<16; i++) {
		if (handlers[i]){
			extable[i]=(void*)((handlers[i]->info & ~3) + 8);
	    } else {
			extable[i]=defhandler;
		}
	}
}

//////////////////////////////entrypoint///////////////////////////////[00]
void Kputc(u8 c) {
	*((u8*)0x1f80380c) = c;
}

void Kprintnum(unsigned int n)
{
    int i = 0;
    while(i < 8) {
        u32 a = n>>28;
        if( a < 10 ) Kputc('0'+a);
        else Kputc('a'+(a-10));
        n <<= 4;
        ++i;
    }
}

int _start() {
	int *src, *dst;
	int i;

	_dprintf("%s\n", __FUNCTION__);

	for (i=0; i<16; i++){
		handlers[i] = NULL;
	}
	defhandler=NULL;
	extable = (void*)0x440;

	// Install the exception handler to address 0
	for (src=(int*)iopException, dst=0; (u32)dst<0x100; src++, dst++){
		*dst = *src;
	}

	RegisterDefaultExceptionHandler((struct exHandler *)&defaultEH);
	RegisterLibraryEntries(&excepman_stub);

	return 0;
}

///////////////////////////////////////////////////////////////////////[02]
int excepman_deinit(){
	return 0;
}

///////////////////////////////////////////////////////////////////////[01]
int excepman_reinit() {
	excepman_deinit();
	return _start();
}

///////////////////////////////////////////////////////////////////////[04]
int RegisterExceptionHandler(int code, struct exHandler *handler)
{
	return RegisterPriorityExceptionHandler(code, 2, handler);
}

// This is a pool of containers. They form a linked list. The info member of each points
// to a registered handler.
struct exHandler _list[32];
struct exHandler *list = NULL;


void link(struct exHandler *e) {
	e->next = list;
	list = e;
}

struct exHandler *unlink() {
	struct exHandler *e;
	if (list == NULL) {
		int i;

		list = _list;
		for (i=0; i<31; i++) {
			list[i].next = &list[i+1];
		}
		list[i].next = 0;
	}
	e = list;
	list = e->next;
	return e;
}

///////////////////////////////////////////////////////////////////////[05]
int RegisterPriorityExceptionHandler(int code, int priority, struct exHandler *handler)
{
	struct exHandler *n, *p, *p_prev;

	_dprintf("%s: code = %d, pri = %d, (handler=%x)\n", __FUNCTION__, code, priority, handler);
	if (code >= 16){
		_dprintf("%s ERROR_BAD_EXCODE\n", __FUNCTION__);
		return ERROR_BAD_EXCODE;
	}
	if (handler->next){
		_dprintf("%s ERROR_USED_EXCODE\n", __FUNCTION__);
		return ERROR_USED_EXCODE;
	}

	n = unlink();

	priority &= 3;
	n->info = ((u32)handler & ~3) | priority;
    n->next = NULL;

	p = handlers[code]; p_prev = NULL;
	// walk along the chain starting at the handlers[code] root to find our place in the queue
	while(p && ((p->info & 3) < priority)){
		p_prev = p; p = p->next;
	}
	n->next = p;
	if (p_prev == NULL){
		handlers[code] = n;
	} else {
		p_prev->next = n;
	}
	
	registerEH();
	return 0;
}

///////////////////////////////////////////////////////////////////////[06]
int RegisterDefaultExceptionHandler(struct exHandler *handler)
{
	_dprintf("%s\n", __FUNCTION__);
	if (handler->next){
		_dprintf("%s ERROR_USED_EXCODE\n", __FUNCTION__);
		return ERROR_USED_EXCODE;
	}
	
	handler->next = defhandler;
	defhandler = &handler->funccode;

	registerEH();
	return 0;
}

///////////////////////////////////////////////////////////////////////[07]
int ReleaseExceptionHandler(int code, struct exHandler *handler)
{
	struct exHandler *p, *p_prev;

	_dprintf("%s: %d\n", __FUNCTION__, code);
	if (code>=16) return ERROR_BAD_EXCODE;

	p_prev = NULL;
	for (p=handlers[code]; p != NULL; p_prev = p, p=p->next) {
		if ((struct exHandler *)(p->info & ~3) == handler) {
			// found it
			// Mark the handler as no longer used
			((struct exHandler *)(p->info & ~3))->next = NULL;

			if (p == handlers[code]){
				// special case, no more list
				handlers[code] = NULL;
			} else {
				// Remove container p from the list
				p_prev->next = p->next;
			}

			link(p);		// Add the container back to the pool
			registerEH();
			return 0;
		}
	}
	return	ERROR_EXCODE_NOTFOUND;
}

///////////////////////////////////////////////////////////////////////[08]
int ReleaseDefaultExceptionHandler(struct exHandler *handler)
{
	struct exHandler *p;

	_dprintf("%s\n", __FUNCTION__);
	for (p=defhandler; p->next; p=p->next-8) {
		if (p->next == handler->funccode) {
			p->next = handler->next;
			handler->next = 0;
			registerEH();
			return 0;
		}
	}
	return ERROR_EXCODE_NOTFOUND;
}

///////////////////////////////////////////////////////////////////////[03]
void *GetExHandlersTable()
{
	return extable;
}

// 256 bytes of this code are copied to memory address 0 in order to install the exception handling code.
// Take careful note of the alignment of the code at 0x40 and 0x80 if you change it at all.
void iopException() {
	__asm__ (
		".set noat\n"
		"nop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\n"

		/* 0x0040 */
		/* This the breakpoint exception vector location. */
		
		"sw    $26, 0x0420($0)\n" 
		"mfc0  $27, $14\n"
		"mfc0  $26, $13\n"
		"sw    $27, 0x0424($0)\n"
		"sw    $26, 0x0428($0)\n"
		"mfc0  $27, $12\n"
		"mfc0  $26, $7\n"
		"sw    $27, 0x042C($0)\n"
		"sw    $26, 0x0430($0)\n"
		"lw    $27, 0x047C($0)\n"
		"mtc0  $0,  $7\n"
		"jr    $27\n"
		"nop\n"
		"nop\n"
		"nop\n"
		"nop\n"

		/* 0x0080 */
		/* This is the general exception vector location. */
		"sw    $1,  0x0400($0)\n"     /* $1  saved at 0x400                */
		"sw    $26, 0x0410($0)\n"     /* $26 saved at 0x410                */
		"mfc0  $1,  $14\n"
		"mfc0  $26, $12\n"
		"sw    $1 , 0x0404($0)\n"     /* EPC    -> 0x404                   */
		"sw    $26, 0x0408($0)\n"     /* STATUS -> 0x408                   */
		"mfc0  $1,  $13\n"
		"sw    $1,  0x040C($0)\n"     /*  CAUSE -> 0x40C                   */
		"andi  $1,  0x3C\n"           /*  Isolate EXECODE                  */
		"lw    $1,  0x0440($1)\n"
		"jr    $1\n"                  /* jump via EXECODE table at 0x440   */
		"nop\n"
		"nop\n"
		"nop\n"
		"nop\n"
		"nop\n"

		"nop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\n"
		"nop\nnop\nnop\nnop\n"
		".set at\n"
	);
}
