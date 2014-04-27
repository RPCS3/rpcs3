/*
  _____     ___ ____ 
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2002, David Ryan (Oobles@hotmail.com)
  ------------------------------------------------------------------------
  iop_sifcmd.s		   Serial Interface Command Functions.
			   taken from .irx files with symbol table 
*/

	.text
	.set	noreorder


/* ############################### SIFCMD STUB ######## */
/* # Added by Oobles, 5th March 2002                  # */

	.local	sifcmd_stub
sifcmd_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000101
	.ascii	"sifcmd\0\0"
	.align	2

	.globl  SifInitCmd			# 0x04
SifInitCmd:
	j	$31
	li	$0, 0x04

	.globl	SifExitCmd			# 0x05
SifExitCmd:
	j	$31
	li	$0, 0x05

	.globl	SifGetSreg			# 0x06
SifGetSreg:
	j	$31
	li	$0, 0x06

	.globl	SifSetSreg			# 0x07
SifSetSreg:
	j	$31
	li	$0, 0x07

	.globl	SifSetCmdBuffer			# 0x08
SifSetCmdBuffer:
	j	$31
	li	$0, 0x08


	.globl	SifAddCmdHandler		# 0x0a
SifAddCmdHandler:
	j	$31
	li	$0, 0x0a

	.globl	SifRemoveCmdHandler		# 0x0b
SifRemoveCmdHandler:
	j	$31
	li	$0, 0x0b

	.globl  SifSendCmd			# 0x0c
SifSendCmd:
	j	$31
	li	$0, 0x0c

	.globl	iSifSendCmd			# 0x0d
iSifSendCmd:
	j	$31
	li	$0, 0x0d

	.globl	SifInitRpc			# 0x0E
SifInitRpc:
	j	$31
	li	$0, 0x0E

	.globl	SifBindRpc			# 0x0F
SifBindRpc:
	j	$31
	li	$0, 0x0F

	.globl	SifCallRpc			# 0x10
SifCallRpc:
	j	$31
	li	$0, 0x10

	.globl	SifRegisterRpc			# 0x11
SifRegisterRpc:
	j	$31
	li	$0, 0x11

	.globl	SifCheckStatRpc			# 0x12
SifCheckStatRpc:
	j	$31
	li	$0, 0x12

	.globl	SifSetRpcQueue			# 0x13
SifSetRpcQueue:
	j	$31
	li	$0, 0x13

	.globl	SifGetNextRequest		# 0x14
SifGetNextRequest:
	j	$31
	li	$0, 0x14

	.globl	SifExecRequest			# 0x15
SifExecRequest:
	j	$31
	li	$0, 0x15

	.globl	SifRpcLoop			# 0x16
SifRpcLoop:
	j	$31
	li	$0, 0x16

	.globl	SifRpcGetOtherData		# 0x17
SifRpcGetOtherData:
	j	$31
	li	$0, 0x17

	.globl	SifRemoveRpc			# 0x18
SifRemoveRpc:
	j	$31
	li	$0, 0x18

	.globl	SifRemoveRpcQueue		# 0x19
SifRemoveRpcQueue:
	j	$31
	li	$0, 0x19

	.globl	SifSendCmdIntr			# 0x20
SifSendCmdIntr:
	j	$31
	li	$0, 0x20

	.globl	iSifSendCmdIntr			# 0x21
iSifSendCmdIntr:
	j	$31
	li	$0, 0x21

	.word	0
	.word	0


