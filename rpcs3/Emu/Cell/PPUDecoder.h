#pragma once

#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/Decoder.h"


#define START_OPCODES_GROUP(group, reg) \
	case(##group##): \
		temp=##reg##;\
		switch(temp)\
		{

#define END_OPCODES_GROUP(group) \
		default:\
			m_op.UNK(m_code, opcode, temp);\
		break;\
		}\
	break

#define ADD_OPCODE(name, ...) case(##name##):m_op.##name##(__VA_ARGS__); break

class PPU_Decoder : public Decoder
{
	u32 m_code;
	PPU_Opcodes& m_op;

	//This field is used in rotate instructions to specify the first 1 bit of a 64-bit mask
	OP_REG mb()			const { return GetField(21, 25) | (m_code & 0x20); }

	//This field is used in rotate instructions to specify the last 1 bit of a 64-bit mask
	OP_REG me()			const { return GetField(21, 25) | (m_code & 0x20); }

	//This field is used to specify a shift amount
	OP_REG sh()			const { return GetField(16, 20) | ((m_code & 0x2) << 4); }

	//This field is used to specify a special-purpose register for the mtspr and mfspr instructions
	OP_REG SPR()		const { return GetField(11, 20); }

	//
	OP_REG VD()			const { return GetField(6, 10); }

	//
	OP_REG VA()			const { return GetField(11, 15); }

	//
	OP_REG VB()			const { return GetField(16, 20); }

	//This field is used to specify a GPR to be used as a destination
	OP_REG RD()			const { return GetField(6, 10); }

	//This field is used to specify a GPR to be used as a source
	OP_REG RS()			const { return GetField(6, 10); }

	//This field is used to specify a GPR to be used as a source or destination
	OP_REG RA()			const { return GetField(11, 15); }

	//This field is used to specify a GPR to be used as a source
	OP_REG RB()			const { return GetField(16, 20); }

	//This field is used to specify the number of bytes to move in an immediate string load or store
	OP_REG NB()			const { return GetField(16, 20); }

	//This field is used to specify one of the CR fields, or one of the FPSCR fields, as a destination
	OP_REG CRFD()		const { return GetField(6, 8); }

	//This field is used to specify one of the CR fields, or one of the FPSCR fields, as a source
	OP_REG CRFS()		const { return GetField(11, 13); }

	//This field is used to specify a bit in the CR to be used as a source
	OP_REG CRBA()		const { return GetField(11, 15); }

	//This field is used to specify a bit in the CR to be used as a source
	OP_REG CRBB()		const { return GetField(16, 20); }

	//This field is used to specify a bit in the CR, or in the FPSCR, as the destination of the result of an instruction
	OP_REG CRBD()		const { return GetField(6, 10); }
	
	//
	OP_REG BT()			const { return GetField(6, 10); }

	//
	OP_REG BA()			const { return GetField(11, 15); }

	//
	OP_REG BB()			const { return GetField(16, 20); }

	//
	OP_REG BF()			const { return GetField(6, 10); }

	//This field is used to specify options for the branch conditional instructions
	OP_REG BO()			const { return GetField(6, 10); }

	//This field is used to specify a bit in the CR to be used as the condition of a branch conditional instruction
	OP_REG BI()			const { return GetField(11, 15); }

	//Immediate field specifying a 14-bit signed two's complement branch displacement that is concatenated on the
	//right with ‘00’ and sign-extended to 64 bits.
	OP_sIMM BD()		const { return (s32)(s16)GetField(16, 31); }

	//
	OP_REG BH()			const { return GetField(19, 20); }

	//
	OP_REG BFA()		const { return GetField(11, 13); }
	
	//Field used by the optional data stream variant of the dcbt instruction.
	OP_uIMM TH()		const { return GetField(9, 10); }

	//This field is used to specify the conditions on which to trap
	OP_uIMM TO()		const { return GetField(6, 10); }

	//
	OP_REG MB()			const { return GetField(21, 25); }

	//
	OP_REG ME()			const { return GetField(26, 30); }

	//This field is used to specify a shift amount
	OP_REG SH()			const { return GetField(16, 20); }

	/*
	Absolute address bit.
	0		The immediate field represents an address relative to the current instruction address (CIA). (For more 
			information on the CIA, see Table 8-3.) The effective (logical) address of the branch is either the sum 
			of the LI field sign-extended to 64 bits and the address of the branch instruction or the sum of the BD 
			field sign-extended to 64 bits and the address of the branch instruction.
	1		The immediate field represents an absolute address. The effective address (EA) of the branch is the 
			LI field sign-extended to 64 bits or the BD field sign-extended to 64 bits.
	*/
	OP_REG AA()			const { return GetField(30); }

	//
	OP_sIMM LL() const
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
	OP_REG LK()			const { return GetField(31); }

	//This field is used for extended arithmetic to enable setting OV and SO in the XER
	OP_REG OE()			const { return GetField(21); }

	//Field used to specify whether an integer compare instruction is to compare 64-bit numbers or 32-bit numbers
	OP_REG L()			const { return GetField(10); }

	//
	OP_REG I()			const { return GetField(16, 19); }

	//
	OP_REG DQ()			const { return GetField(16, 27); }

	//This field is used to specify an FPR as the destination
	OP_REG FRD()		const { return GetField(6, 10); }

	//This field is used to specify an FPR as a source
	OP_REG FRS()		const { return GetField(6, 10); }

	//
	OP_REG FLM()		const { return GetField(7, 14); }

	//This field is used to specify an FPR as a source
	OP_REG FRA()		const { return GetField(11, 15); }

	//This field is used to specify an FPR as a source
	OP_REG FRB()		const { return GetField(16, 20); }

	//This field is used to specify an FPR as a source
	OP_REG FRC()		const { return GetField(21, 25); }

	//
	OP_REG FXM()		const { return GetField(12, 19); }

	//
	const s32 SYS()		const { return GetField(6, 31); }

	//Immediate field specifying a 16-bit signed two's complement integer that is sign-extended to 64 bits
	OP_sIMM D()			const { return (s32)(s16)GetField(16, 31); }

	//
	OP_sIMM DS() const
	{
		OP_sIMM d = D();
		if(d < 0) return d - 1;
		return d;
	}

	//This immediate field is used to specify a 16-bit signed integer
	OP_sIMM simm16()	const { return (s32)(s16)m_code; }

	//This immediate field is used to specify a 16-bit unsigned integer
	OP_uIMM uimm16()	const { return (u32)(u16)m_code; }
	
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
	const bool RC()	const { return m_code & 0x1; }

	//Primary opcode field
	OP_uIMM OPCD() const { return GetField(0, 5); }
	
	__forceinline u32 GetField(const u32 p) const
	{
		return (m_code >> (31 - p)) & 0x1;
	}
	
	__forceinline u32 GetField(const u32 from, const u32 to) const
	{
		return (m_code >> (31 - to)) & ((1 << ((to - from) + 1)) - 1);
	}
	
public:
	PPU_Decoder(PPU_Opcodes& op) : m_op(op)
	{
	}

	~PPU_Decoder()
	{
		m_op.Exit();
	}

	virtual void Decode(const u32 code)
	{
		if(code == 0)
		{
			m_op.NULL_OP();
			return;
		}

		m_code = code;

		u32 opcode, temp;
		switch((opcode = OPCD()))
		{
		
		ADD_OPCODE(TDI,		TO(), RA(), simm16());
		ADD_OPCODE(TWI,		TO(), RA(), simm16());

		START_OPCODES_GROUP(G_04, ((m_code >> 1) & 0x3ff))
			ADD_OPCODE(VXOR,	VD(), VA(), VB());
		END_OPCODES_GROUP(G_04);

		ADD_OPCODE(MULLI,	RD(), RA(), simm16());
		ADD_OPCODE(SUBFIC,	RD(), RA(), simm16());
		ADD_OPCODE(CMPLI,	CRFD(), L(), RA(), uimm16());
		ADD_OPCODE(CMPI,	CRFD(), L(), RA(), simm16());
		ADD_OPCODE(ADDIC,	RD(), RA(), simm16());
		ADD_OPCODE(ADDIC_,	RD(), RA(), simm16());
		ADD_OPCODE(ADDI,	RD(), RA(), simm16());
		ADD_OPCODE(ADDIS,	RD(), RA(), simm16());			
		ADD_OPCODE(BC,		BO(), BI(), BD(), AA(), LK());
		ADD_OPCODE(SC,		SYS());
		ADD_OPCODE(B,		LL(), AA(), LK());
	
		START_OPCODES_GROUP(G_13, GetField(21, 30))
			ADD_OPCODE(MCRF,	CRFD(), CRFS());
			ADD_OPCODE(BCLR,	BO(), BI(), BH(), LK());
			ADD_OPCODE(CRNOR,	CRBD(), CRBA(), CRBB());
			ADD_OPCODE(CRANDC,	CRBD(), CRBA(), CRBB());
			ADD_OPCODE(ISYNC);
			ADD_OPCODE(CRXOR,	CRBD(), CRBA(), CRBB());
			ADD_OPCODE(CRNAND,	CRBD(), CRBA(), CRBB());
			ADD_OPCODE(CRAND,	CRBD(), CRBA(), CRBB());
			ADD_OPCODE(CREQV,	CRBD(), CRBA(), CRBB());
			ADD_OPCODE(CRORC,	CRBD(), CRBA(), CRBB());
			ADD_OPCODE(CROR,	CRBD(), CRBA(), CRBB());
			ADD_OPCODE(BCCTR,	BO(), BI(), BH(), LK());
		END_OPCODES_GROUP(G_13);
		
		ADD_OPCODE(RLWIMI,	RA(), RS(), SH(), MB(), ME(), RC());
		ADD_OPCODE(RLWINM,	RA(), RS(), SH(), MB(), ME(), RC());
		ADD_OPCODE(RLWNM,	RA(), RS(), RB(), MB(), ME(), RC());
		ADD_OPCODE(ORI,		RA(), RS(), uimm16());
		ADD_OPCODE(ORIS,	RA(), RS(), uimm16());
		ADD_OPCODE(XORI,	RA(), RS(), uimm16());
		ADD_OPCODE(XORIS,	RA(), RS(), uimm16());
		ADD_OPCODE(ANDI_,	RA(), RS(), uimm16());
		ADD_OPCODE(ANDIS_,	RA(), RS(), uimm16());
	
		START_OPCODES_GROUP(G_1e, GetField(28, 29))
			ADD_OPCODE(RLDICL,	RA(), RS(), sh(), mb(), RC());
			ADD_OPCODE(RLDICR,	RA(), RS(), sh(), me(), RC());
			ADD_OPCODE(RLDIC,	RA(), RS(), sh(), mb(), RC());
			ADD_OPCODE(RLDIMI,	RA(), RS(), sh(), mb(), RC());
		END_OPCODES_GROUP(G_1e);

		START_OPCODES_GROUP(G_1f, GetField(21, 30))
			/*0x000*/ADD_OPCODE(CMP,	CRFD(), L(), RA(), RB());
			/*0x004*/ADD_OPCODE(TW,		TO(), RA(), RB());
			/*0x007*/ADD_OPCODE(LVEBX,	VD(), RA(), RB());
			/*0x008*/ADD_OPCODE(SUBFC,	RD(), RA(), RB(), OE(), RC());
			/*0x009*/ADD_OPCODE(MULHDU,	RD(), RA(), RB(), RC());
			/*0x00a*/ADD_OPCODE(ADDC,	RD(), RA(), RB(), OE(), RC());
			/*0x00b*/ADD_OPCODE(MULHWU,	RD(), RA(), RB(), RC());
			/*0x013*/ADD_OPCODE(MFOCRF,	GetField(11), FXM(), RD());
			/*0x014*/ADD_OPCODE(LWARX,	RD(), RA(), RB());
			/*0x015*/ADD_OPCODE(LDX,	RA(), RS(), RB());
			/*0x017*/ADD_OPCODE(LWZX,	RD(), RA(), RB());
			/*0x018*/ADD_OPCODE(SLW,	RA(), RS(), RB(), RC());
			/*0x01a*/ADD_OPCODE(CNTLZW,	RA(), RS(), RC());
			/*0x01b*/ADD_OPCODE(SLD,	RA(), RS(), RB(), RC());
			/*0x01c*/ADD_OPCODE(AND,	RA(), RS(), RB(), RC());
			/*0x020*/ADD_OPCODE(CMPL,	CRFD(), L(), RA(), RB());
			/*0x027*/ADD_OPCODE(LVEHX,	VD(), RA(), RB());
			/*0x028*/ADD_OPCODE(SUBF,	RD(), RA(), RB(), OE(), RC());
			/*0x036*/ADD_OPCODE(DCBST,	RA(), RB());
			/*0x03a*/ADD_OPCODE(CNTLZD,	RA(), RS(), RC());
			/*0x03c*/ADD_OPCODE(ANDC,	RA(), RS(), RB(), RC());
			/*0x049*/ADD_OPCODE(MULHD,	RD(), RA(), RB(), RC());
			/*0x04b*/ADD_OPCODE(MULHW,	RD(), RA(), RB(), RC());
			/*0x054*/ADD_OPCODE(LDARX,	RD(), RA(), RB());
			/*0x056*/ADD_OPCODE(DCBF,	RA(), RB());
			/*0x057*/ADD_OPCODE(LBZX,	RD(), RA(), RB());
			/*0x067*/ADD_OPCODE(LVX,	VD(), RA(), RB());
			/*0x068*/ADD_OPCODE(NEG,	RD(), RA(), OE(), RC());
			/*0x077*/ADD_OPCODE(LBZUX,	RD(), RA(), RB());
			/*0x07c*/ADD_OPCODE(NOR,	RA(), RS(), RB(), RC());
			/*0x088*/ADD_OPCODE(SUBFE,	RD(), RA(), RB(), OE(), RC());
			/*0x08a*/ADD_OPCODE(ADDE,	RD(), RA(), RB(), OE(), RC());
			/*0x090*/ADD_OPCODE(MTOCRF,	FXM(), RS());
			/*0x095*/ADD_OPCODE(STDX,	RS(), RA(), RB());
			/*0x096*/ADD_OPCODE(STWCX_,	RS(), RA(), RB());
			/*0x097*/ADD_OPCODE(STWX,	RS(), RA(), RB());
			/*0x0b5*/ADD_OPCODE(STDUX,	RS(), RA(), RB());
			/*0x0ca*/ADD_OPCODE(ADDZE,	RD(), RA(), OE(), RC());
			/*0x0d6*/ADD_OPCODE(STDCX_,	RS(), RA(), RB());
			/*0x0d7*/ADD_OPCODE(STBX,	RS(), RA(), RB());
			/*0x0e7*/ADD_OPCODE(STVX,	VD(), RA(), RB());
			/*0x0e9*/ADD_OPCODE(MULLD,	RD(), RA(), RB(), OE(), RC());
			/*0x0ea*/ADD_OPCODE(ADDME,	RD(), RA(), OE(), RC());
			/*0x0eb*/ADD_OPCODE(MULLW,	RD(), RA(), RB(), OE(), RC());
			/*0x0f6*/ADD_OPCODE(DCBTST,	TH(), RA(), RB());
			/*0x10a*/ADD_OPCODE(ADD,	RD(), RA(), RB(), OE(), RC());
			/*0x116*/ADD_OPCODE(DCBT,	RA(), RB(), TH());
			/*0x117*/ADD_OPCODE(LHZX,	RD(), RA(), RB());
			/*0x11c*/ADD_OPCODE(EQV,	RA(), RS(), RB(), RC());
			/*0x136*/ADD_OPCODE(ECIWX,	RD(), RA(), RB());
			/*0x137*/ADD_OPCODE(LHZUX,	RD(), RA(), RB());
			/*0x13c*/ADD_OPCODE(XOR,	RA(), RS(), RB(), RC());
			/*0x153*/ADD_OPCODE(MFSPR,	RD(), SPR());
			/*0x157*/ADD_OPCODE(LHAX,	RD(), RA(), RB());
			/*0x168*/ADD_OPCODE(ABS,	RD(), RA(), OE(), RC());
			/*0x173*/ADD_OPCODE(MFTB,	RD(), SPR());
			/*0x177*/ADD_OPCODE(LHAUX,	RD(), RA(), RB());
			/*0x197*/ADD_OPCODE(STHX,	RS(), RA(), RB());
			/*0x19c*/ADD_OPCODE(ORC,	RA(), RS(), RB(), RC());
			/*0x1b6*/ADD_OPCODE(ECOWX,	RS(), RA(), RB());
			/*0x1bc*/ADD_OPCODE(OR,		RA(), RS(), RB(), RC());
			/*0x1c9*/ADD_OPCODE(DIVDU,	RD(), RA(), RB(), OE(), RC());
			/*0x1cb*/ADD_OPCODE(DIVWU,	RD(), RA(), RB(), OE(), RC());
			/*0x1d3*/ADD_OPCODE(MTSPR,	SPR(), RS());
			/*0x1d6*///DCBI
			/*0x1e9*/ADD_OPCODE(DIVD,	RD(), RA(), RB(), OE(), RC());
			/*0x1eb*/ADD_OPCODE(DIVW,	RD(), RA(), RB(), OE(), RC());
			/*0x216*/ADD_OPCODE(LWBRX,	RD(), RA(), RB());
			/*0x217*/ADD_OPCODE(LFSX,	FRD(), RA(), RB());
			/*0x218*/ADD_OPCODE(SRW,	RA(), RS(), RB(), RC());
			/*0x21b*/ADD_OPCODE(SRD,	RA(), RS(), RB(), RC());
			/*0x237*/ADD_OPCODE(LFSUX,	FRD(), RA(), RB());
			/*0x256*/ADD_OPCODE(SYNC,	GetField(9, 10));
			/*0x257*/ADD_OPCODE(LFDX,	FRD(), RA(), RB());
			/*0x277*/ADD_OPCODE(LFDUX,	FRD(), RA(), RB());
			/*0x297*/ADD_OPCODE(STFSX,	RS(), RA(), RB());
			/*0x316*/ADD_OPCODE(LHBRX,	RD(), RA(), RB());
			/*0x318*/ADD_OPCODE(SRAW,	RA(), RS(), RB(), RC());
			/*0x31A*/ADD_OPCODE(SRAD,	RA(), RS(), RB(), RC());
			/*0x338*/ADD_OPCODE(SRAWI,	RA(), RS(), sh(), RC());
			/*0x33a*/ADD_OPCODE(SRADI1,	RA(), RS(), sh(), RC());
			/*0x33b*/ADD_OPCODE(SRADI2,	RA(), RS(), sh(), RC());
			/*0x356*/ADD_OPCODE(EIEIO);
			/*0x39a*/ADD_OPCODE(EXTSH,	RA(), RS(), RC());
			/*0x3ba*/ADD_OPCODE(EXTSB,	RA(), RS(), RC());
			/*0x3d7*/ADD_OPCODE(STFIWX,	FRS(), RA(), RB());
			/*0x3da*/ADD_OPCODE(EXTSW,	RA(), RS(), RC());
			/*0x3d6*///ICBI
			/*0x3f6*/ADD_OPCODE(DCBZ,	RA(), RB());
		END_OPCODES_GROUP(G_1f);
		
		ADD_OPCODE(LWZ,		RD(), RA(), D());
		ADD_OPCODE(LWZU,	RD(), RA(), D());
		ADD_OPCODE(LBZ,		RD(), RA(), D());
		ADD_OPCODE(LBZU,	RD(), RA(), D());
		ADD_OPCODE(STW,		RS(), RA(), D());
		ADD_OPCODE(STWU,	RS(), RA(), D());
		ADD_OPCODE(STB,		RS(), RA(), D());
		ADD_OPCODE(STBU,	RS(), RA(), D());
		ADD_OPCODE(LHZ,		RD(), RA(), D());
		ADD_OPCODE(LHZU,	RD(), RA(), D());
		ADD_OPCODE(STH,		RS(), RA(), D());
		ADD_OPCODE(STHU,	RS(), RA(), D());
		ADD_OPCODE(LMW,		RD(), RA(), D());
		ADD_OPCODE(STMW,	RS(), RA(), D());
		ADD_OPCODE(LFS,		FRD(), RA(), D());
		ADD_OPCODE(LFSU,	FRD(), RA(), D());
		ADD_OPCODE(LFD,		FRD(), RA(), D());
		ADD_OPCODE(LFDU,	FRD(), RA(), D());
		ADD_OPCODE(STFS,	FRS(), RA(), D());
		ADD_OPCODE(STFSU,	FRS(), RA(), D());
		ADD_OPCODE(STFD,	FRS(), RA(), D());
		ADD_OPCODE(STFDU,	FRS(), RA(), D());

		START_OPCODES_GROUP(G_3a, GetField(30, 31))
			ADD_OPCODE(LD,	RD(), RA(), D());
			ADD_OPCODE(LDU,	RD(), RA(), DS());
		END_OPCODES_GROUP(G_3a);

		START_OPCODES_GROUP(G_3b, GetField(26, 30))
			ADD_OPCODE(FDIVS,	FRD(), FRA(), FRB(), RC());
			ADD_OPCODE(FSUBS,	FRD(), FRA(), FRB(), RC());
			ADD_OPCODE(FADDS,	FRD(), FRA(), FRB(), RC());
			ADD_OPCODE(FSQRTS,	FRD(), FRB(), RC());
			ADD_OPCODE(FRES,	FRD(), FRB(), RC());
			ADD_OPCODE(FMULS,	FRD(), FRA(), FRC(), RC());
			ADD_OPCODE(FMADDS,	FRD(), FRA(), FRC(), FRB(), RC());
			ADD_OPCODE(FMSUBS,	FRD(), FRA(), FRC(), FRB(), RC());
			ADD_OPCODE(FNMSUBS,	FRD(), FRA(), FRC(), FRB(), RC());
			ADD_OPCODE(FNMADDS,	FRD(), FRA(), FRC(), FRB(), RC());
		END_OPCODES_GROUP(G_3b);
		
		START_OPCODES_GROUP(G_3e, GetField(30, 31))
			ADD_OPCODE(STD,	RS(), RA(), D());
			ADD_OPCODE(STDU,	RS(), RA(), DS());
		END_OPCODES_GROUP(G_3e);

		START_OPCODES_GROUP(G_3f, GetField(26, 30))
			
			ADD_OPCODE(FDIV,	FRD(), FRA(), FRB(), RC());
			ADD_OPCODE(FSUB,	FRD(), FRA(), FRB(), RC());
			ADD_OPCODE(FADD,	FRD(), FRA(), FRB(), RC());
			ADD_OPCODE(FSQRT,	FRD(), FRB(), RC());
			ADD_OPCODE(FSEL,	FRD(), FRA(), FRC(), FRB(), RC());
			ADD_OPCODE(FMUL,	FRD(), FRA(), FRC(), RC());
			ADD_OPCODE(FRSQRTE,	FRD(), FRB(), RC());
			ADD_OPCODE(FMSUB,	FRD(), FRA(), FRC(), FRB(), RC());
			ADD_OPCODE(FMADD,	FRD(), FRA(), FRC(), FRB(), RC());
			ADD_OPCODE(FNMSUB,	FRD(), FRA(), FRC(), FRB(), RC());
			ADD_OPCODE(FNMADD,	FRD(), FRA(), FRC(), FRB(), RC());
			ADD_OPCODE(FCMPO,	CRFD(), FRA(), FRB());

			default:
			START_OPCODES_GROUP(0x8, GetField(21, 30))
				ADD_OPCODE(FCMPU,	CRFD(), FRA(), FRB());
				ADD_OPCODE(FRSP,	FRD(), FRB(), RC());
				ADD_OPCODE(FCTIW,	FRD(), FRB(), RC());
				ADD_OPCODE(FCTIWZ,	FRD(), FRB(), RC());
				ADD_OPCODE(FNEG,	FRD(), FRB(), RC());
				ADD_OPCODE(FMR,		FRD(), FRB(), RC());
				ADD_OPCODE(FNABS,	FRD(), FRB(), RC());
				ADD_OPCODE(FABS,	FRD(), FRB(), RC());
				ADD_OPCODE(FCFID,	FRD(), FRB(), RC());
				ADD_OPCODE(FCTID,	FRD(), FRB(), RC());
				ADD_OPCODE(FCTIDZ,	FRD(), FRB(), RC());

				ADD_OPCODE(MTFSB1,	BT(), RC());
				ADD_OPCODE(MCRFS,	BF(), BFA());
				ADD_OPCODE(MTFSB0,	BT(), RC());
				ADD_OPCODE(MTFSFI,	CRFD(), I(), RC());
				ADD_OPCODE(MFFS,	FRD(), RC());
				ADD_OPCODE(MTFSF,	FLM(), FRB(), RC());
			END_OPCODES_GROUP(0x8);
			break;
			}
			break;
		//END_OPCODES_GROUP(G_3f);

		default: m_op.UNK(m_code, opcode, opcode); break;
		}
	}
};

#undef START_OPCODES_GROUP
#undef ADD_OPCODE
#undef ADD_NULL_OPCODE
#undef END_OPCODES_GROUP