#include "stdafx.h"
#include "spu_optimizer.h"
#include "spu_pattern.h"

#include "SPUDisAsm.h"

LOG_CHANNEL(spu_optimizer_log);

const extern spu_decoder<spu_itype> g_spu_itype;

#ifdef SPU_OPTIMIZER_DEBUG
#define SPU_OPTIMIZER_REPORT_FAIL(program, index, context) report_fail(program, index, context)
#else
#define SPU_OPTIMIZER_REPORT_FAIL(program, index, context) ((void)0)
#endif

namespace spu_optimizer
{

#ifdef SPU_OPTIMIZER_DEBUG
	std::string dump_program(const spu_program& program, bool optimized = false)
	{
		SPUDisAsm dis_asm(cpu_disasm_mode::dump, reinterpret_cast<const u8*>(optimized ? program.get_optimized_data().data() : program.get_data().data()), program.lower_bound);
		std::string output;

		for (usz i = 0; i < program.get_data().size(); i++)
		{
			dis_asm.disasm((i * 4) + program.lower_bound);
			fmt::append(output, "(%d) %s", i, dis_asm.last_opcode);
		}

		return output;
	}

	void report_fail(const spu_program& program, u32 index, std::string_view context)
	{
		spu_optimizer_log.warning("Failed on %s\nProgram size %d, instruction at %d, dump:\n%s", context, program.get_data().size(), index, dump_program(program));
	}
#endif

	raw_inst::raw_inst(spu_opcode_t op, history_index ra, history_index rb, history_index rc)
		: itype(g_spu_itype.decode(op.opcode)), ra(ra), rb(rb), rc(rc), ref_count(0)
	{
	}

	spu_optimizer::spu_optimizer(spu_program& program)
	{
		// Copy original data to optimized_data (optimizer will modify optimized_data)
		program.optimized_data() = program.get_data();

		history.reserve(program.get_data().size());
		optimize(program);
	}

	void spu_optimizer::add_refs(const raw_inst& inst)
	{
		if (is_instruction(inst.ra))
			history[to_data_index(inst.ra)].ref_count++;
		if (is_instruction(inst.rb))
			history[to_data_index(inst.rb)].ref_count++;
		if (is_instruction(inst.rc))
			history[to_data_index(inst.rc)].ref_count++;
	}

	void spu_optimizer::release_refs(const raw_inst& inst)
	{
		if (is_instruction(inst.ra))
			history[to_data_index(inst.ra)].ref_count--;
		if (is_instruction(inst.rb))
			history[to_data_index(inst.rb)].ref_count--;
		if (is_instruction(inst.rc))
			history[to_data_index(inst.rc)].ref_count--;
	}

	bool spu_optimizer::can_remove(history_index h) const
	{
		if (!is_instruction(h))
			return false;
		return history[to_data_index(h)].ref_count == 0;
	}

	// NOP opcode value
	static constexpr u32 nop_opcode = 0x40200000;

	// Build an RPCS3_OPTIMIZER opcode
	// Encoding: rt4 (bits 21-27) = dest, ra (bits 7-13) = src, rc (bits 0-6) = variant
	static constexpr u32 make_optimizer_opcode(u32 variant, u32 src, u32 dest)
	{
		return (0xa << 28) | ((dest & 0x7f) << 21) | ((src & 0x7f) << 7) | (variant & 0x7f);
	}

	// Build an RPCS3_OPTIMIZER opcode for two-source operations (e.g., division)
	// Encoding: rt4 (bits 21-27) = dest, rb (bits 14-20) = src2, ra (bits 7-13) = src1, rc (bits 0-6) = variant
	static constexpr u32 make_optimizer_opcode_2src(u32 variant, u32 src1, u32 src2, u32 dest)
	{
		return (0xa << 28) | ((dest & 0x7f) << 21) | ((src2 & 0x7f) << 14) | ((src1 & 0x7f) << 7) | (variant & 0x7f);
	}

	// Helper to get the byte-swapped opcode from program data
	static spu_opcode_t get_opcode(const spu_program& program, u32 data_index)
	{
		return spu_opcode_t{std::bit_cast<be_t<u32>>(program.get_data()[data_index])};
	}

	// Helper to write a host-order opcode to program.optimized_data in big-endian format
	static void set_opcode(spu_program& program, u32 data_index, u32 opcode)
	{
		program.optimized_data()[data_index] = std::bit_cast<u32>(be_t<u32>(opcode));
	}

	bool spu_optimizer::try_remove(spu_program& program, history_index h)
	{
		if (!is_instruction(h))
			return false;

		// Only remove if no other instruction references this one
		if (!can_remove(h))
			return false;

		// Release refs from this instruction to its operands
		release_refs(history[to_data_index(h)]);

		// Replace with NOP
		set_opcode(program, to_data_index(h), nop_opcode);
		return true;
	}

	void spu_optimizer::optimize(spu_program& program)
	{
		// Creates history
		for (const auto& op : program.get_data())
		{
			u32 actual_op = std::bit_cast<be_t<u32>>(op);
			(this->*decode(actual_op))({actual_op});
			cur_history_ctr++;
		}

		// Build reference counts - each instruction increments ref_count of instructions it references
		for (const auto& inst : history)
		{
			add_refs(inst);
		}

		// Look for possible patterns starting from the end and simplify with RPCS3_OPTIMIZER opcode
		for (usz i = history.size(); i > 0; i--)
		{
			const auto& inst = history[i - 1];

			if (inst.itype == spu_itype::FMA)
			{
				if (check_accurate_reciprocal_pattern(program, static_cast<u32>(i - 1)))
					continue;

				if (check_sqrt_pattern(program, static_cast<u32>(i - 1)))
					continue;

				if (check_division_pattern(program, static_cast<u32>(i - 1)))
					continue;
			}
		}
	}

	bool spu_optimizer::check_accurate_reciprocal_pattern(spu_program& program, u32 fma_history_idx)
	{
		namespace pat = pattern;

		// Pattern: FMA(FNMS_result, fi_result, fi_result) or FMA(fi_result, FNMS_result, fi_result)
		// inst<1> = FNMS result, inst<2> = FI result (appears twice due to fi + fi * error)
		auto fma_match = pat::match(history[fma_history_idx], pat::FMA(pat::inst<1>, pat::inst<2>, pat::inst<2>));
		if (!fma_match)
		{
			SPU_OPTIMIZER_REPORT_FAIL(program, fma_history_idx, "AccRE(FMA)");
			return false;
		}

		history_index fnms_h = fma_match->get(pat::inst<1>);
		history_index fi_h = fma_match->get(pat::inst<2>);

		// Validate fi_h and fnms_h point to actual instructions
		if (!is_instruction(fi_h) || !is_instruction(fnms_h))
		{
			SPU_OPTIMIZER_REPORT_FAIL(program, fma_history_idx, "AccRE(is_instruction)");
			return false;
		}

		// Match FI: FI(div, frest_result)
		auto fi_match = pat::match(history, fi_h, pat::FI(pat::inst<1>, pat::inst<2>));
		if (!fi_match)
		{
			SPU_OPTIMIZER_REPORT_FAIL(program, fma_history_idx, "AccRE(FI)");
			return false;
		}

		history_index div_h = fi_match->get(pat::inst<1>);
		history_index frest_h = fi_match->get(pat::inst<2>);

		// Match FREST: FREST(div) - must use same div as FI
		auto frest_match = pat::match(history, frest_h, pat::FREST(div_h));
		if (!frest_match)
		{
			SPU_OPTIMIZER_REPORT_FAIL(program, fma_history_idx, "AccRE(FREST)");
			return false;
		}

		// Match FNMS: FNMS(div, fi, const) or FNMS(fi, div, const)
		// Using inst<3> for the constant input
		auto fnms_match = pat::match(history, fnms_h, pat::FNMS(div_h, fi_h, pat::inst<3>));
		if (!fnms_match)
		{
			SPU_OPTIMIZER_REPORT_FAIL(program, fma_history_idx, "AccRE(FNMS)");
			return false;
		}

		history_index const_h = fnms_match->get(pat::inst<3>);

		// Check that const_h is a 1.0f constant (or 1.0f + 1 ULP)
		// For IOHL+ILHU pattern, const_h2 will be set to the ILHU instruction
		const auto [result, const_h2] = check_float_constant(program, const_h);

		if (!result)
		{
			SPU_OPTIMIZER_REPORT_FAIL(program, fma_history_idx, "AccRE(constant)");
			return false;
		}

		// Pattern matched! Now find the actual div GPR register number.
		// Read the FREST instruction's opcode from program.data to get op.ra (the div register)
		const spu_opcode_t frest_op = get_opcode(program, to_data_index(frest_h));
		const u32 div_reg = frest_op.ra;

		// The FMA destination register is rt4 from the FMA opcode
		const spu_opcode_t fma_op = get_opcode(program, fma_history_idx);
		const u32 dest_reg = fma_op.rt4;

		// Replace FMA with RPCS3_OPTIMIZER
		spu_optimizer_log.warning("Found pattern accurate reciprocal!");

		// Release refs from FMA before replacing it
		release_refs(history[fma_history_idx]);
		set_opcode(program, fma_history_idx, make_optimizer_opcode(const_h2 != 0 ? 1 : 0, div_reg, dest_reg));

		// Try to replace intermediate instructions with NOP (only if not referenced elsewhere)
		try_remove(program, fnms_h);
		try_remove(program, fi_h);
		try_remove(program, frest_h);
		try_remove(program, const_h);  // 1.0f constant (IOHL or ILHU)
		try_remove(program, const_h2); // ILHU if const_h was IOHL

#ifdef SPU_OPTIMIZER_DEBUG
		spu_optimizer_log.warning("Before:\n%s", dump_program(program));
		spu_optimizer_log.warning("After:\n%s", dump_program(program, true));
#endif

		return true;
	}

	bool spu_optimizer::check_sqrt_pattern(spu_program& program, u32 fma_history_idx)
	{
		namespace pat = pattern;

		const auto& fma = history[fma_history_idx];

		// Sqrt pattern: FMA(error, half, sqrt_approx)
		// where: error = FNMS(half, rsqrt, 1.0f)
		//        half = FM(sqrt_approx, 0.5f)
		//        sqrt_approx = FM(rsqrt, src)
		//        rsqrt = FRSQEST(src)

		// FMA.rc should be the sqrt_approx (the addend)

		if (!is_instruction(fma.ra) || !is_instruction(fma.rb) || !is_instruction(fma.rc))
			return false;

		history_index sqrt_h = fma.rc;

		// sqrt_approx must be FM(rsqrt, src)
		auto sqrt_match = pat::match(history, sqrt_h, pat::FM(pat::inst<1>, pat::inst<2>));
		if (!sqrt_match)
			return false;

		history_index rsqrt_h = sqrt_match->get(pat::inst<1>);
		history_index src_h = sqrt_match->get(pat::inst<2>);

		if (!is_instruction(rsqrt_h))
			return false;

		// rsqrt must be FRSQEST(src)
		auto rsqrt_match = pat::match(history, rsqrt_h, pat::FRSQEST(src_h));
		if (!rsqrt_match)
			return false;

		// Now check FMA.ra and FMA.rb - one should be FNMS (error), one should be FM with 0.5 (half)
		history_index fnms_h = 0;
		history_index half_h = 0;

		// Try ra = fnms, rb = half
		const auto& inst_a = history[to_data_index(fma.ra)];
		const auto& inst_b = history[to_data_index(fma.rb)];

		if (inst_a.itype == spu_itype::FNMS && inst_b.itype == spu_itype::FM)
		{
			fnms_h = fma.ra;
			half_h = fma.rb;
		}
		else if (inst_a.itype == spu_itype::FM && inst_b.itype == spu_itype::FNMS)
		{
			fnms_h = fma.rb;
			half_h = fma.ra;
		}
		else
		{
			return false;
		}

		// half must be FM(sqrt_approx, 0.5_const)
		auto half_match = pat::match(history, half_h, pat::FM(sqrt_h, pat::inst<3>));
		if (!half_match)
			return false;

		history_index half_const_h = half_match->get(pat::inst<3>);
		if (!check_half_constant(program, half_const_h))
			return false;

		// fnms must be FNMS(half, rsqrt, 1.0f_const) - computes 1.0f - half * rsqrt
		auto fnms_match = pat::match(history, fnms_h, pat::FNMS(half_h, rsqrt_h, pat::inst<4>));
		if (!fnms_match)
			return false;

		history_index one_const_h = fnms_match->get(pat::inst<4>);

		// Check that one_const_h is 1.0f or 1.0f + 1 ULP
		// For IOHL+ILHU pattern, one_const_h2 will be set to the ILHU instruction
		const auto [result, one_const_h2] = check_float_constant(program, one_const_h);

		if (!result)
			return false;

		// Pattern matched! Get the source register from FRSQEST
		const spu_opcode_t frsqest_op = get_opcode(program, to_data_index(rsqrt_h));
		const u32 src_reg = frsqest_op.ra;

		// The FMA destination register is rt4
		const spu_opcode_t fma_op = get_opcode(program, fma_history_idx);
		const u32 dest_reg = fma_op.rt4;

		// Release refs from FMA before replacing it
		release_refs(history[fma_history_idx]);

		// Replace FMA with RPCS3_OPTIMIZER (variant 2 for sqrt with 1.0f, variant 3 for 1.0f + 1 ULP)
		set_opcode(program, fma_history_idx, make_optimizer_opcode(one_const_h2 != 0 ? 3 : 2, src_reg, dest_reg));

		// Try to replace intermediate instructions with NOP (only if not referenced elsewhere)
		try_remove(program, fnms_h);       // FNMS
		try_remove(program, half_h);       // FM (half)
		try_remove(program, sqrt_h);       // FM (sqrt_approx)
		try_remove(program, rsqrt_h);      // FRSQEST
		try_remove(program, half_const_h); // 0.5f constant
		try_remove(program, one_const_h);  // 1.0f constant (IOHL or ILHU)
		try_remove(program, one_const_h2); // ILHU if one_const_h was IOHL

		spu_optimizer_log.success("Found pattern sqrt!");
		return true;
	}

	bool spu_optimizer::check_division_pattern(spu_program& program, u32 fma_history_idx)
	{
		namespace pat = pattern;

		const auto& fma = history[fma_history_idx];

		// Division pattern: FMA(error, re, approx)
		// where: error = FNMS(approx, divb, diva)  ; diva - approx * divb
		//        re = FI(divb, FREST(divb))        ; refined reciprocal of divb
		//        approx = FM(diva, re)             ; initial quotient approximation

		// FMA.rc should be the approx (the addend)
		if (!is_instruction(fma.ra) || !is_instruction(fma.rb) || !is_instruction(fma.rc))
			return false;

		history_index approx_h = fma.rc;

		// approx must be FM - captures the two operands
		auto approx_match = pat::match(history, approx_h, pat::FM(pat::inst<1>, pat::inst<2>));
		if (!approx_match)
			return false;

		// One operand should be re (FI), the other should be diva
		history_index approx_op1_h = approx_match->get(pat::inst<1>);
		history_index approx_op2_h = approx_match->get(pat::inst<2>);

		// Now check FMA.ra and FMA.rb - one should be FNMS (error), one should be FI (re)
		history_index fnms_h = 0;
		history_index re_h = 0;

		const auto& inst_a = history[to_data_index(fma.ra)];
		const auto& inst_b = history[to_data_index(fma.rb)];

		if (inst_a.itype == spu_itype::FNMS && inst_b.itype == spu_itype::FI)
		{
			fnms_h = fma.ra;
			re_h = fma.rb;
		}
		else if (inst_a.itype == spu_itype::FI && inst_b.itype == spu_itype::FNMS)
		{
			fnms_h = fma.rb;
			re_h = fma.ra;
		}
		else
		{
			return false;
		}

		// re must be one of the FM operands
		history_index diva_h = 0;
		if (re_h == approx_op1_h)
		{
			diva_h = approx_op2_h;
		}
		else if (re_h == approx_op2_h)
		{
			diva_h = approx_op1_h;
		}
		else
		{
			return false;
		}

		// Match FI: FI(divb, frest_result)
		auto re_match = pat::match(history, re_h, pat::FI(pat::inst<3>, pat::inst<4>));
		if (!re_match)
			return false;

		history_index divb_h = re_match->get(pat::inst<3>);
		history_index frest_h = re_match->get(pat::inst<4>);

		if (!is_instruction(frest_h))
			return false;

		// FREST must use the same divb
		auto frest_match = pat::match(history, frest_h, pat::FREST(divb_h));
		if (!frest_match)
			return false;

		// FNMS: error = diva - approx * divb
		// FNMS.rc = diva, FNMS.ra/rb = {approx, divb}
		auto fnms_match = pat::match(history, fnms_h, pat::FNMS(approx_h, divb_h, diva_h));
		if (!fnms_match)
			return false;

		// Pattern matched! Get the register numbers
		// divb from FREST instruction (FREST reads divb)
		const spu_opcode_t frest_op = get_opcode(program, to_data_index(frest_h));
		const u32 divb_reg = frest_op.ra;

		// diva from FNMS.rc (FNMS computes: rc - ra*rb, so rc = diva)
		const spu_opcode_t fnms_op = get_opcode(program, to_data_index(fnms_h));
		const u32 diva_reg = fnms_op.rc;

		// The FMA destination register is rt4
		const spu_opcode_t fma_op = get_opcode(program, fma_history_idx);
		const u32 dest_reg = fma_op.rt4;

		// Release refs from FMA before replacing it
		release_refs(history[fma_history_idx]);

		// Replace FMA with RPCS3_OPTIMIZER variant 4 (fast division)
		set_opcode(program, fma_history_idx, make_optimizer_opcode_2src(4, diva_reg, divb_reg, dest_reg));

		// Try to replace intermediate instructions with NOP (only if not referenced elsewhere)
		// Order matters: fnms references approx, approx references re, re references frest
		try_remove(program, fnms_h);   // FNMS (references approx, divb, diva)
		try_remove(program, approx_h); // FM (references re, diva)
		try_remove(program, re_h);     // FI (references frest, divb)
		try_remove(program, frest_h);  // FREST (references divb)

		spu_optimizer_log.warning("Found pattern division!");
		return true;
	}

	std::pair<bool, history_index> spu_optimizer::check_float_constant(spu_program& program, history_index h)
	{
		history_index second_h = 0;

		if (!is_instruction(h))
			return {false, 0};

		const auto& inst = history[to_data_index(h)];

		// Case 1: ILHU loads upper 16 bits, lower 16 bits are zero
		// 1.0f = 0x3F800000 → ILHU 0x3F80
		if (inst.itype == spu_itype::ILHU)
		{
			// No input history to check, just need the immediate value
			// ILHU sets each 32-bit word to (i16 << 16)
			const spu_opcode_t op = get_opcode(program, to_data_index(h));
			return op.i16 == 0x3F80 ? std::pair(true, 0) : std::pair(false, 0);
		}

		// Case 2: IOHL applied to an ILHU result
		// 1.0f + 1 ULP = 0x3F800001 → ILHU 0x3F80 then IOHL 0x0001
		if (inst.itype == spu_itype::IOHL)
		{
			const spu_opcode_t op = get_opcode(program, to_data_index(h));
			if (op.i16 == 0x0001)
			{
				// Check that the input to IOHL is ILHU(0x3F80)
				history_index iohl_input_h = inst.ra;
				if (is_instruction(iohl_input_h))
				{
					const auto& ilhu_inst = history[to_data_index(iohl_input_h)];
					if (ilhu_inst.itype == spu_itype::ILHU)
					{
						const spu_opcode_t ilhu_op = get_opcode(program, to_data_index(iohl_input_h));
						if (ilhu_op.i16 == 0x3F80)
						{
							// Return the ILHU instruction index so caller can try to remove it too
							second_h = iohl_input_h;
							return {true, second_h};
						}
					}
				}
			}
			return {false, 0};
		}

		return {false, 0};
	}

	bool spu_optimizer::check_half_constant(spu_program& program, history_index h)
	{
		if (!is_instruction(h))
			return false;

		const auto& inst = history[to_data_index(h)];

		// 0.5f = 0x3F000000 → ILHU 0x3F00
		if (inst.itype == spu_itype::ILHU)
		{
			const spu_opcode_t op = get_opcode(program, to_data_index(h));
			return op.i16 == 0x3F00;
		}

		return false;
	}

	const spu_decoder<spu_optimizer> s_spu_optimizer;

	decltype(&spu_optimizer::UNK) spu_optimizer::decode(u32 op)
	{
		return s_spu_optimizer.decode(op);
	}

	// Affects spu.status, spu.ch_out_mbox; control flow
	void spu_optimizer::STOP(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
	}

	void spu_optimizer::LNOP(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
	}

	// Affects memory fence
	void spu_optimizer::SYNC(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
	}

	// Affects memory fence
	void spu_optimizer::DSYNC(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
	}

	// Affects SPR (reads as zero)
	void spu_optimizer::MFSPR(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects channels
	void spu_optimizer::RDCH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects channels
	void spu_optimizer::RCHCNT(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SF(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::OR(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::BG(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SFH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::NOR(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ABSDB(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROT(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTM(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTMA(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SHL(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTHM(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTMAH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SHLH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTMI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTMAI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SHLI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTHI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTHMI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTMAHI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SHLHI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::A(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::AND(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CG(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::AH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::NAND(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::AVGB(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects SPR (writes are ignored but still tracked)
	void spu_optimizer::MTSPR(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt]));
	}

	// Affects channels
	void spu_optimizer::WRCH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt]));
	}

	// Affects control flow; reads rt (condition), ra (target)
	void spu_optimizer::BIZ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt], gpr_history[op.ra]));
	}

	// Affects control flow; reads rt (condition), ra (target)
	void spu_optimizer::BINZ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt], gpr_history[op.ra]));
	}

	// Affects control flow; reads rt (condition), ra (target)
	void spu_optimizer::BIHZ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt], gpr_history[op.ra]));
	}

	// Affects control flow; reads rt (condition), ra (target)
	void spu_optimizer::BIHNZ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt], gpr_history[op.ra]));
	}

	// Affects control flow
	void spu_optimizer::STOPD(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
	}

	// Affects memory (store); reads ra, rb (address), rt (data)
	void spu_optimizer::STQX(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt], gpr_history[op.ra], gpr_history[op.rb]));
	}

	// Affects control flow; reads ra (target)
	void spu_optimizer::BI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
	}

	// Affects control flow; reads ra (target), writes rt (link)
	void spu_optimizer::BISL(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects control flow; reads srr0
	void spu_optimizer::IRET(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
	}

	// Affects control flow; reads ra (target), writes rt (link)
	void spu_optimizer::BISLED(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::HBR(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
	}

	void spu_optimizer::GB(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::GBH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::GBB(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FSM(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FSMH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FSMB(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FREST(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FRSQEST(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects memory (load); reads ra, rb (address)
	void spu_optimizer::LQX(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTQBYBI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTQMBYBI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SHLQBYBI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CBX(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CHX(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CWX(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CDX(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTQBI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTQMBI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SHLQBI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTQBY(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTQMBY(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SHLQBY(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ORX(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CBD(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CHD(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CWD(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CDD(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTQBII(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTQMBII(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SHLQBII(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTQBYI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ROTQMBYI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SHLQBYI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::NOP(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
	}

	void spu_optimizer::CGT(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::XOR(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CGTH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::EQV(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CGTB(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SUMB(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects control flow (conditional halt)
	void spu_optimizer::HGT(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
	}

	void spu_optimizer::CLZ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::XSWD(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::XSHW(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CNTB(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::XSBH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CLGT(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ANDC(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FCGT(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::DFCGT(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FA(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FS(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FM(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CLGTH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ORC(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FCMGT(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::DFCMGT(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::DFA(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::DFS(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::DFM(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CLGTB(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects control flow (conditional halt)
	void spu_optimizer::HLGT(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
	}

	// Reads rt as accumulator; rt = (ra * rb) + rt
	void spu_optimizer::DFMA(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rt]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Reads rt as accumulator; rt = (ra * rb) - rt
	void spu_optimizer::DFMS(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rt]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Reads rt as accumulator; rt = -((ra * rb) - rt)
	void spu_optimizer::DFNMS(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rt]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Reads rt as accumulator; rt = -((ra * rb) + rt)
	void spu_optimizer::DFNMA(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rt]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CEQ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::MPYHHU(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Reads rt as accumulator; rt = ra + rb + (rt & 1)
	void spu_optimizer::ADDX(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rt]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Reads rt as accumulator; rt = rb - ra - (~rt & 1)
	void spu_optimizer::SFX(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rt]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Reads rt as accumulator; carry from ra + rb + (rt & 1)
	void spu_optimizer::CGX(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rt]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Reads rt as accumulator; borrow from rb - ra - (1 - (rt & 1))
	void spu_optimizer::BGX(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rt]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Reads rt as accumulator; rt += ra[16:31] * rb[16:31]
	void spu_optimizer::MPYHHA(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rt]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Reads rt as accumulator; rt += unsigned(ra[16:31] * rb[16:31])
	void spu_optimizer::MPYHHAU(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rt]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects FPSCR (reads it into rt)
	void spu_optimizer::FSCRRD(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FESD(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FRDS(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects FPSCR (writes ra into it)
	void spu_optimizer::FSCRWR(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
	}

	void spu_optimizer::DFTSV(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FCEQ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::DFCEQ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::MPY(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::MPYH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::MPYHH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::MPYS(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CEQH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FCMEQ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::DFCMEQ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::MPYU(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CEQB(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::FI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects control flow (conditional halt)
	void spu_optimizer::HEQ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb]));
	}

	void spu_optimizer::CFLTS(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CFLTU(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CSFLT(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CUFLT(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects control flow; reads rt (condition)
	void spu_optimizer::BRZ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt]));
	}

	// Affects memory (store); reads rt (data)
	void spu_optimizer::STQA(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt]));
	}

	// Affects control flow; reads rt (condition)
	void spu_optimizer::BRNZ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt]));
	}

	// Affects control flow; reads rt (condition)
	void spu_optimizer::BRHZ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt]));
	}

	// Affects control flow; reads rt (condition)
	void spu_optimizer::BRHNZ(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt]));
	}

	// Affects memory (store); reads rt (data)
	void spu_optimizer::STQR(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt]));
	}

	// Affects control flow
	void spu_optimizer::BRA(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
	}

	// Affects memory (load)
	void spu_optimizer::LQA(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects control flow; writes rt (link)
	void spu_optimizer::BRASL(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects control flow
	void spu_optimizer::BR(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
	}

	void spu_optimizer::FSMBI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects control flow; writes rt (link)
	void spu_optimizer::BRSL(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects memory (load)
	void spu_optimizer::LQR(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::IL(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ILHU(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ILH(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Reads rt as accumulator; rt = rt | i16
	void spu_optimizer::IOHL(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ORI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));

		// ORI with immediate 0 is a register move; propagate source history directly
		if (op.si10 == 0)
			gpr_history[op.rt] = gpr_history[op.ra];
		else
			gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ORHI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ORBI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SFI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::SFHI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ANDI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ANDHI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::ANDBI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::AI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::AHI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects memory (store); reads ra (address), rt (data)
	void spu_optimizer::STQD(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.rt], gpr_history[op.ra]));
	}

	// Affects memory (load); reads ra (address)
	void spu_optimizer::LQD(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::XORI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::XORHI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::XORBI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CGTI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CGTHI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CGTBI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects control flow (conditional halt)
	void spu_optimizer::HGTI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
	}

	void spu_optimizer::CLGTI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CLGTHI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CLGTBI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects control flow (conditional halt)
	void spu_optimizer::HLGTI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
	}

	void spu_optimizer::MPYI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::MPYUI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CEQI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CEQHI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	void spu_optimizer::CEQBI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// Affects control flow (conditional halt)
	void spu_optimizer::HEQI(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra]));
	}

	void spu_optimizer::HBRA(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
	}

	void spu_optimizer::HBRR(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
	}

	void spu_optimizer::ILA(spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		gpr_history[op.rt] = cur_history_ctr;
	}

	// 4-operand: rt4 = (rc & rb) | (~rc & ra)
	void spu_optimizer::SELB(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rc]));
		gpr_history[op.rt4] = cur_history_ctr;
	}

	// 4-operand: rt4 = shuffle(ra, rb, rc)
	void spu_optimizer::SHUFB(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rc]));
		gpr_history[op.rt4] = cur_history_ctr;
	}

	// 4-operand: rt4 = rc + (ra * rb)
	void spu_optimizer::MPYA(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rc]));
		gpr_history[op.rt4] = cur_history_ctr;
	}

	// 4-operand: rt4 = rc - (ra * rb)
	void spu_optimizer::FNMS(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rc]));
		gpr_history[op.rt4] = cur_history_ctr;
	}

	// 4-operand: rt4 = (ra * rb) + rc
	void spu_optimizer::FMA(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rc]));
		gpr_history[op.rt4] = cur_history_ctr;
	}

	// 4-operand: rt4 = (ra * rb) - rc
	void spu_optimizer::FMS(spu_opcode_t op)
	{
		history.push_back(raw_inst(op, gpr_history[op.ra], gpr_history[op.rb], gpr_history[op.rc]));
		gpr_history[op.rt4] = cur_history_ctr;
	}

	void spu_optimizer::UNK([[maybe_unused]] spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		// fmt::throw_exception("Unknown instruction encountered in spu optimizer!");
	}

	void spu_optimizer::RPCS3_OPTIMIZER([[maybe_unused]] spu_opcode_t op)
	{
		history.push_back(raw_inst(op));
		// fmt::throw_exception("Trying to optimize a program already containing an optimized instruction!");
	}

} // namespace spu_optimizer
