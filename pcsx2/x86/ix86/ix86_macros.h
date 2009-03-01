/*  Pcsx2 - Pc Ps2 Emulator
*  Copyright (C) 2002-2009  Pcsx2 Team
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*  
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*  
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/*
* ix86 definitions v0.6.2
*  Authors: linuzappz <linuzappz@pcsx.net>
*           alexey silinov
*           goldfinger
*           shadow < shadow@pcsx2.net >
*			 cottonvibes(@gmail.com)
*/

#pragma once

//------------------------------------------------------------------
// jump/align functions
//------------------------------------------------------------------
#define x86SetPtr			ex86SetPtr<0>
#define x86SetJ8			ex86SetJ8<0>
#define x86SetJ8A			ex86SetJ8A<0>
#define x86SetJ16			ex86SetJ16<0>
#define x86SetJ16A			ex86SetJ16A<0>
#define x86SetJ32			ex86SetJ32<0>
#define x86SetJ32A			ex86SetJ32A<0>
#define x86Align			ex86Align<0>
#define x86AlignExecutable	ex86AlignExecutable<0>
//------------------------------------------------------------------

//------------------------------------------------------------------
// *IX86 intructions*
//------------------------------------------------------------------

#define STC eSTC<0>
#define CLC eCLC<0>
#define NOP eNOP<0>

//------------------------------------------------------------------
// mov instructions
//------------------------------------------------------------------
#define MOV64RtoR eMOV64RtoR<0>
#define MOV64RtoM eMOV64RtoM<0>
#define MOV64MtoR eMOV64MtoR<0>
#define MOV64I32toM eMOV64I32toM<0>
#define MOV64I32toR eMOV64I32toR<0>
#define MOV64ItoR eMOV64ItoR<0>
#define MOV64ItoRmOffset eMOV64ItoRmOffset<0>
#define MOV64RmOffsettoR eMOV64RmOffsettoR<0>
#define MOV64RmStoR eMOV64RmStoR<0>
#define MOV64RtoRmOffset eMOV64RtoRmOffset<0>
#define MOV64RtoRmS eMOV64RtoRmS<0>
#define MOV32RtoR eMOV32RtoR<0>
#define MOV32RtoM eMOV32RtoM<0>
#define MOV32MtoR eMOV32MtoR<0>
#define MOV32RmtoR eMOV32RmtoR<0>
#define MOV32RmtoROffset eMOV32RmtoROffset<0>
#define MOV32RmStoR eMOV32RmStoR<0>
#define MOV32RmSOffsettoR eMOV32RmSOffsettoR<0>
#define MOV32RtoRm eMOV32RtoRm<0>
#define MOV32RtoRmS eMOV32RtoRmS<0>
#define MOV32ItoR eMOV32ItoR<0>
#define MOV32ItoM eMOV32ItoM<0>
#define MOV32ItoRmOffset eMOV32ItoRmOffset<0>
#define MOV32RtoRmOffset eMOV32RtoRmOffset<0>
#define MOV16RtoM eMOV16RtoM<0>
#define MOV16MtoR eMOV16MtoR<0>
#define MOV16RmtoR eMOV16RmtoR<0>
#define MOV16RmtoROffset eMOV16RmtoROffset<0>
#define MOV16RmSOffsettoR eMOV16RmSOffsettoR<0>
#define MOV16RtoRm eMOV16RtoRm<0>
#define MOV16ItoM eMOV16ItoM<0>
#define MOV16RtoRmS eMOV16RtoRmS<0>
#define MOV16ItoR eMOV16ItoR<0>
#define MOV16ItoRmOffset eMOV16ItoRmOffset<0>
#define MOV16RtoRmOffset eMOV16RtoRmOffset<0>
#define MOV8RtoM eMOV8RtoM<0>
#define MOV8MtoR eMOV8MtoR<0>
#define MOV8RmtoR eMOV8RmtoR<0>
#define MOV8RmtoROffset eMOV8RmtoROffset<0>
#define MOV8RmSOffsettoR eMOV8RmSOffsettoR<0>
#define MOV8RtoRm eMOV8RtoRm<0>
#define MOV8ItoM eMOV8ItoM<0>
#define MOV8ItoR eMOV8ItoR<0>
#define MOV8ItoRmOffset eMOV8ItoRmOffset<0>
#define MOV8RtoRmOffset eMOV8RtoRmOffset<0>
#define MOVSX32R8toR eMOVSX32R8toR<0>
#define MOVSX32Rm8toR eMOVSX32Rm8toR<0>
#define MOVSX32Rm8toROffset eMOVSX32Rm8toROffset<0>
#define MOVSX32M8toR eMOVSX32M8toR<0>
#define MOVSX32R16toR eMOVSX32R16toR<0>
#define MOVSX32Rm16toR eMOVSX32Rm16toR<0>
#define MOVSX32Rm16toROffset eMOVSX32Rm16toROffset<0>
#define MOVSX32M16toR eMOVSX32M16toR<0>
#define MOVZX32R8toR eMOVZX32R8toR<0>
#define MOVZX32Rm8toR eMOVZX32Rm8toR<0>
#define MOVZX32Rm8toROffset eMOVZX32Rm8toROffset<0>
#define MOVZX32M8toR eMOVZX32M8toR<0>
#define MOVZX32R16toR eMOVZX32R16toR<0>
#define MOVZX32Rm16toR eMOVZX32Rm16toR<0>
#define MOVZX32Rm16toROffset eMOVZX32Rm16toROffset<0>
#define MOVZX32M16toR eMOVZX32M16toR<0>
#define CMOVBE32RtoR eCMOVBE32RtoR<0>
#define CMOVBE32MtoR eCMOVBE32MtoR<0>
#define CMOVB32RtoR eCMOVB32RtoR<0>
#define CMOVB32MtoR eCMOVB32MtoR<0>
#define CMOVAE32RtoR eCMOVAE32RtoR<0>
#define CMOVAE32MtoR eCMOVAE32MtoR<0>
#define CMOVA32RtoR eCMOVA32RtoR<0>
#define CMOVA32MtoR eCMOVA32MtoR<0>
#define CMOVO32RtoR eCMOVO32RtoR<0>
#define CMOVO32MtoR eCMOVO32MtoR<0>
#define CMOVP32RtoR eCMOVP32RtoR<0>
#define CMOVP32MtoR eCMOVP32MtoR<0>
#define CMOVS32RtoR eCMOVS32RtoR<0>
#define CMOVS32MtoR eCMOVS32MtoR<0>
#define CMOVNO32RtoR eCMOVNO32RtoR<0>
#define CMOVNO32MtoR eCMOVNO32MtoR<0>
#define CMOVNP32RtoR eCMOVNP32RtoR<0>
#define CMOVNP32MtoR eCMOVNP32MtoR<0>
#define CMOVNS32RtoR eCMOVNS32RtoR<0>
#define CMOVNS32MtoR eCMOVNS32MtoR<0>
#define CMOVNE32RtoR eCMOVNE32RtoR<0>
#define CMOVNE32MtoR eCMOVNE32MtoR<0>
#define CMOVE32RtoR eCMOVE32RtoR<0>
#define CMOVE32MtoR eCMOVE32MtoR<0>
#define CMOVG32RtoR eCMOVG32RtoR<0>
#define CMOVG32MtoR eCMOVG32MtoR<0>
#define CMOVGE32RtoR eCMOVGE32RtoR<0>
#define CMOVGE32MtoR eCMOVGE32MtoR<0>
#define CMOVL32RtoR eCMOVL32RtoR<0>
#define CMOVL32MtoR eCMOVL32MtoR<0>
#define CMOVLE32RtoR eCMOVLE32RtoR<0>
#define CMOVLE32MtoR eCMOVLE32MtoR<0>
//------------------------------------------------------------------
// arithmetic instructions 
//------------------------------------------------------------------
#define ADD64ItoR eADD64ItoR<0>
#define ADD64MtoR eADD64MtoR<0>
#define ADD32ItoEAX eADD32ItoEAX<0>
#define ADD32ItoR eADD32ItoR<0>
#define ADD32ItoM eADD32ItoM<0>
#define ADD32ItoRmOffset eADD32ItoRmOffset<0>
#define ADD32RtoR eADD32RtoR<0>
#define ADD32RtoM eADD32RtoM<0>
#define ADD32MtoR eADD32MtoR<0>
#define ADD16RtoR eADD16RtoR<0>
#define ADD16ItoR eADD16ItoR<0>
#define ADD16ItoM eADD16ItoM<0>
#define ADD16RtoM eADD16RtoM<0>
#define ADD16MtoR eADD16MtoR<0>
#define ADD8MtoR eADD8MtoR<0>
#define ADC32ItoR eADC32ItoR<0>
#define ADC32ItoM eADC32ItoM<0>
#define ADC32RtoR eADC32RtoR<0>
#define ADC32MtoR eADC32MtoR<0>
#define ADC32RtoM eADC32RtoM<0>
#define INC32R eINC32R<0>
#define INC32M eINC32M<0>
#define INC16R eINC16R<0>
#define INC16M eINC16M<0>
#define SUB64MtoR eSUB64MtoR<0>
#define SUB32ItoR eSUB32ItoR<0>
#define SUB32ItoM eSUB32ItoM<0>
#define SUB32RtoR eSUB32RtoR<0>
#define SUB32MtoR eSUB32MtoR<0>
#define SUB32RtoM eSUB32RtoM<0>
#define SUB16RtoR eSUB16RtoR<0>
#define SUB16ItoR eSUB16ItoR<0>
#define SUB16ItoM eSUB16ItoM<0>
#define SUB16MtoR eSUB16MtoR<0>
#define SBB64RtoR eSBB64RtoR<0>
#define SBB32ItoR eSBB32ItoR<0>
#define SBB32ItoM eSBB32ItoM<0>
#define SBB32RtoR eSBB32RtoR<0>
#define SBB32MtoR eSBB32MtoR<0>
#define SBB32RtoM eSBB32RtoM<0>
#define DEC32R eDEC32R<0>
#define DEC32M eDEC32M<0>
#define DEC16R eDEC16R<0>
#define DEC16M eDEC16M<0>
#define MUL32R eMUL32R<0>
#define MUL32M eMUL32M<0>
#define IMUL32R eIMUL32R<0>
#define IMUL32M eIMUL32M<0>
#define IMUL32RtoR eIMUL32RtoR<0>
#define DIV32R eDIV32R<0>
#define DIV32M eDIV32M<0>
#define IDIV32R eIDIV32R<0>
#define IDIV32M eIDIV32M<0>
//------------------------------------------------------------------
// shifting instructions 
//------------------------------------------------------------------
#define SHL64ItoR eSHL64ItoR<0>
#define SHL64CLtoR eSHL64CLtoR<0>
#define SHR64ItoR eSHR64ItoR<0>
#define SHR64CLtoR eSHR64CLtoR<0>
#define SAR64ItoR eSAR64ItoR<0>
#define SAR64CLtoR eSAR64CLtoR<0>
#define SHL32ItoR eSHL32ItoR<0>
#define SHL32ItoM eSHL32ItoM<0>
#define SHL32CLtoR eSHL32CLtoR<0>
#define SHL16ItoR eSHL16ItoR<0>
#define SHL8ItoR eSHL8ItoR<0>
#define SHR32ItoR eSHR32ItoR<0>
#define SHR32ItoM eSHR32ItoM<0>
#define SHR32CLtoR eSHR32CLtoR<0>
#define SHR16ItoR eSHR16ItoR<0>
#define SHR8ItoR eSHR8ItoR<0>
#define SAR32ItoR eSAR32ItoR<0>
#define SAR32ItoM eSAR32ItoM<0>
#define SAR32CLtoR eSAR32CLtoR<0>
#define SAR16ItoR eSAR16ItoR<0>
#define ROR32ItoR eROR32ItoR<0>
#define RCR32ItoR eRCR32ItoR<0>
#define RCR32ItoM eRCR32ItoM<0>
#define SHLD32ItoR eSHLD32ItoR<0>
#define SHRD32ItoR eSHRD32ItoR<0>
//------------------------------------------------------------------
// logical instructions
//------------------------------------------------------------------
#define OR64ItoR eOR64ItoR<0>
#define OR64MtoR eOR64MtoR<0>
#define OR64RtoR eOR64RtoR<0>
#define OR64RtoM eOR64RtoM<0>
#define OR32ItoR eOR32ItoR<0>
#define OR32ItoM eOR32ItoM<0>
#define OR32RtoR eOR32RtoR<0>
#define OR32RtoM eOR32RtoM<0>
#define OR32MtoR eOR32MtoR<0>
#define OR16RtoR eOR16RtoR<0>
#define OR16ItoR eOR16ItoR<0>
#define OR16ItoM eOR16ItoM<0>
#define OR16MtoR eOR16MtoR<0>
#define OR16RtoM eOR16RtoM<0>
#define OR8RtoR eOR8RtoR<0>
#define OR8RtoM eOR8RtoM<0>
#define OR8ItoM eOR8ItoM<0>
#define OR8MtoR eOR8MtoR<0>
#define XOR64ItoR eXOR64ItoR<0>
#define XOR64RtoR eXOR64RtoR<0>
#define XOR64MtoR eXOR64MtoR<0>
#define XOR64RtoR eXOR64RtoR<0>
#define XOR64RtoM eXOR64RtoM<0>
#define XOR32ItoR eXOR32ItoR<0>
#define XOR32ItoM eXOR32ItoM<0>
#define XOR32RtoR eXOR32RtoR<0>
#define XOR16RtoR eXOR16RtoR<0>
#define XOR32RtoM eXOR32RtoM<0>
#define XOR32MtoR eXOR32MtoR<0>
#define XOR16RtoM eXOR16RtoM<0>
#define XOR16ItoR eXOR16ItoR<0>
#define AND64I32toR eAND64I32toR<0>
#define AND64MtoR eAND64MtoR<0>
#define AND64RtoM eAND64RtoM<0>
#define AND64RtoR eAND64RtoR<0>
#define AND64I32toM eAND64I32toM<0>
#define AND32ItoR eAND32ItoR<0>
#define AND32I8toR eAND32I8toR<0>
#define AND32ItoM eAND32ItoM<0>
#define AND32I8toM eAND32I8toM<0>
#define AND32RtoR eAND32RtoR<0>
#define AND32RtoM eAND32RtoM<0>
#define AND32MtoR eAND32MtoR<0>
#define AND16RtoR eAND16RtoR<0>
#define AND16ItoR eAND16ItoR<0>
#define AND16ItoM eAND16ItoM<0>
#define AND16RtoM eAND16RtoM<0>
#define AND16MtoR eAND16MtoR<0>
#define AND8ItoR eAND8ItoR<0>
#define AND8ItoM eAND8ItoM<0>
#define AND8RtoM eAND8RtoM<0>
#define AND8MtoR eAND8MtoR<0>
#define AND8RtoR eAND8RtoR<0>
#define NOT64R eNOT64R<0>
#define NOT32R eNOT32R<0>
#define NOT32M eNOT32M<0>
#define NEG64R eNEG64R<0>
#define NEG32R eNEG32R<0>
#define NEG32M eNEG32M<0>
#define NEG16R eNEG16R<0>
//------------------------------------------------------------------
// jump/call instructions
//------------------------------------------------------------------
#define JMP8 eJMP8<0>
#define JP8 eJP8<0>
#define JNP8 eJNP8<0>
#define JE8 eJE8<0>
#define JZ8 eJZ8<0>
#define JG8 eJG8<0>
#define JGE8 eJGE8<0>
#define JS8 eJS8<0>
#define JNS8 eJNS8<0>
#define JL8 eJL8<0>
#define JA8 eJA8<0>
#define JAE8 eJAE8<0>
#define JB8 eJB8<0>
#define JBE8 eJBE8<0>
#define JLE8 eJLE8<0>
#define JNE8 eJNE8<0>
#define JNZ8 eJNZ8<0>
#define JNG8 eJNG8<0>
#define JNGE8 eJNGE8<0>
#define JNL8 eJNL8<0>
#define JNLE8 eJNLE8<0>
#define JO8 eJO8<0>
#define JNO8 eJNO8<0>
#define JMP32 eJMP32<0>
#define JNS32 eJNS32<0>
#define JS32 eJS32<0>
#define JB32 eJB32<0>
#define JE32 eJE32<0>
#define JZ32 eJZ32<0>
#define JG32 eJG32<0>
#define JGE32 eJGE32<0>
#define JL32 eJL32<0>
#define JLE32 eJLE32<0>
#define JA32 eJA32<0>
#define JAE32 eJAE32<0>
#define JNE32 eJNE32<0>
#define JNZ32 eJNZ32<0>
#define JNG32 eJNG32<0>
#define JNGE32 eJNGE32<0>
#define JNL32 eJNL32<0>
#define JNLE32 eJNLE32<0>
#define JO32 eJO32<0>
#define JNO32 eJNO32<0>
#define JS32 eJS32<0>
#define JMPR eJMPR<0>
#define JMP32M eJMP32M<0>
#define CALLFunc eCALLFunc<0>
#define CALL32 eCALL32<0>
#define CALL32R eCALL32R<0>
#define CALL32M eCALL32M<0>
//------------------------------------------------------------------
// misc instructions
//------------------------------------------------------------------
#define CMP64I32toR eCMP64I32toR<0>
#define CMP64MtoR eCMP64MtoR<0>
#define CMP64RtoR eCMP64RtoR<0>
#define CMP32ItoR eCMP32ItoR<0>
#define CMP32ItoM eCMP32ItoM<0>
#define CMP32RtoR eCMP32RtoR<0>
#define CMP32MtoR eCMP32MtoR<0>
#define CMP32I8toRm eCMP32I8toRm<0>
#define CMP32I8toRmOffset8 eCMP32I8toRmOffset8<0>
#define CMP32I8toM eCMP32I8toM<0>
#define CMP16ItoR eCMP16ItoR<0>
#define CMP16ItoM eCMP16ItoM<0>
#define CMP16RtoR eCMP16RtoR<0>
#define CMP16MtoR eCMP16MtoR<0>
#define CMP8ItoR eCMP8ItoR<0>
#define CMP8MtoR eCMP8MtoR<0>
#define TEST32ItoR eTEST32ItoR<0>
#define TEST32ItoM eTEST32ItoM<0>
#define TEST32RtoR eTEST32RtoR<0>
#define TEST32ItoRm eTEST32ItoRm<0>
#define TEST16ItoR eTEST16ItoR<0>
#define TEST16RtoR eTEST16RtoR<0>
#define TEST8RtoR eTEST8RtoR<0>
#define TEST8ItoR eTEST8ItoR<0>
#define TEST8ItoM eTEST8ItoM<0>
#define SETS8R eSETS8R<0>
#define SETL8R eSETL8R<0>
#define SETGE8R eSETGE8R<0>
#define SETG8R eSETG8R<0>
#define SETA8R eSETA8R<0>
#define SETAE8R eSETAE8R<0>
#define SETB8R eSETB8R<0>
#define SETNZ8R eSETNZ8R<0>
#define SETZ8R eSETZ8R<0>
#define SETE8R eSETE8R<0>
#define PUSH32I ePUSH32I<0>
#define PUSH32R ePUSH32R<0>
#define PUSH32M ePUSH32M<0>
#define PUSH32I ePUSH32I<0>
#define POP32R ePOP32R<0>
#define PUSHA32 ePUSHA32<0>
#define POPA32 ePOPA32<0>
#define PUSHR ePUSHR<0>
#define POPR ePOPR<0>
#define PUSHFD ePUSHFD<0>
#define POPFD ePOPFD<0>
#define RET eRET<0>
#define RET2 eRET2<0>
#define CBW eCBW<0>
#define CWDE eCWDE<0>
#define CWD eCWD<0>
#define CDQ eCDQ<0>
#define CDQE eCDQE<0>
#define LAHF eLAHF<0>
#define SAHF eSAHF<0>
#define BT32ItoR eBT32ItoR<0>
#define BTR32ItoR eBTR32ItoR<0>
#define BSRRtoR eBSRRtoR<0>
#define BSWAP32R eBSWAP32R<0>
#define LEA16RtoR eLEA16RtoR<0>
#define LEA32RtoR eLEA32RtoR<0>
#define LEA16RRtoR eLEA16RRtoR<0>
#define LEA32RRtoR eLEA32RRtoR<0>
#define LEA16RStoR eLEA16RStoR<0>
#define LEA32RStoR eLEA32RStoR<0>
//------------------------------------------------------------------

//------------------------------------------------------------------
// FPU instructions 
//------------------------------------------------------------------
#define FILD32 eFILD32<0>
#define FISTP32 eFISTP32<0>
#define FLD32 eFLD32<0>
#define FLD eFLD<0>
#define FLD1 eFLD1<0>
#define FLDL2E eFLDL2E<0>
#define FST32 eFST32<0>
#define FSTP32 eFSTP32<0>
#define FSTP eFSTP<0>
#define FLDCW eFLDCW<0>
#define FNSTCW eFNSTCW<0>
#define FADD32Rto0 eFADD32Rto0<0>
#define FADD320toR eFADD320toR<0>
#define FSUB32Rto0 eFSUB32Rto0<0>
#define FSUB320toR eFSUB320toR<0>
#define FSUBP eFSUBP<0>
#define FMUL32Rto0 eFMUL32Rto0<0>
#define FMUL320toR eFMUL320toR<0>
#define FDIV32Rto0 eFDIV32Rto0<0>
#define FDIV320toR eFDIV320toR<0>
#define FDIV320toRP eFDIV320toRP<0>
#define FADD32 eFADD32<0>
#define FSUB32 eFSUB32<0>
#define FMUL32 eFMUL32<0>
#define FDIV32 eFDIV32<0>
#define FCOMI eFCOMI<0>
#define FCOMIP eFCOMIP<0>
#define FUCOMI eFUCOMI<0>
#define FUCOMIP eFUCOMIP<0>
#define FCOM32 eFCOM32<0>
#define FABS eFABS<0>
#define FSQRT eFSQRT<0>
#define FPATAN eFPATAN<0>
#define FSIN eFSIN<0>
#define FCHS eFCHS<0>
#define FCMOVB32 eFCMOVB32<0>
#define FCMOVE32 eFCMOVE32<0>
#define FCMOVBE32 eFCMOVBE32<0>
#define FCMOVU32 eFCMOVU32<0>
#define FCMOVNB32 eFCMOVNB32<0>
#define FCMOVNE32 eFCMOVNE32<0>
#define FCMOVNBE32 eFCMOVNBE32<0>
#define FCMOVNU32 eFCMOVNU32<0>
#define FCOMP32 eFCOMP32<0>
#define FNSTSWtoAX eFNSTSWtoAX<0>
#define FXAM eFXAM<0>
#define FDECSTP eFDECSTP<0>
#define FRNDINT eFRNDINT<0>
#define FXCH eFXCH<0>
#define F2XM1 eF2XM1<0>
#define FSCALE eFSCALE<0>
//------------------------------------------------------------------

//------------------------------------------------------------------
// MMX instructions
//------------------------------------------------------------------
#define EMMS eEMMS<0>
#define MOVQMtoR eMOVQMtoR<0>
#define MOVQRtoM eMOVQRtoM<0>
#define PANDRtoR ePANDRtoR<0>
#define PANDNRtoR ePANDNRtoR<0>
#define PANDMtoR ePANDMtoR<0>
#define PANDNRtoR ePANDNRtoR<0>
#define PANDNMtoR ePANDNMtoR<0>
#define PORRtoR ePORRtoR<0>
#define PORMtoR ePORMtoR<0>
#define PXORRtoR ePXORRtoR<0>
#define PXORMtoR ePXORMtoR<0>
#define PSLLQRtoR ePSLLQRtoR<0>
#define PSLLQMtoR ePSLLQMtoR<0>
#define PSLLQItoR ePSLLQItoR<0>
#define PSRLQRtoR ePSRLQRtoR<0>
#define PSRLQMtoR ePSRLQMtoR<0>
#define PSRLQItoR ePSRLQItoR<0>
#define PADDUSBRtoR ePADDUSBRtoR<0>
#define PADDUSBMtoR ePADDUSBMtoR<0>
#define PADDUSWRtoR ePADDUSWRtoR<0>
#define PADDUSWMtoR ePADDUSWMtoR<0>
#define PADDBRtoR ePADDBRtoR<0>
#define PADDBMtoR ePADDBMtoR<0>
#define PADDWRtoR ePADDWRtoR<0>
#define PADDWMtoR ePADDWMtoR<0>
#define PADDDRtoR ePADDDRtoR<0>
#define PADDDMtoR ePADDDMtoR<0>
#define PADDSBRtoR ePADDSBRtoR<0>
#define PADDSWRtoR ePADDSWRtoR<0>
#define PADDQMtoR ePADDQMtoR<0>
#define PADDQRtoR ePADDQRtoR<0>
#define PSUBSBRtoR ePSUBSBRtoR<0>
#define PSUBSWRtoR ePSUBSWRtoR<0>
#define PSUBBRtoR ePSUBBRtoR<0>
#define PSUBWRtoR ePSUBWRtoR<0>
#define PSUBDRtoR ePSUBDRtoR<0>
#define PSUBDMtoR ePSUBDMtoR<0>
#define PSUBQMtoR ePSUBQMtoR<0>
#define PSUBQRtoR ePSUBQRtoR<0>
#define PMULUDQMtoR ePMULUDQMtoR<0>
#define PMULUDQRtoR ePMULUDQRtoR<0>
#define PCMPEQBRtoR ePCMPEQBRtoR<0>
#define PCMPEQWRtoR ePCMPEQWRtoR<0>
#define PCMPEQDRtoR ePCMPEQDRtoR<0>
#define PCMPEQDMtoR ePCMPEQDMtoR<0>
#define PCMPGTBRtoR ePCMPGTBRtoR<0>
#define PCMPGTWRtoR ePCMPGTWRtoR<0>
#define PCMPGTDRtoR ePCMPGTDRtoR<0>
#define PCMPGTDMtoR ePCMPGTDMtoR<0>
#define PSRLWItoR ePSRLWItoR<0>
#define PSRLDItoR ePSRLDItoR<0>
#define PSRLDRtoR ePSRLDRtoR<0>
#define PSLLWItoR ePSLLWItoR<0>
#define PSLLDItoR ePSLLDItoR<0>
#define PSLLDRtoR ePSLLDRtoR<0>
#define PSRAWItoR ePSRAWItoR<0>
#define PSRADItoR ePSRADItoR<0>
#define PSRADRtoR ePSRADRtoR<0>
#define PUNPCKLDQRtoR ePUNPCKLDQRtoR<0>
#define PUNPCKLDQMtoR ePUNPCKLDQMtoR<0>
#define PUNPCKHDQRtoR ePUNPCKHDQRtoR<0>
#define PUNPCKHDQMtoR ePUNPCKHDQMtoR<0>
#define MOVQ64ItoR eMOVQ64ItoR<0>
#define MOVQRtoR eMOVQRtoR<0>
#define MOVQRmtoROffset eMOVQRmtoROffset<0>
#define MOVQRtoRmOffset eMOVQRtoRmOffset<0>
#define MOVDMtoMMX eMOVDMtoMMX<0>
#define MOVDMMXtoM eMOVDMMXtoM<0>
#define MOVD32RtoMMX eMOVD32RtoMMX<0>
#define MOVD32RmtoMMX eMOVD32RmtoMMX<0>
#define MOVD32RmOffsettoMMX eMOVD32RmOffsettoMMX<0>
#define MOVD32MMXtoR eMOVD32MMXtoR<0>
#define MOVD32MMXtoRm eMOVD32MMXtoRm<0>
#define MOVD32MMXtoRmOffset eMOVD32MMXtoRmOffset<0>
#define PINSRWRtoMMX ePINSRWRtoMMX<0>
#define PSHUFWRtoR ePSHUFWRtoR<0>
#define PSHUFWMtoR ePSHUFWMtoR<0>
#define MASKMOVQRtoR eMASKMOVQRtoR<0>
#define PMOVMSKBMMXtoR ePMOVMSKBMMXtoR<0>
//------------------------------------------------------------------
// PACKSSWB,PACKSSDW: Pack Saturate Signed Word 64bits
//------------------------------------------------------------------
#define PACKSSWBMMXtoMMX ePACKSSWBMMXtoMMX<0>
#define PACKSSDWMMXtoMMX ePACKSSDWMMXtoMMX<0>
//------------------------------------------------------------------

//------------------------------------------------------------------
// *SSE instructions* 
//------------------------------------------------------------------
#define SSE_STMXCSR eSSE_STMXCSR<0>
#define SSE_LDMXCSR eSSE_LDMXCSR<0>
#define SSE_MOVAPS_M128_to_XMM eSSE_MOVAPS_M128_to_XMM<0>
#define SSE_MOVAPS_XMM_to_M128 eSSE_MOVAPS_XMM_to_M128<0>
#define SSE_MOVAPS_XMM_to_XMM eSSE_MOVAPS_XMM_to_XMM<0>
#define SSE_MOVUPS_M128_to_XMM eSSE_MOVUPS_M128_to_XMM<0>
#define SSE_MOVUPS_XMM_to_M128 eSSE_MOVUPS_XMM_to_M128<0>
#define SSE_MOVSS_M32_to_XMM eSSE_MOVSS_M32_to_XMM<0>
#define SSE_MOVSS_XMM_to_M32 eSSE_MOVSS_XMM_to_M32<0>
#define SSE_MOVSS_XMM_to_Rm eSSE_MOVSS_XMM_to_Rm<0>
#define SSE_MOVSS_XMM_to_XMM eSSE_MOVSS_XMM_to_XMM<0>
#define SSE_MOVSS_RmOffset_to_XMM eSSE_MOVSS_RmOffset_to_XMM<0>
#define SSE_MOVSS_XMM_to_RmOffset eSSE_MOVSS_XMM_to_RmOffset<0>
#define SSE_MASKMOVDQU_XMM_to_XMM eSSE_MASKMOVDQU_XMM_to_XMM<0>
#define SSE_MOVLPS_M64_to_XMM eSSE_MOVLPS_M64_to_XMM<0>
#define SSE_MOVLPS_XMM_to_M64 eSSE_MOVLPS_XMM_to_M64<0>
#define SSE_MOVLPS_RmOffset_to_XMM eSSE_MOVLPS_RmOffset_to_XMM<0>
#define SSE_MOVLPS_XMM_to_RmOffset eSSE_MOVLPS_XMM_to_RmOffset<0>
#define SSE_MOVHPS_M64_to_XMM eSSE_MOVHPS_M64_to_XMM<0>
#define SSE_MOVHPS_XMM_to_M64 eSSE_MOVHPS_XMM_to_M64<0>
#define SSE_MOVHPS_RmOffset_to_XMM eSSE_MOVHPS_RmOffset_to_XMM<0>
#define SSE_MOVHPS_XMM_to_RmOffset eSSE_MOVHPS_XMM_to_RmOffset<0>
#define SSE_MOVLHPS_XMM_to_XMM eSSE_MOVLHPS_XMM_to_XMM<0>
#define SSE_MOVHLPS_XMM_to_XMM eSSE_MOVHLPS_XMM_to_XMM<0>
#define SSE_MOVLPSRmtoR eSSE_MOVLPSRmtoR<0>
#define SSE_MOVLPSRmtoROffset eSSE_MOVLPSRmtoROffset<0>
#define SSE_MOVLPSRtoRm eSSE_MOVLPSRtoRm<0>
#define SSE_MOVLPSRtoRmOffset eSSE_MOVLPSRtoRmOffset<0>
#define SSE_MOVAPSRmStoR eSSE_MOVAPSRmStoR<0>
#define SSE_MOVAPSRtoRmS eSSE_MOVAPSRtoRmS<0>
#define SSE_MOVAPSRtoRmOffset eSSE_MOVAPSRtoRmOffset<0>
#define SSE_MOVAPSRmtoROffset eSSE_MOVAPSRmtoROffset<0>
#define SSE_MOVUPSRmStoR eSSE_MOVUPSRmStoR<0>
#define SSE_MOVUPSRtoRmS eSSE_MOVUPSRtoRmS<0>
#define SSE_MOVUPSRtoRm eSSE_MOVUPSRtoRm<0>
#define SSE_MOVUPSRmtoR eSSE_MOVUPSRmtoR<0>
#define SSE_MOVUPSRmtoROffset eSSE_MOVUPSRmtoROffset<0>
#define SSE_MOVUPSRtoRmOffset eSSE_MOVUPSRtoRmOffset<0>
#define SSE_RCPPS_XMM_to_XMM eSSE_RCPPS_XMM_to_XMM<0>
#define SSE_RCPPS_M128_to_XMM eSSE_RCPPS_M128_to_XMM<0>
#define SSE_RCPSS_XMM_to_XMM eSSE_RCPSS_XMM_to_XMM<0>
#define SSE_RCPSS_M32_to_XMM eSSE_RCPSS_M32_to_XMM<0>
#define SSE_ORPS_M128_to_XMM eSSE_ORPS_M128_to_XMM<0>
#define SSE_ORPS_XMM_to_XMM eSSE_ORPS_XMM_to_XMM<0>
#define SSE_XORPS_M128_to_XMM eSSE_XORPS_M128_to_XMM<0>
#define SSE_XORPS_XMM_to_XMM eSSE_XORPS_XMM_to_XMM<0>
#define SSE_ANDPS_M128_to_XMM eSSE_ANDPS_M128_to_XMM<0>
#define SSE_ANDPS_XMM_to_XMM eSSE_ANDPS_XMM_to_XMM<0>
#define SSE_ANDNPS_M128_to_XMM eSSE_ANDNPS_M128_to_XMM<0>
#define SSE_ANDNPS_XMM_to_XMM eSSE_ANDNPS_XMM_to_XMM<0>
#define SSE_ADDPS_M128_to_XMM eSSE_ADDPS_M128_to_XMM<0>
#define SSE_ADDPS_XMM_to_XMM eSSE_ADDPS_XMM_to_XMM<0>
#define SSE_ADDSS_M32_to_XMM eSSE_ADDSS_M32_to_XMM<0>
#define SSE_ADDSS_XMM_to_XMM eSSE_ADDSS_XMM_to_XMM<0>
#define SSE_SUBPS_M128_to_XMM eSSE_SUBPS_M128_to_XMM<0>
#define SSE_SUBPS_XMM_to_XMM eSSE_SUBPS_XMM_to_XMM<0>
#define SSE_SUBSS_M32_to_XMM eSSE_SUBSS_M32_to_XMM<0>
#define SSE_SUBSS_XMM_to_XMM eSSE_SUBSS_XMM_to_XMM<0>
#define SSE_MULPS_M128_to_XMM eSSE_MULPS_M128_to_XMM<0>
#define SSE_MULPS_XMM_to_XMM eSSE_MULPS_XMM_to_XMM<0>
#define SSE_MULSS_M32_to_XMM eSSE_MULSS_M32_to_XMM<0>
#define SSE_MULSS_XMM_to_XMM eSSE_MULSS_XMM_to_XMM<0>
#define SSE_CMPEQSS_M32_to_XMM eSSE_CMPEQSS_M32_to_XMM<0>
#define SSE_CMPEQSS_XMM_to_XMM eSSE_CMPEQSS_XMM_to_XMM<0>
#define SSE_CMPLTSS_M32_to_XMM eSSE_CMPLTSS_M32_to_XMM<0>
#define SSE_CMPLTSS_XMM_to_XMM eSSE_CMPLTSS_XMM_to_XMM<0>
#define SSE_CMPLESS_M32_to_XMM eSSE_CMPLESS_M32_to_XMM<0>
#define SSE_CMPLESS_XMM_to_XMM eSSE_CMPLESS_XMM_to_XMM<0>
#define SSE_CMPUNORDSS_M32_to_XMM eSSE_CMPUNORDSS_M32_to_XMM<0>
#define SSE_CMPUNORDSS_XMM_to_XMM eSSE_CMPUNORDSS_XMM_to_XMM<0>
#define SSE_CMPNESS_M32_to_XMM eSSE_CMPNESS_M32_to_XMM<0>
#define SSE_CMPNESS_XMM_to_XMM eSSE_CMPNESS_XMM_to_XMM<0>
#define SSE_CMPNLTSS_M32_to_XMM eSSE_CMPNLTSS_M32_to_XMM<0>
#define SSE_CMPNLTSS_XMM_to_XMM eSSE_CMPNLTSS_XMM_to_XMM<0>
#define SSE_CMPNLESS_M32_to_XMM eSSE_CMPNLESS_M32_to_XMM<0>
#define SSE_CMPNLESS_XMM_to_XMM eSSE_CMPNLESS_XMM_to_XMM<0>
#define SSE_CMPORDSS_M32_to_XMM eSSE_CMPORDSS_M32_to_XMM<0>
#define SSE_CMPORDSS_XMM_to_XMM eSSE_CMPORDSS_XMM_to_XMM<0>
#define SSE_UCOMISS_M32_to_XMM eSSE_UCOMISS_M32_to_XMM<0>
#define SSE_UCOMISS_XMM_to_XMM eSSE_UCOMISS_XMM_to_XMM<0>
#define SSE_PMAXSW_MM_to_MM eSSE_PMAXSW_MM_to_MM<0>
#define SSE_PMINSW_MM_to_MM eSSE_PMINSW_MM_to_MM<0>
#define SSE_CVTPI2PS_MM_to_XMM eSSE_CVTPI2PS_MM_to_XMM<0>
#define SSE_CVTPS2PI_M64_to_MM eSSE_CVTPS2PI_M64_to_MM<0>
#define SSE_CVTPS2PI_XMM_to_MM eSSE_CVTPS2PI_XMM_to_MM<0>
#define SSE_CVTPI2PS_M64_to_XMM eSSE_CVTPI2PS_M64_to_XMM<0>
#define SSE_CVTTSS2SI_M32_to_R32 eSSE_CVTTSS2SI_M32_to_R32<0>
#define SSE_CVTTSS2SI_XMM_to_R32 eSSE_CVTTSS2SI_XMM_to_R32<0>
#define SSE_CVTSI2SS_M32_to_XMM eSSE_CVTSI2SS_M32_to_XMM<0>
#define SSE_CVTSI2SS_R_to_XMM eSSE_CVTSI2SS_R_to_XMM<0>
#define SSE_MAXPS_M128_to_XMM eSSE_MAXPS_M128_to_XMM<0>
#define SSE_MAXPS_XMM_to_XMM eSSE_MAXPS_XMM_to_XMM<0>
#define SSE_MAXSS_M32_to_XMM eSSE_MAXSS_M32_to_XMM<0>
#define SSE_MAXSS_XMM_to_XMM eSSE_MAXSS_XMM_to_XMM<0>
#define SSE_MINPS_M128_to_XMM eSSE_MINPS_M128_to_XMM<0>
#define SSE_MINPS_XMM_to_XMM eSSE_MINPS_XMM_to_XMM<0>
#define SSE_MINSS_M32_to_XMM eSSE_MINSS_M32_to_XMM<0>
#define SSE_MINSS_XMM_to_XMM eSSE_MINSS_XMM_to_XMM<0>
#define SSE_RSQRTPS_M128_to_XMM eSSE_RSQRTPS_M128_to_XMM<0>
#define SSE_RSQRTPS_XMM_to_XMM eSSE_RSQRTPS_XMM_to_XMM<0>
#define SSE_RSQRTSS_M32_to_XMM eSSE_RSQRTSS_M32_to_XMM<0>
#define SSE_RSQRTSS_XMM_to_XMM eSSE_RSQRTSS_XMM_to_XMM<0>
#define SSE_SQRTPS_M128_to_XMM eSSE_SQRTPS_M128_to_XMM<0>
#define SSE_SQRTPS_XMM_to_XMM eSSE_SQRTPS_XMM_to_XMM<0>
#define SSE_SQRTSS_M32_to_XMM eSSE_SQRTSS_M32_to_XMM<0>
#define SSE_SQRTSS_XMM_to_XMM eSSE_SQRTSS_XMM_to_XMM<0>
#define SSE_UNPCKLPS_M128_to_XMM eSSE_UNPCKLPS_M128_to_XMM<0>
#define SSE_UNPCKLPS_XMM_to_XMM eSSE_UNPCKLPS_XMM_to_XMM<0>
#define SSE_UNPCKHPS_M128_to_XMM eSSE_UNPCKHPS_M128_to_XMM<0>
#define SSE_UNPCKHPS_XMM_to_XMM eSSE_UNPCKHPS_XMM_to_XMM<0>
#define SSE_SHUFPS_XMM_to_XMM eSSE_SHUFPS_XMM_to_XMM<0>
#define SSE_SHUFPS_M128_to_XMM eSSE_SHUFPS_M128_to_XMM<0>
#define SSE_SHUFPS_RmOffset_to_XMM eSSE_SHUFPS_RmOffset_to_XMM<0>
#define SSE_CMPEQPS_M128_to_XMM eSSE_CMPEQPS_M128_to_XMM<0>
#define SSE_CMPEQPS_XMM_to_XMM eSSE_CMPEQPS_XMM_to_XMM<0>
#define SSE_CMPLTPS_M128_to_XMM eSSE_CMPLTPS_M128_to_XMM<0>
#define SSE_CMPLTPS_XMM_to_XMM eSSE_CMPLTPS_XMM_to_XMM<0>
#define SSE_CMPLEPS_M128_to_XMM eSSE_CMPLEPS_M128_to_XMM<0>
#define SSE_CMPLEPS_XMM_to_XMM eSSE_CMPLEPS_XMM_to_XMM<0>
#define SSE_CMPUNORDPS_M128_to_XMM eSSE_CMPUNORDPS_M128_to_XMM<0>
#define SSE_CMPUNORDPS_XMM_to_XMM eSSE_CMPUNORDPS_XMM_to_XMM<0>
#define SSE_CMPNEPS_M128_to_XMM eSSE_CMPNEPS_M128_to_XMM<0>
#define SSE_CMPNEPS_XMM_to_XMM eSSE_CMPNEPS_XMM_to_XMM<0>
#define SSE_CMPNLTPS_M128_to_XMM eSSE_CMPNLTPS_M128_to_XMM<0>
#define SSE_CMPNLTPS_XMM_to_XMM eSSE_CMPNLTPS_XMM_to_XMM<0>
#define SSE_CMPNLEPS_M128_to_XMM eSSE_CMPNLEPS_M128_to_XMM<0>
#define SSE_CMPNLEPS_XMM_to_XMM eSSE_CMPNLEPS_XMM_to_XMM<0>
#define SSE_CMPORDPS_M128_to_XMM eSSE_CMPORDPS_M128_to_XMM<0>
#define SSE_CMPORDPS_XMM_to_XMM eSSE_CMPORDPS_XMM_to_XMM<0>
#define SSE_DIVPS_M128_to_XMM eSSE_DIVPS_M128_to_XMM<0>
#define SSE_DIVPS_XMM_to_XMM eSSE_DIVPS_XMM_to_XMM<0>
#define SSE_DIVSS_M32_to_XMM eSSE_DIVSS_M32_to_XMM<0>
#define SSE_DIVSS_XMM_to_XMM eSSE_DIVSS_XMM_to_XMM<0>
//------------------------------------------------------------------

//------------------------------------------------------------------
// *SSE 2 Instructions* 
//------------------------------------------------------------------
#define SSE2_MOVDQA_M128_to_XMM eSSE2_MOVDQA_M128_to_XMM<0>
#define SSE2_MOVDQA_XMM_to_M128 eSSE2_MOVDQA_XMM_to_M128<0>
#define SSE2_MOVDQA_XMM_to_XMM eSSE2_MOVDQA_XMM_to_XMM<0>
#define SSE2_MOVDQU_M128_to_XMM eSSE2_MOVDQU_M128_to_XMM<0>
#define SSE2_MOVDQU_XMM_to_M128 eSSE2_MOVDQU_XMM_to_M128<0>
#define SSE2_MOVDQU_XMM_to_XMM eSSE2_MOVDQU_XMM_to_XMM<0>
#define SSE2_PSRLW_XMM_to_XMM eSSE2_PSRLW_XMM_to_XMM<0>
#define SSE2_PSRLW_M128_to_XMM eSSE2_PSRLW_M128_to_XMM<0> 
#define SSE2_PSRLW_I8_to_XMM eSSE2_PSRLW_I8_to_XMM<0>
#define SSE2_PSRLD_XMM_to_XMM eSSE2_PSRLD_XMM_to_XMM<0>
#define SSE2_PSRLD_M128_to_XMM eSSE2_PSRLD_M128_to_XMM<0>
#define SSE2_PSRLD_I8_to_XMM eSSE2_PSRLD_I8_to_XMM<0>
#define SSE2_PSRLQ_XMM_to_XMM eSSE2_PSRLQ_XMM_to_XMM<0> 
#define SSE2_PSRLQ_M128_to_XMM eSSE2_PSRLQ_M128_to_XMM<0> 
#define SSE2_PSRLQ_I8_to_XMM eSSE2_PSRLQ_I8_to_XMM<0> 
#define SSE2_PSRLDQ_I8_to_XMM eSSE2_PSRLDQ_I8_to_XMM<0> 
#define SSE2_PSRAW_XMM_to_XMM eSSE2_PSRAW_XMM_to_XMM<0> 
#define SSE2_PSRAW_M128_to_XMM eSSE2_PSRAW_M128_to_XMM<0> 
#define SSE2_PSRAW_I8_to_XMM eSSE2_PSRAW_I8_to_XMM<0> 
#define SSE2_PSRAD_XMM_to_XMM eSSE2_PSRAD_XMM_to_XMM<0> 
#define SSE2_PSRAD_M128_to_XMM eSSE2_PSRAD_M128_to_XMM<0> 
#define SSE2_PSRAD_I8_to_XMM eSSE2_PSRAD_I8_to_XMM<0> 
#define SSE2_PSLLW_XMM_to_XMM eSSE2_PSLLW_XMM_to_XMM<0> 
#define SSE2_PSLLW_M128_to_XMM eSSE2_PSLLW_M128_to_XMM<0> 
#define SSE2_PSLLW_I8_to_XMM eSSE2_PSLLW_I8_to_XMM<0> 
#define SSE2_PSLLD_XMM_to_XMM eSSE2_PSLLD_XMM_to_XMM<0> 
#define SSE2_PSLLD_M128_to_XMM eSSE2_PSLLD_M128_to_XMM<0> 
#define SSE2_PSLLD_I8_to_XMM eSSE2_PSLLD_I8_to_XMM<0> 
#define SSE2_PSLLQ_XMM_to_XMM eSSE2_PSLLQ_XMM_to_XMM<0> 
#define SSE2_PSLLQ_M128_to_XMM eSSE2_PSLLQ_M128_to_XMM<0> 
#define SSE2_PSLLQ_I8_to_XMM eSSE2_PSLLQ_I8_to_XMM<0> 
#define SSE2_PSLLDQ_I8_to_XMM eSSE2_PSLLDQ_I8_to_XMM<0> 
#define SSE2_PMAXSW_XMM_to_XMM eSSE2_PMAXSW_XMM_to_XMM<0>  
#define SSE2_PMAXSW_M128_to_XMM eSSE2_PMAXSW_M128_to_XMM<0>  
#define SSE2_PMAXUB_XMM_to_XMM eSSE2_PMAXUB_XMM_to_XMM<0>  
#define SSE2_PMAXUB_M128_to_XMM eSSE2_PMAXUB_M128_to_XMM<0>  
#define SSE2_PMINSW_XMM_to_XMM eSSE2_PMINSW_XMM_to_XMM<0>  
#define SSE2_PMINSW_M128_to_XMM eSSE2_PMINSW_M128_to_XMM<0>  
#define SSE2_PMINUB_XMM_to_XMM eSSE2_PMINUB_XMM_to_XMM<0>  
#define SSE2_PMINUB_M128_to_XMM eSSE2_PMINUB_M128_to_XMM<0>  
#define SSE2_PADDSB_XMM_to_XMM eSSE2_PADDSB_XMM_to_XMM<0>  
#define SSE2_PADDSB_M128_to_XMM eSSE2_PADDSB_M128_to_XMM<0>  
#define SSE2_PADDSW_XMM_to_XMM eSSE2_PADDSW_XMM_to_XMM<0>  
#define SSE2_PADDSW_M128_to_XMM eSSE2_PADDSW_M128_to_XMM<0>  
#define SSE2_PSUBSB_XMM_to_XMM eSSE2_PSUBSB_XMM_to_XMM<0>  
#define SSE2_PSUBSB_M128_to_XMM eSSE2_PSUBSB_M128_to_XMM<0>  
#define SSE2_PSUBSW_XMM_to_XMM eSSE2_PSUBSW_XMM_to_XMM<0>  
#define SSE2_PSUBSW_M128_to_XMM eSSE2_PSUBSW_M128_to_XMM<0>  
#define SSE2_PSUBUSB_XMM_to_XMM eSSE2_PSUBUSB_XMM_to_XMM<0>  
#define SSE2_PSUBUSB_M128_to_XMM eSSE2_PSUBUSB_M128_to_XMM<0>  
#define SSE2_PSUBUSW_XMM_to_XMM eSSE2_PSUBUSW_XMM_to_XMM<0>  
#define SSE2_PSUBUSW_M128_to_XMM eSSE2_PSUBUSW_M128_to_XMM<0>  
#define SSE2_PAND_XMM_to_XMM eSSE2_PAND_XMM_to_XMM<0>  
#define SSE2_PAND_M128_to_XMM eSSE2_PAND_M128_to_XMM<0>  
#define SSE2_PANDN_XMM_to_XMM eSSE2_PANDN_XMM_to_XMM<0>  
#define SSE2_PANDN_M128_to_XMM eSSE2_PANDN_M128_to_XMM<0>  
#define SSE2_PXOR_XMM_to_XMM eSSE2_PXOR_XMM_to_XMM<0>  
#define SSE2_PXOR_M128_to_XMM eSSE2_PXOR_M128_to_XMM<0>  
#define SSE2_PADDW_XMM_to_XMM eSSE2_PADDW_XMM_to_XMM<0> 
#define SSE2_PADDW_M128_to_XMM eSSE2_PADDW_M128_to_XMM<0> 
#define SSE2_PADDUSB_XMM_to_XMM eSSE2_PADDUSB_XMM_to_XMM<0>  
#define SSE2_PADDUSB_M128_to_XMM eSSE2_PADDUSB_M128_to_XMM<0>  
#define SSE2_PADDUSW_XMM_to_XMM eSSE2_PADDUSW_XMM_to_XMM<0>  
#define SSE2_PADDUSW_M128_to_XMM eSSE2_PADDUSW_M128_to_XMM<0>  
#define SSE2_PADDB_XMM_to_XMM eSSE2_PADDB_XMM_to_XMM<0> 
#define SSE2_PADDB_M128_to_XMM eSSE2_PADDB_M128_to_XMM<0> 
#define SSE2_PADDD_XMM_to_XMM eSSE2_PADDD_XMM_to_XMM<0> 
#define SSE2_PADDD_M128_to_XMM eSSE2_PADDD_M128_to_XMM<0> 
#define SSE2_PADDQ_XMM_to_XMM eSSE2_PADDQ_XMM_to_XMM<0> 
#define SSE2_PADDQ_M128_to_XMM eSSE2_PADDQ_M128_to_XMM<0> 
#define SSE2_PMADDWD_XMM_to_XMM eSSE2_PMADDWD_XMM_to_XMM<0>
#define SSE2_MOVSD_XMM_to_XMM eSSE2_MOVSD_XMM_to_XMM<0>  
#define SSE2_MOVQ_M64_to_XMM eSSE2_MOVQ_M64_to_XMM<0>  
#define SSE2_MOVQ_XMM_to_XMM eSSE2_MOVQ_XMM_to_XMM<0>  
#define SSE2_MOVQ_XMM_to_M64 eSSE2_MOVQ_XMM_to_M64<0>
#define SSE2_MOVDQ2Q_XMM_to_MM eSSE2_MOVDQ2Q_XMM_to_MM<0>
#define SSE2_MOVQ2DQ_MM_to_XMM eSSE2_MOVQ2DQ_MM_to_XMM<0>
#define SSE2_MOVDQARtoRmOffset eSSE2_MOVDQARtoRmOffset<0>
#define SSE2_MOVDQARmtoROffset eSSE2_MOVDQARmtoROffset<0>
#define SSE2_CVTDQ2PS_M128_to_XMM eSSE2_CVTDQ2PS_M128_to_XMM<0>  
#define SSE2_CVTDQ2PS_XMM_to_XMM eSSE2_CVTDQ2PS_XMM_to_XMM<0>  
#define SSE2_CVTPS2DQ_M128_to_XMM eSSE2_CVTPS2DQ_M128_to_XMM<0>  
#define SSE2_CVTPS2DQ_XMM_to_XMM eSSE2_CVTPS2DQ_XMM_to_XMM<0>  
#define SSE2_CVTTPS2DQ_XMM_to_XMM eSSE2_CVTTPS2DQ_XMM_to_XMM<0>  
#define SSE2_MAXPD_M128_to_XMM eSSE2_MAXPD_M128_to_XMM<0>  
#define SSE2_MAXPD_XMM_to_XMM eSSE2_MAXPD_XMM_to_XMM<0>  
#define SSE2_MINPD_M128_to_XMM eSSE2_MINPD_M128_to_XMM<0>  
#define SSE2_MINPD_XMM_to_XMM eSSE2_MINPD_XMM_to_XMM<0>  
#define SSE2_PSHUFD_XMM_to_XMM eSSE2_PSHUFD_XMM_to_XMM<0>
#define SSE2_PSHUFD_M128_to_XMM eSSE2_PSHUFD_M128_to_XMM<0>
#define SSE2_PSHUFLW_XMM_to_XMM eSSE2_PSHUFLW_XMM_to_XMM<0>
#define SSE2_PSHUFLW_M128_to_XMM eSSE2_PSHUFLW_M128_to_XMM<0>
#define SSE2_PSHUFHW_XMM_to_XMM eSSE2_PSHUFHW_XMM_to_XMM<0>
#define SSE2_PSHUFHW_M128_to_XMM eSSE2_PSHUFHW_M128_to_XMM<0>
#define SSE2_SHUFPD_XMM_to_XMM eSSE2_SHUFPD_XMM_to_XMM<0>
#define SSE2_SHUFPD_M128_to_XMM eSSE2_SHUFPD_M128_to_XMM<0>
//------------------------------------------------------------------
// PACKSSWB,PACKSSDW: Pack Saturate Signed Word 
//------------------------------------------------------------------
#define SSE2_PACKSSWB_XMM_to_XMM	eSSE2_PACKSSWB_XMM_to_XMM<0>
#define SSE2_PACKSSWB_M128_to_XMM	eSSE2_PACKSSWB_M128_to_XMM<0>
#define SSE2_PACKSSDW_XMM_to_XMM	eSSE2_PACKSSDW_XMM_to_XMM<0>
#define SSE2_PACKSSDW_M128_to_XMM	eSSE2_PACKSSDW_M128_to_XMM<0>
#define SSE2_PACKUSWB_XMM_to_XMM	eSSE2_PACKUSWB_XMM_to_XMM<0>
#define SSE2_PACKUSWB_M128_to_XMM	eSSE2_PACKUSWB_M128_to_XMM<0>
//------------------------------------------------------------------
// PUNPCKHWD: Unpack 16bit high  
//------------------------------------------------------------------
#define SSE2_PUNPCKLBW_XMM_to_XMM	eSSE2_PUNPCKLBW_XMM_to_XMM<0>
#define SSE2_PUNPCKLBW_M128_to_XMM	eSSE2_PUNPCKLBW_M128_to_XMM<0>
#define SSE2_PUNPCKHBW_XMM_to_XMM	eSSE2_PUNPCKHBW_XMM_to_XMM<0>
#define SSE2_PUNPCKHBW_M128_to_XMM	eSSE2_PUNPCKHBW_M128_to_XMM<0>
#define SSE2_PUNPCKLWD_XMM_to_XMM	eSSE2_PUNPCKLWD_XMM_to_XMM<0>
#define SSE2_PUNPCKLWD_M128_to_XMM	eSSE2_PUNPCKLWD_M128_to_XMM<0>
#define SSE2_PUNPCKHWD_XMM_to_XMM	eSSE2_PUNPCKHWD_XMM_to_XMM<0>
#define SSE2_PUNPCKHWD_M128_to_XMM	eSSE2_PUNPCKHWD_M128_to_XMM<0>
#define SSE2_PUNPCKLDQ_XMM_to_XMM	eSSE2_PUNPCKLDQ_XMM_to_XMM<0>
#define SSE2_PUNPCKLDQ_M128_to_XMM	eSSE2_PUNPCKLDQ_M128_to_XMM<0>
#define SSE2_PUNPCKHDQ_XMM_to_XMM	eSSE2_PUNPCKHDQ_XMM_to_XMM<0>
#define SSE2_PUNPCKHDQ_M128_to_XMM	eSSE2_PUNPCKHDQ_M128_to_XMM<0>
#define SSE2_PUNPCKLQDQ_XMM_to_XMM	eSSE2_PUNPCKLQDQ_XMM_to_XMM<0>
#define SSE2_PUNPCKLQDQ_M128_to_XMM	eSSE2_PUNPCKLQDQ_M128_to_XMM<0>
#define SSE2_PUNPCKHQDQ_XMM_to_XMM	eSSE2_PUNPCKHQDQ_XMM_to_XMM<0>
#define SSE2_PUNPCKHQDQ_M128_to_XMM	eSSE2_PUNPCKHQDQ_M128_to_XMM<0>
#define SSE2_PMULLW_XMM_to_XMM		eSSE2_PMULLW_XMM_to_XMM<0>
#define SSE2_PMULLW_M128_to_XMM		eSSE2_PMULLW_M128_to_XMM<0>
#define SSE2_PMULHW_XMM_to_XMM		eSSE2_PMULHW_XMM_to_XMM<0>
#define SSE2_PMULHW_M128_to_XMM		eSSE2_PMULHW_M128_to_XMM<0>
#define SSE2_PMULUDQ_XMM_to_XMM		eSSE2_PMULUDQ_XMM_to_XMM<0>
#define SSE2_PMULUDQ_M128_to_XMM	eSSE2_PMULUDQ_M128_to_XMM<0>
//------------------------------------------------------------------
// PMOVMSKB: Create 16bit mask from signs of 8bit integers 
//------------------------------------------------------------------
#define SSE_MOVMSKPS_XMM_to_R32		eSSE_MOVMSKPS_XMM_to_R32<0>
#define SSE2_PMOVMSKB_XMM_to_R32	eSSE2_PMOVMSKB_XMM_to_R32<0>
#define SSE2_MOVMSKPD_XMM_to_R32	eSSE2_MOVMSKPD_XMM_to_R32<0>
//------------------------------------------------------------------
// PEXTRW,PINSRW: Packed Extract/Insert Word  
//------------------------------------------------------------------
#define SSE_PEXTRW_XMM_to_R32		eSSE_PEXTRW_XMM_to_R32<0>
#define SSE_PINSRW_R32_to_XMM		eSSE_PINSRW_R32_to_XMM<0>
//------------------------------------------------------------------
// PSUBx: Subtract Packed Integers   
//------------------------------------------------------------------
#define SSE2_PSUBB_XMM_to_XMM		eSSE2_PSUBB_XMM_to_XMM<0>
#define SSE2_PSUBB_M128_to_XMM		eSSE2_PSUBB_M128_to_XMM<0>
#define SSE2_PSUBW_XMM_to_XMM		eSSE2_PSUBW_XMM_to_XMM<0>
#define SSE2_PSUBW_M128_to_XMM		eSSE2_PSUBW_M128_to_XMM<0>
#define SSE2_PSUBD_XMM_to_XMM		eSSE2_PSUBD_XMM_to_XMM<0>
#define SSE2_PSUBD_M128_to_XMM		eSSE2_PSUBD_M128_to_XMM<0>
#define SSE2_PSUBQ_XMM_to_XMM		eSSE2_PSUBQ_XMM_to_XMM<0>
#define SSE2_PSUBQ_M128_to_XMM		eSSE2_PSUBQ_M128_to_XMM<0>
//------------------------------------------------------------------
// PCMPxx: Compare Packed Integers   
//------------------------------------------------------------------
#define SSE2_PCMPGTB_XMM_to_XMM		eSSE2_PCMPGTB_XMM_to_XMM<0>
#define SSE2_PCMPGTB_M128_to_XMM	eSSE2_PCMPGTB_M128_to_XMM<0>
#define SSE2_PCMPGTW_XMM_to_XMM		eSSE2_PCMPGTW_XMM_to_XMM<0>
#define SSE2_PCMPGTW_M128_to_XMM	eSSE2_PCMPGTW_M128_to_XMM<0>
#define SSE2_PCMPGTD_XMM_to_XMM		eSSE2_PCMPGTD_XMM_to_XMM<0>
#define SSE2_PCMPGTD_M128_to_XMM	eSSE2_PCMPGTD_M128_to_XMM<0>
#define SSE2_PCMPEQB_XMM_to_XMM		eSSE2_PCMPEQB_XMM_to_XMM<0>
#define SSE2_PCMPEQB_M128_to_XMM	eSSE2_PCMPEQB_M128_to_XMM<0>
#define SSE2_PCMPEQW_XMM_to_XMM		eSSE2_PCMPEQW_XMM_to_XMM<0>
#define SSE2_PCMPEQW_M128_to_XMM	eSSE2_PCMPEQW_M128_to_XMM<0>
#define SSE2_PCMPEQD_XMM_to_XMM		eSSE2_PCMPEQD_XMM_to_XMM<0>
#define SSE2_PCMPEQD_M128_to_XMM	eSSE2_PCMPEQD_M128_to_XMM<0>
//------------------------------------------------------------------
// MOVD: Move Dword(32bit) to /from XMM reg 
//------------------------------------------------------------------
#define SSE2_MOVD_M32_to_XMM		eSSE2_MOVD_M32_to_XMM<0>
#define SSE2_MOVD_R_to_XMM			eSSE2_MOVD_R_to_XMM<0>
#define SSE2_MOVD_Rm_to_XMM			eSSE2_MOVD_Rm_to_XMM<0>
#define SSE2_MOVD_RmOffset_to_XMM	eSSE2_MOVD_RmOffset_to_XMM<0>
#define SSE2_MOVD_XMM_to_M32		eSSE2_MOVD_XMM_to_M32<0>
#define SSE2_MOVD_XMM_to_R			eSSE2_MOVD_XMM_to_R<0>
#define SSE2_MOVD_XMM_to_Rm			eSSE2_MOVD_XMM_to_Rm<0>
#define SSE2_MOVD_XMM_to_RmOffset	eSSE2_MOVD_XMM_to_RmOffset<0>
#define SSE2_MOVQ_XMM_to_R			eSSE2_MOVQ_XMM_to_R<0>
#define SSE2_MOVQ_R_to_XMM			eSSE2_MOVQ_R_to_XMM<0>
//------------------------------------------------------------------
// POR : SSE Bitwise OR   
//------------------------------------------------------------------
#define SSE2_POR_XMM_to_XMM			eSSE2_POR_XMM_to_XMM<0>
#define SSE2_POR_M128_to_XMM		eSSE2_POR_M128_to_XMM<0>
//------------------------------------------------------------------

//------------------------------------------------------------------
// SSE3 
//------------------------------------------------------------------
#define SSE3_HADDPS_XMM_to_XMM		eSSE3_HADDPS_XMM_to_XMM<0>
#define SSE3_HADDPS_M128_to_XMM		eSSE3_HADDPS_M128_to_XMM<0>
#define SSE3_MOVSLDUP_XMM_to_XMM	eSSE3_MOVSLDUP_XMM_to_XMM<0>
#define SSE3_MOVSLDUP_M128_to_XMM	eSSE3_MOVSLDUP_M128_to_XMM<0>
#define SSE3_MOVSHDUP_XMM_to_XMM	eSSE3_MOVSHDUP_XMM_to_XMM<0>
#define SSE3_MOVSHDUP_M128_to_XMM	eSSE3_MOVSHDUP_M128_to_XMM<0>
//------------------------------------------------------------------

//------------------------------------------------------------------
// SSSE3
//------------------------------------------------------------------
#define SSSE3_PABSB_XMM_to_XMM		eSSSE3_PABSB_XMM_to_XMM<0>
#define SSSE3_PABSW_XMM_to_XMM		eSSSE3_PABSW_XMM_to_XMM<0>
#define SSSE3_PABSD_XMM_to_XMM		eSSSE3_PABSD_XMM_to_XMM<0>
#define SSSE3_PALIGNR_XMM_to_XMM	eSSSE3_PALIGNR_XMM_to_XMM<0>
//------------------------------------------------------------------

//------------------------------------------------------------------
// SSE4.1
//------------------------------------------------------------------
#define SSE4_DPPS_XMM_to_XMM		eSSE4_DPPS_XMM_to_XMM<0>
#define SSE4_DPPS_M128_to_XMM		eSSE4_DPPS_M128_to_XMM<0>
#define SSE4_INSERTPS_XMM_to_XMM	eSSE4_INSERTPS_XMM_to_XMM<0>
#define SSE4_EXTRACTPS_XMM_to_R32	eSSE4_EXTRACTPS_XMM_to_R32<0>
#define SSE4_BLENDPS_XMM_to_XMM		eSSE4_BLENDPS_XMM_to_XMM<0>
#define SSE4_BLENDVPS_XMM_to_XMM	eSSE4_BLENDVPS_XMM_to_XMM<0>
#define SSE4_BLENDVPS_M128_to_XMM	eSSE4_BLENDVPS_M128_to_XMM<0>
#define SSE4_PMOVSXDQ_XMM_to_XMM	eSSE4_PMOVSXDQ_XMM_to_XMM<0>
#define SSE4_PINSRD_R32_to_XMM		eSSE4_PINSRD_R32_to_XMM<0>
#define SSE4_PMAXSD_XMM_to_XMM		eSSE4_PMAXSD_XMM_to_XMM<0>
#define SSE4_PMINSD_XMM_to_XMM		eSSE4_PMINSD_XMM_to_XMM<0>
#define SSE4_PMAXUD_XMM_to_XMM		eSSE4_PMAXUD_XMM_to_XMM<0>
#define SSE4_PMINUD_XMM_to_XMM		eSSE4_PMINUD_XMM_to_XMM<0>
#define SSE4_PMAXSD_M128_to_XMM		eSSE4_PMAXSD_M128_to_XMM<0>
#define SSE4_PMINSD_M128_to_XMM		eSSE4_PMINSD_M128_to_XMM<0>
#define SSE4_PMAXUD_M128_to_XMM		eSSE4_PMAXUD_M128_to_XMM<0>
#define SSE4_PMINUD_M128_to_XMM		eSSE4_PMINUD_M128_to_XMM<0>
#define SSE4_PMULDQ_XMM_to_XMM		eSSE4_PMULDQ_XMM_to_XMM<0>
//------------------------------------------------------------------

//------------------------------------------------------------------
// 3DNOW instructions
//------------------------------------------------------------------
#define FEMMS			eFEMMS<0>
#define PFCMPEQMtoR		ePFCMPEQMtoR<0>
#define PFCMPGTMtoR		ePFCMPGTMtoR<0>
#define PFCMPGEMtoR		ePFCMPGEMtoR<0>
#define PFADDMtoR		ePFADDMtoR<0>
#define PFADDRtoR		ePFADDRtoR<0>
#define PFSUBMtoR		ePFSUBMtoR<0>
#define PFSUBRtoR		ePFSUBRtoR<0>
#define PFMULMtoR		ePFMULMtoR<0>
#define PFMULRtoR		ePFMULRtoR<0>
#define PFRCPMtoR		ePFRCPMtoR<0>
#define PFRCPRtoR		ePFRCPRtoR<0>
#define PFRCPIT1RtoR	ePFRCPIT1RtoR<0>
#define PFRCPIT2RtoR	ePFRCPIT2RtoR<0>
#define PFRSQRTRtoR		ePFRSQRTRtoR<0>
#define PFRSQIT1RtoR	ePFRSQIT1RtoR<0>
#define PF2IDMtoR		ePF2IDMtoR<0>
#define PI2FDMtoR		ePI2FDMtoR<0>
#define PI2FDRtoR		ePI2FDRtoR<0>
#define PFMAXMtoR		ePFMAXMtoR<0>
#define PFMAXRtoR		ePFMAXRtoR<0>
#define PFMINMtoR		ePFMINMtoR<0>
#define PFMINRtoR		ePFMINRtoR<0>
//------------------------------------------------------------------
