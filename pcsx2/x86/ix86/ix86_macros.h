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
#define x86SetPtr			ex86SetPtr<_EmitterId_>
#define x86SetJ8			ex86SetJ8<_EmitterId_>
#define x86SetJ8A			ex86SetJ8A<_EmitterId_>
#define x86SetJ16			ex86SetJ16<_EmitterId_>
#define x86SetJ16A			ex86SetJ16A<_EmitterId_>
#define x86SetJ32			ex86SetJ32<_EmitterId_>
#define x86SetJ32A			ex86SetJ32A<_EmitterId_>
#define x86Align			ex86Align<_EmitterId_>
#define x86AlignExecutable	ex86AlignExecutable<_EmitterId_>
//------------------------------------------------------------------

//------------------------------------------------------------------
// *IX86 intructions*
//------------------------------------------------------------------

#define STC eSTC<_EmitterId_>
#define CLC eCLC<_EmitterId_>
#define NOP eNOP<_EmitterId_>

//------------------------------------------------------------------
// mov instructions
//------------------------------------------------------------------
#define MOV64RtoR eMOV64RtoR<_EmitterId_>
#define MOV64RtoM eMOV64RtoM<_EmitterId_>
#define MOV64MtoR eMOV64MtoR<_EmitterId_>
#define MOV64I32toM eMOV64I32toM<_EmitterId_>
#define MOV64I32toR eMOV64I32toR<_EmitterId_>
#define MOV64ItoR eMOV64ItoR<_EmitterId_>
#define MOV64ItoRmOffset eMOV64ItoRmOffset<_EmitterId_>
#define MOV64RmOffsettoR eMOV64RmOffsettoR<_EmitterId_>
#define MOV64RmStoR eMOV64RmStoR<_EmitterId_>
#define MOV64RtoRmOffset eMOV64RtoRmOffset<_EmitterId_>
#define MOV64RtoRmS eMOV64RtoRmS<_EmitterId_>
#define MOV32RtoR eMOV32RtoR<_EmitterId_>
#define MOV32RtoM eMOV32RtoM<_EmitterId_>
#define MOV32MtoR eMOV32MtoR<_EmitterId_>
#define MOV32RmtoR eMOV32RmtoR<_EmitterId_>
#define MOV32RmtoROffset eMOV32RmtoROffset<_EmitterId_>
#define MOV32RmStoR eMOV32RmStoR<_EmitterId_>
#define MOV32RmSOffsettoR eMOV32RmSOffsettoR<_EmitterId_>
#define MOV32RtoRm eMOV32RtoRm<_EmitterId_>
#define MOV32RtoRmS eMOV32RtoRmS<_EmitterId_>
#define MOV32ItoR eMOV32ItoR<_EmitterId_>
#define MOV32ItoM eMOV32ItoM<_EmitterId_>
#define MOV32ItoRmOffset eMOV32ItoRmOffset<_EmitterId_>
#define MOV32RtoRmOffset eMOV32RtoRmOffset<_EmitterId_>
#define MOV16RtoM eMOV16RtoM<_EmitterId_>
#define MOV16MtoR eMOV16MtoR<_EmitterId_>
#define MOV16RmtoR eMOV16RmtoR<_EmitterId_>
#define MOV16RmtoROffset eMOV16RmtoROffset<_EmitterId_>
#define MOV16RmSOffsettoR eMOV16RmSOffsettoR<_EmitterId_>
#define MOV16RtoRm eMOV16RtoRm<_EmitterId_>
#define MOV16ItoM eMOV16ItoM<_EmitterId_>
#define MOV16RtoRmS eMOV16RtoRmS<_EmitterId_>
#define MOV16ItoR eMOV16ItoR<_EmitterId_>
#define MOV16ItoRmOffset eMOV16ItoRmOffset<_EmitterId_>
#define MOV16RtoRmOffset eMOV16RtoRmOffset<_EmitterId_>
#define MOV8RtoM eMOV8RtoM<_EmitterId_>
#define MOV8MtoR eMOV8MtoR<_EmitterId_>
#define MOV8RmtoR eMOV8RmtoR<_EmitterId_>
#define MOV8RmtoROffset eMOV8RmtoROffset<_EmitterId_>
#define MOV8RmSOffsettoR eMOV8RmSOffsettoR<_EmitterId_>
#define MOV8RtoRm eMOV8RtoRm<_EmitterId_>
#define MOV8ItoM eMOV8ItoM<_EmitterId_>
#define MOV8ItoR eMOV8ItoR<_EmitterId_>
#define MOV8ItoRmOffset eMOV8ItoRmOffset<_EmitterId_>
#define MOV8RtoRmOffset eMOV8RtoRmOffset<_EmitterId_>
#define MOVSX32R8toR eMOVSX32R8toR<_EmitterId_>
#define MOVSX32Rm8toR eMOVSX32Rm8toR<_EmitterId_>
#define MOVSX32Rm8toROffset eMOVSX32Rm8toROffset<_EmitterId_>
#define MOVSX32M8toR eMOVSX32M8toR<_EmitterId_>
#define MOVSX32R16toR eMOVSX32R16toR<_EmitterId_>
#define MOVSX32Rm16toR eMOVSX32Rm16toR<_EmitterId_>
#define MOVSX32Rm16toROffset eMOVSX32Rm16toROffset<_EmitterId_>
#define MOVSX32M16toR eMOVSX32M16toR<_EmitterId_>
#define MOVZX32R8toR eMOVZX32R8toR<_EmitterId_>
#define MOVZX32Rm8toR eMOVZX32Rm8toR<_EmitterId_>
#define MOVZX32Rm8toROffset eMOVZX32Rm8toROffset<_EmitterId_>
#define MOVZX32M8toR eMOVZX32M8toR<_EmitterId_>
#define MOVZX32R16toR eMOVZX32R16toR<_EmitterId_>
#define MOVZX32Rm16toR eMOVZX32Rm16toR<_EmitterId_>
#define MOVZX32Rm16toROffset eMOVZX32Rm16toROffset<_EmitterId_>
#define MOVZX32M16toR eMOVZX32M16toR<_EmitterId_>
#define CMOVBE32RtoR eCMOVBE32RtoR<_EmitterId_>
#define CMOVBE32MtoR eCMOVBE32MtoR<_EmitterId_>
#define CMOVB32RtoR eCMOVB32RtoR<_EmitterId_>
#define CMOVB32MtoR eCMOVB32MtoR<_EmitterId_>
#define CMOVAE32RtoR eCMOVAE32RtoR<_EmitterId_>
#define CMOVAE32MtoR eCMOVAE32MtoR<_EmitterId_>
#define CMOVA32RtoR eCMOVA32RtoR<_EmitterId_>
#define CMOVA32MtoR eCMOVA32MtoR<_EmitterId_>
#define CMOVO32RtoR eCMOVO32RtoR<_EmitterId_>
#define CMOVO32MtoR eCMOVO32MtoR<_EmitterId_>
#define CMOVP32RtoR eCMOVP32RtoR<_EmitterId_>
#define CMOVP32MtoR eCMOVP32MtoR<_EmitterId_>
#define CMOVS32RtoR eCMOVS32RtoR<_EmitterId_>
#define CMOVS32MtoR eCMOVS32MtoR<_EmitterId_>
#define CMOVNO32RtoR eCMOVNO32RtoR<_EmitterId_>
#define CMOVNO32MtoR eCMOVNO32MtoR<_EmitterId_>
#define CMOVNP32RtoR eCMOVNP32RtoR<_EmitterId_>
#define CMOVNP32MtoR eCMOVNP32MtoR<_EmitterId_>
#define CMOVNS32RtoR eCMOVNS32RtoR<_EmitterId_>
#define CMOVNS32MtoR eCMOVNS32MtoR<_EmitterId_>
#define CMOVNE32RtoR eCMOVNE32RtoR<_EmitterId_>
#define CMOVNE32MtoR eCMOVNE32MtoR<_EmitterId_>
#define CMOVE32RtoR eCMOVE32RtoR<_EmitterId_>
#define CMOVE32MtoR eCMOVE32MtoR<_EmitterId_>
#define CMOVG32RtoR eCMOVG32RtoR<_EmitterId_>
#define CMOVG32MtoR eCMOVG32MtoR<_EmitterId_>
#define CMOVGE32RtoR eCMOVGE32RtoR<_EmitterId_>
#define CMOVGE32MtoR eCMOVGE32MtoR<_EmitterId_>
#define CMOVL32RtoR eCMOVL32RtoR<_EmitterId_>
#define CMOVL32MtoR eCMOVL32MtoR<_EmitterId_>
#define CMOVLE32RtoR eCMOVLE32RtoR<_EmitterId_>
#define CMOVLE32MtoR eCMOVLE32MtoR<_EmitterId_>
//------------------------------------------------------------------
// arithmetic instructions 
//------------------------------------------------------------------
#define ADD64ItoR eADD64ItoR<_EmitterId_>
#define ADD64MtoR eADD64MtoR<_EmitterId_>
#define ADD32ItoEAX eADD32ItoEAX<_EmitterId_>
#define ADD32ItoR eADD32ItoR<_EmitterId_>
#define ADD32ItoM eADD32ItoM<_EmitterId_>
#define ADD32ItoRmOffset eADD32ItoRmOffset<_EmitterId_>
#define ADD32RtoR eADD32RtoR<_EmitterId_>
#define ADD32RtoM eADD32RtoM<_EmitterId_>
#define ADD32MtoR eADD32MtoR<_EmitterId_>
#define ADD16RtoR eADD16RtoR<_EmitterId_>
#define ADD16ItoR eADD16ItoR<_EmitterId_>
#define ADD16ItoM eADD16ItoM<_EmitterId_>
#define ADD16RtoM eADD16RtoM<_EmitterId_>
#define ADD16MtoR eADD16MtoR<_EmitterId_>
#define ADD8MtoR eADD8MtoR<_EmitterId_>
#define ADC32ItoR eADC32ItoR<_EmitterId_>
#define ADC32ItoM eADC32ItoM<_EmitterId_>
#define ADC32RtoR eADC32RtoR<_EmitterId_>
#define ADC32MtoR eADC32MtoR<_EmitterId_>
#define ADC32RtoM eADC32RtoM<_EmitterId_>
#define INC32R eINC32R<_EmitterId_>
#define INC32M eINC32M<_EmitterId_>
#define INC16R eINC16R<_EmitterId_>
#define INC16M eINC16M<_EmitterId_>
#define SUB64MtoR eSUB64MtoR<_EmitterId_>
#define SUB32ItoR eSUB32ItoR<_EmitterId_>
#define SUB32ItoM eSUB32ItoM<_EmitterId_>
#define SUB32RtoR eSUB32RtoR<_EmitterId_>
#define SUB32MtoR eSUB32MtoR<_EmitterId_>
#define SUB32RtoM eSUB32RtoM<_EmitterId_>
#define SUB16RtoR eSUB16RtoR<_EmitterId_>
#define SUB16ItoR eSUB16ItoR<_EmitterId_>
#define SUB16ItoM eSUB16ItoM<_EmitterId_>
#define SUB16MtoR eSUB16MtoR<_EmitterId_>
#define SBB64RtoR eSBB64RtoR<_EmitterId_>
#define SBB32ItoR eSBB32ItoR<_EmitterId_>
#define SBB32ItoM eSBB32ItoM<_EmitterId_>
#define SBB32RtoR eSBB32RtoR<_EmitterId_>
#define SBB32MtoR eSBB32MtoR<_EmitterId_>
#define SBB32RtoM eSBB32RtoM<_EmitterId_>
#define DEC32R eDEC32R<_EmitterId_>
#define DEC32M eDEC32M<_EmitterId_>
#define DEC16R eDEC16R<_EmitterId_>
#define DEC16M eDEC16M<_EmitterId_>
#define MUL32R eMUL32R<_EmitterId_>
#define MUL32M eMUL32M<_EmitterId_>
#define IMUL32R eIMUL32R<_EmitterId_>
#define IMUL32M eIMUL32M<_EmitterId_>
#define IMUL32RtoR eIMUL32RtoR<_EmitterId_>
#define DIV32R eDIV32R<_EmitterId_>
#define DIV32M eDIV32M<_EmitterId_>
#define IDIV32R eIDIV32R<_EmitterId_>
#define IDIV32M eIDIV32M<_EmitterId_>
//------------------------------------------------------------------
// shifting instructions 
//------------------------------------------------------------------
#define SHL64ItoR eSHL64ItoR<_EmitterId_>
#define SHL64CLtoR eSHL64CLtoR<_EmitterId_>
#define SHR64ItoR eSHR64ItoR<_EmitterId_>
#define SHR64CLtoR eSHR64CLtoR<_EmitterId_>
#define SAR64ItoR eSAR64ItoR<_EmitterId_>
#define SAR64CLtoR eSAR64CLtoR<_EmitterId_>
#define SHL32ItoR eSHL32ItoR<_EmitterId_>
#define SHL32ItoM eSHL32ItoM<_EmitterId_>
#define SHL32CLtoR eSHL32CLtoR<_EmitterId_>
#define SHL16ItoR eSHL16ItoR<_EmitterId_>
#define SHL8ItoR eSHL8ItoR<_EmitterId_>
#define SHR32ItoR eSHR32ItoR<_EmitterId_>
#define SHR32ItoM eSHR32ItoM<_EmitterId_>
#define SHR32CLtoR eSHR32CLtoR<_EmitterId_>
#define SHR16ItoR eSHR16ItoR<_EmitterId_>
#define SHR8ItoR eSHR8ItoR<_EmitterId_>
#define SAR32ItoR eSAR32ItoR<_EmitterId_>
#define SAR32ItoM eSAR32ItoM<_EmitterId_>
#define SAR32CLtoR eSAR32CLtoR<_EmitterId_>
#define SAR16ItoR eSAR16ItoR<_EmitterId_>
#define ROR32ItoR eROR32ItoR<_EmitterId_>
#define RCR32ItoR eRCR32ItoR<_EmitterId_>
#define RCR32ItoM eRCR32ItoM<_EmitterId_>
#define SHLD32ItoR eSHLD32ItoR<_EmitterId_>
#define SHRD32ItoR eSHRD32ItoR<_EmitterId_>
//------------------------------------------------------------------
// logical instructions
//------------------------------------------------------------------
#define OR64ItoR eOR64ItoR<_EmitterId_>
#define OR64MtoR eOR64MtoR<_EmitterId_>
#define OR64RtoR eOR64RtoR<_EmitterId_>
#define OR64RtoM eOR64RtoM<_EmitterId_>
#define OR32ItoR eOR32ItoR<_EmitterId_>
#define OR32ItoM eOR32ItoM<_EmitterId_>
#define OR32RtoR eOR32RtoR<_EmitterId_>
#define OR32RtoM eOR32RtoM<_EmitterId_>
#define OR32MtoR eOR32MtoR<_EmitterId_>
#define OR16RtoR eOR16RtoR<_EmitterId_>
#define OR16ItoR eOR16ItoR<_EmitterId_>
#define OR16ItoM eOR16ItoM<_EmitterId_>
#define OR16MtoR eOR16MtoR<_EmitterId_>
#define OR16RtoM eOR16RtoM<_EmitterId_>
#define OR8RtoR eOR8RtoR<_EmitterId_>
#define OR8RtoM eOR8RtoM<_EmitterId_>
#define OR8ItoM eOR8ItoM<_EmitterId_>
#define OR8MtoR eOR8MtoR<_EmitterId_>
#define XOR64ItoR eXOR64ItoR<_EmitterId_>
#define XOR64RtoR eXOR64RtoR<_EmitterId_>
#define XOR64MtoR eXOR64MtoR<_EmitterId_>
#define XOR64RtoR eXOR64RtoR<_EmitterId_>
#define XOR64RtoM eXOR64RtoM<_EmitterId_>
#define XOR32ItoR eXOR32ItoR<_EmitterId_>
#define XOR32ItoM eXOR32ItoM<_EmitterId_>
#define XOR32RtoR eXOR32RtoR<_EmitterId_>
#define XOR16RtoR eXOR16RtoR<_EmitterId_>
#define XOR32RtoM eXOR32RtoM<_EmitterId_>
#define XOR32MtoR eXOR32MtoR<_EmitterId_>
#define XOR16RtoM eXOR16RtoM<_EmitterId_>
#define XOR16ItoR eXOR16ItoR<_EmitterId_>
#define AND64I32toR eAND64I32toR<_EmitterId_>
#define AND64MtoR eAND64MtoR<_EmitterId_>
#define AND64RtoM eAND64RtoM<_EmitterId_>
#define AND64RtoR eAND64RtoR<_EmitterId_>
#define AND64I32toM eAND64I32toM<_EmitterId_>
#define AND32ItoR eAND32ItoR<_EmitterId_>
#define AND32I8toR eAND32I8toR<_EmitterId_>
#define AND32ItoM eAND32ItoM<_EmitterId_>
#define AND32I8toM eAND32I8toM<_EmitterId_>
#define AND32RtoR eAND32RtoR<_EmitterId_>
#define AND32RtoM eAND32RtoM<_EmitterId_>
#define AND32MtoR eAND32MtoR<_EmitterId_>
#define AND32RmtoR eAND32RmtoR<_EmitterId_>
#define AND32RmtoROffset eAND32RmtoROffset<_EmitterId_>
#define AND16RtoR eAND16RtoR<_EmitterId_>
#define AND16ItoR eAND16ItoR<_EmitterId_>
#define AND16ItoM eAND16ItoM<_EmitterId_>
#define AND16RtoM eAND16RtoM<_EmitterId_>
#define AND16MtoR eAND16MtoR<_EmitterId_>
#define AND8ItoR eAND8ItoR<_EmitterId_>
#define AND8ItoM eAND8ItoM<_EmitterId_>
#define AND8RtoM eAND8RtoM<_EmitterId_>
#define AND8MtoR eAND8MtoR<_EmitterId_>
#define AND8RtoR eAND8RtoR<_EmitterId_>
#define NOT64R eNOT64R<_EmitterId_>
#define NOT32R eNOT32R<_EmitterId_>
#define NOT32M eNOT32M<_EmitterId_>
#define NEG64R eNEG64R<_EmitterId_>
#define NEG32R eNEG32R<_EmitterId_>
#define NEG32M eNEG32M<_EmitterId_>
#define NEG16R eNEG16R<_EmitterId_>
//------------------------------------------------------------------
// jump/call instructions
//------------------------------------------------------------------
#define JMP8 eJMP8<_EmitterId_>
#define JP8 eJP8<_EmitterId_>
#define JNP8 eJNP8<_EmitterId_>
#define JE8 eJE8<_EmitterId_>
#define JZ8 eJZ8<_EmitterId_>
#define JG8 eJG8<_EmitterId_>
#define JGE8 eJGE8<_EmitterId_>
#define JS8 eJS8<_EmitterId_>
#define JNS8 eJNS8<_EmitterId_>
#define JL8 eJL8<_EmitterId_>
#define JA8 eJA8<_EmitterId_>
#define JAE8 eJAE8<_EmitterId_>
#define JB8 eJB8<_EmitterId_>
#define JBE8 eJBE8<_EmitterId_>
#define JLE8 eJLE8<_EmitterId_>
#define JNE8 eJNE8<_EmitterId_>
#define JNZ8 eJNZ8<_EmitterId_>
#define JNG8 eJNG8<_EmitterId_>
#define JNGE8 eJNGE8<_EmitterId_>
#define JNL8 eJNL8<_EmitterId_>
#define JNLE8 eJNLE8<_EmitterId_>
#define JO8 eJO8<_EmitterId_>
#define JNO8 eJNO8<_EmitterId_>
#define JMP32 eJMP32<_EmitterId_>
#define JNS32 eJNS32<_EmitterId_>
#define JS32 eJS32<_EmitterId_>
#define JB32 eJB32<_EmitterId_>
#define JE32 eJE32<_EmitterId_>
#define JZ32 eJZ32<_EmitterId_>
#define JG32 eJG32<_EmitterId_>
#define JGE32 eJGE32<_EmitterId_>
#define JL32 eJL32<_EmitterId_>
#define JLE32 eJLE32<_EmitterId_>
#define JA32 eJA32<_EmitterId_>
#define JAE32 eJAE32<_EmitterId_>
#define JNE32 eJNE32<_EmitterId_>
#define JNZ32 eJNZ32<_EmitterId_>
#define JNG32 eJNG32<_EmitterId_>
#define JNGE32 eJNGE32<_EmitterId_>
#define JNL32 eJNL32<_EmitterId_>
#define JNLE32 eJNLE32<_EmitterId_>
#define JO32 eJO32<_EmitterId_>
#define JNO32 eJNO32<_EmitterId_>
#define JS32 eJS32<_EmitterId_>
#define JMPR eJMPR<_EmitterId_>
#define JMP32M eJMP32M<_EmitterId_>
#define CALLFunc eCALLFunc<_EmitterId_>
#define CALL32 eCALL32<_EmitterId_>
#define CALL32R eCALL32R<_EmitterId_>
#define CALL32M eCALL32M<_EmitterId_>
//------------------------------------------------------------------
// misc instructions
//------------------------------------------------------------------
#define CMP64I32toR eCMP64I32toR<_EmitterId_>
#define CMP64MtoR eCMP64MtoR<_EmitterId_>
#define CMP64RtoR eCMP64RtoR<_EmitterId_>
#define CMP32ItoR eCMP32ItoR<_EmitterId_>
#define CMP32ItoM eCMP32ItoM<_EmitterId_>
#define CMP32RtoR eCMP32RtoR<_EmitterId_>
#define CMP32MtoR eCMP32MtoR<_EmitterId_>
#define CMP32I8toRm eCMP32I8toRm<_EmitterId_>
#define CMP32I8toRmOffset8 eCMP32I8toRmOffset8<_EmitterId_>
#define CMP32I8toM eCMP32I8toM<_EmitterId_>
#define CMP16ItoR eCMP16ItoR<_EmitterId_>
#define CMP16ItoM eCMP16ItoM<_EmitterId_>
#define CMP16RtoR eCMP16RtoR<_EmitterId_>
#define CMP16MtoR eCMP16MtoR<_EmitterId_>
#define CMP8ItoR eCMP8ItoR<_EmitterId_>
#define CMP8MtoR eCMP8MtoR<_EmitterId_>
#define TEST32ItoR eTEST32ItoR<_EmitterId_>
#define TEST32ItoM eTEST32ItoM<_EmitterId_>
#define TEST32RtoR eTEST32RtoR<_EmitterId_>
#define TEST32ItoRm eTEST32ItoRm<_EmitterId_>
#define TEST16ItoR eTEST16ItoR<_EmitterId_>
#define TEST16RtoR eTEST16RtoR<_EmitterId_>
#define TEST8RtoR eTEST8RtoR<_EmitterId_>
#define TEST8ItoR eTEST8ItoR<_EmitterId_>
#define TEST8ItoM eTEST8ItoM<_EmitterId_>
#define SETS8R eSETS8R<_EmitterId_>
#define SETL8R eSETL8R<_EmitterId_>
#define SETGE8R eSETGE8R<_EmitterId_>
#define SETG8R eSETG8R<_EmitterId_>
#define SETA8R eSETA8R<_EmitterId_>
#define SETAE8R eSETAE8R<_EmitterId_>
#define SETB8R eSETB8R<_EmitterId_>
#define SETNZ8R eSETNZ8R<_EmitterId_>
#define SETZ8R eSETZ8R<_EmitterId_>
#define SETE8R eSETE8R<_EmitterId_>
#define PUSH32I ePUSH32I<_EmitterId_>
#define PUSH32R ePUSH32R<_EmitterId_>
#define PUSH32M ePUSH32M<_EmitterId_>
#define PUSH32I ePUSH32I<_EmitterId_>
#define POP32R ePOP32R<_EmitterId_>
#define PUSHA32 ePUSHA32<_EmitterId_>
#define POPA32 ePOPA32<_EmitterId_>
#define PUSHR ePUSHR<_EmitterId_>
#define POPR ePOPR<_EmitterId_>
#define PUSHFD ePUSHFD<_EmitterId_>
#define POPFD ePOPFD<_EmitterId_>
#define RET eRET<_EmitterId_>
#define CBW eCBW<_EmitterId_>
#define CWDE eCWDE<_EmitterId_>
#define CWD eCWD<_EmitterId_>
#define CDQ eCDQ<_EmitterId_>
#define CDQE eCDQE<_EmitterId_>
#define LAHF eLAHF<_EmitterId_>
#define SAHF eSAHF<_EmitterId_>
#define BT32ItoR eBT32ItoR<_EmitterId_>
#define BTR32ItoR eBTR32ItoR<_EmitterId_>
#define BSRRtoR eBSRRtoR<_EmitterId_>
#define BSWAP32R eBSWAP32R<_EmitterId_>
#define LEA16RtoR eLEA16RtoR<_EmitterId_>
#define LEA32RtoR eLEA32RtoR<_EmitterId_>
#define LEA16RRtoR eLEA16RRtoR<_EmitterId_>
#define LEA32RRtoR eLEA32RRtoR<_EmitterId_>
#define LEA16RStoR eLEA16RStoR<_EmitterId_>
#define LEA32RStoR eLEA32RStoR<_EmitterId_>
//------------------------------------------------------------------

//------------------------------------------------------------------
// FPU instructions 
//------------------------------------------------------------------
#define FILD32 eFILD32<_EmitterId_>
#define FISTP32 eFISTP32<_EmitterId_>
#define FLD32 eFLD32<_EmitterId_>
#define FLD eFLD<_EmitterId_>
#define FLD1 eFLD1<_EmitterId_>
#define FLDL2E eFLDL2E<_EmitterId_>
#define FST32 eFST32<_EmitterId_>
#define FSTP32 eFSTP32<_EmitterId_>
#define FSTP eFSTP<_EmitterId_>
#define FLDCW eFLDCW<_EmitterId_>
#define FNSTCW eFNSTCW<_EmitterId_>
#define FADD32Rto0 eFADD32Rto0<_EmitterId_>
#define FADD320toR eFADD320toR<_EmitterId_>
#define FSUB32Rto0 eFSUB32Rto0<_EmitterId_>
#define FSUB320toR eFSUB320toR<_EmitterId_>
#define FSUBP eFSUBP<_EmitterId_>
#define FMUL32Rto0 eFMUL32Rto0<_EmitterId_>
#define FMUL320toR eFMUL320toR<_EmitterId_>
#define FDIV32Rto0 eFDIV32Rto0<_EmitterId_>
#define FDIV320toR eFDIV320toR<_EmitterId_>
#define FDIV320toRP eFDIV320toRP<_EmitterId_>
#define FADD32 eFADD32<_EmitterId_>
#define FSUB32 eFSUB32<_EmitterId_>
#define FMUL32 eFMUL32<_EmitterId_>
#define FDIV32 eFDIV32<_EmitterId_>
#define FCOMI eFCOMI<_EmitterId_>
#define FCOMIP eFCOMIP<_EmitterId_>
#define FUCOMI eFUCOMI<_EmitterId_>
#define FUCOMIP eFUCOMIP<_EmitterId_>
#define FCOM32 eFCOM32<_EmitterId_>
#define FABS eFABS<_EmitterId_>
#define FSQRT eFSQRT<_EmitterId_>
#define FPATAN eFPATAN<_EmitterId_>
#define FSIN eFSIN<_EmitterId_>
#define FCHS eFCHS<_EmitterId_>
#define FCMOVB32 eFCMOVB32<_EmitterId_>
#define FCMOVE32 eFCMOVE32<_EmitterId_>
#define FCMOVBE32 eFCMOVBE32<_EmitterId_>
#define FCMOVU32 eFCMOVU32<_EmitterId_>
#define FCMOVNB32 eFCMOVNB32<_EmitterId_>
#define FCMOVNE32 eFCMOVNE32<_EmitterId_>
#define FCMOVNBE32 eFCMOVNBE32<_EmitterId_>
#define FCMOVNU32 eFCMOVNU32<_EmitterId_>
#define FCOMP32 eFCOMP32<_EmitterId_>
#define FNSTSWtoAX eFNSTSWtoAX<_EmitterId_>
#define FXAM eFXAM<_EmitterId_>
#define FDECSTP eFDECSTP<_EmitterId_>
#define FRNDINT eFRNDINT<_EmitterId_>
#define FXCH eFXCH<_EmitterId_>
#define F2XM1 eF2XM1<_EmitterId_>
#define FSCALE eFSCALE<_EmitterId_>
//------------------------------------------------------------------

//------------------------------------------------------------------
// MMX instructions
//------------------------------------------------------------------
#define EMMS eEMMS<_EmitterId_>
#define MOVQMtoR eMOVQMtoR<_EmitterId_>
#define MOVQRtoM eMOVQRtoM<_EmitterId_>
#define PANDRtoR ePANDRtoR<_EmitterId_>
#define PANDNRtoR ePANDNRtoR<_EmitterId_>
#define PANDMtoR ePANDMtoR<_EmitterId_>
#define PANDNRtoR ePANDNRtoR<_EmitterId_>
#define PANDNMtoR ePANDNMtoR<_EmitterId_>
#define PORRtoR ePORRtoR<_EmitterId_>
#define PORMtoR ePORMtoR<_EmitterId_>
#define PXORRtoR ePXORRtoR<_EmitterId_>
#define PXORMtoR ePXORMtoR<_EmitterId_>
#define PSLLQRtoR ePSLLQRtoR<_EmitterId_>
#define PSLLQMtoR ePSLLQMtoR<_EmitterId_>
#define PSLLQItoR ePSLLQItoR<_EmitterId_>
#define PSRLQRtoR ePSRLQRtoR<_EmitterId_>
#define PSRLQMtoR ePSRLQMtoR<_EmitterId_>
#define PSRLQItoR ePSRLQItoR<_EmitterId_>
#define PADDUSBRtoR ePADDUSBRtoR<_EmitterId_>
#define PADDUSBMtoR ePADDUSBMtoR<_EmitterId_>
#define PADDUSWRtoR ePADDUSWRtoR<_EmitterId_>
#define PADDUSWMtoR ePADDUSWMtoR<_EmitterId_>
#define PADDBRtoR ePADDBRtoR<_EmitterId_>
#define PADDBMtoR ePADDBMtoR<_EmitterId_>
#define PADDWRtoR ePADDWRtoR<_EmitterId_>
#define PADDWMtoR ePADDWMtoR<_EmitterId_>
#define PADDDRtoR ePADDDRtoR<_EmitterId_>
#define PADDDMtoR ePADDDMtoR<_EmitterId_>
#define PADDSBRtoR ePADDSBRtoR<_EmitterId_>
#define PADDSWRtoR ePADDSWRtoR<_EmitterId_>
#define PADDQMtoR ePADDQMtoR<_EmitterId_>
#define PADDQRtoR ePADDQRtoR<_EmitterId_>
#define PSUBSBRtoR ePSUBSBRtoR<_EmitterId_>
#define PSUBSWRtoR ePSUBSWRtoR<_EmitterId_>
#define PSUBBRtoR ePSUBBRtoR<_EmitterId_>
#define PSUBWRtoR ePSUBWRtoR<_EmitterId_>
#define PSUBDRtoR ePSUBDRtoR<_EmitterId_>
#define PSUBDMtoR ePSUBDMtoR<_EmitterId_>
#define PSUBQMtoR ePSUBQMtoR<_EmitterId_>
#define PSUBQRtoR ePSUBQRtoR<_EmitterId_>
#define PMULUDQMtoR ePMULUDQMtoR<_EmitterId_>
#define PMULUDQRtoR ePMULUDQRtoR<_EmitterId_>
#define PCMPEQBRtoR ePCMPEQBRtoR<_EmitterId_>
#define PCMPEQWRtoR ePCMPEQWRtoR<_EmitterId_>
#define PCMPEQDRtoR ePCMPEQDRtoR<_EmitterId_>
#define PCMPEQDMtoR ePCMPEQDMtoR<_EmitterId_>
#define PCMPGTBRtoR ePCMPGTBRtoR<_EmitterId_>
#define PCMPGTWRtoR ePCMPGTWRtoR<_EmitterId_>
#define PCMPGTDRtoR ePCMPGTDRtoR<_EmitterId_>
#define PCMPGTDMtoR ePCMPGTDMtoR<_EmitterId_>
#define PSRLWItoR ePSRLWItoR<_EmitterId_>
#define PSRLDItoR ePSRLDItoR<_EmitterId_>
#define PSRLDRtoR ePSRLDRtoR<_EmitterId_>
#define PSLLWItoR ePSLLWItoR<_EmitterId_>
#define PSLLDItoR ePSLLDItoR<_EmitterId_>
#define PSLLDRtoR ePSLLDRtoR<_EmitterId_>
#define PSRAWItoR ePSRAWItoR<_EmitterId_>
#define PSRADItoR ePSRADItoR<_EmitterId_>
#define PSRADRtoR ePSRADRtoR<_EmitterId_>
#define PUNPCKLDQRtoR ePUNPCKLDQRtoR<_EmitterId_>
#define PUNPCKLDQMtoR ePUNPCKLDQMtoR<_EmitterId_>
#define PUNPCKHDQRtoR ePUNPCKHDQRtoR<_EmitterId_>
#define PUNPCKHDQMtoR ePUNPCKHDQMtoR<_EmitterId_>
#define MOVQ64ItoR eMOVQ64ItoR<_EmitterId_>
#define MOVQRtoR eMOVQRtoR<_EmitterId_>
#define MOVQRmtoROffset eMOVQRmtoROffset<_EmitterId_>
#define MOVQRtoRmOffset eMOVQRtoRmOffset<_EmitterId_>
#define MOVDMtoMMX eMOVDMtoMMX<_EmitterId_>
#define MOVDMMXtoM eMOVDMMXtoM<_EmitterId_>
#define MOVD32RtoMMX eMOVD32RtoMMX<_EmitterId_>
#define MOVD32RmtoMMX eMOVD32RmtoMMX<_EmitterId_>
#define MOVD32RmOffsettoMMX eMOVD32RmOffsettoMMX<_EmitterId_>
#define MOVD32MMXtoR eMOVD32MMXtoR<_EmitterId_>
#define MOVD32MMXtoRm eMOVD32MMXtoRm<_EmitterId_>
#define MOVD32MMXtoRmOffset eMOVD32MMXtoRmOffset<_EmitterId_>
#define PINSRWRtoMMX ePINSRWRtoMMX<_EmitterId_>
#define PSHUFWRtoR ePSHUFWRtoR<_EmitterId_>
#define PSHUFWMtoR ePSHUFWMtoR<_EmitterId_>
#define MASKMOVQRtoR eMASKMOVQRtoR<_EmitterId_>
#define PMOVMSKBMMXtoR ePMOVMSKBMMXtoR<_EmitterId_>
//------------------------------------------------------------------
// PACKSSWB,PACKSSDW: Pack Saturate Signed Word 64bits
//------------------------------------------------------------------
#define PACKSSWBMMXtoMMX ePACKSSWBMMXtoMMX<_EmitterId_>
#define PACKSSDWMMXtoMMX ePACKSSDWMMXtoMMX<_EmitterId_>
//------------------------------------------------------------------

//------------------------------------------------------------------
// *SSE instructions* 
//------------------------------------------------------------------
#define SSE_STMXCSR eSSE_STMXCSR<_EmitterId_>
#define SSE_LDMXCSR eSSE_LDMXCSR<_EmitterId_>
#define SSE_MOVAPS_M128_to_XMM eSSE_MOVAPS_M128_to_XMM<_EmitterId_>
#define SSE_MOVAPS_XMM_to_M128 eSSE_MOVAPS_XMM_to_M128<_EmitterId_>
#define SSE_MOVAPS_XMM_to_XMM eSSE_MOVAPS_XMM_to_XMM<_EmitterId_>
#define SSE_MOVUPS_M128_to_XMM eSSE_MOVUPS_M128_to_XMM<_EmitterId_>
#define SSE_MOVUPS_XMM_to_M128 eSSE_MOVUPS_XMM_to_M128<_EmitterId_>
#define SSE_MOVSS_M32_to_XMM eSSE_MOVSS_M32_to_XMM<_EmitterId_>
#define SSE_MOVSS_XMM_to_M32 eSSE_MOVSS_XMM_to_M32<_EmitterId_>
#define SSE_MOVSS_XMM_to_Rm eSSE_MOVSS_XMM_to_Rm<_EmitterId_>
#define SSE_MOVSS_XMM_to_XMM eSSE_MOVSS_XMM_to_XMM<_EmitterId_>
#define SSE_MOVSS_RmOffset_to_XMM eSSE_MOVSS_RmOffset_to_XMM<_EmitterId_>
#define SSE_MOVSS_XMM_to_RmOffset eSSE_MOVSS_XMM_to_RmOffset<_EmitterId_>
#define SSE_MASKMOVDQU_XMM_to_XMM eSSE_MASKMOVDQU_XMM_to_XMM<_EmitterId_>
#define SSE_MOVLPS_M64_to_XMM eSSE_MOVLPS_M64_to_XMM<_EmitterId_>
#define SSE_MOVLPS_XMM_to_M64 eSSE_MOVLPS_XMM_to_M64<_EmitterId_>
#define SSE_MOVLPS_RmOffset_to_XMM eSSE_MOVLPS_RmOffset_to_XMM<_EmitterId_>
#define SSE_MOVLPS_XMM_to_RmOffset eSSE_MOVLPS_XMM_to_RmOffset<_EmitterId_>
#define SSE_MOVHPS_M64_to_XMM eSSE_MOVHPS_M64_to_XMM<_EmitterId_>
#define SSE_MOVHPS_XMM_to_M64 eSSE_MOVHPS_XMM_to_M64<_EmitterId_>
#define SSE_MOVHPS_RmOffset_to_XMM eSSE_MOVHPS_RmOffset_to_XMM<_EmitterId_>
#define SSE_MOVHPS_XMM_to_RmOffset eSSE_MOVHPS_XMM_to_RmOffset<_EmitterId_>
#define SSE_MOVLHPS_XMM_to_XMM eSSE_MOVLHPS_XMM_to_XMM<_EmitterId_>
#define SSE_MOVHLPS_XMM_to_XMM eSSE_MOVHLPS_XMM_to_XMM<_EmitterId_>
#define SSE_MOVLPSRmtoR eSSE_MOVLPSRmtoR<_EmitterId_>
#define SSE_MOVLPSRmtoROffset eSSE_MOVLPSRmtoROffset<_EmitterId_>
#define SSE_MOVLPSRtoRm eSSE_MOVLPSRtoRm<_EmitterId_>
#define SSE_MOVLPSRtoRmOffset eSSE_MOVLPSRtoRmOffset<_EmitterId_>
#define SSE_MOVAPSRmStoR eSSE_MOVAPSRmStoR<_EmitterId_>
#define SSE_MOVAPSRtoRmS eSSE_MOVAPSRtoRmS<_EmitterId_>
#define SSE_MOVAPSRtoRmOffset eSSE_MOVAPSRtoRmOffset<_EmitterId_>
#define SSE_MOVAPSRmtoROffset eSSE_MOVAPSRmtoROffset<_EmitterId_>
#define SSE_MOVUPSRmStoR eSSE_MOVUPSRmStoR<_EmitterId_>
#define SSE_MOVUPSRtoRmS eSSE_MOVUPSRtoRmS<_EmitterId_>
#define SSE_MOVUPSRtoRm eSSE_MOVUPSRtoRm<_EmitterId_>
#define SSE_MOVUPSRmtoR eSSE_MOVUPSRmtoR<_EmitterId_>
#define SSE_MOVUPSRmtoROffset eSSE_MOVUPSRmtoROffset<_EmitterId_>
#define SSE_MOVUPSRtoRmOffset eSSE_MOVUPSRtoRmOffset<_EmitterId_>
#define SSE_RCPPS_XMM_to_XMM eSSE_RCPPS_XMM_to_XMM<_EmitterId_>
#define SSE_RCPPS_M128_to_XMM eSSE_RCPPS_M128_to_XMM<_EmitterId_>
#define SSE_RCPSS_XMM_to_XMM eSSE_RCPSS_XMM_to_XMM<_EmitterId_>
#define SSE_RCPSS_M32_to_XMM eSSE_RCPSS_M32_to_XMM<_EmitterId_>
#define SSE_ORPS_M128_to_XMM eSSE_ORPS_M128_to_XMM<_EmitterId_>
#define SSE_ORPS_XMM_to_XMM eSSE_ORPS_XMM_to_XMM<_EmitterId_>
#define SSE_XORPS_M128_to_XMM eSSE_XORPS_M128_to_XMM<_EmitterId_>
#define SSE_XORPS_XMM_to_XMM eSSE_XORPS_XMM_to_XMM<_EmitterId_>
#define SSE_ANDPS_M128_to_XMM eSSE_ANDPS_M128_to_XMM<_EmitterId_>
#define SSE_ANDPS_XMM_to_XMM eSSE_ANDPS_XMM_to_XMM<_EmitterId_>
#define SSE_ANDNPS_M128_to_XMM eSSE_ANDNPS_M128_to_XMM<_EmitterId_>
#define SSE_ANDNPS_XMM_to_XMM eSSE_ANDNPS_XMM_to_XMM<_EmitterId_>
#define SSE_ADDPS_M128_to_XMM eSSE_ADDPS_M128_to_XMM<_EmitterId_>
#define SSE_ADDPS_XMM_to_XMM eSSE_ADDPS_XMM_to_XMM<_EmitterId_>
#define SSE_ADDSS_M32_to_XMM eSSE_ADDSS_M32_to_XMM<_EmitterId_>
#define SSE_ADDSS_XMM_to_XMM eSSE_ADDSS_XMM_to_XMM<_EmitterId_>
#define SSE_SUBPS_M128_to_XMM eSSE_SUBPS_M128_to_XMM<_EmitterId_>
#define SSE_SUBPS_XMM_to_XMM eSSE_SUBPS_XMM_to_XMM<_EmitterId_>
#define SSE_SUBSS_M32_to_XMM eSSE_SUBSS_M32_to_XMM<_EmitterId_>
#define SSE_SUBSS_XMM_to_XMM eSSE_SUBSS_XMM_to_XMM<_EmitterId_>
#define SSE_MULPS_M128_to_XMM eSSE_MULPS_M128_to_XMM<_EmitterId_>
#define SSE_MULPS_XMM_to_XMM eSSE_MULPS_XMM_to_XMM<_EmitterId_>
#define SSE_MULSS_M32_to_XMM eSSE_MULSS_M32_to_XMM<_EmitterId_>
#define SSE_MULSS_XMM_to_XMM eSSE_MULSS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPEQSS_M32_to_XMM eSSE_CMPEQSS_M32_to_XMM<_EmitterId_>
#define SSE_CMPEQSS_XMM_to_XMM eSSE_CMPEQSS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPLTSS_M32_to_XMM eSSE_CMPLTSS_M32_to_XMM<_EmitterId_>
#define SSE_CMPLTSS_XMM_to_XMM eSSE_CMPLTSS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPLESS_M32_to_XMM eSSE_CMPLESS_M32_to_XMM<_EmitterId_>
#define SSE_CMPLESS_XMM_to_XMM eSSE_CMPLESS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPUNORDSS_M32_to_XMM eSSE_CMPUNORDSS_M32_to_XMM<_EmitterId_>
#define SSE_CMPUNORDSS_XMM_to_XMM eSSE_CMPUNORDSS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPNESS_M32_to_XMM eSSE_CMPNESS_M32_to_XMM<_EmitterId_>
#define SSE_CMPNESS_XMM_to_XMM eSSE_CMPNESS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPNLTSS_M32_to_XMM eSSE_CMPNLTSS_M32_to_XMM<_EmitterId_>
#define SSE_CMPNLTSS_XMM_to_XMM eSSE_CMPNLTSS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPNLESS_M32_to_XMM eSSE_CMPNLESS_M32_to_XMM<_EmitterId_>
#define SSE_CMPNLESS_XMM_to_XMM eSSE_CMPNLESS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPORDSS_M32_to_XMM eSSE_CMPORDSS_M32_to_XMM<_EmitterId_>
#define SSE_CMPORDSS_XMM_to_XMM eSSE_CMPORDSS_XMM_to_XMM<_EmitterId_>
#define SSE_UCOMISS_M32_to_XMM eSSE_UCOMISS_M32_to_XMM<_EmitterId_>
#define SSE_UCOMISS_XMM_to_XMM eSSE_UCOMISS_XMM_to_XMM<_EmitterId_>
#define SSE_PMAXSW_MM_to_MM eSSE_PMAXSW_MM_to_MM<_EmitterId_>
#define SSE_PMINSW_MM_to_MM eSSE_PMINSW_MM_to_MM<_EmitterId_>
#define SSE_CVTPI2PS_MM_to_XMM eSSE_CVTPI2PS_MM_to_XMM<_EmitterId_>
#define SSE_CVTPS2PI_M64_to_MM eSSE_CVTPS2PI_M64_to_MM<_EmitterId_>
#define SSE_CVTPS2PI_XMM_to_MM eSSE_CVTPS2PI_XMM_to_MM<_EmitterId_>
#define SSE_CVTPI2PS_M64_to_XMM eSSE_CVTPI2PS_M64_to_XMM<_EmitterId_>
#define SSE_CVTTSS2SI_M32_to_R32 eSSE_CVTTSS2SI_M32_to_R32<_EmitterId_>
#define SSE_CVTTSS2SI_XMM_to_R32 eSSE_CVTTSS2SI_XMM_to_R32<_EmitterId_>
#define SSE_CVTSI2SS_M32_to_XMM eSSE_CVTSI2SS_M32_to_XMM<_EmitterId_>
#define SSE_CVTSI2SS_R_to_XMM eSSE_CVTSI2SS_R_to_XMM<_EmitterId_>
#define SSE_MAXPS_M128_to_XMM eSSE_MAXPS_M128_to_XMM<_EmitterId_>
#define SSE_MAXPS_XMM_to_XMM eSSE_MAXPS_XMM_to_XMM<_EmitterId_>
#define SSE_MAXSS_M32_to_XMM eSSE_MAXSS_M32_to_XMM<_EmitterId_>
#define SSE_MAXSS_XMM_to_XMM eSSE_MAXSS_XMM_to_XMM<_EmitterId_>
#define SSE_MINPS_M128_to_XMM eSSE_MINPS_M128_to_XMM<_EmitterId_>
#define SSE_MINPS_XMM_to_XMM eSSE_MINPS_XMM_to_XMM<_EmitterId_>
#define SSE_MINSS_M32_to_XMM eSSE_MINSS_M32_to_XMM<_EmitterId_>
#define SSE_MINSS_XMM_to_XMM eSSE_MINSS_XMM_to_XMM<_EmitterId_>
#define SSE_RSQRTPS_M128_to_XMM eSSE_RSQRTPS_M128_to_XMM<_EmitterId_>
#define SSE_RSQRTPS_XMM_to_XMM eSSE_RSQRTPS_XMM_to_XMM<_EmitterId_>
#define SSE_RSQRTSS_M32_to_XMM eSSE_RSQRTSS_M32_to_XMM<_EmitterId_>
#define SSE_RSQRTSS_XMM_to_XMM eSSE_RSQRTSS_XMM_to_XMM<_EmitterId_>
#define SSE_SQRTPS_M128_to_XMM eSSE_SQRTPS_M128_to_XMM<_EmitterId_>
#define SSE_SQRTPS_XMM_to_XMM eSSE_SQRTPS_XMM_to_XMM<_EmitterId_>
#define SSE_SQRTSS_M32_to_XMM eSSE_SQRTSS_M32_to_XMM<_EmitterId_>
#define SSE_SQRTSS_XMM_to_XMM eSSE_SQRTSS_XMM_to_XMM<_EmitterId_>
#define SSE_UNPCKLPS_M128_to_XMM eSSE_UNPCKLPS_M128_to_XMM<_EmitterId_>
#define SSE_UNPCKLPS_XMM_to_XMM eSSE_UNPCKLPS_XMM_to_XMM<_EmitterId_>
#define SSE_UNPCKHPS_M128_to_XMM eSSE_UNPCKHPS_M128_to_XMM<_EmitterId_>
#define SSE_UNPCKHPS_XMM_to_XMM eSSE_UNPCKHPS_XMM_to_XMM<_EmitterId_>
#define SSE_SHUFPS_XMM_to_XMM eSSE_SHUFPS_XMM_to_XMM<_EmitterId_>
#define SSE_SHUFPS_M128_to_XMM eSSE_SHUFPS_M128_to_XMM<_EmitterId_>
#define SSE_SHUFPS_RmOffset_to_XMM eSSE_SHUFPS_RmOffset_to_XMM<_EmitterId_>
#define SSE_CMPEQPS_M128_to_XMM eSSE_CMPEQPS_M128_to_XMM<_EmitterId_>
#define SSE_CMPEQPS_XMM_to_XMM eSSE_CMPEQPS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPLTPS_M128_to_XMM eSSE_CMPLTPS_M128_to_XMM<_EmitterId_>
#define SSE_CMPLTPS_XMM_to_XMM eSSE_CMPLTPS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPLEPS_M128_to_XMM eSSE_CMPLEPS_M128_to_XMM<_EmitterId_>
#define SSE_CMPLEPS_XMM_to_XMM eSSE_CMPLEPS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPUNORDPS_M128_to_XMM eSSE_CMPUNORDPS_M128_to_XMM<_EmitterId_>
#define SSE_CMPUNORDPS_XMM_to_XMM eSSE_CMPUNORDPS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPNEPS_M128_to_XMM eSSE_CMPNEPS_M128_to_XMM<_EmitterId_>
#define SSE_CMPNEPS_XMM_to_XMM eSSE_CMPNEPS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPNLTPS_M128_to_XMM eSSE_CMPNLTPS_M128_to_XMM<_EmitterId_>
#define SSE_CMPNLTPS_XMM_to_XMM eSSE_CMPNLTPS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPNLEPS_M128_to_XMM eSSE_CMPNLEPS_M128_to_XMM<_EmitterId_>
#define SSE_CMPNLEPS_XMM_to_XMM eSSE_CMPNLEPS_XMM_to_XMM<_EmitterId_>
#define SSE_CMPORDPS_M128_to_XMM eSSE_CMPORDPS_M128_to_XMM<_EmitterId_>
#define SSE_CMPORDPS_XMM_to_XMM eSSE_CMPORDPS_XMM_to_XMM<_EmitterId_>
#define SSE_DIVPS_M128_to_XMM eSSE_DIVPS_M128_to_XMM<_EmitterId_>
#define SSE_DIVPS_XMM_to_XMM eSSE_DIVPS_XMM_to_XMM<_EmitterId_>
#define SSE_DIVSS_M32_to_XMM eSSE_DIVSS_M32_to_XMM<_EmitterId_>
#define SSE_DIVSS_XMM_to_XMM eSSE_DIVSS_XMM_to_XMM<_EmitterId_>
//------------------------------------------------------------------

//------------------------------------------------------------------
// *SSE 2 Instructions* 
//------------------------------------------------------------------

#define SSE2_MOVDQA_M128_to_XMM eSSE2_MOVDQA_M128_to_XMM<_EmitterId_>
#define SSE2_MOVDQA_XMM_to_M128 eSSE2_MOVDQA_XMM_to_M128<_EmitterId_>
#define SSE2_MOVDQA_XMM_to_XMM eSSE2_MOVDQA_XMM_to_XMM<_EmitterId_>
#define SSE2_MOVDQU_M128_to_XMM eSSE2_MOVDQU_M128_to_XMM<_EmitterId_>
#define SSE2_MOVDQU_XMM_to_M128 eSSE2_MOVDQU_XMM_to_M128<_EmitterId_>
#define SSE2_MOVDQU_XMM_to_XMM eSSE2_MOVDQU_XMM_to_XMM<_EmitterId_>
#define SSE2_PSRLW_XMM_to_XMM eSSE2_PSRLW_XMM_to_XMM<_EmitterId_>
#define SSE2_PSRLW_M128_to_XMM eSSE2_PSRLW_M128_to_XMM<_EmitterId_> 
#define SSE2_PSRLW_I8_to_XMM eSSE2_PSRLW_I8_to_XMM<_EmitterId_>
#define SSE2_PSRLD_XMM_to_XMM eSSE2_PSRLD_XMM_to_XMM<_EmitterId_>
#define SSE2_PSRLD_M128_to_XMM eSSE2_PSRLD_M128_to_XMM<_EmitterId_>
#define SSE2_PSRLD_I8_to_XMM eSSE2_PSRLD_I8_to_XMM<_EmitterId_>
#define SSE2_PSRLQ_XMM_to_XMM eSSE2_PSRLQ_XMM_to_XMM<_EmitterId_> 
#define SSE2_PSRLQ_M128_to_XMM eSSE2_PSRLQ_M128_to_XMM<_EmitterId_> 
#define SSE2_PSRLQ_I8_to_XMM eSSE2_PSRLQ_I8_to_XMM<_EmitterId_> 
#define SSE2_PSRLDQ_I8_to_XMM eSSE2_PSRLDQ_I8_to_XMM<_EmitterId_> 
#define SSE2_PSRAW_XMM_to_XMM eSSE2_PSRAW_XMM_to_XMM<_EmitterId_> 
#define SSE2_PSRAW_M128_to_XMM eSSE2_PSRAW_M128_to_XMM<_EmitterId_> 
#define SSE2_PSRAW_I8_to_XMM eSSE2_PSRAW_I8_to_XMM<_EmitterId_> 
#define SSE2_PSRAD_XMM_to_XMM eSSE2_PSRAD_XMM_to_XMM<_EmitterId_> 
#define SSE2_PSRAD_M128_to_XMM eSSE2_PSRAD_M128_to_XMM<_EmitterId_> 
#define SSE2_PSRAD_I8_to_XMM eSSE2_PSRAD_I8_to_XMM<_EmitterId_> 
#define SSE2_PSLLW_XMM_to_XMM eSSE2_PSLLW_XMM_to_XMM<_EmitterId_> 
#define SSE2_PSLLW_M128_to_XMM eSSE2_PSLLW_M128_to_XMM<_EmitterId_> 
#define SSE2_PSLLW_I8_to_XMM eSSE2_PSLLW_I8_to_XMM<_EmitterId_> 
#define SSE2_PSLLD_XMM_to_XMM eSSE2_PSLLD_XMM_to_XMM<_EmitterId_> 
#define SSE2_PSLLD_M128_to_XMM eSSE2_PSLLD_M128_to_XMM<_EmitterId_> 
#define SSE2_PSLLD_I8_to_XMM eSSE2_PSLLD_I8_to_XMM<_EmitterId_> 
#define SSE2_PSLLQ_XMM_to_XMM eSSE2_PSLLQ_XMM_to_XMM<_EmitterId_> 
#define SSE2_PSLLQ_M128_to_XMM eSSE2_PSLLQ_M128_to_XMM<_EmitterId_> 
#define SSE2_PSLLQ_I8_to_XMM eSSE2_PSLLQ_I8_to_XMM<_EmitterId_> 
#define SSE2_PSLLDQ_I8_to_XMM eSSE2_PSLLDQ_I8_to_XMM<_EmitterId_> 
#define SSE2_PMAXSW_XMM_to_XMM eSSE2_PMAXSW_XMM_to_XMM<_EmitterId_>  
#define SSE2_PMAXSW_M128_to_XMM eSSE2_PMAXSW_M128_to_XMM<_EmitterId_>  
#define SSE2_PMAXUB_XMM_to_XMM eSSE2_PMAXUB_XMM_to_XMM<_EmitterId_>  
#define SSE2_PMAXUB_M128_to_XMM eSSE2_PMAXUB_M128_to_XMM<_EmitterId_>  
#define SSE2_PMINSW_XMM_to_XMM eSSE2_PMINSW_XMM_to_XMM<_EmitterId_>  
#define SSE2_PMINSW_M128_to_XMM eSSE2_PMINSW_M128_to_XMM<_EmitterId_>  
#define SSE2_PMINUB_XMM_to_XMM eSSE2_PMINUB_XMM_to_XMM<_EmitterId_>  
#define SSE2_PMINUB_M128_to_XMM eSSE2_PMINUB_M128_to_XMM<_EmitterId_>  
#define SSE2_PADDSB_XMM_to_XMM eSSE2_PADDSB_XMM_to_XMM<_EmitterId_>  
#define SSE2_PADDSB_M128_to_XMM eSSE2_PADDSB_M128_to_XMM<_EmitterId_>  
#define SSE2_PADDSW_XMM_to_XMM eSSE2_PADDSW_XMM_to_XMM<_EmitterId_>  
#define SSE2_PADDSW_M128_to_XMM eSSE2_PADDSW_M128_to_XMM<_EmitterId_>  
#define SSE2_PSUBSB_XMM_to_XMM eSSE2_PSUBSB_XMM_to_XMM<_EmitterId_>  
#define SSE2_PSUBSB_M128_to_XMM eSSE2_PSUBSB_M128_to_XMM<_EmitterId_>  
#define SSE2_PSUBSW_XMM_to_XMM eSSE2_PSUBSW_XMM_to_XMM<_EmitterId_>  
#define SSE2_PSUBSW_M128_to_XMM eSSE2_PSUBSW_M128_to_XMM<_EmitterId_>  
#define SSE2_PSUBUSB_XMM_to_XMM eSSE2_PSUBUSB_XMM_to_XMM<_EmitterId_>  
#define SSE2_PSUBUSB_M128_to_XMM eSSE2_PSUBUSB_M128_to_XMM<_EmitterId_>  
#define SSE2_PSUBUSW_XMM_to_XMM eSSE2_PSUBUSW_XMM_to_XMM<_EmitterId_>  
#define SSE2_PSUBUSW_M128_to_XMM eSSE2_PSUBUSW_M128_to_XMM<_EmitterId_>  
#define SSE2_PAND_XMM_to_XMM eSSE2_PAND_XMM_to_XMM<_EmitterId_>  
#define SSE2_PAND_M128_to_XMM eSSE2_PAND_M128_to_XMM<_EmitterId_>  
#define SSE2_PANDN_XMM_to_XMM eSSE2_PANDN_XMM_to_XMM<_EmitterId_>  
#define SSE2_PANDN_M128_to_XMM eSSE2_PANDN_M128_to_XMM<_EmitterId_>  
#define SSE2_PXOR_XMM_to_XMM eSSE2_PXOR_XMM_to_XMM<_EmitterId_>  
#define SSE2_PXOR_M128_to_XMM eSSE2_PXOR_M128_to_XMM<_EmitterId_>  
#define SSE2_PADDW_XMM_to_XMM eSSE2_PADDW_XMM_to_XMM<_EmitterId_> 
#define SSE2_PADDW_M128_to_XMM eSSE2_PADDW_M128_to_XMM<_EmitterId_> 
#define SSE2_PADDUSB_XMM_to_XMM eSSE2_PADDUSB_XMM_to_XMM<_EmitterId_>  
#define SSE2_PADDUSB_M128_to_XMM eSSE2_PADDUSB_M128_to_XMM<_EmitterId_>  
#define SSE2_PADDUSW_XMM_to_XMM eSSE2_PADDUSW_XMM_to_XMM<_EmitterId_>  
#define SSE2_PADDUSW_M128_to_XMM eSSE2_PADDUSW_M128_to_XMM<_EmitterId_>  
#define SSE2_PADDB_XMM_to_XMM eSSE2_PADDB_XMM_to_XMM<_EmitterId_> 
#define SSE2_PADDB_M128_to_XMM eSSE2_PADDB_M128_to_XMM<_EmitterId_> 
#define SSE2_PADDD_XMM_to_XMM eSSE2_PADDD_XMM_to_XMM<_EmitterId_> 
#define SSE2_PADDD_M128_to_XMM eSSE2_PADDD_M128_to_XMM<_EmitterId_> 
#define SSE2_PADDQ_XMM_to_XMM eSSE2_PADDQ_XMM_to_XMM<_EmitterId_> 
#define SSE2_PADDQ_M128_to_XMM eSSE2_PADDQ_M128_to_XMM<_EmitterId_> 
#define SSE2_PMADDWD_XMM_to_XMM eSSE2_PMADDWD_XMM_to_XMM<_EmitterId_>
#define SSE2_MOVSD_XMM_to_XMM eSSE2_MOVSD_XMM_to_XMM<_EmitterId_>  
#define SSE2_MOVQ_M64_to_XMM eSSE2_MOVQ_M64_to_XMM<_EmitterId_>  
#define SSE2_MOVQ_XMM_to_XMM eSSE2_MOVQ_XMM_to_XMM<_EmitterId_>  
#define SSE2_MOVQ_XMM_to_M64 eSSE2_MOVQ_XMM_to_M64<_EmitterId_>
#define SSE2_MOVDQ2Q_XMM_to_MM eSSE2_MOVDQ2Q_XMM_to_MM<_EmitterId_>
#define SSE2_MOVQ2DQ_MM_to_XMM eSSE2_MOVQ2DQ_MM_to_XMM<_EmitterId_>
#define SSE2_MOVDQARtoRmOffset eSSE2_MOVDQARtoRmOffset<_EmitterId_>
#define SSE2_MOVDQARmtoROffset eSSE2_MOVDQARmtoROffset<_EmitterId_>
#define SSE2_CVTDQ2PS_M128_to_XMM eSSE2_CVTDQ2PS_M128_to_XMM<_EmitterId_>  
#define SSE2_CVTDQ2PS_XMM_to_XMM eSSE2_CVTDQ2PS_XMM_to_XMM<_EmitterId_>  
#define SSE2_CVTPS2DQ_M128_to_XMM eSSE2_CVTPS2DQ_M128_to_XMM<_EmitterId_>  
#define SSE2_CVTPS2DQ_XMM_to_XMM eSSE2_CVTPS2DQ_XMM_to_XMM<_EmitterId_>  
#define SSE2_CVTTPS2DQ_XMM_to_XMM eSSE2_CVTTPS2DQ_XMM_to_XMM<_EmitterId_>  
#define SSE2_MAXPD_M128_to_XMM eSSE2_MAXPD_M128_to_XMM<_EmitterId_>  
#define SSE2_MAXPD_XMM_to_XMM eSSE2_MAXPD_XMM_to_XMM<_EmitterId_>  
#define SSE2_MINPD_M128_to_XMM eSSE2_MINPD_M128_to_XMM<_EmitterId_>  
#define SSE2_MINPD_XMM_to_XMM eSSE2_MINPD_XMM_to_XMM<_EmitterId_>  
#define SSE2_PSHUFD_XMM_to_XMM eSSE2_PSHUFD_XMM_to_XMM<_EmitterId_>
#define SSE2_PSHUFD_M128_to_XMM eSSE2_PSHUFD_M128_to_XMM<_EmitterId_>
#define SSE2_PSHUFLW_XMM_to_XMM eSSE2_PSHUFLW_XMM_to_XMM<_EmitterId_>
#define SSE2_PSHUFLW_M128_to_XMM eSSE2_PSHUFLW_M128_to_XMM<_EmitterId_>
#define SSE2_PSHUFHW_XMM_to_XMM eSSE2_PSHUFHW_XMM_to_XMM<_EmitterId_>
#define SSE2_PSHUFHW_M128_to_XMM eSSE2_PSHUFHW_M128_to_XMM<_EmitterId_>
#define SSE2_SHUFPD_XMM_to_XMM eSSE2_SHUFPD_XMM_to_XMM<_EmitterId_>
#define SSE2_SHUFPD_M128_to_XMM eSSE2_SHUFPD_M128_to_XMM<_EmitterId_>
#define SSE2_ORPD_M128_to_XMM eSSE2_ORPD_M128_to_XMM<_EmitterId_>
#define SSE2_ORPD_XMM_to_XMM eSSE2_ORPD_XMM_to_XMM<_EmitterId_>
#define SSE2_XORPD_M128_to_XMM eSSE2_XORPD_M128_to_XMM<_EmitterId_>
#define SSE2_XORPD_XMM_to_XMM eSSE2_XORPD_XMM_to_XMM<_EmitterId_>
#define SSE2_ANDPD_M128_to_XMM eSSE2_ANDPD_M128_to_XMM<_EmitterId_> 
#define SSE2_ANDPD_XMM_to_XMM eSSE2_ANDPD_XMM_to_XMM<_EmitterId_>
#define SSE2_ANDNPD_M128_to_XMM eSSE2_ANDNPD_M128_to_XMM<_EmitterId_> 
#define SSE2_ANDNPD_XMM_to_XMM eSSE2_ANDNPD_XMM_to_XMM<_EmitterId_> 
#define SSE2_ADDSD_M64_to_XMM eSSE2_ADDSD_M64_to_XMM<_EmitterId_> 
#define SSE2_ADDSD_XMM_to_XMM eSSE2_ADDSD_XMM_to_XMM<_EmitterId_> 
#define SSE2_SUBSD_M64_to_XMM eSSE2_SUBSD_M64_to_XMM<_EmitterId_> 
#define SSE2_SUBSD_XMM_to_XMM eSSE2_SUBSD_XMM_to_XMM<_EmitterId_> 
#define SSE2_MULSD_M64_to_XMM eSSE2_MULSD_M64_to_XMM<_EmitterId_> 
#define SSE2_MULSD_XMM_to_XMM eSSE2_MULSD_XMM_to_XMM<_EmitterId_> 
#define SSE2_CMPEQSD_M64_to_XMM eSSE2_CMPEQSD_M64_to_XMM<_EmitterId_> 
#define SSE2_CMPEQSD_XMM_to_XMM eSSE2_CMPEQSD_XMM_to_XMM<_EmitterId_> 
#define SSE2_CMPLTSD_M64_to_XMM eSSE2_CMPLTSD_M64_to_XMM<_EmitterId_> 
#define SSE2_CMPLTSD_XMM_to_XMM eSSE2_CMPLTSD_XMM_to_XMM<_EmitterId_> 
#define SSE2_CMPLESD_M64_to_XMM eSSE2_CMPLESD_M64_to_XMM<_EmitterId_> 
#define SSE2_CMPLESD_XMM_to_XMM eSSE2_CMPLESD_XMM_to_XMM<_EmitterId_> 
#define SSE2_CMPUNORDSD_M64_to_XMM eSSE2_CMPUNORDSD_M64_to_XMM<_EmitterId_> 
#define SSE2_CMPUNORDSD_XMM_to_XMM eSSE2_CMPUNORDSD_XMM_to_XMM<_EmitterId_> 
#define SSE2_CMPNESD_M64_to_XMM eSSE2_CMPNESD_M64_to_XMM<_EmitterId_> 
#define SSE2_CMPNESD_XMM_to_XMM eSSE2_CMPNESD_XMM_to_XMM<_EmitterId_> 
#define SSE2_CMPNLTSD_M64_to_XMM eSSE2_CMPNLTSD_M64_to_XMM<_EmitterId_> 
#define SSE2_CMPNLTSD_XMM_to_XMM eSSE2_CMPNLTSD_XMM_to_XMM<_EmitterId_> 
#define SSE2_CMPNLESD_M64_to_XMM eSSE2_CMPNLESD_M64_to_XMM<_EmitterId_> 
#define SSE2_CMPNLESD_XMM_to_XMM eSSE2_CMPNLESD_XMM_to_XMM<_EmitterId_> 
#define SSE2_CMPORDSD_M64_to_XMM eSSE2_CMPORDSD_M64_to_XMM<_EmitterId_> 
#define SSE2_CMPORDSD_XMM_to_XMM eSSE2_CMPORDSD_XMM_to_XMM<_EmitterId_> 
#define SSE2_UCOMISD_M64_to_XMM eSSE2_UCOMISD_M64_to_XMM<_EmitterId_> 
#define SSE2_UCOMISD_XMM_to_XMM eSSE2_UCOMISD_XMM_to_XMM<_EmitterId_> 
#define SSE2_CVTSS2SD_M32_to_XMM eSSE2_CVTSS2SD_M32_to_XMM<_EmitterId_> 
#define SSE2_CVTSS2SD_XMM_to_XMM eSSE2_CVTSS2SD_XMM_to_XMM<_EmitterId_>
#define SSE2_CVTSD2SS_M64_to_XMM eSSE2_CVTSD2SS_M64_to_XMM<_EmitterId_> 
#define SSE2_CVTSD2SS_XMM_to_XMM eSSE2_CVTSD2SS_XMM_to_XMM<_EmitterId_>
#define SSE2_MAXSD_M64_to_XMM eSSE2_MAXSD_M64_to_XMM<_EmitterId_> 
#define SSE2_MAXSD_XMM_to_XMM eSSE2_MAXSD_XMM_to_XMM<_EmitterId_>
#define SSE2_MINSD_M64_to_XMM eSSE2_MINSD_M64_to_XMM<_EmitterId_>
#define SSE2_MINSD_XMM_to_XMM eSSE2_MINSD_XMM_to_XMM<_EmitterId_>
#define SSE2_SQRTSD_M64_to_XMM eSSE2_SQRTSD_M64_to_XMM<_EmitterId_>
#define SSE2_SQRTSD_XMM_to_XMM eSSE2_SQRTSD_XMM_to_XMM<_EmitterId_>
#define SSE2_DIVSD_M64_to_XMM eSSE2_DIVSD_M64_to_XMM<_EmitterId_>
#define SSE2_DIVSD_XMM_to_XMM eSSE2_DIVSD_XMM_to_XMM<_EmitterId_>
//------------------------------------------------------------------
// PACKSSWB,PACKSSDW: Pack Saturate Signed Word 
//------------------------------------------------------------------
#define SSE2_PACKSSWB_XMM_to_XMM	eSSE2_PACKSSWB_XMM_to_XMM<_EmitterId_>
#define SSE2_PACKSSWB_M128_to_XMM	eSSE2_PACKSSWB_M128_to_XMM<_EmitterId_>
#define SSE2_PACKSSDW_XMM_to_XMM	eSSE2_PACKSSDW_XMM_to_XMM<_EmitterId_>
#define SSE2_PACKSSDW_M128_to_XMM	eSSE2_PACKSSDW_M128_to_XMM<_EmitterId_>
#define SSE2_PACKUSWB_XMM_to_XMM	eSSE2_PACKUSWB_XMM_to_XMM<_EmitterId_>
#define SSE2_PACKUSWB_M128_to_XMM	eSSE2_PACKUSWB_M128_to_XMM<_EmitterId_>
//------------------------------------------------------------------
// PUNPCKHWD: Unpack 16bit high  
//------------------------------------------------------------------
#define SSE2_PUNPCKLBW_XMM_to_XMM	eSSE2_PUNPCKLBW_XMM_to_XMM<_EmitterId_>
#define SSE2_PUNPCKLBW_M128_to_XMM	eSSE2_PUNPCKLBW_M128_to_XMM<_EmitterId_>
#define SSE2_PUNPCKHBW_XMM_to_XMM	eSSE2_PUNPCKHBW_XMM_to_XMM<_EmitterId_>
#define SSE2_PUNPCKHBW_M128_to_XMM	eSSE2_PUNPCKHBW_M128_to_XMM<_EmitterId_>
#define SSE2_PUNPCKLWD_XMM_to_XMM	eSSE2_PUNPCKLWD_XMM_to_XMM<_EmitterId_>
#define SSE2_PUNPCKLWD_M128_to_XMM	eSSE2_PUNPCKLWD_M128_to_XMM<_EmitterId_>
#define SSE2_PUNPCKHWD_XMM_to_XMM	eSSE2_PUNPCKHWD_XMM_to_XMM<_EmitterId_>
#define SSE2_PUNPCKHWD_M128_to_XMM	eSSE2_PUNPCKHWD_M128_to_XMM<_EmitterId_>
#define SSE2_PUNPCKLDQ_XMM_to_XMM	eSSE2_PUNPCKLDQ_XMM_to_XMM<_EmitterId_>
#define SSE2_PUNPCKLDQ_M128_to_XMM	eSSE2_PUNPCKLDQ_M128_to_XMM<_EmitterId_>
#define SSE2_PUNPCKHDQ_XMM_to_XMM	eSSE2_PUNPCKHDQ_XMM_to_XMM<_EmitterId_>
#define SSE2_PUNPCKHDQ_M128_to_XMM	eSSE2_PUNPCKHDQ_M128_to_XMM<_EmitterId_>
#define SSE2_PUNPCKLQDQ_XMM_to_XMM	eSSE2_PUNPCKLQDQ_XMM_to_XMM<_EmitterId_>
#define SSE2_PUNPCKLQDQ_M128_to_XMM	eSSE2_PUNPCKLQDQ_M128_to_XMM<_EmitterId_>
#define SSE2_PUNPCKHQDQ_XMM_to_XMM	eSSE2_PUNPCKHQDQ_XMM_to_XMM<_EmitterId_>
#define SSE2_PUNPCKHQDQ_M128_to_XMM	eSSE2_PUNPCKHQDQ_M128_to_XMM<_EmitterId_>
#define SSE2_PMULLW_XMM_to_XMM		eSSE2_PMULLW_XMM_to_XMM<_EmitterId_>
#define SSE2_PMULLW_M128_to_XMM		eSSE2_PMULLW_M128_to_XMM<_EmitterId_>
#define SSE2_PMULHW_XMM_to_XMM		eSSE2_PMULHW_XMM_to_XMM<_EmitterId_>
#define SSE2_PMULHW_M128_to_XMM		eSSE2_PMULHW_M128_to_XMM<_EmitterId_>
#define SSE2_PMULUDQ_XMM_to_XMM		eSSE2_PMULUDQ_XMM_to_XMM<_EmitterId_>
#define SSE2_PMULUDQ_M128_to_XMM	eSSE2_PMULUDQ_M128_to_XMM<_EmitterId_>
//------------------------------------------------------------------
// PMOVMSKB: Create 16bit mask from signs of 8bit integers 
//------------------------------------------------------------------
#define SSE_MOVMSKPS_XMM_to_R32		eSSE_MOVMSKPS_XMM_to_R32<_EmitterId_>
#define SSE2_PMOVMSKB_XMM_to_R32	eSSE2_PMOVMSKB_XMM_to_R32<_EmitterId_>
#define SSE2_MOVMSKPD_XMM_to_R32	eSSE2_MOVMSKPD_XMM_to_R32<_EmitterId_>
//------------------------------------------------------------------
// PEXTRW,PINSRW: Packed Extract/Insert Word  
//------------------------------------------------------------------
#define SSE_PEXTRW_XMM_to_R32		eSSE_PEXTRW_XMM_to_R32<_EmitterId_>
#define SSE_PINSRW_R32_to_XMM		eSSE_PINSRW_R32_to_XMM<_EmitterId_>
//------------------------------------------------------------------
// PSUBx: Subtract Packed Integers   
//------------------------------------------------------------------
#define SSE2_PSUBB_XMM_to_XMM		eSSE2_PSUBB_XMM_to_XMM<_EmitterId_>
#define SSE2_PSUBB_M128_to_XMM		eSSE2_PSUBB_M128_to_XMM<_EmitterId_>
#define SSE2_PSUBW_XMM_to_XMM		eSSE2_PSUBW_XMM_to_XMM<_EmitterId_>
#define SSE2_PSUBW_M128_to_XMM		eSSE2_PSUBW_M128_to_XMM<_EmitterId_>
#define SSE2_PSUBD_XMM_to_XMM		eSSE2_PSUBD_XMM_to_XMM<_EmitterId_>
#define SSE2_PSUBD_M128_to_XMM		eSSE2_PSUBD_M128_to_XMM<_EmitterId_>
#define SSE2_PSUBQ_XMM_to_XMM		eSSE2_PSUBQ_XMM_to_XMM<_EmitterId_>
#define SSE2_PSUBQ_M128_to_XMM		eSSE2_PSUBQ_M128_to_XMM<_EmitterId_>
//------------------------------------------------------------------
// PCMPxx: Compare Packed Integers   
//------------------------------------------------------------------
#define SSE2_PCMPGTB_XMM_to_XMM		eSSE2_PCMPGTB_XMM_to_XMM<_EmitterId_>
#define SSE2_PCMPGTB_M128_to_XMM	eSSE2_PCMPGTB_M128_to_XMM<_EmitterId_>
#define SSE2_PCMPGTW_XMM_to_XMM		eSSE2_PCMPGTW_XMM_to_XMM<_EmitterId_>
#define SSE2_PCMPGTW_M128_to_XMM	eSSE2_PCMPGTW_M128_to_XMM<_EmitterId_>
#define SSE2_PCMPGTD_XMM_to_XMM		eSSE2_PCMPGTD_XMM_to_XMM<_EmitterId_>
#define SSE2_PCMPGTD_M128_to_XMM	eSSE2_PCMPGTD_M128_to_XMM<_EmitterId_>
#define SSE2_PCMPEQB_XMM_to_XMM		eSSE2_PCMPEQB_XMM_to_XMM<_EmitterId_>
#define SSE2_PCMPEQB_M128_to_XMM	eSSE2_PCMPEQB_M128_to_XMM<_EmitterId_>
#define SSE2_PCMPEQW_XMM_to_XMM		eSSE2_PCMPEQW_XMM_to_XMM<_EmitterId_>
#define SSE2_PCMPEQW_M128_to_XMM	eSSE2_PCMPEQW_M128_to_XMM<_EmitterId_>
#define SSE2_PCMPEQD_XMM_to_XMM		eSSE2_PCMPEQD_XMM_to_XMM<_EmitterId_>
#define SSE2_PCMPEQD_M128_to_XMM	eSSE2_PCMPEQD_M128_to_XMM<_EmitterId_>
//------------------------------------------------------------------
// MOVD: Move Dword(32bit) to /from XMM reg 
//------------------------------------------------------------------
#define SSE2_MOVD_M32_to_XMM		eSSE2_MOVD_M32_to_XMM<_EmitterId_>
#define SSE2_MOVD_R_to_XMM			eSSE2_MOVD_R_to_XMM<_EmitterId_>
#define SSE2_MOVD_Rm_to_XMM			eSSE2_MOVD_Rm_to_XMM<_EmitterId_>
#define SSE2_MOVD_RmOffset_to_XMM	eSSE2_MOVD_RmOffset_to_XMM<_EmitterId_>
#define SSE2_MOVD_XMM_to_M32		eSSE2_MOVD_XMM_to_M32<_EmitterId_>
#define SSE2_MOVD_XMM_to_R			eSSE2_MOVD_XMM_to_R<_EmitterId_>
#define SSE2_MOVD_XMM_to_Rm			eSSE2_MOVD_XMM_to_Rm<_EmitterId_>
#define SSE2_MOVD_XMM_to_RmOffset	eSSE2_MOVD_XMM_to_RmOffset<_EmitterId_>
#define SSE2_MOVQ_XMM_to_R			eSSE2_MOVQ_XMM_to_R<_EmitterId_>
#define SSE2_MOVQ_R_to_XMM			eSSE2_MOVQ_R_to_XMM<_EmitterId_>
//------------------------------------------------------------------
// POR : SSE Bitwise OR   
//------------------------------------------------------------------
#define SSE2_POR_XMM_to_XMM			eSSE2_POR_XMM_to_XMM<_EmitterId_>
#define SSE2_POR_M128_to_XMM		eSSE2_POR_M128_to_XMM<_EmitterId_>
//------------------------------------------------------------------

//------------------------------------------------------------------
// SSE3 
//------------------------------------------------------------------
#define SSE3_HADDPS_XMM_to_XMM		eSSE3_HADDPS_XMM_to_XMM<_EmitterId_>
#define SSE3_HADDPS_M128_to_XMM		eSSE3_HADDPS_M128_to_XMM<_EmitterId_>
#define SSE3_MOVSLDUP_XMM_to_XMM	eSSE3_MOVSLDUP_XMM_to_XMM<_EmitterId_>
#define SSE3_MOVSLDUP_M128_to_XMM	eSSE3_MOVSLDUP_M128_to_XMM<_EmitterId_>
#define SSE3_MOVSHDUP_XMM_to_XMM	eSSE3_MOVSHDUP_XMM_to_XMM<_EmitterId_>
#define SSE3_MOVSHDUP_M128_to_XMM	eSSE3_MOVSHDUP_M128_to_XMM<_EmitterId_>
//------------------------------------------------------------------

//------------------------------------------------------------------
// SSSE3
//------------------------------------------------------------------
#define SSSE3_PABSB_XMM_to_XMM		eSSSE3_PABSB_XMM_to_XMM<_EmitterId_>
#define SSSE3_PABSW_XMM_to_XMM		eSSSE3_PABSW_XMM_to_XMM<_EmitterId_>
#define SSSE3_PABSD_XMM_to_XMM		eSSSE3_PABSD_XMM_to_XMM<_EmitterId_>
#define SSSE3_PALIGNR_XMM_to_XMM	eSSSE3_PALIGNR_XMM_to_XMM<_EmitterId_>
#define SSSE3_PSIGNB_XMM_to_XMM		eSSSE3_PSIGNB_XMM_to_XMM<_EmitterId_>
#define SSSE3_PSIGNW_XMM_to_XMM		eSSSE3_PSIGNW_XMM_to_XMM<_EmitterId_>
#define SSSE3_PSIGND_XMM_to_XMM		eSSSE3_PSIGND_XMM_to_XMM<_EmitterId_>
//------------------------------------------------------------------

//------------------------------------------------------------------
// SSE4.1
//------------------------------------------------------------------
#define SSE4_DPPS_XMM_to_XMM		eSSE4_DPPS_XMM_to_XMM<_EmitterId_>
#define SSE4_DPPS_M128_to_XMM		eSSE4_DPPS_M128_to_XMM<_EmitterId_>
#define SSE4_INSERTPS_XMM_to_XMM	eSSE4_INSERTPS_XMM_to_XMM<_EmitterId_>
#define SSE4_EXTRACTPS_XMM_to_R32	eSSE4_EXTRACTPS_XMM_to_R32<_EmitterId_>
#define SSE4_BLENDPS_XMM_to_XMM		eSSE4_BLENDPS_XMM_to_XMM<_EmitterId_>
#define SSE4_BLENDVPS_XMM_to_XMM	eSSE4_BLENDVPS_XMM_to_XMM<_EmitterId_>
#define SSE4_BLENDVPS_M128_to_XMM	eSSE4_BLENDVPS_M128_to_XMM<_EmitterId_>
#define SSE4_PMOVSXDQ_XMM_to_XMM	eSSE4_PMOVSXDQ_XMM_to_XMM<_EmitterId_>
#define SSE4_PMOVZXDQ_XMM_to_XMM	eSSE4_PMOVZXDQ_XMM_to_XMM<_EmitterId_>
#define SSE4_PINSRD_R32_to_XMM		eSSE4_PINSRD_R32_to_XMM<_EmitterId_>
#define SSE4_PMAXSD_XMM_to_XMM		eSSE4_PMAXSD_XMM_to_XMM<_EmitterId_>
#define SSE4_PMINSD_XMM_to_XMM		eSSE4_PMINSD_XMM_to_XMM<_EmitterId_>
#define SSE4_PMAXUD_XMM_to_XMM		eSSE4_PMAXUD_XMM_to_XMM<_EmitterId_>
#define SSE4_PMINUD_XMM_to_XMM		eSSE4_PMINUD_XMM_to_XMM<_EmitterId_>
#define SSE4_PMAXSD_M128_to_XMM		eSSE4_PMAXSD_M128_to_XMM<_EmitterId_>
#define SSE4_PMINSD_M128_to_XMM		eSSE4_PMINSD_M128_to_XMM<_EmitterId_>
#define SSE4_PMAXUD_M128_to_XMM		eSSE4_PMAXUD_M128_to_XMM<_EmitterId_>
#define SSE4_PMINUD_M128_to_XMM		eSSE4_PMINUD_M128_to_XMM<_EmitterId_>
#define SSE4_PMULDQ_XMM_to_XMM		eSSE4_PMULDQ_XMM_to_XMM<_EmitterId_>
//------------------------------------------------------------------

//------------------------------------------------------------------
// 3DNOW instructions
//------------------------------------------------------------------
#define FEMMS			eFEMMS<_EmitterId_>
#define PFCMPEQMtoR		ePFCMPEQMtoR<_EmitterId_>
#define PFCMPGTMtoR		ePFCMPGTMtoR<_EmitterId_>
#define PFCMPGEMtoR		ePFCMPGEMtoR<_EmitterId_>
#define PFADDMtoR		ePFADDMtoR<_EmitterId_>
#define PFADDRtoR		ePFADDRtoR<_EmitterId_>
#define PFSUBMtoR		ePFSUBMtoR<_EmitterId_>
#define PFSUBRtoR		ePFSUBRtoR<_EmitterId_>
#define PFMULMtoR		ePFMULMtoR<_EmitterId_>
#define PFMULRtoR		ePFMULRtoR<_EmitterId_>
#define PFRCPMtoR		ePFRCPMtoR<_EmitterId_>
#define PFRCPRtoR		ePFRCPRtoR<_EmitterId_>
#define PFRCPIT1RtoR	ePFRCPIT1RtoR<_EmitterId_>
#define PFRCPIT2RtoR	ePFRCPIT2RtoR<_EmitterId_>
#define PFRSQRTRtoR		ePFRSQRTRtoR<_EmitterId_>
#define PFRSQIT1RtoR	ePFRSQIT1RtoR<_EmitterId_>
#define PF2IDMtoR		ePF2IDMtoR<_EmitterId_>
#define PI2FDMtoR		ePI2FDMtoR<_EmitterId_>
#define PI2FDRtoR		ePI2FDRtoR<_EmitterId_>
#define PFMAXMtoR		ePFMAXMtoR<_EmitterId_>
#define PFMAXRtoR		ePFMAXRtoR<_EmitterId_>
#define PFMINMtoR		ePFMINMtoR<_EmitterId_>
#define PFMINRtoR		ePFMINRtoR<_EmitterId_>
//------------------------------------------------------------------
