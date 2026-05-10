#pragma once

#include "Emu/CPU/CPUDisAsm.h"

class PPCDisAsm : public CPUDisAsm
{
protected:
	PPCDisAsm(cpu_disasm_mode mode, const u8* offset, u32 start_pc = 0) : CPUDisAsm(mode, offset, start_pc)
	{
	}

	virtual u32 DisAsmBranchTarget(const s32 imm) override = 0;

	usz insert_char_if(usz pos, bool insert, char c)
	{
		if (!insert)
		{
			return pos;
		}

		ensure(std::exchange(last_opcode[pos], c) == ' ' && last_opcode[pos + 1] == ' ');
		return pos + 1;
	}

	usz insert_char_if(std::string_view op, bool insert, char c = '.')
	{
		return insert_char_if(op.size(), insert, c);
	}

	void DisAsm_V4(std::string_view op, u32 v0, u32 v1, u32 v2, u32 v3)
	{
		fmt::append(last_opcode, "%-*s v%d,v%d,v%d,v%d", PadOp(), op, v0, v1, v2, v3);
	}
	void DisAsm_V3_UIMM(std::string_view op, u32 v0, u32 v1, u32 v2, u32 uimm)
	{
		fmt::append(last_opcode, "%-*s v%d,v%d,v%d,%s", PadOp(), op, v0, v1, v2, uimm);
	}
	void DisAsm_V3(std::string_view op, u32 v0, u32 v1, u32 v2)
	{
		fmt::append(last_opcode, "%-*s v%d,v%d,v%d", PadOp(), op, v0, v1, v2);
	}
	void DisAsm_V2_UIMM(std::string_view op, u32 v0, u32 v1, u32 uimm)
	{
		fmt::append(last_opcode, "%-*s v%d,v%d,%s", PadOp(), op, v0, v1, uimm);
	}
	void DisAsm_V2(std::string_view op, u32 v0, u32 v1)
	{
		fmt::append(last_opcode, "%-*s v%d,v%d", PadOp(), op, v0, v1);
	}
	void DisAsm_V1_SIMM(std::string_view op, u32 v0, s32 simm)
	{
		fmt::append(last_opcode, "%-*s v%d,%s", PadOp(), op, v0, SignedHex(simm));
	}
	void DisAsm_V1(std::string_view op, u32 v0)
	{
		fmt::append(last_opcode, "%-*s v%d", PadOp(), op, v0);
	}
	void DisAsm_V1_R2(std::string_view op, u32 v0, u32 r1, u32 r2)
	{
		fmt::append(last_opcode, "%-*s v%d,r%d,r%d", PadOp(), op, v0, r1, r2);
	}
	void DisAsm_CR1_F2_RC(std::string_view op, u32 cr0, u32 f0, u32 f1, u32 rc)
	{
		fmt::append(last_opcode, "%-*s cr%d,f%d,f%d", PadOp(op, rc ? 1 : 0), op, cr0, f0, f1);
		insert_char_if(op, !!rc);
	}
	void DisAsm_CR1_F2(std::string_view op, u32 cr0, u32 f0, u32 f1)
	{
		DisAsm_CR1_F2_RC(op, cr0, f0, f1, false);
	}
	void DisAsm_INT1_R2(std::string_view op, u32 i0, u32 r0, u32 r1)
	{
		fmt::append(last_opcode, "%-*s %d,r%d,r%d", PadOp(), op, i0, r0, r1);
	}
	void DisAsm_INT1_R1_IMM(std::string_view op, u32 i0, u32 r0, s32 imm0)
	{
		fmt::append(last_opcode, "%-*s %d,r%d,%s", PadOp(), op, i0, r0, SignedHex(imm0));
	}
	void DisAsm_INT1_R1_RC(std::string_view op, u32 i0, u32 r0, u32 rc)
	{
		fmt::append(last_opcode, "%-*s %d,r%d", PadOp(op, rc ? 1 : 0), op, i0, r0);
		insert_char_if(op, !!rc);
	}
	void DisAsm_INT1_R1(std::string_view op, u32 i0, u32 r0)
	{
		DisAsm_INT1_R1_RC(op, i0, r0, false);
	}
	void DisAsm_F4_RC(std::string_view op, u32 f0, u32 f1, u32 f2, u32 f3, u32 rc)
	{
		fmt::append(last_opcode, "%-*s f%d,f%d,f%d,f%d", PadOp(op, rc ? 1 : 0), op, f0, f1, f2, f3);
		insert_char_if(op, !!rc);
	}
	void DisAsm_F3_RC(std::string_view op, u32 f0, u32 f1, u32 f2, u32 rc)
	{
		fmt::append(last_opcode, "%-*s f%d,f%d,f%d", PadOp(op, rc ? 1 : 0), op, f0, f1, f2);
		insert_char_if(op, !!rc);
	}
	void DisAsm_F3(std::string_view op, u32 f0, u32 f1, u32 f2)
	{
		DisAsm_F3_RC(op, f0, f1, f2, false);
	}
	void DisAsm_F2_RC(std::string_view op, u32 f0, u32 f1, u32 rc)
	{
		fmt::append(last_opcode, "%-*s f%d,f%d", PadOp(op, rc ? 1 : 0), op, f0, f1);
		insert_char_if(op, !!rc);
	}
	void DisAsm_F2(std::string_view op, u32 f0, u32 f1)
	{
		DisAsm_F2_RC(op, f0, f1, false);
	}
	void DisAsm_F1_R2(std::string_view op, u32 f0, u32 r0, u32 r1)
	{
		if(m_mode == cpu_disasm_mode::compiler_elf)
		{
			fmt::append(last_opcode, "%-*s f%d,r%d,r%d", PadOp(), op, f0, r0, r1);
			return;
		}

		fmt::append(last_opcode, "%-*s f%d,r%d(r%d)", PadOp(), op, f0, r0, r1);
	}
	void DisAsm_F1_IMM_R1_RC(std::string_view op, u32 f0, s32 imm0, u32 r0, u32 rc)
	{
		if(m_mode == cpu_disasm_mode::compiler_elf)
		{
			fmt::append(last_opcode, "%-*s f%d,r%d,%s", PadOp(op, rc ? 1 : 0), op, f0, r0, SignedHex(imm0));
			insert_char_if(op, !!rc);
			return;
		}

		fmt::append(last_opcode, "%-*s f%d,%s(r%d)", PadOp(op, rc ? 1 : 0), op, f0, SignedHex(imm0), r0);
		insert_char_if(op, !!rc);
	}
	void DisAsm_F1_IMM_R1(std::string_view op, u32 f0, s32 imm0, u32 r0)
	{
		DisAsm_F1_IMM_R1_RC(op, f0, imm0, r0, false);
	}
	void DisAsm_F1_RC(std::string_view op, u32 f0, u32 rc)
	{
		fmt::append(last_opcode, "%-*s f%d", PadOp(op, rc ? 1 : 0), op, f0);
		insert_char_if(op, !!rc);
	}
	void DisAsm_R1_RC(std::string_view op, u32 r0, u32 rc)
	{
		fmt::append(last_opcode, "%-*s r%d", PadOp(op, rc ? 1 : 0), op, r0);
		insert_char_if(op, !!rc);
	}
	void DisAsm_R1(std::string_view op, u32 r0)
	{
		DisAsm_R1_RC(op, r0, false);
	}
	void DisAsm_R2_OE_RC(std::string_view op, u32 r0, u32 r1, u32 oe, u32 rc)
	{
		fmt::append(last_opcode, "%-*s r%d,r%d", PadOp(op, (rc ? 1 : 0) + (oe ? 1 : 0)), op, r0, r1);
		insert_char_if(insert_char_if(op, !!oe, 'o'), !!rc, '.');
	}
	void DisAsm_R2_RC(std::string_view op, u32 r0, u32 r1, u32 rc)
	{
		DisAsm_R2_OE_RC(op, r0, r1, false, rc);
	}
	void DisAsm_R2(std::string_view op, u32 r0, u32 r1)
	{
		DisAsm_R2_RC(op, r0, r1, false);
	}
	void DisAsm_R3_OE_RC(std::string_view op, u32 r0, u32 r1, u32 r2, u32 oe, u32 rc)
	{
		fmt::append(last_opcode, "%-*s r%d,r%d,r%d", PadOp(op, (rc ? 1 : 0) + (oe ? 1 : 0)), op, r0, r1, r2);
		insert_char_if(insert_char_if(op, !!oe, 'o'), !!rc, '.');
	}
	void DisAsm_R3_INT2_RC(std::string_view op, u32 r0, u32 r1, u32 r2, s32 i0, s32 i1, u32 rc)
	{
		fmt::append(last_opcode, "%-*s r%d,r%d,r%d,%d,%d", PadOp(op, rc ? 1 : 0), op, r0, r1, r2, i0, i1);
		insert_char_if(op, !!rc);
	}
	void DisAsm_R3_RC(std::string_view op, u32 r0, u32 r1, u32 r2, u32 rc)
	{
		DisAsm_R3_OE_RC(op, r0, r1, r2, false, rc);
	}
	void DisAsm_R3(std::string_view op, u32 r0, u32 r1, u32 r2)
	{
		DisAsm_R3_RC(op, r0, r1, r2, false);
	}
	void DisAsm_R2_INT3_RC(std::string_view op, u32 r0, u32 r1, s32 i0, s32 i1, s32 i2, u32 rc)
	{
		fmt::append(last_opcode, "%-*s r%d,r%d,%d,%d,%d", PadOp(op, rc ? 1 : 0), op, r0, r1, i0, i1, i2);
		insert_char_if(op, !!rc);
	}
	void DisAsm_R2_INT3(std::string_view op, u32 r0, u32 r1, s32 i0, s32 i1, s32 i2)
	{
		DisAsm_R2_INT3_RC(op, r0, r1, i0, i1, i2, false);
	}
	void DisAsm_R2_INT2_RC(std::string_view op, u32 r0, u32 r1, s32 i0, s32 i1, u32 rc)
	{
		fmt::append(last_opcode, "%-*s r%d,r%d,%d,%d", PadOp(op, rc ? 1 : 0), op, r0, r1, i0, i1);
		insert_char_if(op, !!rc);
	}
	void DisAsm_R2_INT2(std::string_view op, u32 r0, u32 r1, s32 i0, s32 i1)
	{
		DisAsm_R2_INT2_RC(op, r0, r1, i0, i1, false);
	}
	void DisAsm_R2_INT1_RC(std::string_view op, u32 r0, u32 r1, s32 i0, u32 rc)
	{
		fmt::append(last_opcode, "%-*s r%d,r%d,%d", PadOp(op, rc ? 1 : 0), op, r0, r1, i0);
		insert_char_if(op, !!rc);
	}
	void DisAsm_R2_INT1(std::string_view op, u32 r0, u32 r1, s32 i0)
	{
		DisAsm_R2_INT1_RC(op, r0, r1, i0, false);
	}
	void DisAsm_R2_IMM(std::string_view op, u32 r0, u32 r1, s32 imm0)
	{
		if(m_mode == cpu_disasm_mode::compiler_elf)
		{
			fmt::append(last_opcode, "%-*s r%d,r%d,%s", PadOp(), op, r0, r1, SignedHex(imm0));
			return;
		}

		fmt::append(last_opcode, "%-*s r%d,%s(r%d)", PadOp(), op, r0, SignedHex(imm0), r1);
	}
	void DisAsm_R1_IMM(std::string_view op, u32 r0, s32 imm0)
	{
		fmt::append(last_opcode, "%-*s r%d,%s", PadOp(), op, r0, SignedHex(imm0));
	}
	void DisAsm_IMM_R1(std::string_view op, s32 imm0, u32 r0)
	{
		fmt::append(last_opcode, "%-*s %d,r%d  #%x", PadOp(), op, imm0, r0, imm0);
	}
	void DisAsm_CR1_R1_IMM(std::string_view op, u32 cr0, u32 r0, s32 imm0)
	{
		fmt::append(last_opcode, "%-*s cr%d,r%d,%s", PadOp(), op, cr0, r0, SignedHex(imm0));
	}
	void DisAsm_CR1_R2_RC(std::string_view op, u32 cr0, u32 r0, u32 r1, u32 rc)
	{
		fmt::append(last_opcode, "%-*s cr%d,r%d,r%d", PadOp(op, rc ? 1 : 0), op, cr0, r0, r1);
		insert_char_if(op, !!rc);
	}
	void DisAsm_CR1_R2(std::string_view op, u32 cr0, u32 r0, u32 r1)
	{
		DisAsm_CR1_R2_RC(op, cr0, r0, r1, false);
	}
	void DisAsm_CR2(std::string_view op, u32 cr0, u32 cr1)
	{
		fmt::append(last_opcode, "%-*s cr%d,cr%d", PadOp(), op, cr0, cr1);
	}
	void DisAsm_INT3(std::string_view op, s32 i0, s32 i1, s32 i2)
	{
		fmt::append(last_opcode, "%-*s %d,%d,%d", PadOp(), op, i0, i1, i2);
	}
	void DisAsm_INT1(std::string_view op, s32 i0)
	{
		fmt::append(last_opcode, "%-*s %d", PadOp(), op, i0);
	}
	void DisAsm_BRANCH(std::string_view op, s32 pc)
	{
		fmt::append(last_opcode, "%-*s 0x%x", PadOp(), op, DisAsmBranchTarget(pc));
	}
	void DisAsm_BRANCH_A(std::string_view op, s32 pc)
	{
		fmt::append(last_opcode, "%-*s 0x%x", PadOp(), op, pc);
	}
	void DisAsm_B2_BRANCH(std::string_view op, u32 b0, u32 b1, s32 pc)
	{
		fmt::append(last_opcode, "%-*s %d,%d,0x%x ", PadOp(), op, b0, b1, DisAsmBranchTarget(pc));
	}
	void DisAsm_CR_BRANCH(std::string_view op, u32 cr, s32 pc)
	{
		fmt::append(last_opcode, "%-*s cr%d,0x%x ", PadOp(), op, cr, DisAsmBranchTarget(pc));
	}
	void DisAsm_CR_BRANCH_HINT(std::string_view op, u32 cr, u32 bh)
	{
		fmt::append(last_opcode, "%-*s cr%d,0x%x ", PadOp(), op, cr, bh);
	}
};
