#pragma once

#include "Emu/Cell/SPUOpcodes.h"
#include "Emu/Cell/Decoder.h"

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

class SPU_Decoder : public Decoder
{
	u32 m_code;
	SPU_Opcodes& m_op;

	OP_REG RC()			const { return GetField(4, 10); }
	OP_REG RT()			const { return GetField(25, 31); }
	OP_REG RA()			const { return GetField(18, 24); }
	OP_REG RB()			const { return GetField(11, 17); }

	OP_sIMM i7()		const { return GetField(11, 17); }
	OP_sIMM i10()		const { return GetField(8, 17); }
	OP_sIMM i16()		const { return GetField(9, 24); }
	OP_sIMM i18()		const { return GetField(7, 24); }

	OP_sIMM ROH()		const { return GetField(16, 17); }
	OP_sIMM ROL()		const { return GetField(25, 31); }
	OP_sIMM RO()		const { return ROL()/* | (ROH() << 8)*/; }

	OP_uIMM RR()		const { return GetField(0, 10); }
	OP_uIMM RRR()		const { return GetField(0, 3); }
	OP_uIMM RI7()		const { return GetField(0, 10); }
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
		m_code = code;

		switch(RR()) //& RI7 //0 - 10
		{
		ADD_OPCODE(STOP,(GetField(18, 31)));
		ADD_OPCODE(LNOP,());
		ADD_OPCODE(RDCH,(RT(), RA()));
		ADD_OPCODE(RCHCNT,(RT(), RA()));
		ADD_OPCODE(SF,(RT(), RA(), RB()));
		ADD_OPCODE(SHLI,(RT(), RA(), i7()));
		ADD_OPCODE(A,(RT(), RA(), RB()));
		ADD_OPCODE(SPU_AND,(RT(), RA(), RB()));
		ADD_OPCODE(LQX,(RT(), RA(), RB()));
		ADD_OPCODE(WRCH,(RA(), RT()));
		ADD_OPCODE(STQX,(RT(), RA(), RB()));
		ADD_OPCODE(BI,(RA()));
		ADD_OPCODE(BISL,(RT(), RA()));
		ADD_OPCODE(HBR,(GetField(11), RO(), RA()));
		ADD_OPCODE(CWX,(RT(), RA(), RB()));
		ADD_OPCODE(ROTQBY,(RT(), RA(), RB()));
		ADD_OPCODE(ROTQBYI,(RT(), RA(), i7()));
		ADD_OPCODE(SHLQBYI,(RT(), RA(), i7()));
		ADD_OPCODE(SPU_NOP,(RT()));
		ADD_OPCODE(CLGT,(RT(), RA(), RB()));
		}

		switch(RI16()) //0 - 8
		{
		ADD_OPCODE(BRZ,(RT(), exts16(i16())));
		ADD_OPCODE(BRHZ,(RT(), exts16(i16())));
		ADD_OPCODE(BRHNZ,(RT(), exts16(i16())));
		ADD_OPCODE(STQR,(RT(), i16()));
		ADD_OPCODE(BR,(exts16(i16())));
		ADD_OPCODE(FSMBI,(RT(), i16()));
		ADD_OPCODE(BRSL,(RT(), exts16(i16())));
		ADD_OPCODE(IL,(RT(), exts16(i16())));
		ADD_OPCODE(LQR,(RT(), exts16(i16())));
		}

		switch(RI10()) //0 - 7
		{
		ADD_OPCODE(SPU_ORI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(AI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(AHI,(RT(), RA(), exts10(i10())));
		ADD_OPCODE(STQD,(RT(), exts10(i10()) << 4, RA()));
		ADD_OPCODE(LQD,(RT(), exts10(i10()) << 4, RA()));
		ADD_OPCODE(CLGTI,(RT(), RA(), i10()));
		ADD_OPCODE(CLGTHI,(RT(), RA(), i10()));
		ADD_OPCODE(CEQI,(RT(), RA(), i10()));
		}

		switch(RI18()) //0 - 6
		{
		ADD_OPCODE(HBRR,(RO(), exts16(i16())));
		ADD_OPCODE(ILA,(RT(), i18()));
		}

		switch(RRR()) //0 - 3
		{
		ADD_OPCODE(SELB,(RC(), RA(), RB(), RT()));
		ADD_OPCODE(SHUFB,(RC(), RA(), RB(), RT()));
		}
		
		m_op.UNK(m_code, 0, 0);
	}
};

#undef START_OPCODES_GROUP_
#undef START_OPCODES_GROUP
#undef ADD_OPCODE
#undef ADD_NULL_OPCODE
#undef END_OPCODES_GROUP