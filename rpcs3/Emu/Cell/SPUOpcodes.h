#pragma once

#define OP_REG const u32
#define OP_sIMM const s32
#define OP_uIMM const u32
#define START_OPCODES_GROUP(x) /*x*/
#define ADD_OPCODE(name, regs) virtual void(##name##)##regs##=0
#define ADD_NULL_OPCODE(name) virtual void(##name##)()=0
#define END_OPCODES_GROUP(x) /*x*/

enum SPU_0_10_Opcodes
{
	STOP = 0x0,
	LNOP = 0x1,
	RDCH = 0xd,
	RCHCNT = 0xf,
	SF = 0x40,
	SHLI = 0x7b,
	A = 0xc0,
	SPU_AND = 0xc1,
	LQX = 0x1c4,
	WRCH = 0x10d,
	STQX = 0x144,
	BI = 0x1a8,
	BISL = 0x1a9,
	HBR = 0x1ac,
	CWX = 0x1d6,
	ROTQBY = 0x1dc,
	ROTQBYI = 0x1fc,
	SHLQBYI = 0x1ff,
	SPU_NOP = 0x201,
	CLGT = 0x2c0,
};

enum SPU_0_8_Opcodes
{
	BRZ = 0x40,
	BRHZ = 0x44,
	BRHNZ = 0x46,
	STQR = 0x47,
	BR = 0x64,
	FSMBI = 0x65,
	BRSL = 0x66,
	LQR = 0x67,
	IL = 0x81,
};

enum SPU_0_7_Opcodes
{
	SPU_ORI = 0x4,
	AI = 0x1c,
	AHI = 0x1d,
	STQD = 0x24,
	LQD = 0x34,
	CLGTI = 0x5c,
	CLGTHI = 0x5d,
	CEQI = 0x7c,
};

enum SPU_0_6_Opcodes
{
	HBRR = 0x9,
	ILA = 0x21,
};

enum SPU_0_3_Opcodes
{
	SELB = 0x8,
	SHUFB = 0xb,
};

class SPU_Opcodes
{
public:
	static u32 branchTarget(const u64 pc, const s32 imm)
	{
		return (pc + ((imm << 2) & ~0x3)) & 0x3fff0;
    }
	
	virtual void Exit()=0;

/*
- 1:	Unk/0xb0c2c58c ->  11c:	b0 c2 c5 8c 	shufb	$6,$11,$11,$12
- 2:	Unk/0x5800c505 ->  124:	58 00 c5 05 	clgt	$5,$10,$3
- 3:	Unk/0x5c000184 ->  128:	5c 00 01 84 	clgti	$4,$3,0
- 4:	Unk/0x18210282 ->  12c:	18 21 02 82 	and		$2,$5,$4
- 5:	Unk/0x00000000 ->  154:	00 00 00 00 	stop
- 6:	Unk/0x00200000 ->  1b4:	00 20 00 00 	lnop
- 7:	Unk/0x0f608487 ->  1c0:	0f 60 84 87 	shli	$7,$9,2
- 8:	Unk/0x18140384 ->  1c8:	18 14 03 84 	a		$4,$7,$80
- 9:	Unk/0x38940386 ->  1cc:	38 94 03 86 	lqx		$6,$7,$80
- 10:	Unk/0x3b810305 ->  1d0:	3b 81 03 05 	rotqby	$5,$6,$4
- 11:	Unk/0x35200280 ->  1d4:	35 20 02 80 	bisl	$0,$5
- 12:	Unk/0x237ff982 ->  1e4:	23 7f f9 82 	brhnz	$2,0x1b0
- 13:	Unk/0x35800014 ->  204:	35 80 00 14 	hbr		0x254,$0
- 14:	Unk/0x5d13c482 ->  224:	5d 13 c4 82 	clgthi	$2,$9,79
- 15:	Unk/0x3ac1828d ->  240:	3a c1 82 8d 	cwx		$13,$5,$6
- 16:	Unk/0x2881828b ->  24c:	28 81 82 8b 	stqx	$11,$5,$6
- 17:	Unk/0x21a00b82 ->  25c:	21 a0 0b 82 	wrch	$ch23,$2
- 18:	Unk/0x01e00c05 ->  260:	01 e0 0c 05 	rchcnt	$5,$ch24
- 19:	Unk/0x7c004284 ->  264:	7c 00 42 84 	ceqi	$4,$5,1
- 20:	Unk/0x207ffe84 ->  26c:	20 7f fe 84 	brz		$4,0x260
- 21:	Unk/0x01a00c03 ->  27c:	01 a0 0c 03 	rdch	$3,$ch24
*/
	//0 - 10
	ADD_OPCODE(STOP,(OP_uIMM code));
	ADD_OPCODE(LNOP,());
	ADD_OPCODE(RDCH,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(RCHCNT,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(SF,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(SHLI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(A,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(SPU_AND,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(LQX,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(WRCH,(OP_REG ra, OP_REG rt));
	ADD_OPCODE(STQX,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(BI,(OP_REG ra));
	ADD_OPCODE(BISL,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(HBR,(OP_REG p, OP_REG ro, OP_REG ra));
	ADD_OPCODE(CWX,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTQBY,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTQBYI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(SHLQBYI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(SPU_NOP,(OP_REG rt));
	ADD_OPCODE(CLGT,(OP_REG rt, OP_REG ra, OP_REG rb));

	//0 - 8
	ADD_OPCODE(BRZ,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(BRHZ,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(BRHNZ,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(STQR,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(BR,(OP_sIMM i16));
	ADD_OPCODE(FSMBI,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(BRSL,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(IL,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(LQR,(OP_REG rt, OP_sIMM i16));

	//0 - 7
	ADD_OPCODE(SPU_ORI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(AI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(AHI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(STQD,(OP_REG rt, OP_sIMM i10, OP_REG ra));
	ADD_OPCODE(LQD,(OP_REG rt, OP_sIMM i10, OP_REG ra));
	ADD_OPCODE(CLGTI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(CLGTHI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(CEQI,(OP_REG rt, OP_REG ra, OP_sIMM i10));

	//0 - 6
	ADD_OPCODE(HBRR,(OP_sIMM ro, OP_sIMM i16));
	ADD_OPCODE(ILA,(OP_REG rt, OP_sIMM i18));

	//0 - 3
	ADD_OPCODE(SELB,(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt));
	ADD_OPCODE(SHUFB,(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt));

	ADD_OPCODE(UNK,(const s32 code, const s32 opcode, const s32 gcode));
};

#undef START_OPCODES_GROUP
#undef ADD_OPCODE
#undef ADD_NULL_OPCODE
#undef END_OPCODES_GROUP