/*
  _____     ___ ____ 
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2002, David Ryan (oobles@hotmail.com)
  ------------------------------------------------------------------------
  iop_heaplib.s		   Heap library function imports.
			   taken from heaplib in bios.
*/

	.text
	.set	noreorder


/* ############################### SYSMEM STUB ######## */
	.local	sysmem_stub
sysmem_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000101
	.ascii	"heaplib\0"
	.align	2

	.globl	CreateHeap			# 004
CreateHeap:
	j	$31
	li	$0, 4

	.globl	DestroyHeap			# 005
DestroyHeap:
	j	$31
	li	$0, 5

	.globl	HeapMalloc			# 006
HeapMalloc:
	j	$31
	li	$0, 6

	.globl	HeapFree			# 007
HeapFree:
	j	$31
	li	$0, 7

	.globl	HeapSize			# 008 
HeapSize:
	j	$31
	li	$0, 8 

	.word	0
	.word	0


