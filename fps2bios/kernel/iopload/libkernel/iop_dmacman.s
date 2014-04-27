/*
  _____     ___ ____ 
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2002, David Ryan (oobles@hotmail.com)
  ------------------------------------------------------------------------
  iop_dmacman.s		   DMAC library function imports.
			   taken from dmacman in bios.
*/

	.text
	.set	noreorder


/* ############################### DMACMAN STUB ######## */
	.local	dmacman_stub
dmacman_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000101
	.ascii	"dmacman\0"
	.align	2

	.globl	dmacSetDMA			# 028
dmacSetDMA:
	j	$31
	li	$0, 28

	.globl	dmacStartTransfer	# 032
dmacStartTransfer:
	j	$31
	li	$0, 32

	.globl	dmacSetVal			# 033
dmacSetVal:
	j	$31
	li	$0, 33

	.globl	dmacEnableDMAch		# 034
dmacEnableDMAch:
	j	$31
	li	$0, 34

	.globl	dmacDisableDMAch	# 035
dmacDisableDMAch:
	j	$31
	li	$0, 35


	.word	0
	.word	0


