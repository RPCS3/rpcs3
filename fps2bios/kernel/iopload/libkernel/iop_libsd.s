/*
  _____     ___ ____ 
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2002, David Ryan (Oobles@hotmail.com)
  ------------------------------------------------------------------------
  iop_libsd.s		   Sound Library
			   taken from .irx files with symbol table 
			   Missing calls by Julian Tyler (lovely@crm114.net)
*/

	.text
	.set	noreorder


/* ############################### LIBSD STUB ######### */
/* # Added by Oobles, 7th March 2002                  # */

	.local	libsd_stub
libsd_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000104
	.ascii	"libsd\0\0"
	.align	2

/* Added by Julian Tyler (lovely) */
	.globl	SdQuit				# 0x02
SdQuit:
	j	$31
	li	$0, 0x02

	.globl  SdInit				# 0x04
SdInit:
	j	$31
	li	$0, 0x04

	.globl  SdSetParam			# 0x05
SdSetParam:
	j	$31
	li	$0, 0x05

	.globl	SdGetParam			# 0x06
SdGetParam:
	j	$31
	li	$0, 0x06

	.globl	SdSetSwitch			# 0x07
SdSetSwitch:
	j	$31
	li	$0, 0x07

/* Added by Julian Tyler (lovely) */
	.globl	SdGetSwitch			# 0x08
SdGetSwitch:
	j	$31
	li	$0, 0x08

	.globl	SdSetAddr			# 0x09
SdSetAddr:
	j	$31
	li	$0, 0x09

	.globl	SdGetAddr			# 0x0a
SdGetAddr:
	j	$31
	li	$0, 0x0a

	.globl	SdSetCoreAttr			# 0x0b
SdSetCoreAttr:
	j	$31
	li	$0, 0x0b

/* Added by Julian Tyler (lovely) 013-016 */
	.globl	SdGetCoreAttr			# 012
SdGetCoreAttr:
	j	$31
	li	$0, 0x0c

	.globl	SdNote2Pitch			# 013
SdNote2Pitch:
	j	$31
	li	$0, 0x0d

	.globl	SdPitch2Note			# 014
SdPitch2Note:
	j	$31
	li	$0, 0x0e

	.globl	SdProcBatch			# 015
SdProcBatch:
	j	$31
	li	$0, 0x0f

	.globl	SdProcBatchEx			# 016
SdProcBatchEx:
	j	$31
	li	$0, 0x10

	.globl	SdVoiceTrans			# 0x11
SdVoiceTrans:
	j	$31
	li	$0, 0x11

/* Added by Julian Tyler (lovely) 018-022 */

	.globl	SdBlockTrans			# 018
SdBlockTrans:
	j	$31
	li	$0, 0x12

	.globl	SdVoiceTransStatus		# 019
SdVoiceTransStatus:
	j	$31
	li	$0, 0x13

	.globl	SdBlockTransStatus		# 020
SdBlockTransStatus:
	j	$31
	li	$0, 0x14

	.globl	SdSetTransCallback		# 021
SdSetTransCallback:
	j	$31
	li	$0, 0x15

	.globl	SdSetIRQCallback		# 022
SdSetIRQCallback:
	j	$31
	li	$0, 0x16

	.globl	SdSetEffectAttr			# 0x17
SdSetEffectAttr:
	j	$31
	li	$0, 0x17

/* Added by Julian Tyler (lovely) 024-025 */

	.globl	SdGetEffectAttr			# 024
SdGetEffectAttr:
	j	$31
	li	$0, 0x18

	.globl	SdClearEffectWorkArea		# 025
SdClearEffectWorkArea:
	j	$31
	li	$0, 0x19

	.globl	SdSetTransIntrHandler		# 0x1a
SdSetTransIntrHandler:
	j	$31
	li	$0, 0x1a

	.globl	SdSetSpu2IntrHandler		# 0x1b
SdSetSpu2IntrHandler:
	j	$31
	li	$0, 0x1b

	.word	0
	.word	0

