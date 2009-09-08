/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _R5900_OPCODETABLES_H
#define _R5900_OPCODETABLES_H

#include "Pcsx2Defs.h"

// TODO : Move these into the OpcodeTables namespace
extern  void (*Int_COP2PrintTable[32])();
extern  void (*Int_COP2BC2PrintTable[32])();
extern  void (*Int_COP2SPECIAL1PrintTable[64])();
extern  void (*Int_COP2SPECIAL2PrintTable[128])();

void COP2_BC2();
void COP2_SPECIAL();
void COP2_SPECIAL2();
void COP2_Unknown();


namespace R5900
{
	namespace Dynarec {
	namespace OpcodeImpl
	{
		void recNULL();
		void recUnknown();
		void recMMI_Unknown();
		void recCOP0_Unknown();
		void recCOP1_Unknown();

		void recCOP2();

		void recCACHE();
		void recPREF();
		void recSYSCALL();
		void recBREAK();
		void recSYNC();

		void recMFSA();
		void recMTSA();
		void recMTSAB();
		void recMTSAH();

		void recTGE();
		void recTGEU();
		void recTLT();
		void recTLTU();
		void recTEQ();
		void recTNE();
		void recTGEI();
		void recTGEIU(); 
		void recTLTI();
		void recTLTIU();
		void recTEQI();
		void recTNEI();

	} }

	///////////////////////////////////////////////////////////////////////////
	// Encapsulates information about every opcode on the Emotion Engine and
	// it's many co-processors.
	struct OPCODE
	{
		// Textual name of the instruction.
		const char Name[16];

		// Number of cycles this instruction normally uses.
		u8 cycles;

		const OPCODE& (*getsubclass)();

		// Process the instruction using the interpreter.
		// The action is performed immediately on the EE's cpu state.
		void (*interpret)();

		// Generate recompiled code for this instruction, injected into
		// the current EErec block state.
		void (*recompile)();

		// Generates a string representation of the instruction and it's parameters,
		// and pastes it into the given output parameter.
		void (*disasm)( std::string& output );
	};

	// Returns the current real instruction, as per the current cpuRegs settings.
	const OPCODE& GetCurrentInstruction();

	namespace OpcodeTables
	{
		using ::R5900::OPCODE;

		extern const OPCODE tbl_Standard[64];

		/*extern const OPCODE Standard[64];
		extern const OPCODE Special[64];
		extern const OPCODE RegImm[32];
		extern const OPCODE MMI[64];
		extern const OPCODE MMI0[32];
		extern const OPCODE MMI1[32];
		extern const OPCODE MMI2[32];
		extern const OPCODE MMI3[32];
		
		extern const OPCODE COP0[32];
		extern const OPCODE COP0_BC0[32];
		extern const OPCODE COP0_C0[64];

		extern const OPCODE COP1[32];
		extern const OPCODE COP1_BC1[32];
		extern const OPCODE COP1_S[64];
		extern const OPCODE COP1_W[64];*/
	}

	namespace Opcodes
	{
		using ::R5900::OPCODE;

		const OPCODE& Class_SPECIAL();
		const OPCODE& Class_REGIMM();
		const OPCODE& Class_MMI();
		const OPCODE& Class_MMI0();
		const OPCODE& Class_MMI1();
		const OPCODE& Class_MMI2();
		const OPCODE& Class_MMI3();

		const OPCODE& Class_COP0();
		const OPCODE& Class_COP0_BC0();
		const OPCODE& Class_COP0_C0();

		const OPCODE& Class_COP1();
		const OPCODE& Class_COP1_BC1();
		const OPCODE& Class_COP1_S();
		const OPCODE& Class_COP1_W();
	}

	namespace OpcodeDisasm
	{
//****************************************************************
		void Unknown( std::string& output );
		void COP0_Unknown( std::string& output );
		void COP1_Unknown( std::string& output );
		void MMI_Unknown( std::string& output );

		void COP2( std::string& output );

// **********************Standard Opcodes**************************
		void J( std::string& output );
		void JAL( std::string& output );
		void BEQ( std::string& output );
		void BNE( std::string& output );
		void BLEZ( std::string& output );
		void BGTZ( std::string& output );
		void ADDI( std::string& output );
		void ADDIU( std::string& output );
		void SLTI( std::string& output );
		void SLTIU( std::string& output );
		void ANDI( std::string& output );
		void ORI( std::string& output );
		void XORI( std::string& output );
		void LUI( std::string& output );
		void BEQL( std::string& output );
		void BNEL( std::string& output );
		void BLEZL( std::string& output );
		void BGTZL( std::string& output );
		void DADDI( std::string& output );
		void DADDIU( std::string& output );
		void LDL( std::string& output );
		void LDR( std::string& output );
		void LB( std::string& output );
		void LH( std::string& output );
		void LWL( std::string& output );
		void LW( std::string& output );
		void LBU( std::string& output );
		void LHU( std::string& output );
		void LWR( std::string& output );
		void LWU( std::string& output );
		void SB( std::string& output );
		void SH( std::string& output );
		void SWL( std::string& output );
		void SW( std::string& output );
		void SDL( std::string& output );
		void SDR( std::string& output );
		void SWR( std::string& output );
		void CACHE( std::string& output );
		void LWC1( std::string& output );
		void PREF( std::string& output );
		void LQC2( std::string& output );
		void LD( std::string& output );
		void SQC2( std::string& output );
		void SD( std::string& output );
		void LQ( std::string& output );
		void SQ( std::string& output );
		void SWC1( std::string& output );
//*****************end of standard opcodes**********************
//********************SPECIAL OPCODES***************************
		void SLL( std::string& output );
		void SRL( std::string& output );
		void SRA( std::string& output );
		void SLLV( std::string& output );
		void SRLV( std::string& output );
		void SRAV( std::string& output );
		void JR( std::string& output );
		void JALR( std::string& output );
		void SYSCALL( std::string& output );
		void BREAK( std::string& output );
		void SYNC( std::string& output );
		void MFHI( std::string& output );
		void MTHI( std::string& output );
		void MFLO( std::string& output );
		void MTLO( std::string& output );
		void DSLLV( std::string& output );
		void DSRLV( std::string& output );
		void DSRAV( std::string& output );
		void MULT( std::string& output );
		void MULTU( std::string& output );
		void DIV( std::string& output );
		void DIVU( std::string& output );
		void ADD( std::string& output );
		void ADDU( std::string& output );
		void SUB( std::string& output );
		void SUBU( std::string& output );
		void AND( std::string& output );
		void OR( std::string& output );
		void XOR( std::string& output );
		void NOR( std::string& output );
		void SLT( std::string& output );
		void SLTU( std::string& output );
		void DADD( std::string& output );
		void DADDU( std::string& output );
		void DSUB( std::string& output );
		void DSUBU( std::string& output );
		void TGE( std::string& output );
		void TGEU( std::string& output );
		void TLT( std::string& output );
		void TLTU( std::string& output );
		void TEQ( std::string& output );
		void TNE( std::string& output );
		void DSLL( std::string& output );
		void DSRL( std::string& output );
		void DSRA( std::string& output );
		void DSLL32( std::string& output );
		void DSRL32( std::string& output );
		void DSRA32( std::string& output );
		void MOVZ( std::string& output );
		void MOVN( std::string& output );
		void MFSA( std::string& output );
		void MTSA( std::string& output );
//*******************END OF SPECIAL OPCODES************************
//***********************REGIMM OPCODES****************************
		void BLTZ( std::string& output );
		void BGEZ( std::string& output );
		void BLTZL( std::string& output );
		void BGEZL( std::string& output );
		void TGEI( std::string& output );
		void TGEIU( std::string& output );
		void TLTI( std::string& output );
		void TLTIU( std::string& output );
		void TEQI( std::string& output );
		void TNEI( std::string& output );
		void BLTZAL( std::string& output );
		void BGEZAL( std::string& output );
		void BLTZALL( std::string& output );
		void BGEZALL( std::string& output );
		void MTSAB( std::string& output );
		void MTSAH( std::string& output );
//*******************END OF REGIMM OPCODES***********************
//***********************MMI OPCODES*****************************
		void MADD( std::string& output );
		void MADDU( std::string& output );
		void PLZCW( std::string& output );
		void MADD1( std::string& output );
		void MADDU1( std::string& output );
		void MFHI1( std::string& output );
		void MTHI1( std::string& output );
		void MFLO1( std::string& output );
		void MTLO1( std::string& output );
		void MULT1( std::string& output );
		void MULTU1( std::string& output );
		void DIV1( std::string& output );
		void DIVU1( std::string& output );
		void PMFHL( std::string& output );
		void PMTHL( std::string& output );
		void PSLLH( std::string& output );
		void PSRLH( std::string& output );
		void PSRAH( std::string& output );
		void PSLLW( std::string& output );
		void PSRLW( std::string& output );
		void PSRAW( std::string& output );
//********************END OF MMI OPCODES***********************
//***********************MMI0 OPCODES**************************
		void PADDW( std::string& output );  
		void PSUBW( std::string& output );  
		void PCGTW( std::string& output );  
		void PMAXW( std::string& output ); 
		void PADDH( std::string& output );  
		void PSUBH( std::string& output );  
		void PCGTH( std::string& output );  
		void PMAXH( std::string& output ); 
		void PADDB( std::string& output );  
		void PSUBB( std::string& output );  
		void PCGTB( std::string& output );
		void PADDSW( std::string& output ); 
		void PSUBSW( std::string& output ); 
		void PEXTLW( std::string& output );  
		void PPACW( std::string& output ); 
		void PADDSH( std::string& output );
		void PSUBSH( std::string& output );
		void PEXTLH( std::string& output ); 
		void PPACH( std::string& output );
		void PADDSB( std::string& output );
		void PSUBSB( std::string& output );
		void PEXTLB( std::string& output ); 
		void PPACB( std::string& output );
		void PEXT5( std::string& output ); 
		void PPAC5( std::string& output );
//******************END OF MMI0 OPCODES***********************
//*********************MMI1 OPCODES***************************
		void PABSW( std::string& output );
		void PCEQW( std::string& output );
		void PMINW( std::string& output ); 
		void PADSBH( std::string& output );
		void PABSH( std::string& output );
		void PCEQH( std::string& output );
		void PMINH( std::string& output );  
		void PCEQB( std::string& output ); 
		void PADDUW( std::string& output );
		void PSUBUW( std::string& output ); 
		void PEXTUW( std::string& output );  
		void PADDUH( std::string& output );
		void PSUBUH( std::string& output ); 
		void PEXTUH( std::string& output ); 
		void PADDUB( std::string& output );
		void PSUBUB( std::string& output );
		void PEXTUB( std::string& output );
		void QFSRV( std::string& output ); 
//*****************END OF MMI1 OPCODES***********************
//*********************MMI2 OPCODES**************************
		void PMADDW( std::string& output );
		void PSLLVW( std::string& output );
		void PSRLVW( std::string& output ); 
		void PMSUBW( std::string& output );
		void PMFHI( std::string& output );
		void PMFLO( std::string& output );
		void PINTH( std::string& output );
		void PMULTW( std::string& output );
		void PDIVW( std::string& output );
		void PCPYLD( std::string& output );
		void PMADDH( std::string& output );
		void PHMADH( std::string& output );
		void PAND( std::string& output );
		void PXOR( std::string& output ); 
		void PMSUBH( std::string& output );
		void PHMSBH( std::string& output );
		void PEXEH( std::string& output );
		void PREVH( std::string& output ); 
		void PMULTH( std::string& output );
		void PDIVBW( std::string& output );
		void PEXEW( std::string& output );
		void PROT3W( std::string& output );
//********************END OF MMI2 OPCODES********************
//***********************MMI3 OPCODES************************
		void PMADDUW( std::string& output );
		void PSRAVW( std::string& output ); 
		void PMTHI( std::string& output );
		void PMTLO( std::string& output );
		void PINTEH( std::string& output ); 
		void PMULTUW( std::string& output );
		void PDIVUW( std::string& output );
		void PCPYUD( std::string& output ); 
		void POR( std::string& output );
		void PNOR( std::string& output );  
		void PEXCH( std::string& output );
		void PCPYH( std::string& output ); 
		void PEXCW( std::string& output );
//*********************END OF MMI3 OPCODES*******************
//************************COP0 OPCODES***********************
		void MFC0( std::string& output );
		void MTC0( std::string& output );
		void BC0F( std::string& output );
		void BC0T( std::string& output );
		void BC0FL( std::string& output );
		void BC0TL( std::string& output );
		void TLBR( std::string& output );
		void TLBWI( std::string& output );
		void TLBWR( std::string& output );
		void TLBP( std::string& output );
		void ERET( std::string& output );
		void DI( std::string& output );
		void EI( std::string& output );
//***********************END OF COP0*************************
//**************COP1 - Floating Point Unit (FPU)*************
		void MFC1( std::string& output );
		void CFC1( std::string& output );
		void MTC1( std::string& output );
		void CTC1( std::string& output );
		void BC1F( std::string& output );
		void BC1T( std::string& output );
		void BC1FL( std::string& output );
		void BC1TL( std::string& output );
		void ADD_S( std::string& output );  
		void SUB_S( std::string& output );  
		void MUL_S( std::string& output );  
		void DIV_S( std::string& output );  
		void SQRT_S( std::string& output ); 
		void ABS_S( std::string& output );  
		void MOV_S( std::string& output ); 
		void NEG_S( std::string& output ); 
		void RSQRT_S( std::string& output );  
		void ADDA_S( std::string& output ); 
		void SUBA_S( std::string& output ); 
		void MULA_S( std::string& output );
		void MADD_S( std::string& output ); 
		void MSUB_S( std::string& output ); 
		void MADDA_S( std::string& output ); 
		void MSUBA_S( std::string& output );
		void CVT_W( std::string& output );
		void MAX_S( std::string& output );
		void MIN_S( std::string& output );
		void C_F( std::string& output );
		void C_EQ( std::string& output );
		void C_LT( std::string& output );
		void C_LE( std::string& output );
		void CVT_S( std::string& output );
//**********************END OF COP1***********************
	}

	namespace Interpreter {
	namespace OpcodeImpl
	{
		using namespace ::R5900;

		void COP2();

		void Unknown();
		void MMI_Unknown();
		void COP0_Unknown();
		void COP1_Unknown();

// **********************Standard Opcodes**************************
		void J();
		void JAL();
		void BEQ();
		void BNE();
		void BLEZ();
		void BGTZ();
		void ADDI();
		void ADDIU();
		void SLTI();
		void SLTIU();
		void ANDI();
		void ORI();
		void XORI();
		void LUI();
		void BEQL();
		void BNEL();
		void BLEZL();
		void BGTZL();
		void DADDI();
		void DADDIU();
		void LDL();
		void LDR();
		void LB();
		void LH();
		void LWL();
		void LW();
		void LBU();
		void LHU();
		void LWR();
		void LWU();
		void SB();
		void SH();
		void SWL();
		void SW();
		void SDL();
		void SDR();
		void SWR();
		void CACHE();
		void LWC1();
		void PREF();
		void LQC2();
		void LD();
		void SQC2();
		void SD();
		void LQ();
		void SQ();
		void SWC1();
//*****************end of standard opcodes**********************
//********************SPECIAL OPCODES***************************
		void SLL();
		void SRL();
		void SRA();
		void SLLV();
		void SRLV();
		void SRAV();
		void JR();
		void JALR();
		void SYSCALL();
		void BREAK();
		void SYNC();
		void MFHI();
		void MTHI();
		void MFLO();
		void MTLO();
		void DSLLV();
		void DSRLV();
		void DSRAV();
		void MULT();
		void MULTU();
		void DIV();
		void DIVU();
		void ADD();
		void ADDU();
		void SUB();
		void SUBU();
		void AND();
		void OR();
		void XOR();
		void NOR();
		void SLT();
		void SLTU();
		void DADD();
		void DADDU();
		void DSUB();
		void DSUBU();
		void TGE();
		void TGEU();
		void TLT();
		void TLTU();
		void TEQ();
		void TNE();
		void DSLL();
		void DSRL();
		void DSRA();
		void DSLL32();
		void DSRL32();
		void DSRA32();
		void MOVZ();
		void MOVN();
		void MFSA();
		void MTSA();
//*******************END OF SPECIAL OPCODES************************
//***********************REGIMM OPCODES****************************
		void BLTZ();
		void BGEZ();
		void BLTZL();
		void BGEZL();
		void TGEI();
		void TGEIU();
		void TLTI();
		void TLTIU();
		void TEQI();
		void TNEI();
		void BLTZAL();
		void BGEZAL();
		void BLTZALL();
		void BGEZALL();
		void MTSAB();
		void MTSAH();
//*******************END OF REGIMM OPCODES***********************
//***********************MMI OPCODES*****************************
		void MADD();
		void MADDU();
		void MADD1();
		void MADDU1();
		void MFHI1();
		void MTHI1();
		void MFLO1();
		void MTLO1();
		void MULT1();
		void MULTU1();
		void DIV1();
		void DIVU1();

		namespace MMI {
		void PLZCW();
		void PMFHL();
		void PMTHL();
		void PSLLH();
		void PSRLH();
		void PSRAH();
		void PSLLW();
		void PSRLW();
		void PSRAW();
//********************END OF MMI OPCODES***********************
//***********************MMI0 OPCODES**************************
		void PADDW();  
		void PSUBW();  
		void PCGTW();  
		void PMAXW(); 
		void PADDH();  
		void PSUBH();  
		void PCGTH();  
		void PMAXH(); 
		void PADDB();  
		void PSUBB();  
		void PCGTB();
		void PADDSW(); 
		void PSUBSW(); 
		void PEXTLW();  
		void PPACW(); 
		void PADDSH();
		void PSUBSH();
		void PEXTLH(); 
		void PPACH();
		void PADDSB();
		void PSUBSB();
		void PEXTLB(); 
		void PPACB();
		void PEXT5(); 
		void PPAC5();
//******************END OF MMI0 OPCODES***********************
//*********************MMI1 OPCODES***************************
		void PABSW();
		void PCEQW();
		void PMINW(); 
		void PADSBH();
		void PABSH();
		void PCEQH();
		void PMINH();  
		void PCEQB(); 
		void PADDUW();
		void PSUBUW(); 
		void PEXTUW();  
		void PADDUH();
		void PSUBUH(); 
		void PEXTUH(); 
		void PADDUB();
		void PSUBUB();
		void PEXTUB();
		void QFSRV(); 
//*****************END OF MMI1 OPCODES***********************
//*********************MMI2 OPCODES**************************
		void PMADDW();
		void PSLLVW();
		void PSRLVW(); 
		void PMSUBW();
		void PMFHI();
		void PMFLO();
		void PINTH();
		void PMULTW();
		void PDIVW();
		void PCPYLD();
		void PMADDH();
		void PHMADH();
		void PAND();
		void PXOR(); 
		void PMSUBH();
		void PHMSBH();
		void PEXEH();
		void PREVH(); 
		void PMULTH();
		void PDIVBW();
		void PEXEW();
		void PROT3W();
//********************END OF MMI2 OPCODES********************
//***********************MMI3 OPCODES************************
		void PMADDUW();
		void PSRAVW(); 
		void PMTHI();
		void PMTLO();
		void PINTEH(); 
		void PMULTUW();
		void PDIVUW();
		void PCPYUD(); 
		void POR();
		void PNOR();  
		void PEXCH();
		void PCPYH(); 
		void PEXCW();
		}
//**********************END OF MMI3 OPCODES******************** 
//*************************COP0 OPCODES************************
		namespace COP0 {
		void MFC0();
		void MTC0();
		void BC0F();
		void BC0T();
		void BC0FL();
		void BC0TL();
		void TLBR();
		void TLBWI();
		void TLBWR();
		void TLBP();
		void ERET();
		void DI();
		void EI();
		}
//********************END OF COP0 OPCODES************************
//************COP1 OPCODES - Floating Point Unit*****************
		namespace COP1 {
		void MFC1();
		void CFC1();
		void MTC1();
		void CTC1();
		void BC1F();
		void BC1T();
		void BC1FL();
		void BC1TL();
		void ADD_S();  
		void SUB_S();  
		void MUL_S();  
		void DIV_S();  
		void SQRT_S(); 
		void ABS_S();  
		void MOV_S(); 
		void NEG_S(); 
		void RSQRT_S();  
		void ADDA_S(); 
		void SUBA_S(); 
		void MULA_S();
		void MADD_S(); 
		void MSUB_S(); 
		void MADDA_S(); 
		void MSUBA_S();
		void CVT_W();
		void MAX_S();
		void MIN_S();
		void C_F();
		void C_EQ();
		void C_LT();
		void C_LE();
		void CVT_S();
		}
	} }
}	// End namespace R5900

//****************************************************************************
//** COP2 - (VU0)                                                           **
//****************************************************************************
void QMFC2(); 
void CFC2(); 
void QMTC2();
void CTC2();  
void BC2F();
void BC2T();
void BC2FL();
void BC2TL();
//*****************SPECIAL 1 VUO TABLE*******************************
void VADDx();       
void VADDy();       
void VADDz();       
void VADDw();       
void VSUBx();        
void VSUBy();        
void VSUBz();
void VSUBw(); 
void VMADDx();
void VMADDy();
void VMADDz();
void VMADDw();
void VMSUBx();
void VMSUBy();
void VMSUBz();       
void VMSUBw(); 
void VMAXx();       
void VMAXy();       
void VMAXz();       
void VMAXw();       
void VMINIx();       
void VMINIy();       
void VMINIz();       
void VMINIw(); 
void VMULx();       
void VMULy();       
void VMULz();       
void VMULw();       
void VMULq();        
void VMAXi();        
void VMULi();        
void VMINIi();
void VADDq();
void VMADDq();      
void VADDi();       
void VMADDi();      
void VSUBq();        
void VMSUBq();       
void VSUBi();        
void VMSUBi(); 
void VADD();        
void VMADD();       
void VMUL();        
void VMAX();        
void VSUB();         
void VMSUB();       
void VOPMSUB();      
void VMINI();  
void VIADD();       
void VISUB();       
void VIADDI();       
void VIAND();        
void VIOR();        
void VCALLMS();     
void VCALLMSR();   
//***********************************END OF SPECIAL1 VU0 TABLE*****************************
//******************************SPECIAL2 VUO TABLE*****************************************
void VADDAx();      
void VADDAy();      
void VADDAz();      
void VADDAw();      
void VSUBAx();      
void VSUBAy();      
void VSUBAz();      
void VSUBAw();
void VMADDAx();     
void VMADDAy();     
void VMADDAz();     
void VMADDAw();     
void VMSUBAx();     
void VMSUBAy();     
void VMSUBAz();     
void VMSUBAw();
void VITOF0();      
void VITOF4();      
void VITOF12();     
void VITOF15();     
void VFTOI0();      
void VFTOI4();      
void VFTOI12();     
void VFTOI15();
void VMULAx();      
void VMULAy();      
void VMULAz();      
void VMULAw();      
void VMULAq();      
void VABS();        
void VMULAi();      
void VCLIPw();
void VADDAq();      
void VMADDAq();     
void VADDAi();      
void VMADDAi();     
void VSUBAq();      
void VMSUBAq();     
void VSUBAi();      
void VMSUBAi();
void VADDA();       
void VMADDA();      
void VMULA();       
void VSUBA();       
void VMSUBA();      
void VOPMULA();     
void VNOP();   
void VMOVE();       
void VMR32();       
void VLQI();        
void VSQI();        
void VLQD();        
void VSQD();   
void VDIV();        
void VSQRT();       
void VRSQRT();      
void VWAITQ();     
void VMTIR();       
void VMFIR();       
void VILWR();       
void VISWR();  
void VRNEXT();      
void VRGET();       
void VRINIT();      
void VRXOR();  
//*******************END OF SPECIAL2 *********************

#endif
