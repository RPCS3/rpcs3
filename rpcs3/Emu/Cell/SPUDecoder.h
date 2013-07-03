#pragma once

#include "Emu/Cell/SPUOpcodes.h"
#include "Emu/Cell/PPCDecoder.h"

#define START_OPCODES_GROUP_(group, reg) \
	case(##group##): \
		temp=##reg##;\
		switch(temp)\
		{

#define START_OPCODES_GROUP(group, reg) START_OPCODES_GROUP_(##group##, ##reg##())

#define END_OPCODES_GROUP(group) \
		default:\
			m_op.UNK(m_code, opcode, temp);\
		break;\
		}\
	break

#define ADD_OPCODE(name, regs) case(##name##):m_op.##name####regs##; return
#define ADD_NULL_OPCODE(name) ADD_OPCODE(##name##, ())

class SPU_Decoder : public PPC_Decoder
{
	u32 m_code;
	SPU_Opcodes& m_op;

	OP_REG RC()			const { return GetField(4, 10); }
	OP_REG RT()			const { return GetField(25, 31); }
	OP_REG RA()			const { return GetField(18, 24); }
	OP_REG RB()			const { return GetField(11, 17); }

	OP_uIMM i7()		const { return GetField(11, 17); }
	OP_uIMM i8()		const { return GetField(10, 17); }
	OP_uIMM i10()		const { return GetField(8, 17); }
	OP_uIMM i16()		const { return GetField(9, 24); }
	OP_uIMM i18()		const { return GetField(7, 24); }

	OP_uIMM ROH()		const { return GetField(16, 17); }
	OP_uIMM ROL()		const { return GetField(25, 31); }
	OP_uIMM RO()		const { return ROL() | (ROH() << 8); }

	OP_uIMM RR()		const { return GetField(0, 10); }
	OP_uIMM RRR()		const { return GetField(0, 3); }
	OP_uIMM RI7()		const { return GetField(0, 10); }
	OP_uIMM RI8()		const { return GetField(0, 9); }
	OP_uIMM RI10()		const { return GetField(0, 7); }
	OP_uIMM RI16()		const { return GetField(0, 8); }
	OP_uIMM RI18()		const { return GetField(0, 6); }
	
	__forceinline u32 GetField(const u32 p) const
	{
		return (m_code >> (31 - p)) & 0x1;
	}
	
	__forceinline u32 GetField(const u32 from, const u32 to) const
	{
		return (m_code >> (31 - to)) & ((1 << ((to - from) + 1)) - 1);
	}

	OP_sIMM exts18(OP_sIMM i18) const
	{
		if(i18 & 0x20000) return i18 - 0x40000;
		return i18;
	}

	OP_sIMM exts16(OP_sIMM i16) const
	{
		return (s32)(s16)i16;
	}

	OP_sIMM exts10(OP_sIMM i10) const
	{
		if(i10 & 0x200) return i10 - 0x400;
		return i10;
	}

	OP_sIMM exts7(OP_sIMM i7) const
	{
		if(i7 & 0x40) return i7 - 0x80;
		return i7;
	}
	
public:
	SPU_Decoder(SPU_Opcodes& op) : m_op(op)
	{
	}

	~SPU_Decoder()
	{
		m_op.Exit();
		delete &m_op;
	}

	virtual void Decode(const u32 code)
	{
		using namespace SPU_opcodes;

		m_code = code;

		switch(RR()) //& RI7 //0 - 10
		{
		ADD_OPCODE(STOP,(GetField(18, 31)));
		ADD_OPCODE(LNOP,());
		ADD_OPCODE(SYNC,(GetField(11)));
		ADD_OPCODE(DSYNC,());
		ADD_OPCODE(MFSPR,(RT(), RA()));
		ADD_OPCODE(RDCH,(RT(), RA()));
		ADD_OPCODE(RCHCNT,(RT(), RA()));
		ADD_OPCODE(SF,(RT(), RA(), RB()));
		ADD_OPCODE(OR,(RT(), RA(), RB()));
		ADD_OPCODE(BG,(RT(), RA(), RB()));
		ADD_OPCODE(SFH,(RT(), RA(), RB()));
		ADD_OPCODE(NOR,(RT(), RA(), RB()));
		ADD_OPCODE(ABSDB,(RT(), RA(), RB()));
		ADD_OPCODE(ROT,(RT(), RA(), RB()));
		ADD_OPCODE(ROTM,(RT(), RA(), RB()));
		ADD_OPCODE(ROTMA,(RT(), RA(), RB()));
		ADD_OPCODE(SHL,(RT(), RA(), RB()));
		ADD_OPCODE(ROTH,(RT(), RA(), RB()));
		ADD_OPCODE(ROTHM,(RT(), RA(), RB()));
		ADD_OPCODE(ROTMAH,(RT(), RA(), RB()));
		ADD_OPCODE(SHLH,(RT(), RA(), RB()));
		ADD_OPCODE(ROTI,(RT(), RA(), RB()));
		ADD_OPCODE(ROTMI,(RT(), RA(), RB()));
		ADD_OPCODE(ROTMAI,(RT(), RA(), RB()));
		ADD_OPCODE(SHLI,(RT(), RA(), i7()));
		ADD_OPCODE(ROTHI,(RT(), RA(), i7()));
		ADD_OPCODE(ROTHMI,(RT(), RA(), i7()));
		ADD_OPCODE(ROTMAHI,(RT(), RA(), i7()));
		ADD_OPCODE(SHLHI,(RT(), RA(), i7()));
		ADD_OPCODE(A,(RT(), RA(), RB()));
		ADD_OPCODE(AND,(RT(), RA(), RB()));
		ADD_OPCODE(CG,(RT(), RA(), RB()));
		ADD_OPCODE(AH,(RT(), RA(), RB()));
		ADD_OPCODE(NAND,(RT(), RA(), RB()));
		ADD_OPCODE(AVGB,(RT(), RA(), RB()));
		ADD_OPCODE(MTSPR,(RT(), RA()));
		ADD_OPCODE(WRCH,(RA(), RT()));
		ADD_OPCODE(BIZ,(RT(), RA()));
		ADD_OPCODE(BINZ,(RT(), RA()));
		ADD_OPCODE(BIHZ,(RT(), RA()));
		ADD_OPCODE(BIHNZ,(RT(), RA()));
		ADD_OPCODE(STOPD,(RT(), RA(), RB()));
		ADD_OPCODE(STQX,(RT(), RA(), RB()));
		ADD_OPCODE(BI,(RA()));
		ADD_OPCODE(BISL,(RT(), RA()));
		ADD_OPCODE(IRET,(RA()));
		ADD_OPCODE(BISLED,(RT(), RA()));
		ADD_OPCODE(HBR,(GetField(11), RO(), RA()));
		ADD_OPCODE(GB,(RT(), RA()));
		ADD_OPCODE(GBH,(RT(), RA()));
		ADD_OPCODE(GBB,(RT(), RA()));
		ADD_OPCODE(FSM,(RT(), RA()));
		ADD_OPCODE(FSMH,(RT(), RA()));
		ADD_OPCODE(FSMB,(RT(), RA()));
		ADD_OPCODE(FREST,(RT(), RA()));
		ADD_OPCODE(FRSQEST,(RT(), RA()));
		ADD_OPCODE(LQX,(RT(), RA(), RB()));
		ADD_OPCODE(ROTQBYBI,(RT(), RA(), RB()));
		ADD_OPCODE(ROTQMBYBI,(RT(), RA(), RB()));
		ADD_OPCODE(SHLQBYBI,(RT(), RA(), RB()));
		ADD_OPCODE(CBX,(RT(), RA(), RB()));
		ADD_OPCODE(CHX,(RT(), RA(), RB()));
		ADD_OPCODE(CWX,(RT(), RA(), RB()));
		ADD_OPCODE(CDX,(RT(), RA(), RB()));
		ADD_OPCODE(ROTQBI,(RT(), RA(), RB()));
		ADD_OPCODE(ROTQMBI,(RT(), RA(), RB()));
		ADD_OPCODE(SHLQBI,(RT(), RA(), RB()));
		ADD_OPCODE(ROTQBY,(RT(), RA(), RB()));
		ADD_OPCODE(ROTQMBY,(RT(), RA(), RB()));
		ADD_OPCODE(SHLQBY,(RT(), RA(), RB()));
		ADD_OPCODE(ORX,(RT(), RA()));
		ADD_OPCODE(CBD,(RT(), RA(), exts7(i7())));
		ADD_OPCODE(CHD,(RT(), RA(), exts7(i7())));
		ADD_OPCODE(CWD,(RT(), RA(), exts7(i7())));
		ADD_OPCODE(CDD,(RT(), RA(), exts7(i7())));
		ADD_OPCODE(ROTQBII,(RT(), RA(), i7()));
		ADD_OPCODE(ROTQMBII,(RT(), RA(), i7()));
		ADD_OPCODE(SHLQBII,(RT(), RA(), i7()));
		ADD_OPCODE(ROTQBYI,(RT(), RA(), i7()));
		ADD_OPCODE(ROTQMBYI,(RT(), RA(), i7()));
		ADD_OPCODE(SHLQBYI,(RT(), RA(), i7()));
		ADD_OPCODE(NOP,(RT()));
		ADD_OPCODE(CGT,(RT(), RA(), RB()));
		ADD_OPCODE(XOR,(RT(), RA(), RB()));
		ADD_OPCODE(CGTH,(RT(), RA(), RB()));
		ADD_OPCODE(EQV,(RT(), RA(), RB()));
		ADD_OPCODE(CGTB,(RT(), RA(), RB()));
		ADD_OPCODE(SUMB,(RT(), RA(), RB()));
		ADD_OPCODE(HGT,(RT(), RA(), RB()));
		ADD_OPCODE(CLZ,(RT(), RA()));
		ADD_OPCODE(XSWD,(RT(), RA()));
		ADD_OPCODE(XSHW,(RT(), RA()));
		ADD_OPCODE(CNTB,(RT(), RA()));
		ADD_OPCODE(XSBH,(RT(), RA()));
		ADD_OPCODE(CLGT,(RT(), RA(), RB()));
		ADD_OPCODE(ANDC,(RT(), RA(), RB()));
		ADD_OPCODE(FCGT,(RT(), RA(), RB()));
		ADD_OPCODE(DFCGT,(RT(), RA(), RB()));
		ADD_OPCODE(FA,(RT(), RA(), RB()));
		ADD_OPCODE(FS,(RT(), RA(), RB()));
		ADD_OPCODE(FM,(RT(), RA(), RB()));
		ADD_OPCODE(CLGTH,(RT(), RA(), RB()));
		ADD_OPCODE(ORC,(RT(), RA(), RB()));
		ADD_OPCODE(FCMGT,(RT(), RA(), RB()));
		ADD_OPCODE(DFCMGT,(RT(), RA(), RB()));
		ADD_OPCODE(DFA,(RT(), RA(), RB()));
		ADD_OPCODE(DFS,(RT(), RA(), RB()));
		ADD_OPCODE(DFM,(RT(), RA(), RB()));
		ADD_OPCODE(CLGTB,(RT(), RA(), RB()));
		ADD_OPCODE(HLGT,(RT(), RA(), RB()));
		ADD_OPCODE(DFMA,(RT(), RA(), RB()));
		ADD_OPCODE(DFMS,(RT(), RA(), RB()));
		ADD_OPCODE(DFNMS,(RT(), RA(), RB()));
		ADD_OPCODE(DFNMA,(RT(), RA(), RB()));
		ADD_OPCODE(CEQ,(RT(), RA(), RB()));
		ADD_OPCODE(MPYHHU,(RT(), RA(), RB()));
		ADD_OPCODE(ADDX,(RT(), RA(), RB()));
		ADD_OPCODE(SFX,(RT(), RA(), RB()));
		ADD_OPCODE(CGX,(RT(), RA(), RB()));
		ADD_OPCODE(BGX,(RT(), RA(), RB()));
		ADD_OPCODE(MPYHHA,(RT(), RA(), RB()));
		ADD_OPCODE(MPYHHAU,(RT(), RA(), RB()));
		ADD_OPCODE(FSCRRD,(RT()));
		ADD_OPCODE(FESD,(RT(), RA()));
		ADD_OPCODE(FRDS,(RT(), RA()));
		ADD_OPCODE(FSCRWR,(RT(), RA()));
		ADD_OPCODE(DFTSV,(RT(), RA(), i7()));
		ADD_OPCODE(FCEQ,(RT(), RA(), RB()));
		ADD_OPCODE(DFCEQ,(RT(), RA(), RB()));
		ADD_OPCODE(MPY,(RT(), RA(), RB()));
		ADD_OPCODE(MPYH,(RT(), RA(), RB()));
		ADD_OPCODE(MPYHH,(RT(), RA(), RB()));
		ADD_OPCODE(MPYS,(RT(), RA(), RB()));
		ADD_OPCODE(CEQH,(RT(), RA(), RB()));
		ADD_OPCODE(FCMEQ,(RT(), RA(), RB()));
		ADD_OPCODE(DFCMEQ,(RT(), RA(), RB()));
		ADD_OPCODE(MPYU,(RT(), RA(), RB()));
		ADD_OPCODE(CEQB,(RT(), RA(), RB()));
		ADD_OPCODE(FI,(RT(), RA(), RB()));
		ADD_OPCODE(HEQ,(RT(), RA(), RB()));
		}

		switch(RI8())  //0 - 9
		{
		ADD_OPCODE(CFLTS,(RT(), RA(), i8()));
		ADD_OPCODE(CFLTU,(RT(), RA(), i8()));
		ADD_OPCODE(CSFLT,(RT(), RA(), i8()));
		ADD_OPCODE(CUFLT,(RT(), RA(), i8()));
		}

		switch(RI16()) //0 - 8
		{
		ADD_OPCODE(BRZ,(RT(), exts16(i16())));
		ADD_OPCODE(STQA,(RT(), exts16(i16())));
		ADD_OPCODE(BRNZ,(RT(), exts16(i16())));
		ADD_OPCODE(BRHZ,(RT(), exts16(i16())));
		ADD_OPCODE(BRHNZ,(RT(), exts16(i16())));
		ADD_OPCODE(STQR,(RT(), i16()));
		ADD_OPCODE(BRA,(exts16(i16())));
		ADD_OPCODE(LQA,(RT(), exts16(i16())));
		ADD_OPCODE(BRASL,(RT(), exts16(i16())));
		ADD_OPCODE(BR,(exts16(i16())));
		ADD_OPCODE(FSMBI,(RT(), i16()));
		ADD_OPCODE(BRSL,(RT(), exts16(i16())));
		ADD_OPCODE(LQR,(RT(), exts16(i16())));
		ADD_OPCODE(IL,(RT(), exts16(i16())));
		ADD_OPCODE(ILHU,(RT(), i16()));
		ADD_OPCODE(ILH,(RT(), i16()));
		ADD_OPCODE(IOHL,(RT(), i16()));
		}

		switch(RI10()) //0 - 7
		{
		ADD_OPCODE(ORI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(ORHI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(ORBI,(RT(), RA(), i10()));
		ADD_OPCODE(SFI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(SFHI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(ANDI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(ANDHI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(ANDBI,(RT(), RA(), i10()));
		ADD_OPCODE(AI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(AHI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(STQD,(RT(), exts10(i10()) << 4, RA()));
		ADD_OPCODE(LQD,(RT(), exts10(i10()) << 4, RA()));
		ADD_OPCODE(XORI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(XORHI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(XORBI,(RT(), RA(), i10()));
		ADD_OPCODE(CGTI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(CGTHI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(CGTBI,(RT(), RA(), i10()));
		ADD_OPCODE(HGTI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(CLGTI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(CLGTHI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(CLGTBI,(RT(), RA(), i10()));
		ADD_OPCODE(HLGTI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(MPYI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(MPYUI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(CEQI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(CEQHI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(CEQBI,(RT(), RA(), i10()));
		ADD_OPCODE(HEQI,(RT(), RA(), exts10(i10())));
		}

		switch(RI18()) //0 - 6
		{
		ADD_OPCODE(HBRA,(RO(), i16() << 2));
		ADD_OPCODE(HBRR,(RO(), exts16(i16())));
		ADD_OPCODE(ILA,(RT(), i18()));
		}

		switch(RRR()) //0 - 3
		{
		ADD_OPCODE(SELB,(RC(), RA(), RB(), RT()));
		ADD_OPCODE(SHUFB,(RC(), RA(), RB(), RT()));
		ADD_OPCODE(MPYA,(RC(), RA(), RB(), RT()));
		ADD_OPCODE(FNMS,(RC(), RA(), RB(), RT()));
		ADD_OPCODE(FMA,(RC(), RA(), RB(), RT()));
		ADD_OPCODE(FMS,(RC(), RA(), RB(), RT()));
		}
		
		m_op.UNK(m_code, 0, 0);
	}
};

#undef START_OPCODES_GROUP_
#undef START_OPCODES_GROUP
#undef ADD_OPCODE
#undef ADD_NULL_OPCODE
#undef END_OPCODES_GROUP