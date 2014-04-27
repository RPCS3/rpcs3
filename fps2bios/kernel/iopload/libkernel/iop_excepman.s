/*
  _____     ___ ____ 
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2002, David Ryan (Oobles@hotmail.com)
  ------------------------------------------------------------------------
  iop_excepman.s		   Exception Manager Functions.
*/

	.text
	.set	noreorder


/* ############################### EXCEPMAN STUB ####### */
/* # Added by linuzappz, 10th May 2004                 # */

	.local	excepman_stub
excepman_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000101
	.ascii	"excepman"
	.align	2

	.globl	GetExHandlersTable			# 003
GetExHandlersTable:
	j	$31
	li	$0, 3

	.globl	RegisterExceptionHandler		# 004
RegisterExceptionHandler:
	j	$31
	li	$0, 4

	.globl	RegisterPriorityExceptionHandler	# 005
RegisterPriorityExceptionHandler:
	j	$31
	li	$0, 5

	.globl	RegisterDefaultExceptionHandler		# 006
RegisterDefaultExceptionHandler:
	j	$31
	li	$0, 6

	.globl	ReleaseExceptionHandler			# 007
ReleaseExceptionHandler:
	j	$31
	li	$0, 7

	.globl	ReleaseDefaultExceptionHandler		# 008
ReleaseDefaultExceptionHandler:
	j	$31
	li	$0, 8


	.word	0
	.word	0


