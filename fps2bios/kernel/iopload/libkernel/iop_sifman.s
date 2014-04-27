/*
  _____     ___ ____ 
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2001, Gustavo Scotti (gustavo@scotti.com)
  ------------------------------------------------------------------------
  iop_sifman.s		   Serial Interface Manager Functions.
			   taken from .irx files with symbol table.
*/

	.text
	.set	noreorder


/* ############################### SIFMAN STUB ######## */
/* # Added by Oobles, 7th March 2002                  # */

	.local	sifman_stub
sifman_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000101
	.ascii	"sifman\0\0"
	.align	2

	.globl  SifDeinit			# 0x03
SifDeinit:
	j	$31
	li	$0, 0x03

	.globl  SifSIF2Init			# 0x04
SifSIF2Init:
	j	$31
	li	$0, 0x04

	.globl  SifInit				# 0x05
SifInit:
	j	$31
	li	$0, 0x05

	.globl	SifSetDChain			# 0x06
SifSetDChain:
	j	$31
	li	$0, 0x06

	.globl	SifSetDma			# 0x07
SifSetDma:
	j	$31
	li	$0, 0x07

	.globl	SifDmaStat			# 0x08
SifDmaStat:
	j	$31
	li	$0, 0x08

	.globl	SifSend				# 0x09
SifSend:
	j	$31
	li	$0, 0x09

	.globl	SifSendSync			# 0x0A
SifSendSync:
	j	$31
	li	$0, 0x0A

	.globl	SifIsSending			# 0x0B
SifIsSending:
	j	$31
	li	$0, 0x0B

	.globl	SifSetSIF0DMA			# 0x0C
SifSetSIF0DMA:
	j	$31
	li	$0, 0x0C

	.globl	SifSendSync0			# 0x0D
SifSendSync0:
	j	$31
	li	$0, 0x0D

	.globl	SifIsSending0			# 0x0E
SifIsSending0:
	j	$31
	li	$0, 0x0E

	.globl	SifSetSIF1DMA			# 0x0F
SifSetSIF1DMA:
	j	$31
	li	$0, 0x0F

	.globl	SifSendSync1			# 0x10
SifSendSync1:
	j	$31
	li	$0, 0x10

	.globl	SifIsSending1			# 0x11
SifIsSending1:
	j	$31
	li	$0, 0x11

	.globl	SifSetSIF2DMA			# 0x12
SifSetSIF2DMA:
	j	$31
	li	$0, 0x12

	.globl	SifSendSync2			# 0x13
SifSendSync2:
	j	$31
	li	$0, 0x13

	.globl	SifIsSending2			# 0x14
SifIsSending2:
	j	$31
	li	$0, 0x14

	.globl	SifGetEEIOPflags		# 0x15
SifGetEEIOPflags:
	j	$31
	li	$0, 0x15

	.globl	SifSetEEIOPflags		# 0x16
SifSetEEIOPflags:
	j	$31
	li	$0, 0x16

	.globl	SifGetIOPEEflags		# 0x17
SifGetIOPEEflags:
	j	$31
	li	$0, 0x17

	.globl	SifSetIOPEEflags		# 0x18
SifSetIOPEEflags:
	j	$31
	li	$0, 0x18

	.globl	SifGetEErcvaddr			# 0x19
SifGetEErcvaddr:
	j	$31
	li	$0, 0x19

	.globl	SifGetIOPrcvaddr		# 0x1A
SifGetIOPrcvaddr:
	j	$31
	li	$0, 0x1A

	.globl	SifSetIOPrcvaddr		# 0x1B
SifSetIOPrcvaddr:
	j	$31
	li	$0, 0x1B

	.globl	SifSet1450_2			# 0x1C
SifSet1450_2:
	j	$31
	li	$0, 0x1C

	.globl	SifCheckInit			# 0x1D
SifCheckInit:
	j	$31
	li	$0, 0x1D

	.globl	SifSet0CB			# 0x1E
SifSet0CB:
	j	$31
	li	$0, 0x1E

	.globl	SifReset0CB			# 0x1F
SifReset0CB:
	j	$31
	li	$0, 0x1F

	.globl	SifSetDmaIntr			# 0x20
SifSetDmaIntr:
	j	$31
	li	$0, 0x20

	.word	0
	.word	0


