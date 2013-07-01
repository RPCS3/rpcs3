#pragma once

#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/Decoder.h"


#define START_OPCODES_GROUP(group, reg) \
	case(##group##): \
		temp=##reg##;\
		switch(temp)\
		{

#define START_OPCODES_SUB_GROUP(reg) \
	default:\
		temp=##reg##;\
		switch(temp)\
		{

#define END_OPCODES_GROUP(group) \
		default:\
			m_op.UNK(m_code, opcode, temp);\
		break;\
		}\
	break

#define END_OPCODES_SUB_GROUP() \
		default:\
			m_op.UNK(m_code, opcode, temp);\
		break;\
		}
#define END_OPCODES_ND_GROUP(group) \
	} \
	break;

#define ADD_OPCODE(name, ...) case(##name##):m_op.##name##(__VA_ARGS__); break

class PPU_Decoder : public Decoder
{
	u32 m_code;
	PPU_Opcodes& m_op;

	//This field is used in rotate instructions to specify the first 1 bit of a 64-bit mask
	OP_REG mb()			{ return GetField(21, 25) | (m_code & 0x20); }

	//This field is used in rotate instructions to specify the last 1 bit of a 64-bit mask
	OP_REG me()			{ return GetField(21, 25) | (m_code & 0x20); }

	//This field is used to specify a shift amount
	OP_REG sh()			{ return GetField(16, 20) | ((m_code & 0x2) << 4); }

	//This field is used to specify a special-purpose register for the mtspr and mfspr instructions
	OP_REG SPR()		{ return GetField(11, 20); }

	//
	OP_REG VS()			{ return GetField(6, 10); }

	//
	OP_REG VD()			{ return GetField(6, 10); }

	//
	OP_REG VA()			{ return GetField(11, 15); }

	//
	OP_REG VB()			{ return GetField(16, 20); }

	//
	OP_REG VC()			{ return GetField(21, 25); }

	//
	OP_uIMM VUIMM()		{ return GetField(11, 15); }

	//
	OP_sIMM VSIMM()		{  int i5 = GetField(11, 15);
							if(i5 & 0x10) return i5 - 0x20;
							return i5; }

	//
	OP_uIMM VSH()		{ return GetField(22, 25); }

	//This field is used to specify a GPR to be used as a destination
	OP_REG RD()			{ return GetField(6, 10); }

	//This field is used to specify a GPR to be used as a source
	OP_REG RS()			{ return GetField(6, 10); }

	//This field is used to specify a GPR to be used as a source or destination
	OP_REG RA()			{ return GetField(11, 15); }

	//This field is used to specify a GPR to be used as a source
	OP_REG RB()			{ return GetField(16, 20); }

	//This field is used to specify the number of bytes to move in an immediate string load or store
	OP_REG NB()			{ return GetField(16, 20); }

	//This field is used to specify one of the CR fields, or one of the FPSCR fields, as a destination
	OP_REG CRFD()		{ return GetField(6, 8); }

	//This field is used to specify one of the CR fields, or one of the FPSCR fields, as a source
	OP_REG CRFS()		{ return GetField(11, 13); }

	//This field is used to specify a bit in the CR to be used as a source
	OP_REG CRBA()		{ return GetField(11, 15); }

	//This field is used to specify a bit in the CR to be used as a source
	OP_REG CRBB()		{ return GetField(16, 20); }

	//This field is used to specify a bit in the CR, or in the FPSCR, as the destination of the result of an instruction
	OP_REG CRBD()		{ return GetField(6, 10); }
	
	//
	OP_REG BT()			{ return GetField(6, 10); }

	//
	OP_REG BA()			{ return GetField(11, 15); }

	//
	OP_REG BB()			{ return GetField(16, 20); }

	//
	OP_REG BF()			{ return GetField(6, 10); }

	//This field is used to specify options for the branch conditional instructions
	OP_REG BO()			{ return GetField(6, 10); }

	//This field is used to specify a bit in the CR to be used as the condition of a branch conditional instruction
	OP_REG BI()			{ return GetField(11, 15); }

	//Immediate field specifying a 14-bit signed two's complement branch displacement that is concatenated on the
	//right with ‘00’ and sign-extended to 64 bits.
	OP_sIMM BD()		{ return (s32)(s16)GetField(16, 31); }

	//
	OP_REG BH()			{ return GetField(19, 20); }

	//
	OP_REG BFA()		{ return GetField(11, 13); }
	
	//Field used by the optional data stream variant of the dcbt instruction.
	OP_uIMM TH()		{ return GetField(9, 10); }

	//This field is used to specify the conditions on which to trap
	OP_uIMM TO()		{ return GetField(6, 10); }

	//
	OP_REG MB()			{ return GetField(21, 25); }

	//
	OP_REG ME()			{ return GetField(26, 30); }

	//This field is used to specify a shift amount
	OP_REG SH()			{ return GetField(16, 20); }

	/*
	Absolute address bit.
	0		The immediate field represents an address relative to the current instruction address (CIA). (For more 
			information on the CIA, see Table 8-3.) The effective (logical) address of the branch is either the sum 
			of the LI field sign-extended to 64 bits and the address of the branch instruction or the sum of the BD 
			field sign-extended to 64 bits and the address of the branch instruction.
	1		The immediate field represents an absolute address. The effective address (EA) of the branch is the 
			LI field sign-extended to 64 bits or the BD field sign-extended to 64 bits.
	*/
	OP_REG AA()			{ return GetField(30); }

	//
	OP_sIMM LL()
	{
		OP_sIMM ll = m_code & 0x03fffffc;
		if (ll & 0x02000000) return ll - 0x04000000;
		return ll;
	}
	/*
	Link bit.
	0		Does not update the link register (LR).
	1		Updates the LR. If the instruction is a branch instruction, the address of the instruction following the 
			branch instruction is placed into the LR.
	*/
	OP_REG LK()			{ return GetField(31); }

	//This field is used for extended arithmetic to enable setting OV and SO in the XER
	OP_REG OE()			{ return GetField(21); }

	//Field used to specify whether an integer compare instruction is to compare 64-bit numbers or 32-bit numbers
	OP_REG L_10()			{ return GetField(10); }
	OP_REG L_6()			{ return GetField(6); }
	OP_REG L_9_10()			{ return GetField(9, 10); }
	OP_REG L_11()			{ return GetField(11); }
	//
	OP_REG I()			{ return GetField(16, 19); }

	//
	OP_REG DQ()			{ return GetField(16, 27); }

	//This field is used to specify an FPR as the destination
	OP_REG FRD()		{ return GetField(6, 10); }

	//This field is used to specify an FPR as a source
	OP_REG FRS()		{ return GetField(6, 10); }

	//
	OP_REG FLM()		{ return GetField(7, 14); }

	//This field is used to specify an FPR as a source
	OP_REG FRA()		{ return GetField(11, 15); }

	//This field is used to specify an FPR as a source
	OP_REG FRB()		{ return GetField(16, 20); }

	//This field is used to specify an FPR as a source
	OP_REG FRC()		{ return GetField(21, 25); }

	//This field mask is used to identify the CR fields that are to be updated by the mtcrf instruction.
	OP_REG CRM()		{ return GetField(12, 19); }

	//
	OP_sIMM SYS()		{ return GetField(6, 31); }

	//Immediate field specifying a 16-bit signed two's complement integer that is sign-extended to 64 bits
	OP_sIMM D()			{ return (s32)(s16)GetField(16, 31); }

	//
	OP_sIMM DS()
	{
		OP_sIMM d = D();
		if(d < 0) return d - 1;
		return d;
	}

	//This immediate field is used to specify a 16-bit signed integer
	OP_sIMM simm16()	{ return (s32)(s16)m_code; }

	//This immediate field is used to specify a 16-bit unsigned integer
	OP_uIMM uimm16()	{ return (u32)(u16)m_code; }
	
	/*
	Record bit.
	0		Does not update the condition register (CR).
	1		Updates the CR to reflect the result of the operation.
			For integer instructions, CR bits [0–2] are set to reflect the result as a signed quantity and CR bit [3] 
			receives a copy of the summary overflow bit, XER[SO]. The result as an unsigned quantity or a bit 
			string can be deduced from the EQ bit. For floating-point instructions, CR bits [4–7] are set to reflect 
			floating-point exception, floating-point enabled exception, floating-point invalid operation exception, 
			and floating-point overflow exception. 
	*/
	bool RC()	{ return m_code & 0x1; }

	//Primary opcode field
	OP_uIMM OPCD() { return GetField(0, 5); }

	OP_uIMM GD_04()		{ return GetField(26, 31); } //0x3f
	OP_uIMM GD_04_0()	{ return GetField(21, 31); } //0x7ff
	OP_uIMM GD_13()		{ return GetField(21, 30); } //0x3ff
	OP_uIMM GD_1e()		{ return GetField(28, 29); } //0x3
	OP_uIMM GD_1f()		{ return GetField(21, 30); } //0x3ff
	OP_uIMM GD_3a()		{ return GetField(30, 31); } //0x3
	OP_uIMM GD_3b()		{ return GetField(26, 30); } //0x1f
	OP_uIMM GD_3e()		{ return GetField(30, 31); } //0x3
	OP_uIMM GD_3f()		{ return GetField(26, 30); } //0x1f
	OP_uIMM GD_3f_0()	{ return GetField(21, 30); } //0x3ff

	OP_uIMM STRM() { return GetField(9, 10); }

	OP_uIMM GetCode() { return m_code; }
	
	__forceinline u32 GetField(const u32 p)
	{
		return (m_code >> (31 - p)) & 0x1;
	}
	
	__forceinline u32 GetField(const u32 from, const u32 to)
	{
		return (m_code >> (31 - to)) & ((1 << ((to - from) + 1)) - 1);
	}
	
public:
	template<typename TD, typename TO>
	instr_caller* instr_bind(TD* parent, void (TO::*func)())
	{
		return new instr_binder_0<TO>(parent, func);
	}

	template<typename TO>
	instr_caller* instr_bind(void (TO::*func)())
	{
		return instr_bind(&m_op, func);
	}

	template<typename TO, typename TD, typename T1>
	instr_caller* instr_bind(void (TO::*func)(T1), T1 (TD::*arg_func_1)())
	{
		return new instr_binder_1<TO, TD, T1>(&m_op, this, func, arg_func_1);
	}

	template<typename TO, typename TD, typename T1, typename T2>
	instr_caller* instr_bind(void (TO::*func)(T1, T2), T1 (TD::*arg_func_1)(), T2 (TD::*arg_func_2)())
	{
		return new instr_binder_2<TO, TD, T1, T2>(&m_op, this, func, arg_func_1, arg_func_2);
	}

	template<typename TO, typename TD, typename T1, typename T2, typename T3>
	instr_caller* instr_bind(void (TO::*func)(T1, T2, T3), T1 (TD::*arg_func_1)(), T2 (TD::*arg_func_2)(), T3 (TD::*arg_func_3)())
	{
		return new instr_binder_3<TO, TD, T1, T2, T3>(&m_op, this, func, arg_func_1, arg_func_2, arg_func_3);
	}

	template<typename TO, typename TD, typename T1, typename T2, typename T3, typename T4>
	instr_caller* instr_bind(void (TO::*func)(T1, T2, T3, T4), T1 (TD::*arg_func_1)(), T2 (TD::*arg_func_2)(), T3 (TD::*arg_func_3)(), T4 (TD::*arg_func_4)())
	{
		return new instr_binder_4<TO, TD, T1, T2, T3, T4>(&m_op, this, func, arg_func_1, arg_func_2, arg_func_3, arg_func_4);
	}

	template<typename TO, typename TD, typename T1, typename T2, typename T3, typename T4, typename T5>
	instr_caller* instr_bind(void (TO::*func)(T1, T2, T3, T4, T5), T1 (TD::*arg_func_1)(), T2 (TD::*arg_func_2)(), T3 (TD::*arg_func_3)(), T4 (TD::*arg_func_4)(), T5 (TD::*arg_func_5)())
	{
		return new instr_binder_5<TO, TD, T1, T2, T3, T4, T5>(&m_op, this, func, arg_func_1, arg_func_2, arg_func_3, arg_func_4, arg_func_5);
	}

	template<typename TO, typename TD, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
	instr_caller* instr_bind(void (TO::*func)(T1, T2, T3, T4, T5, T6), T1 (TD::*arg_func_1)(), T2 (TD::*arg_func_2)(), T3 (TD::*arg_func_3)(), T4 (TD::*arg_func_4)(), T5 (TD::*arg_func_5)(), T6 (TD::*arg_func_6)())
	{
		return new instr_binder_6<TO, TD, T1, T2, T3, T4, T5, T6>(&m_op, this, func, arg_func_1, arg_func_2, arg_func_3, arg_func_4, arg_func_5, arg_func_6);
	}

	void UNK_MAIN()
	{
		if(!m_code)
		{
			m_op.NULL_OP();
		}
		else
		{
			u32 opcd = OPCD();
			m_op.UNK(m_code, opcd, opcd);
		}
	}

	PPU_Decoder(PPU_Opcodes& op) : m_op(op)
	{
#define bind_instr(list, instr, ...) list->set_instr(instr, instr_bind(&PPU_Opcodes::##instr, ##__VA_ARGS__))

		using namespace PPU_opcodes;
		auto main_list = new_list<0x40>(this, &PPU_Decoder::OPCD, instr_bind(this, &PPU_Decoder::UNK_MAIN));
		auto g04_0_list = new_list<0x800>(this, &PPU_Decoder::GD_04_0, instr_bind(&PPU_Opcodes::UNK, &PPU_Decoder::GetCode, &PPU_Decoder::OPCD, &PPU_Decoder::GD_04_0));
		auto g04_list = new_list<0x40>(this, &PPU_Decoder::GD_04, g04_0_list);
		auto g13_list = new_list<0x400>(this, &PPU_Decoder::GD_13, instr_bind(&PPU_Opcodes::UNK, &PPU_Decoder::GetCode, &PPU_Decoder::OPCD, &PPU_Decoder::GD_13));
		auto g1e_list = new_list<0x4>(this, &PPU_Decoder::GD_1e, instr_bind(&PPU_Opcodes::UNK, &PPU_Decoder::GetCode, &PPU_Decoder::OPCD, &PPU_Decoder::GD_1e));
		auto g1f_list = new_list<0x400>(this, &PPU_Decoder::GD_1f, instr_bind(&PPU_Opcodes::UNK, &PPU_Decoder::GetCode, &PPU_Decoder::OPCD, &PPU_Decoder::GD_1f));
		auto g3a_list = new_list<0x4>(this, &PPU_Decoder::GD_3a, instr_bind(&PPU_Opcodes::UNK, &PPU_Decoder::GetCode, &PPU_Decoder::OPCD, &PPU_Decoder::GD_3a));
		auto g3b_list = new_list<0x20>(this, &PPU_Decoder::GD_3b, instr_bind(&PPU_Opcodes::UNK, &PPU_Decoder::GetCode, &PPU_Decoder::OPCD, &PPU_Decoder::GD_3b));
		auto g3e_list = new_list<0x4>(this, &PPU_Decoder::GD_3e, instr_bind(&PPU_Opcodes::UNK, &PPU_Decoder::GetCode, &PPU_Decoder::OPCD, &PPU_Decoder::GD_3e));
		auto g3f_0_list = new_list<0x400>(this, &PPU_Decoder::GD_3f_0, instr_bind(&PPU_Opcodes::UNK, &PPU_Decoder::GetCode, &PPU_Decoder::OPCD, &PPU_Decoder::GD_3f_0));
		auto g3f_list = new_list<0x20>(this, &PPU_Decoder::GD_3f, g3f_0_list);

		bind_instr(main_list, TDI, &PPU_Decoder::TO, &PPU_Decoder::RA, &PPU_Decoder::simm16);
		bind_instr(main_list, TWI, &PPU_Decoder::TO, &PPU_Decoder::RA, &PPU_Decoder::simm16);
		main_list->set_instr(G_04, g04_list);
		bind_instr(main_list, MULLI, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::simm16);
		bind_instr(main_list, SUBFIC, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::simm16);
		bind_instr(main_list, CMPLI, &PPU_Decoder::CRFD, &PPU_Decoder::L_10, &PPU_Decoder::RA, &PPU_Decoder::uimm16);
		bind_instr(main_list, CMPI, &PPU_Decoder::CRFD, &PPU_Decoder::L_10, &PPU_Decoder::RA, &PPU_Decoder::simm16);
		bind_instr(main_list, ADDIC, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::simm16);
		bind_instr(main_list, ADDIC_, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::simm16);
		bind_instr(main_list, ADDI, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::simm16);
		bind_instr(main_list, ADDIS, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::simm16);
		bind_instr(main_list, BC, &PPU_Decoder::BO, &PPU_Decoder::BI, &PPU_Decoder::BD, &PPU_Decoder::AA, &PPU_Decoder::LK);
		bind_instr(main_list, SC, &PPU_Decoder::SYS);
		bind_instr(main_list, B, &PPU_Decoder::LL, &PPU_Decoder::AA, &PPU_Decoder::LK);
		main_list->set_instr(G_13, g13_list);
		bind_instr(main_list, RLWIMI, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::SH, &PPU_Decoder::MB, &PPU_Decoder::ME, &PPU_Decoder::RC);
		bind_instr(main_list, RLWINM, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::SH, &PPU_Decoder::MB, &PPU_Decoder::ME, &PPU_Decoder::RC);
		bind_instr(main_list, RLWNM, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::MB, &PPU_Decoder::ME, &PPU_Decoder::RC);
		bind_instr(main_list, ORI, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::uimm16);
		bind_instr(main_list, ORIS, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::uimm16);
		bind_instr(main_list, XORI, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::uimm16);
		bind_instr(main_list, XORIS, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::uimm16);
		bind_instr(main_list, ANDI_, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::uimm16);
		bind_instr(main_list, ANDIS_, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::uimm16);
		main_list->set_instr(G_1e, g1e_list);
		main_list->set_instr(G_1f, g1f_list);
		bind_instr(main_list, LWZ, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, LWZU, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, LBZ, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, LBZU, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, STW, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, STWU, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, STB, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, STBU, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, LHZ, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, LHZU, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, STH, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, STHU, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, LMW, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, STMW, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, LFS, &PPU_Decoder::FRD, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, LFSU, &PPU_Decoder::FRD, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, LFD, &PPU_Decoder::FRD, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, LFDU, &PPU_Decoder::FRD, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, STFS, &PPU_Decoder::FRS, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, STFSU, &PPU_Decoder::FRS, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, STFD, &PPU_Decoder::FRS, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(main_list, STFDU, &PPU_Decoder::FRS, &PPU_Decoder::RA, &PPU_Decoder::D);
		main_list->set_instr(G_3a, g3a_list);
		main_list->set_instr(G_3b, g3b_list);
		main_list->set_instr(G_3e, g3e_list);
		main_list->set_instr(G_3f, g3f_list);

		bind_instr(g04_list, VMADDFP, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VC);
		bind_instr(g04_list, VMHADDSHS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VC);
		bind_instr(g04_list, VMHRADDSHS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VC);
		bind_instr(g04_list, VMLADDUHM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VC);
		bind_instr(g04_list, VMSUMMBM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VC);
		bind_instr(g04_list, VMSUMSHM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VC);
		bind_instr(g04_list, VMSUMSHS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VC);
		bind_instr(g04_list, VMSUMUBM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VC);
		bind_instr(g04_list, VMSUMUHM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VC);
		bind_instr(g04_list, VMSUMUHS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VC);
		bind_instr(g04_list, VNMSUBFP, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VC);
		bind_instr(g04_list, VPERM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VC);
		bind_instr(g04_list, VSEL, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VC);
		bind_instr(g04_list, VSLDOI, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB, &PPU_Decoder::VSH);

		bind_instr(g04_0_list, MFVSCR, &PPU_Decoder::VD);
		bind_instr(g04_0_list, MTVSCR, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VADDCUW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VADDFP, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VADDSBS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VADDSHS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VADDSWS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VADDUBM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VADDUBS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VADDUHM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VADDUHS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VADDUWM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VADDUWS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VAND, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VANDC, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VAVGSB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VAVGSH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VAVGSW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VAVGUB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VAVGUH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VAVGUW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCFSX, &PPU_Decoder::VD, &PPU_Decoder::VUIMM, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCFUX, &PPU_Decoder::VD, &PPU_Decoder::VUIMM, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPBFP, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPBFP_, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPEQFP, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPEQFP_, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPEQUB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPEQUB_, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPEQUH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPEQUH_, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPEQUW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPEQUW_, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGEFP, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGEFP_, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTFP, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTFP_, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTSB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTSB_, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTSH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTSH_, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTSW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTSW_, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTUB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTUB_, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTUH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTUH_, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTUW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCMPGTUW_, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCTSXS, &PPU_Decoder::VD, &PPU_Decoder::VUIMM, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VCTUXS, &PPU_Decoder::VD, &PPU_Decoder::VUIMM, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VEXPTEFP, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VLOGEFP, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMAXFP, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMAXSB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMAXSH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMAXSW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMAXUB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMAXUH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMAXUW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMINFP, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMINSB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMINSH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMINSW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMINUB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMINUH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMINUW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMRGHB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMRGHH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMRGHW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMRGLB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMRGLH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMRGLW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMULESB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMULESH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMULEUB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMULEUH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMULOSB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMULOSH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMULOUB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VMULOUH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VNOR, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VOR, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VPKPX, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VPKSHSS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VPKSHUS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VPKSWSS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VPKSWUS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VPKUHUM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VPKUHUS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VPKUWUM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VPKUWUS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VREFP, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VRFIM, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VRFIN, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VRFIP, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VRFIZ, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VRLB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VRLH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VRLW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VRSQRTEFP, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSL, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSLB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSLH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSLO, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSLW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSPLTB, &PPU_Decoder::VD, &PPU_Decoder::VUIMM, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSPLTH, &PPU_Decoder::VD, &PPU_Decoder::VUIMM, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSPLTISB, &PPU_Decoder::VD, &PPU_Decoder::VSIMM);
		bind_instr(g04_0_list, VSPLTISH, &PPU_Decoder::VD, &PPU_Decoder::VSIMM);
		bind_instr(g04_0_list, VSPLTISW, &PPU_Decoder::VD, &PPU_Decoder::VSIMM);
		bind_instr(g04_0_list, VSPLTW, &PPU_Decoder::VD, &PPU_Decoder::VUIMM, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSR, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSRAB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSRAH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSRAW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSRB, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSRH, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSRO, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSRW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUBCUW, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUBFP, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUBSBS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUBSHS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUBSWS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUBUBM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUBUBS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUBUHM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUBUHS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUBUWM, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUBUWS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUMSWS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUM2SWS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUM4SBS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUM4SHS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VSUM4UBS, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VUPKHPX, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VUPKHSB, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VUPKHSH, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VUPKLPX, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VUPKLSB, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VUPKLSH, &PPU_Decoder::VD, &PPU_Decoder::VB);
		bind_instr(g04_0_list, VXOR, &PPU_Decoder::VD, &PPU_Decoder::VA, &PPU_Decoder::VB);

		bind_instr(g13_list, MCRF, &PPU_Decoder::CRFD, &PPU_Decoder::CRFS);
		bind_instr(g13_list, BCLR, &PPU_Decoder::BO, &PPU_Decoder::BI, &PPU_Decoder::BH, &PPU_Decoder::LK);
		bind_instr(g13_list, CRNOR, &PPU_Decoder::CRBD, &PPU_Decoder::CRBA, &PPU_Decoder::CRBB);
		bind_instr(g13_list, CRANDC, &PPU_Decoder::CRBD, &PPU_Decoder::CRBA, &PPU_Decoder::CRBB);
		bind_instr(g13_list, ISYNC);
		bind_instr(g13_list, CRXOR, &PPU_Decoder::CRBD, &PPU_Decoder::CRBA, &PPU_Decoder::CRBB);
		bind_instr(g13_list, CRNAND, &PPU_Decoder::CRBD, &PPU_Decoder::CRBA, &PPU_Decoder::CRBB);
		bind_instr(g13_list, CRAND, &PPU_Decoder::CRBD, &PPU_Decoder::CRBA, &PPU_Decoder::CRBB);
		bind_instr(g13_list, CREQV, &PPU_Decoder::CRBD, &PPU_Decoder::CRBA, &PPU_Decoder::CRBB);
		bind_instr(g13_list, CRORC, &PPU_Decoder::CRBD, &PPU_Decoder::CRBA, &PPU_Decoder::CRBB);
		bind_instr(g13_list, CROR, &PPU_Decoder::CRBD, &PPU_Decoder::CRBA, &PPU_Decoder::CRBB);
		bind_instr(g13_list, BCCTR, &PPU_Decoder::BO, &PPU_Decoder::BI, &PPU_Decoder::BH, &PPU_Decoder::LK);

		bind_instr(g1e_list, RLDICL, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::sh, &PPU_Decoder::mb, &PPU_Decoder::RC);
		bind_instr(g1e_list, RLDICR, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::sh, &PPU_Decoder::me, &PPU_Decoder::RC);
		bind_instr(g1e_list, RLDIC, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::sh, &PPU_Decoder::mb, &PPU_Decoder::RC);
		bind_instr(g1e_list, RLDIMI, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::sh, &PPU_Decoder::mb, &PPU_Decoder::RC);

		/*0x000*/bind_instr(g1f_list, CMP, &PPU_Decoder::CRFD, &PPU_Decoder::L_10, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x004*/bind_instr(g1f_list, TW, &PPU_Decoder::TO, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x006*/bind_instr(g1f_list, LVSL, &PPU_Decoder::VD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x007*/bind_instr(g1f_list, LVEBX, &PPU_Decoder::VD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x008*/bind_instr(g1f_list, SUBFC, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x009*/bind_instr(g1f_list, MULHDU, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x00a*/bind_instr(g1f_list, ADDC, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x00b*/bind_instr(g1f_list, MULHWU, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x013*/bind_instr(g1f_list, MFOCRF, &PPU_Decoder::L_11, &PPU_Decoder::RD, &PPU_Decoder::CRM);
		/*0x014*/bind_instr(g1f_list, LWARX, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x015*/bind_instr(g1f_list, LDX, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB);
		/*0x017*/bind_instr(g1f_list, LWZX, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x018*/bind_instr(g1f_list, SLW, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x01a*/bind_instr(g1f_list, CNTLZW, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RC);
		/*0x01b*/bind_instr(g1f_list, SLD, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x01c*/bind_instr(g1f_list, AND, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x020*/bind_instr(g1f_list, CMPL, &PPU_Decoder::CRFD, &PPU_Decoder::L_10, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x026*/bind_instr(g1f_list, LVSR, &PPU_Decoder::VD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x027*/bind_instr(g1f_list, LVEHX, &PPU_Decoder::VD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x028*/bind_instr(g1f_list, SUBF, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x036*/bind_instr(g1f_list, DCBST, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x03a*/bind_instr(g1f_list, CNTLZD, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RC);
		/*0x03c*/bind_instr(g1f_list, ANDC, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x047*/bind_instr(g1f_list, LVEWX, &PPU_Decoder::VD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x049*/bind_instr(g1f_list, MULHD, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x04b*/bind_instr(g1f_list, MULHW, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x054*/bind_instr(g1f_list, LDARX, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x056*/bind_instr(g1f_list, DCBF, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x057*/bind_instr(g1f_list, LBZX, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x067*/bind_instr(g1f_list, LVX, &PPU_Decoder::VD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x068*/bind_instr(g1f_list, NEG, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x077*/bind_instr(g1f_list, LBZUX, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x07c*/bind_instr(g1f_list, NOR, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x087*/bind_instr(g1f_list, STVEBX, &PPU_Decoder::VS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x088*/bind_instr(g1f_list, SUBFE, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x08a*/bind_instr(g1f_list, ADDE, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x090*/bind_instr(g1f_list, MTOCRF, &PPU_Decoder::CRM, &PPU_Decoder::RS);
		/*0x095*/bind_instr(g1f_list, STDX, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x096*/bind_instr(g1f_list, STWCX_, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x097*/bind_instr(g1f_list, STWX, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x0a7*/bind_instr(g1f_list, STVEHX, &PPU_Decoder::VS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x0b5*/bind_instr(g1f_list, STDUX, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x0c7*/bind_instr(g1f_list, STVEWX, &PPU_Decoder::VS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x0ca*/bind_instr(g1f_list, ADDZE, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x0d6*/bind_instr(g1f_list, STDCX_, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x0d7*/bind_instr(g1f_list, STBX, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x0e7*/bind_instr(g1f_list, STVX, &PPU_Decoder::VS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x0e9*/bind_instr(g1f_list, MULLD, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x0ea*/bind_instr(g1f_list, ADDME, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x0eb*/bind_instr(g1f_list, MULLW, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x0f6*/bind_instr(g1f_list, DCBTST, &PPU_Decoder::TH, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x10a*/bind_instr(g1f_list, ADD, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x116*/bind_instr(g1f_list, DCBT, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::TH);
		/*0x117*/bind_instr(g1f_list, LHZX, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x11c*/bind_instr(g1f_list, EQV, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x136*/bind_instr(g1f_list, ECIWX, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x137*/bind_instr(g1f_list, LHZUX, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x13c*/bind_instr(g1f_list, XOR, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x153*/bind_instr(g1f_list, MFSPR, &PPU_Decoder::RD, &PPU_Decoder::SPR);
		/*0x156*/bind_instr(g1f_list, DST, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::STRM, &PPU_Decoder::L_6);
		/*0x157*/bind_instr(g1f_list, LHAX, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x167*/bind_instr(g1f_list, LVXL, &PPU_Decoder::VD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x168*/bind_instr(g1f_list, ABS, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x173*/bind_instr(g1f_list, MFTB, &PPU_Decoder::RD, &PPU_Decoder::SPR);
		/*0x176*/bind_instr(g1f_list, DSTST, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::STRM, &PPU_Decoder::L_6);
		/*0x177*/bind_instr(g1f_list, LHAUX, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x197*/bind_instr(g1f_list, STHX, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x19c*/bind_instr(g1f_list, ORC, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x1b6*/bind_instr(g1f_list, ECOWX, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x1bc*/bind_instr(g1f_list, OR, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x1c9*/bind_instr(g1f_list, DIVDU, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x1cb*/bind_instr(g1f_list, DIVWU, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x1d3*/bind_instr(g1f_list, MTSPR, &PPU_Decoder::SPR, &PPU_Decoder::RS);
		/*0x1d6*///DCBI
		/*0x1dc*/bind_instr(g1f_list, NAND, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x1e7*/bind_instr(g1f_list, STVXL, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x1e9*/bind_instr(g1f_list, DIVD, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x1eb*/bind_instr(g1f_list, DIVW, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB, &PPU_Decoder::OE, &PPU_Decoder::RC);
		/*0x207*/bind_instr(g1f_list, LVLX, &PPU_Decoder::VD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x216*/bind_instr(g1f_list, LWBRX, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x217*/bind_instr(g1f_list, LFSX, &PPU_Decoder::FRD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x218*/bind_instr(g1f_list, SRW, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x21b*/bind_instr(g1f_list, SRD, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x227*/bind_instr(g1f_list, LVRX, &PPU_Decoder::VD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x237*/bind_instr(g1f_list, LFSUX, &PPU_Decoder::FRD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x256*/bind_instr(g1f_list, SYNC, &PPU_Decoder::L_9_10);
		/*0x257*/bind_instr(g1f_list, LFDX, &PPU_Decoder::FRD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x277*/bind_instr(g1f_list, LFDUX, &PPU_Decoder::FRD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x287*/bind_instr(g1f_list, STVLX, &PPU_Decoder::VS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x297*/bind_instr(g1f_list, STFSX, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x2a7*/bind_instr(g1f_list, STVRX, &PPU_Decoder::VS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x2d7*/bind_instr(g1f_list, STFDX, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x307*/bind_instr(g1f_list, LVLXL, &PPU_Decoder::VD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x316*/bind_instr(g1f_list, LHBRX, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x318*/bind_instr(g1f_list, SRAW, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x31a*/bind_instr(g1f_list, SRAD, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RB, &PPU_Decoder::RC);
		/*0x327*/bind_instr(g1f_list, LVRXL, &PPU_Decoder::VD, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x336*/bind_instr(g1f_list, DSS, &PPU_Decoder::STRM, &PPU_Decoder::L_6);
		/*0x338*/bind_instr(g1f_list, SRAWI, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::sh, &PPU_Decoder::RC);
		/*0x33a*/bind_instr(g1f_list, SRADI1, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::sh, &PPU_Decoder::RC);
		/*0x33b*/bind_instr(g1f_list, SRADI2, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::sh, &PPU_Decoder::RC);
		/*0x356*/bind_instr(g1f_list, EIEIO);
		/*0x39a*/bind_instr(g1f_list, EXTSH, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RC);
		/*0x3ba*/bind_instr(g1f_list, EXTSB, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RC);
		/*0x3d7*/bind_instr(g1f_list, STFIWX, &PPU_Decoder::FRS, &PPU_Decoder::RA, &PPU_Decoder::RB);
		/*0x3da*/bind_instr(g1f_list, EXTSW, &PPU_Decoder::RA, &PPU_Decoder::RS, &PPU_Decoder::RC);
		/*0x3d6*///ICBI
		/*0x3f6*/bind_instr(g1f_list, DCBZ, &PPU_Decoder::RA, &PPU_Decoder::RB);

		bind_instr(g3a_list, LD, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(g3a_list, LDU, &PPU_Decoder::RD, &PPU_Decoder::RA, &PPU_Decoder::DS);

		bind_instr(g3b_list, FDIVS, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3b_list, FSUBS, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3b_list, FADDS, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3b_list, FSQRTS, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3b_list, FRES, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3b_list, FMULS, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRC, &PPU_Decoder::RC);
		bind_instr(g3b_list, FMADDS, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRC, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3b_list, FMSUBS, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRC, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3b_list, FNMSUBS, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRC, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3b_list, FNMADDS, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRC, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		
		bind_instr(g3e_list, STD, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::D);
		bind_instr(g3e_list, STDU, &PPU_Decoder::RS, &PPU_Decoder::RA, &PPU_Decoder::DS);

		bind_instr(g3f_list, FSEL, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRC, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_list, FMUL, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRC, &PPU_Decoder::RC);
		bind_instr(g3f_list, FMSUB, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRC, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_list, FMADD, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRC, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_list, FNMSUB, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRC, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_list, FNMADD, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRC, &PPU_Decoder::FRB, &PPU_Decoder::RC);

		bind_instr(g3f_0_list, FDIV, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FSUB, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FADD, &PPU_Decoder::FRD, &PPU_Decoder::FRA, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FSQRT, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FRSQRTE, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FCMPU, &PPU_Decoder::CRFD, &PPU_Decoder::FRA, &PPU_Decoder::FRB);
		bind_instr(g3f_0_list, FRSP, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FCTIW, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FCTIWZ, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FCMPO, &PPU_Decoder::CRFD, &PPU_Decoder::FRA, &PPU_Decoder::FRB);
		bind_instr(g3f_0_list, FNEG, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FMR, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FNABS, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FABS, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FCFID, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FCTID, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, FCTIDZ, &PPU_Decoder::FRD, &PPU_Decoder::FRB, &PPU_Decoder::RC);

		bind_instr(g3f_0_list, MTFSB1, &PPU_Decoder::BT, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, MCRFS, &PPU_Decoder::BF, &PPU_Decoder::BFA);
		bind_instr(g3f_0_list, MTFSB0, &PPU_Decoder::BT, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, MTFSFI, &PPU_Decoder::CRFD, &PPU_Decoder::I, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, MFFS, &PPU_Decoder::FRD, &PPU_Decoder::RC);
		bind_instr(g3f_0_list, MTFSF, &PPU_Decoder::FLM, &PPU_Decoder::FRB, &PPU_Decoder::RC);

		m_instr_decoder = main_list;
	}

	~PPU_Decoder()
	{
		m_op.Exit();
		delete m_instr_decoder;
	}

	virtual void Decode(const u32 code)
	{
		m_code = code;
		(*m_instr_decoder)();
	}
};

#undef START_OPCODES_GROUP
#undef START_OPCODES_SUB_GROUP
#undef ADD_OPCODE
#undef ADD_NULL_OPCODE
#undef END_OPCODES_GROUP
#undef END_OPCODES_ND_GROUP
#undef END_OPCODES_SUB_GROUP