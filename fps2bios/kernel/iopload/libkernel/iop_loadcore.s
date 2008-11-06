/*
  _____     ___ ____ 
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2002, David Ryan (Oobles@hotmail.com)
  ------------------------------------------------------------------------
  iop_loadcore.s	   Core IOP Functions.
			   taken from .irx files with symbol table.
*/

	.text
	.set	noreorder



/* ############################### LOADCORE STUB ###### */
/* # Added by Oobles, 5th March 2002                  # */

	.local	loadcore_stub
loadcore_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000101
	.ascii	"loadcore"
	.align	2

	.globl	GetLibraryEntryTable		# 0x03
GetLibraryEntryTable:
	j	$31
	li	$0, 3

	.globl	FlushIcache			# 0x05
FlushIcache:
	j	$31
	li	$0, 4

	.globl	FlushDcache			# 0x05
FlushDcache:
	j	$31
	li	$0, 5

	.global RegisterLibraryEntries		# 0x06
RegisterLibraryEntries:
	j	$31
	li	$0, 6

	.global ReleaseLibraryEntries
ReleaseLibraryEntries:
	j	$31
	li	$0, 7

	.global QueryLibraryEntryTable
QueryLibraryEntryTable:
	j	$31
	li	$0, 11

	.globl	QueryBootMode			# 0x0c
QueryBootMode:
	j	$31
	li	$0, 0x0c

	.word	0
	.word	0



