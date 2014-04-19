#pragma once
#include "PPCInstrTable.h"
#include "PPCDecoder.h"
#include "PPUOpcodes.h"

namespace PPU_instr
{
	//This field is used in rotate instructions to specify the first 1 bit of a 64-bit mask
	static DoubleCodeField<21, 25, 26, 26, 5> mb;

	//This field is used in rotate instructions to specify the last 1 bit of a 64-bit mask
	static DoubleCodeField<21, 25, 26, 26, 5> me;

	//This field is used to specify a shift amount
	static DoubleCodeField<16, 20, 30, 30, 5> sh;

	//This field is used to specify a special-purpose register for the mtspr and mfspr instructions
	static CodeField<11, 20> SPR;

	//
	static CodeField<6, 10> VS(FIELD_R_VPR);

	//
	static CodeField<6, 10> VD(FIELD_R_VPR);

	//
	static CodeField<11, 15> VA(FIELD_R_VPR);

	//
	static CodeField<16, 20> VB(FIELD_R_VPR);

	//
	static CodeField<21, 25> VC(FIELD_R_VPR);

	//
	static CodeField<11, 15> VUIMM;

	//
	static CodeFieldSigned<11, 15> VSIMM;

	//
	static CodeField<22, 25> VSH;

	//This field is used to specify a GPR to be used as a destination
	static CodeField<6, 10> RD(FIELD_R_GPR);

	//This field is used to specify a GPR to be used as a source
	static CodeField<6, 10> RS(FIELD_R_GPR);

	//This field is used to specify a GPR to be used as a source or destination
	static CodeField<11, 15> RA(FIELD_R_GPR);

	//This field is used to specify a GPR to be used as a source
	static CodeField<16, 20> RB(FIELD_R_GPR);

	//This field is used to specify the number of bytes to move in an immediate string load or store
	static CodeField<16, 20> NB;

	//This field is used to specify one of the CR fields, or one of the FPSCR fields, as a destination
	static CodeField<6, 8> CRFD(FIELD_R_CR);

	//This field is used to specify one of the CR fields, or one of the FPSCR fields, as a source
	static CodeField<11, 13> CRFS(FIELD_R_CR);

	//This field is used to specify a bit in the CR to be used as a source
	static CodeField<11, 15> CRBA(FIELD_R_CR);

	//This field is used to specify a bit in the CR to be used as a source
	static CodeField<16, 20> CRBB(FIELD_R_CR);

	//This field is used to specify a bit in the CR, or in the FPSCR, as the destination of the result of an instruction
	static CodeField<6, 10> CRBD(FIELD_R_CR);

	//This field is used to specify options for the branch conditional instructions
	static CodeField<6, 10> BO;

	//This field is used to specify a bit in the CR to be used as the condition of a branch conditional instruction
	static CodeField<11, 15> BI;

	//Immediate field specifying a 14-bit signed two's complement branch displacement that is concatenated on the
	//right with '00' and sign-extended to 64 bits.
	static CodeFieldSigned<16, 31> BD(FIELD_BRANCH);

	//
	static CodeField<19, 20> BH;

	//
	static CodeField<11, 13> BFA;
	
	//Field used by the optional data stream variant of the dcbt instruction.
	static CodeField<9, 10> TH;

	//This field is used to specify the conditions on which to trap
	static CodeField<6, 10> TO;

	//
	static CodeField<21, 25> MB;

	//
	static CodeField<26, 30> ME;

	//This field is used to specify a shift amount
	static CodeField<16, 20> SH;

	/*
	Absolute address bit.
	0		The immediate field represents an address relative to the current instruction address (CIA). (For more 
			information on the CIA, see Table 8-3.) The effective (logical) address of the branch is either the sum 
			of the LI field sign-extended to 64 bits and the address of the branch instruction or the sum of the BD 
			field sign-extended to 64 bits and the address of the branch instruction.
	1		The immediate field represents an absolute address. The effective address (EA) of the branch is the 
			LI field sign-extended to 64 bits or the BD field sign-extended to 64 bits.
	*/
	static CodeField<30> AA;

	static CodeFieldSignedOffset<6, 29, 2> LI(FIELD_BRANCH);

	//
	static CodeFieldSignedOffset<6, 29, 2> LL(FIELD_BRANCH);
	/*
	Link bit.
	0		Does not update the link register (LR).
	1		Updates the LR. If the instruction is a branch instruction, the address of the instruction following the 
			branch instruction is placed into the LR.
	*/
	static CodeField<31> LK;

	//This field is used for extended arithmetic to enable setting OV and SO in the XER
	static CodeField<21> OE;

	//Field used to specify whether an integer compare instruction is to compare 64-bit numbers or 32-bit numbers
	static CodeField<10> L_10;
	static CodeField<6> L_6;
	static CodeField<9, 10> L_9_10;
	static CodeField<11> L_11;
	//
	static CodeField<16, 19> I;

	//
	static CodeField<16, 27> DQ;

	//This field is used to specify an FPR as the destination
	static CodeField<6, 10> FRD;

	//This field is used to specify an FPR as a source
	static CodeField<6, 10> FRS;

	//
	static CodeField<7, 14> FM;

	//This field is used to specify an FPR as a source
	static CodeField<11, 15> FRA(FIELD_R_FPR);

	//This field is used to specify an FPR as a source
	static CodeField<16, 20> FRB(FIELD_R_FPR);

	//This field is used to specify an FPR as a source
	static CodeField<21, 25> FRC(FIELD_R_FPR);

	//This field mask is used to identify the CR fields that are to be updated by the mtcrf instruction.
	static CodeField<12, 19> CRM;

	//
	static CodeField<6, 31> SYS;

	//Immediate field specifying a 16-bit signed two's complement integer that is sign-extended to 64 bits
	static CodeFieldSigned<16, 31> D;

	//
	static CodeFieldSignedOffset<16, 29, 2> DS;

	//This immediate field is used to specify a 16-bit signed integer
	static CodeFieldSigned<16, 31> simm16;

	//This immediate field is used to specify a 16-bit unsigned integer
	static CodeField<16, 31> uimm16;
	
	/*
	Record bit.
	0		Does not update the condition register (CR).
	1		Updates the CR to reflect the result of the operation.
			For integer instructions, CR bits [0-2] are set to reflect the result as a signed quantity and CR bit [3] 
			receives a copy of the summary overflow bit, XER[SO]. The result as an unsigned quantity or a bit 
			string can be deduced from the EQ bit. For floating-point instructions, CR bits [4-7] are set to reflect 
			floating-point exception, floating-point enabled exception, floating-point invalid operation exception, 
			and floating-point overflow exception. 
	*/
	static CodeField<31> RC;

	//Primary opcode field
	static CodeField<0, 5> OPCD;

	static CodeField<26, 31> GD_04; //0x3f
	static CodeField<21, 31> GD_04_0;//0x7ff
	static CodeField<21, 30> GD_13; //0x3ff
	static CodeField<27, 29> GD_1e; //0x7
	static CodeField<21, 30> GD_1f; //0x3ff
	static CodeField<30, 31> GD_3a; //0x3
	static CodeField<26, 30> GD_3b; //0x1f
	static CodeField<30, 31> GD_3e; //0x3
	static CodeField<26, 30> GD_3f;//0x1f
	static CodeField<21, 30> GD_3f_0; //0x3ff

	static CodeField<9, 10> STRM;

	//static auto main_list = new_list(OPCD, instr_bind(&PPUOpcodes::UNK, GetCode, OPCD, OPCD));
	static InstrList<1 << CodeField<0, 5>::size, ::PPUOpcodes> main_list_obj(OPCD, instr_bind(&PPUOpcodes::UNK, GetCode, OPCD, OPCD));
	static auto main_list = &main_list_obj;
	static auto g04_list = new_list(main_list, PPU_opcodes::G_04, GD_04);
	static auto g04_0_list = new_list(g04_list, GD_04_0, instr_bind(&PPUOpcodes::UNK, GetCode, OPCD, GD_04_0));
	static auto g13_list = new_list(main_list, PPU_opcodes::G_13, GD_13, instr_bind(&PPUOpcodes::UNK, GetCode, OPCD, GD_13));
	static auto g1e_list = new_list(main_list, PPU_opcodes::G_1e, GD_1e, instr_bind(&PPUOpcodes::UNK, GetCode, OPCD, GD_1e));
	static auto g1f_list = new_list(main_list, PPU_opcodes::G_1f, GD_1f, instr_bind(&PPUOpcodes::UNK, GetCode, OPCD, GD_1f));
	static auto g3a_list = new_list(main_list, PPU_opcodes::G_3a, GD_3a, instr_bind(&PPUOpcodes::UNK, GetCode, OPCD, GD_3a));
	static auto g3b_list = new_list(main_list, PPU_opcodes::G_3b, GD_3b, instr_bind(&PPUOpcodes::UNK, GetCode, OPCD, GD_3b));
	static auto g3e_list = new_list(main_list, PPU_opcodes::G_3e, GD_3e, instr_bind(&PPUOpcodes::UNK, GetCode, OPCD, GD_3e));
	static auto g3f_list = new_list(main_list, PPU_opcodes::G_3f, GD_3f);
	static auto g3f_0_list = new_list(g3f_list, GD_3f_0, instr_bind(&PPUOpcodes::UNK, GetCode, OPCD, GD_3f_0));

	#define bind_instr(list, name, ...) \
		static const auto& name = make_instr<PPU_opcodes::name>(list, #name, &PPUOpcodes::name, ##__VA_ARGS__)

	bind_instr(main_list, TDI, TO, RA, simm16);
	bind_instr(main_list, TWI, TO, RA, simm16);
	bind_instr(main_list, MULLI, RD, RA, simm16);
	bind_instr(main_list, SUBFIC, RD, RA, simm16);
	bind_instr(main_list, CMPLI, CRFD, L_10, RA, uimm16);
	bind_instr(main_list, CMPI, CRFD, L_10, RA, simm16);
	bind_instr(main_list, ADDIC, RD, RA, simm16);
	bind_instr(main_list, ADDIC_, RD, RA, simm16);
	bind_instr(main_list, ADDI, RD, RA, simm16);
	bind_instr(main_list, ADDIS, RD, RA, simm16);
	bind_instr(main_list, BC, BO, BI, BD, AA, LK);
	bind_instr(main_list, SC, SYS);
	bind_instr(main_list, B, LI, AA, LK);
	bind_instr(main_list, RLWIMI, RA, RS, SH, MB, ME, RC);
	bind_instr(main_list, RLWINM, RA, RS, SH, MB, ME, RC);
	bind_instr(main_list, RLWNM, RA, RS, RB, MB, ME, RC);
	bind_instr(main_list, ORI, RA, RS, uimm16);
	bind_instr(main_list, ORIS, RA, RS, uimm16);
	bind_instr(main_list, XORI, RA, RS, uimm16);
	bind_instr(main_list, XORIS, RA, RS, uimm16);
	bind_instr(main_list, ANDI_, RA, RS, uimm16);
	bind_instr(main_list, ANDIS_, RA, RS, uimm16);
	bind_instr(main_list, LWZ, RD, RA, D);
	bind_instr(main_list, LWZU, RD, RA, D);
	bind_instr(main_list, LBZ, RD, RA, D);
	bind_instr(main_list, LBZU, RD, RA, D);
	bind_instr(main_list, STW, RS, RA, D);
	bind_instr(main_list, STWU, RS, RA, D);
	bind_instr(main_list, STB, RS, RA, D);
	bind_instr(main_list, STBU, RS, RA, D);
	bind_instr(main_list, LHZ, RD, RA, D);
	bind_instr(main_list, LHZU, RD, RA, D);
	bind_instr(main_list, LHA, RD, RA, D);
	bind_instr(main_list, LHAU, RD, RA, D);
	bind_instr(main_list, STH, RS, RA, D);
	bind_instr(main_list, STHU, RS, RA, D);
	bind_instr(main_list, LMW, RD, RA, D);
	bind_instr(main_list, STMW, RS, RA, D);
	bind_instr(main_list, LFS, FRD, RA, D);
	bind_instr(main_list, LFSU, FRD, RA, D);
	bind_instr(main_list, LFD, FRD, RA, D);
	bind_instr(main_list, LFDU, FRD, RA, D);
	bind_instr(main_list, STFS, FRS, RA, D);
	bind_instr(main_list, STFSU, FRS, RA, D);
	bind_instr(main_list, STFD, FRS, RA, D);
	bind_instr(main_list, STFDU, FRS, RA, D);

	bind_instr(g04_list, VMADDFP, VD, VA, VC, VB);
	bind_instr(g04_list, VMHADDSHS, VD, VA, VB, VC);
	bind_instr(g04_list, VMHRADDSHS, VD, VA, VB, VC);
	bind_instr(g04_list, VMLADDUHM, VD, VA, VB, VC);
	bind_instr(g04_list, VMSUMMBM, VD, VA, VB, VC);
	bind_instr(g04_list, VMSUMSHM, VD, VA, VB, VC);
	bind_instr(g04_list, VMSUMSHS, VD, VA, VB, VC);
	bind_instr(g04_list, VMSUMUBM, VD, VA, VB, VC);
	bind_instr(g04_list, VMSUMUHM, VD, VA, VB, VC);
	bind_instr(g04_list, VMSUMUHS, VD, VA, VB, VC);
	bind_instr(g04_list, VNMSUBFP, VD, VA, VC, VB);
	bind_instr(g04_list, VPERM, VD, VA, VB, VC);
	bind_instr(g04_list, VSEL, VD, VA, VB, VC);
	bind_instr(g04_list, VSLDOI, VD, VA, VB, VSH);

	bind_instr(g04_0_list, MFVSCR, VD);
	bind_instr(g04_0_list, MTVSCR, VB);
	bind_instr(g04_0_list, VADDCUW, VD, VA, VB);
	bind_instr(g04_0_list, VADDFP, VD, VA, VB);
	bind_instr(g04_0_list, VADDSBS, VD, VA, VB);
	bind_instr(g04_0_list, VADDSHS, VD, VA, VB);
	bind_instr(g04_0_list, VADDSWS, VD, VA, VB);
	bind_instr(g04_0_list, VADDUBM, VD, VA, VB);
	bind_instr(g04_0_list, VADDUBS, VD, VA, VB);
	bind_instr(g04_0_list, VADDUHM, VD, VA, VB);
	bind_instr(g04_0_list, VADDUHS, VD, VA, VB);
	bind_instr(g04_0_list, VADDUWM, VD, VA, VB);
	bind_instr(g04_0_list, VADDUWS, VD, VA, VB);
	bind_instr(g04_0_list, VAND, VD, VA, VB);
	bind_instr(g04_0_list, VANDC, VD, VA, VB);
	bind_instr(g04_0_list, VAVGSB, VD, VA, VB);
	bind_instr(g04_0_list, VAVGSH, VD, VA, VB);
	bind_instr(g04_0_list, VAVGSW, VD, VA, VB);
	bind_instr(g04_0_list, VAVGUB, VD, VA, VB);
	bind_instr(g04_0_list, VAVGUH, VD, VA, VB);
	bind_instr(g04_0_list, VAVGUW, VD, VA, VB);
	bind_instr(g04_0_list, VCFSX, VD, VUIMM, VB);
	bind_instr(g04_0_list, VCFUX, VD, VUIMM, VB);
	bind_instr(g04_0_list, VCMPBFP, VD, VA, VB);
	bind_instr(g04_0_list, VCMPBFP_, VD, VA, VB);
	bind_instr(g04_0_list, VCMPEQFP, VD, VA, VB);
	bind_instr(g04_0_list, VCMPEQFP_, VD, VA, VB);
	bind_instr(g04_0_list, VCMPEQUB, VD, VA, VB);
	bind_instr(g04_0_list, VCMPEQUB_, VD, VA, VB);
	bind_instr(g04_0_list, VCMPEQUH, VD, VA, VB);
	bind_instr(g04_0_list, VCMPEQUH_, VD, VA, VB);
	bind_instr(g04_0_list, VCMPEQUW, VD, VA, VB);
	bind_instr(g04_0_list, VCMPEQUW_, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGEFP, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGEFP_, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTFP, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTFP_, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTSB, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTSB_, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTSH, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTSH_, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTSW, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTSW_, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTUB, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTUB_, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTUH, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTUH_, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTUW, VD, VA, VB);
	bind_instr(g04_0_list, VCMPGTUW_, VD, VA, VB);
	bind_instr(g04_0_list, VCTSXS, VD, VUIMM, VB);
	bind_instr(g04_0_list, VCTUXS, VD, VUIMM, VB);
	bind_instr(g04_0_list, VEXPTEFP, VD, VB);
	bind_instr(g04_0_list, VLOGEFP, VD, VB);
	bind_instr(g04_0_list, VMAXFP, VD, VA, VB);
	bind_instr(g04_0_list, VMAXSB, VD, VA, VB);
	bind_instr(g04_0_list, VMAXSH, VD, VA, VB);
	bind_instr(g04_0_list, VMAXSW, VD, VA, VB);
	bind_instr(g04_0_list, VMAXUB, VD, VA, VB);
	bind_instr(g04_0_list, VMAXUH, VD, VA, VB);
	bind_instr(g04_0_list, VMAXUW, VD, VA, VB);
	bind_instr(g04_0_list, VMINFP, VD, VA, VB);
	bind_instr(g04_0_list, VMINSB, VD, VA, VB);
	bind_instr(g04_0_list, VMINSH, VD, VA, VB);
	bind_instr(g04_0_list, VMINSW, VD, VA, VB);
	bind_instr(g04_0_list, VMINUB, VD, VA, VB);
	bind_instr(g04_0_list, VMINUH, VD, VA, VB);
	bind_instr(g04_0_list, VMINUW, VD, VA, VB);
	bind_instr(g04_0_list, VMRGHB, VD, VA, VB);
	bind_instr(g04_0_list, VMRGHH, VD, VA, VB);
	bind_instr(g04_0_list, VMRGHW, VD, VA, VB);
	bind_instr(g04_0_list, VMRGLB, VD, VA, VB);
	bind_instr(g04_0_list, VMRGLH, VD, VA, VB);
	bind_instr(g04_0_list, VMRGLW, VD, VA, VB);
	bind_instr(g04_0_list, VMULESB, VD, VA, VB);
	bind_instr(g04_0_list, VMULESH, VD, VA, VB);
	bind_instr(g04_0_list, VMULEUB, VD, VA, VB);
	bind_instr(g04_0_list, VMULEUH, VD, VA, VB);
	bind_instr(g04_0_list, VMULOSB, VD, VA, VB);
	bind_instr(g04_0_list, VMULOSH, VD, VA, VB);
	bind_instr(g04_0_list, VMULOUB, VD, VA, VB);
	bind_instr(g04_0_list, VMULOUH, VD, VA, VB);
	bind_instr(g04_0_list, VNOR, VD, VA, VB);
	bind_instr(g04_0_list, VOR, VD, VA, VB);
	bind_instr(g04_0_list, VPKPX, VD, VA, VB);
	bind_instr(g04_0_list, VPKSHSS, VD, VA, VB);
	bind_instr(g04_0_list, VPKSHUS, VD, VA, VB);
	bind_instr(g04_0_list, VPKSWSS, VD, VA, VB);
	bind_instr(g04_0_list, VPKSWUS, VD, VA, VB);
	bind_instr(g04_0_list, VPKUHUM, VD, VA, VB);
	bind_instr(g04_0_list, VPKUHUS, VD, VA, VB);
	bind_instr(g04_0_list, VPKUWUM, VD, VA, VB);
	bind_instr(g04_0_list, VPKUWUS, VD, VA, VB);
	bind_instr(g04_0_list, VREFP, VD, VB);
	bind_instr(g04_0_list, VRFIM, VD, VB);
	bind_instr(g04_0_list, VRFIN, VD, VB);
	bind_instr(g04_0_list, VRFIP, VD, VB);
	bind_instr(g04_0_list, VRFIZ, VD, VB);
	bind_instr(g04_0_list, VRLB, VD, VA, VB);
	bind_instr(g04_0_list, VRLH, VD, VA, VB);
	bind_instr(g04_0_list, VRLW, VD, VA, VB);
	bind_instr(g04_0_list, VRSQRTEFP, VD, VB);
	bind_instr(g04_0_list, VSL, VD, VA, VB);
	bind_instr(g04_0_list, VSLB, VD, VA, VB);
	bind_instr(g04_0_list, VSLH, VD, VA, VB);
	bind_instr(g04_0_list, VSLO, VD, VA, VB);
	bind_instr(g04_0_list, VSLW, VD, VA, VB);
	bind_instr(g04_0_list, VSPLTB, VD, VUIMM, VB);
	bind_instr(g04_0_list, VSPLTH, VD, VUIMM, VB);
	bind_instr(g04_0_list, VSPLTISB, VD, VSIMM);
	bind_instr(g04_0_list, VSPLTISH, VD, VSIMM);
	bind_instr(g04_0_list, VSPLTISW, VD, VSIMM);
	bind_instr(g04_0_list, VSPLTW, VD, VUIMM, VB);
	bind_instr(g04_0_list, VSR, VD, VA, VB);
	bind_instr(g04_0_list, VSRAB, VD, VA, VB);
	bind_instr(g04_0_list, VSRAH, VD, VA, VB);
	bind_instr(g04_0_list, VSRAW, VD, VA, VB);
	bind_instr(g04_0_list, VSRB, VD, VA, VB);
	bind_instr(g04_0_list, VSRH, VD, VA, VB);
	bind_instr(g04_0_list, VSRO, VD, VA, VB);
	bind_instr(g04_0_list, VSRW, VD, VA, VB);
	bind_instr(g04_0_list, VSUBCUW, VD, VA, VB);
	bind_instr(g04_0_list, VSUBFP, VD, VA, VB);
	bind_instr(g04_0_list, VSUBSBS, VD, VA, VB);
	bind_instr(g04_0_list, VSUBSHS, VD, VA, VB);
	bind_instr(g04_0_list, VSUBSWS, VD, VA, VB);
	bind_instr(g04_0_list, VSUBUBM, VD, VA, VB);
	bind_instr(g04_0_list, VSUBUBS, VD, VA, VB);
	bind_instr(g04_0_list, VSUBUHM, VD, VA, VB);
	bind_instr(g04_0_list, VSUBUHS, VD, VA, VB);
	bind_instr(g04_0_list, VSUBUWM, VD, VA, VB);
	bind_instr(g04_0_list, VSUBUWS, VD, VA, VB);
	bind_instr(g04_0_list, VSUMSWS, VD, VA, VB);
	bind_instr(g04_0_list, VSUM2SWS, VD, VA, VB);
	bind_instr(g04_0_list, VSUM4SBS, VD, VA, VB);
	bind_instr(g04_0_list, VSUM4SHS, VD, VA, VB);
	bind_instr(g04_0_list, VSUM4UBS, VD, VA, VB);
	bind_instr(g04_0_list, VUPKHPX, VD, VB);
	bind_instr(g04_0_list, VUPKHSB, VD, VB);
	bind_instr(g04_0_list, VUPKHSH, VD, VB);
	bind_instr(g04_0_list, VUPKLPX, VD, VB);
	bind_instr(g04_0_list, VUPKLSB, VD, VB);
	bind_instr(g04_0_list, VUPKLSH, VD, VB);
	bind_instr(g04_0_list, VXOR, VD, VA, VB);

	bind_instr(g13_list, MCRF, CRFD, CRFS);
	bind_instr(g13_list, BCLR, BO, BI, BH, LK);
	bind_instr(g13_list, CRNOR, CRBD, CRBA, CRBB);
	bind_instr(g13_list, CRANDC, CRBD, CRBA, CRBB);
	bind_instr(g13_list, ISYNC);
	bind_instr(g13_list, CRXOR, CRBD, CRBA, CRBB);
	bind_instr(g13_list, CRNAND, CRBD, CRBA, CRBB);
	bind_instr(g13_list, CRAND, CRBD, CRBA, CRBB);
	bind_instr(g13_list, CREQV, CRBD, CRBA, CRBB);
	bind_instr(g13_list, CRORC, CRBD, CRBA, CRBB);
	bind_instr(g13_list, CROR, CRBD, CRBA, CRBB);
	bind_instr(g13_list, BCCTR, BO, BI, BH, LK);

	bind_instr(g1e_list, RLDICL, RA, RS, sh, mb, RC);
	bind_instr(g1e_list, RLDICR, RA, RS, sh, me, RC);
	bind_instr(g1e_list, RLDIC, RA, RS, sh, mb, RC);
	bind_instr(g1e_list, RLDIMI, RA, RS, sh, mb, RC);
	bind_instr(g1e_list, RLDC_LR, RA, RS, RB, mb, AA, RC);

	/*0x000*/bind_instr(g1f_list, CMP, CRFD, L_10, RA, RB);
	/*0x004*/bind_instr(g1f_list, TW, TO, RA, RB);
	/*0x006*/bind_instr(g1f_list, LVSL, VD, RA, RB);
	/*0x007*/bind_instr(g1f_list, LVEBX, VD, RA, RB);
	/*0x008*/bind_instr(g1f_list, SUBFC, RD, RA, RB, OE, RC);
	/*0x009*/bind_instr(g1f_list, MULHDU, RD, RA, RB, RC);
	/*0x00a*/bind_instr(g1f_list, ADDC, RD, RA, RB, OE, RC);
	/*0x00b*/bind_instr(g1f_list, MULHWU, RD, RA, RB, RC);
	/*0x013*/bind_instr(g1f_list, MFOCRF, L_11, RD, CRM);
	/*0x014*/bind_instr(g1f_list, LWARX, RD, RA, RB);
	/*0x015*/bind_instr(g1f_list, LDX, RD, RA, RB);
	/*0x017*/bind_instr(g1f_list, LWZX, RD, RA, RB);
	/*0x018*/bind_instr(g1f_list, SLW, RA, RS, RB, RC);
	/*0x01a*/bind_instr(g1f_list, CNTLZW, RA, RS, RC);
	/*0x01b*/bind_instr(g1f_list, SLD, RA, RS, RB, RC);
	/*0x01c*/bind_instr(g1f_list, AND, RA, RS, RB, RC);
	/*0x020*/bind_instr(g1f_list, CMPL, CRFD, L_10, RA, RB);
	/*0x026*/bind_instr(g1f_list, LVSR, VD, RA, RB);
	/*0x027*/bind_instr(g1f_list, LVEHX, VD, RA, RB);
	/*0x028*/bind_instr(g1f_list, SUBF, RD, RA, RB, OE, RC);
	/*0x035*/bind_instr(g1f_list, LDUX, RD, RA, RB);
	/*0x036*/bind_instr(g1f_list, DCBST, RA, RB);
	/*0x037*/bind_instr(g1f_list, LWZUX, RD, RA, RB);
	/*0x03a*/bind_instr(g1f_list, CNTLZD, RA, RS, RC);
	/*0x03c*/bind_instr(g1f_list, ANDC, RA, RS, RB, RC);
	/*0x047*/bind_instr(g1f_list, LVEWX, VD, RA, RB);
	/*0x049*/bind_instr(g1f_list, MULHD, RD, RA, RB, RC);
	/*0x04b*/bind_instr(g1f_list, MULHW, RD, RA, RB, RC);
	/*0x054*/bind_instr(g1f_list, LDARX, RD, RA, RB);
	/*0x056*/bind_instr(g1f_list, DCBF, RA, RB);
	/*0x057*/bind_instr(g1f_list, LBZX, RD, RA, RB);
	/*0x067*/bind_instr(g1f_list, LVX, VD, RA, RB);
	/*0x068*/bind_instr(g1f_list, NEG, RD, RA, OE, RC);
	/*0x077*/bind_instr(g1f_list, LBZUX, RD, RA, RB);
	/*0x07c*/bind_instr(g1f_list, NOR, RA, RS, RB, RC);
	/*0x087*/bind_instr(g1f_list, STVEBX, VS, RA, RB);
	/*0x088*/bind_instr(g1f_list, SUBFE, RD, RA, RB, OE, RC);
	/*0x08a*/bind_instr(g1f_list, ADDE, RD, RA, RB, OE, RC);
	/*0x090*/bind_instr(g1f_list, MTOCRF, L_11, CRM, RS);
	/*0x095*/bind_instr(g1f_list, STDX, RS, RA, RB);
	/*0x096*/bind_instr(g1f_list, STWCX_, RS, RA, RB);
	/*0x097*/bind_instr(g1f_list, STWX, RS, RA, RB);
	/*0x0a7*/bind_instr(g1f_list, STVEHX, VS, RA, RB);
	/*0x0b5*/bind_instr(g1f_list, STDUX, RS, RA, RB);
	/*0x0b7*/bind_instr(g1f_list, STWUX, RS, RA, RB);
	/*0x0c7*/bind_instr(g1f_list, STVEWX, VS, RA, RB);
	/*0x0c8*/bind_instr(g1f_list, SUBFZE, RD, RA, OE, RC);
	/*0x0ca*/bind_instr(g1f_list, ADDZE, RD, RA, OE, RC);
	/*0x0d6*/bind_instr(g1f_list, STDCX_, RS, RA, RB);
	/*0x0d7*/bind_instr(g1f_list, STBX, RS, RA, RB);
	/*0x0e7*/bind_instr(g1f_list, STVX, VS, RA, RB);
	/*0x0e8*/bind_instr(g1f_list, SUBFME, RD, RA, OE, RC);
	/*0x0e9*/bind_instr(g1f_list, MULLD, RD, RA, RB, OE, RC);
	/*0x0ea*/bind_instr(g1f_list, ADDME, RD, RA, OE, RC);
	/*0x0eb*/bind_instr(g1f_list, MULLW, RD, RA, RB, OE, RC);
	/*0x0f6*/bind_instr(g1f_list, DCBTST, TH, RA, RB);
	/*0x0f7*/bind_instr(g1f_list, STBUX, RS, RA, RB);
	/*0x10a*/bind_instr(g1f_list, ADD, RD, RA, RB, OE, RC);
	/*0x116*/bind_instr(g1f_list, DCBT, RA, RB, TH);
	/*0x117*/bind_instr(g1f_list, LHZX, RD, RA, RB);
	/*0x11c*/bind_instr(g1f_list, EQV, RA, RS, RB, RC);
	/*0x136*/bind_instr(g1f_list, ECIWX, RD, RA, RB);
	/*0x137*/bind_instr(g1f_list, LHZUX, RD, RA, RB);
	/*0x13c*/bind_instr(g1f_list, XOR, RA, RS, RB, RC);
	/*0x153*/bind_instr(g1f_list, MFSPR, RD, SPR);
	/*0x155*/bind_instr(g1f_list, LWAX, RD, RA, RB);
	/*0x156*/bind_instr(g1f_list, DST, RA, RB, STRM, L_6);
	/*0x157*/bind_instr(g1f_list, LHAX, RD, RA, RB);
	/*0x167*/bind_instr(g1f_list, LVXL, VD, RA, RB);
	/*0x173*/bind_instr(g1f_list, MFTB, RD, SPR);
	/*0x175*/bind_instr(g1f_list, LWAUX, RD, RA, RB);
	/*0x176*/bind_instr(g1f_list, DSTST, RA, RB, STRM, L_6);
	/*0x177*/bind_instr(g1f_list, LHAUX, RD, RA, RB);
	/*0x197*/bind_instr(g1f_list, STHX, RS, RA, RB);
	/*0x19c*/bind_instr(g1f_list, ORC, RA, RS, RB, RC);
	/*0x1b6*/bind_instr(g1f_list, ECOWX, RS, RA, RB);
	/*0x1b7*/bind_instr(g1f_list, STHUX, RS, RA, RB);
	/*0x1bc*/bind_instr(g1f_list, OR, RA, RS, RB, RC);
	/*0x1c9*/bind_instr(g1f_list, DIVDU, RD, RA, RB, OE, RC);
	/*0x1cb*/bind_instr(g1f_list, DIVWU, RD, RA, RB, OE, RC);
	/*0x1d3*/bind_instr(g1f_list, MTSPR, SPR, RS);
	/*0x1d6*///DCBI
	/*0x1dc*/bind_instr(g1f_list, NAND, RA, RS, RB, RC);
	/*0x1e7*/bind_instr(g1f_list, STVXL, VS, RA, RB);
	/*0x1e9*/bind_instr(g1f_list, DIVD, RD, RA, RB, OE, RC);
	/*0x1eb*/bind_instr(g1f_list, DIVW, RD, RA, RB, OE, RC);
	/*0x207*/bind_instr(g1f_list, LVLX, VD, RA, RB);
	/*0x214*/bind_instr(g1f_list, LDBRX, RD, RA, RB);
	/*0x216*/bind_instr(g1f_list, LWBRX, RD, RA, RB);
	/*0x217*/bind_instr(g1f_list, LFSX, FRD, RA, RB);
	/*0x218*/bind_instr(g1f_list, SRW, RA, RS, RB, RC);
	/*0x21b*/bind_instr(g1f_list, SRD, RA, RS, RB, RC);
	/*0x227*/bind_instr(g1f_list, LVRX, VD, RA, RB);
	/*0x237*/bind_instr(g1f_list, LFSUX, FRD, RA, RB);
	/*0x255*/bind_instr(g1f_list, LSWI, RD, RA, NB);
	/*0x256*/bind_instr(g1f_list, SYNC, L_9_10);
	/*0x257*/bind_instr(g1f_list, LFDX, FRD, RA, RB);
	/*0x277*/bind_instr(g1f_list, LFDUX, FRD, RA, RB);
	/*0x287*/bind_instr(g1f_list, STVLX, VS, RA, RB);
	/*0x296*/bind_instr(g1f_list, STWBRX, RS, RA, RB);
	/*0x297*/bind_instr(g1f_list, STFSX, FRS, RA, RB);
	/*0x2a7*/bind_instr(g1f_list, STVRX, VS, RA, RB);
	/*0x2d5*/bind_instr(g1f_list, STSWI, RD, RA, NB);
	/*0x2d7*/bind_instr(g1f_list, STFDX, FRS, RA, RB);
	/*0x307*/bind_instr(g1f_list, LVLXL, VD, RA, RB);
	/*0x316*/bind_instr(g1f_list, LHBRX, RD, RA, RB);
	/*0x318*/bind_instr(g1f_list, SRAW, RA, RS, RB, RC);
	/*0x31a*/bind_instr(g1f_list, SRAD, RA, RS, RB, RC);
	/*0x327*/bind_instr(g1f_list, LVRXL, VD, RA, RB);
	/*0x336*/bind_instr(g1f_list, DSS, STRM, L_6);
	/*0x338*/bind_instr(g1f_list, SRAWI, RA, RS, sh, RC);
	/*0x33a*/bind_instr(g1f_list, SRADI1, RA, RS, sh, RC);
	/*0x33b*/bind_instr(g1f_list, SRADI2, RA, RS, sh, RC);
	/*0x356*/bind_instr(g1f_list, EIEIO);
	/*0x387*/bind_instr(g1f_list, STVLXL, VS, RA, RB);
	/*0x396*/bind_instr(g1f_list, STHBRX, RS, RA, RB);
	/*0x39a*/bind_instr(g1f_list, EXTSH, RA, RS, RC);
	/*0x387*/bind_instr(g1f_list, STVRXL, VS, RA, RB);
	/*0x3ba*/bind_instr(g1f_list, EXTSB, RA, RS, RC);
	/*0x3d7*/bind_instr(g1f_list, STFIWX, FRS, RA, RB);
	/*0x3da*/bind_instr(g1f_list, EXTSW, RA, RS, RC);
	/*0x3d6*///ICBI
	/*0x3f6*/bind_instr(g1f_list, DCBZ, RA, RB);

	bind_instr(g3a_list, LD, RD, RA, DS);
	bind_instr(g3a_list, LDU, RD, RA, DS);
	bind_instr(g3a_list, LWA, RD, RA, DS);

	bind_instr(g3b_list, FDIVS, FRD, FRA, FRB, RC);
	bind_instr(g3b_list, FSUBS, FRD, FRA, FRB, RC);
	bind_instr(g3b_list, FADDS, FRD, FRA, FRB, RC);
	bind_instr(g3b_list, FSQRTS, FRD, FRB, RC);
	bind_instr(g3b_list, FRES, FRD, FRB, RC);
	bind_instr(g3b_list, FMULS, FRD, FRA, FRC, RC);
	bind_instr(g3b_list, FMADDS, FRD, FRA, FRC, FRB, RC);
	bind_instr(g3b_list, FMSUBS, FRD, FRA, FRC, FRB, RC);
	bind_instr(g3b_list, FNMSUBS, FRD, FRA, FRC, FRB, RC);
	bind_instr(g3b_list, FNMADDS, FRD, FRA, FRC, FRB, RC);
		
	bind_instr(g3e_list, STD, RS, RA, DS);
	bind_instr(g3e_list, STDU, RS, RA, DS);

	bind_instr(g3f_list, FSEL, FRD, FRA, FRC, FRB, RC);
	bind_instr(g3f_list, FMUL, FRD, FRA, FRC, RC);
	bind_instr(g3f_list, FMSUB, FRD, FRA, FRC, FRB, RC);
	bind_instr(g3f_list, FMADD, FRD, FRA, FRC, FRB, RC);
	bind_instr(g3f_list, FNMSUB, FRD, FRA, FRC, FRB, RC);
	bind_instr(g3f_list, FNMADD, FRD, FRA, FRC, FRB, RC);

	bind_instr(g3f_0_list, FDIV, FRD, FRA, FRB, RC);
	bind_instr(g3f_0_list, FSUB, FRD, FRA, FRB, RC);
	bind_instr(g3f_0_list, FADD, FRD, FRA, FRB, RC);
	bind_instr(g3f_0_list, FSQRT, FRD, FRB, RC);
	bind_instr(g3f_0_list, FRSQRTE, FRD, FRB, RC);
	bind_instr(g3f_0_list, FCMPU, CRFD, FRA, FRB);
	bind_instr(g3f_0_list, FRSP, FRD, FRB, RC);
	bind_instr(g3f_0_list, FCTIW, FRD, FRB, RC);
	bind_instr(g3f_0_list, FCTIWZ, FRD, FRB, RC);
	bind_instr(g3f_0_list, FCMPO, CRFD, FRA, FRB);
	bind_instr(g3f_0_list, FNEG, FRD, FRB, RC);
	bind_instr(g3f_0_list, FMR, FRD, FRB, RC);
	bind_instr(g3f_0_list, FNABS, FRD, FRB, RC);
	bind_instr(g3f_0_list, FABS, FRD, FRB, RC);
	bind_instr(g3f_0_list, FCFID, FRD, FRB, RC);
	bind_instr(g3f_0_list, FCTID, FRD, FRB, RC);
	bind_instr(g3f_0_list, FCTIDZ, FRD, FRB, RC);

	bind_instr(g3f_0_list, MTFSB1, CRBD, RC);
	bind_instr(g3f_0_list, MCRFS, CRFD, CRFS);
	bind_instr(g3f_0_list, MTFSB0, CRBD, RC);
	bind_instr(g3f_0_list, MTFSFI, CRFD, I, RC);
	bind_instr(g3f_0_list, MFFS, FRD, RC);
	bind_instr(g3f_0_list, MTFSF, FM, FRB, RC);

	#undef bind_instr
};