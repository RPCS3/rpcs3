/*
  _____     ___ ____ 
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2002, David Ryan (Oobles@hotmail.com)
  ------------------------------------------------------------------------
  iop_stdio.s		   Stdio Import Library
			   taken from .irx files with symbol table
*/

	.text
	.set	noreorder


/* ############################### STDIO STUB ######## */
	.local	stdio_stub
stdio_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000102
	.ascii	"stdio\0\0\0"
	.align	2

	.globl	printf				# 004

printf:
	j	$31
	li	$0, 4

	.word	0
	.word	0


