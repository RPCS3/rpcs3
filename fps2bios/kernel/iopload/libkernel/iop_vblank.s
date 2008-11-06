/*
  _____     ___ ____
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2002, Nick Van Veen (nickvv@xtra.co.nz)
  ------------------------------------------------------------------------
  iop_vblank.s		   Vblank Manager Functions.
			   taken from .irx files with symbol table.
*/

	.text
	.set	noreorder


/* ################################ VBLANK STUB ####### */
/* # Added by Sjeep, 28th March 2002                  # */

	.local	vblank_stub
vblank_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000101
	.ascii	"vblank\0\0"
	.align	2

	.globl WaitVblankStart
WaitVblankStart:
 	jr      $31
	li      $0,4

	.globl WaitVblankEnd
WaitVblankEnd:
	jr      $31
	li      $0,5

	.globl WaitVblank
WaitVblank:
	jr      $31
	li      $0,6

	.globl WaitNonVblank
WaitNonVblank:
	jr      $31
	li      $0,7

	.globl RegisterVblankHandler
RegisterVblankHandler:
	jr      $31
	li      $0,8

	.globl ReleaseVblankHandler
ReleaseVblankHandler:
	jr      $31
	li      $0,9
