#include <gtest/gtest.h>

#include <array>
#include <bit>
#include <ios>
#include <map>
#include <vector>

#include "util/types.hpp"
#include "Emu/Cell/SPURecompiler.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/system_config.h"
#include "Emu/system_config_types.h"

// Giga SPU analyser regression: a brsl whose return address is a stop-trap leaves
// a dangling target edge after block cleanup, which the reg-state walk must not
// dereference. The SPU program below is made-up data (not from any game).
namespace
{
	constexpr u32 SPU_STOP = 0x00000000u; // stop 0x0 — the no-return trap word

	// RI16: il rt, i16  (opcode 0x081)
	constexpr u32 enc_il(u32 rt, u32 imm)
	{
		return (0x081u << 23) | ((imm & 0xffff) << 7) | (rt & 0x7f);
	}

	// RI16: brsl rt, target  (opcode 0x066). Branch-relative-and-set-link (call).
	constexpr u32 enc_brsl(u32 pos, u32 target)
	{
		const u32 rel = ((target - pos) / 4) & 0xffff;
		return (0x066u << 23) | (rel << 7) | 0u /* rt = $lr ($0) */;
	}

	// RR: bi $0  (opcode 0x1a8) — branch indirect to $lr, i.e. function return.
	constexpr u32 enc_bi_lr()
	{
		return 0x1a8u << 21;
	}
}

TEST(SpuAnalyserGiga, ReturnToStopTrapDoesNotRangeCheckFail)
{
	const auto saved = g_cfg.core.spu_block_size.get();
	g_cfg.core.spu_block_size.set(spu_block_size_type::giga);

	auto rec = spu_recompiler_base::make_asmjit_recompiler();
	ASSERT_TRUE(rec);

	std::array<be_t<u32>, SPU_LS_SIZE / 4> ls{};
	const auto w = [&](u32 addr, u32 instr) { ls[addr / 4] = instr; };

	// entry@0x00 calls funcA@0x10; the call's return address (0x04) is a stop
	// trap, sitting immediately before funcB@0x08 which funcA also calls — so the
	// return-point block at 0x04 is created, then removed (stop word), orphaning it.
	w(0x00, enc_brsl(0x00, 0x10)); // entry -> funcA ; return = 0x04
	w(0x04, SPU_STOP);             // <- the trap that drives the bug
	w(0x08, enc_il(3, 7));         // funcB@0x08
	w(0x0c, enc_bi_lr());          // funcB return
	w(0x10, enc_brsl(0x10, 0x08)); // funcA@0x10 -> funcB ; return = 0x14
	w(0x14, enc_bi_lr());          // funcA return

	std::map<u32, std::vector<u32>> targets;

	// Pre-fix this aborts inside analyse() (Range check failed); reaching the
	// assertions is the regression check.
	const spu_program prog = rec->analyse(ls.data(), 0x00, &targets);
	EXPECT_EQ(prog.entry_point, 0x00u);
	EXPECT_FALSE(prog.data.empty());

	g_cfg.core.spu_block_size.set(saved);
}

// Encoders for the added coverage tests below. enc2_ prefix avoids clashing with
// the reused enc_il / enc_brsl / enc_bi_lr / SPU_STOP above; every word is
// MADE-UP data encoded from the SPU opcode tables.
namespace
{
	// RI16 (magn 2): opcode<<23 | i16<<7 | rt
	constexpr u32 enc2_il(u32 rt, u32 imm)
	{
		return (0x081u << 23) | ((imm & 0xffff) << 7) | (rt & 0x7f);
	}

	// RI18 (magn 4): opcode<<25 | i18<<7 | rt
	constexpr u32 enc2_ila(u32 rt, u32 imm18)
	{
		return (0x021u << 25) | ((imm18 & 0x3ffff) << 7) | (rt & 0x7f);
	}

	// br target - unconditional relative branch (single successor).
	constexpr u32 enc2_br(u32 pos, u32 target)
	{
		const u32 rel = ((target - pos) / 4) & 0xffff;
		return (0x064u << 23) | (rel << 7);
	}

	// brnz rt, target - conditional relative branch (branch target + fall-through).
	constexpr u32 enc2_brnz(u32 rt, u32 pos, u32 target)
	{
		const u32 rel = ((target - pos) / 4) & 0xffff;
		return (0x042u << 23) | (rel << 7) | (rt & 0x7f);
	}
}

// FIX #1 (DISCRIMINATING): the exported target map must be the post-prune,
// self-consistent map. Cleanup removes the 0x04 STOP-trap return block
// (m_block_info[0x04/4] cleared); the prune drops the dead in-range edge
// 0x00 -> 0x04 and appends the SPU_LS_SIZE sentinel. The export was MOVED to run
// AFTER the prune, so callers no longer see 0x04. PASS with the fix; FAIL without
// it (pre-fix the export ran before the prune and leaked 0x04 into m_targets[0x00]).
TEST(SpuAnalyserGiga, OutTargetListHasNoDeadInRangeEdge)
{
	const auto saved = g_cfg.core.spu_block_size.get();
	g_cfg.core.spu_block_size.set(spu_block_size_type::giga);

	auto rec = spu_recompiler_base::make_asmjit_recompiler();
	ASSERT_TRUE(rec);

	std::array<be_t<u32>, SPU_LS_SIZE / 4> ls{};
	const auto w = [&](u32 addr, u32 instr) { ls[addr / 4] = instr; };

	w(0x00, enc_brsl(0x00, 0x10)); // entry -> funcA ; return-point block = 0x04
	w(0x04, SPU_STOP);             // STOP: kills the 0x04 return block
	w(0x08, enc2_il(3, 7));        // funcB@0x08
	w(0x0c, enc_bi_lr());          // funcB return
	w(0x10, enc_brsl(0x10, 0x08)); // funcA@0x10 -> funcB ; return-point = 0x14
	w(0x14, enc_bi_lr());          // funcA return

	std::map<u32, std::vector<u32>> targets;
	const spu_program prog = rec->analyse(ls.data(), 0x00, &targets);

	EXPECT_EQ(prog.entry_point, 0x00u);
	EXPECT_FALSE(prog.data.empty());

	constexpr u32 kDeadBlock = 0x04u;

	for (const auto& [key, succ] : targets)
	{
		EXPECT_LT(key, static_cast<u32>(SPU_LS_SIZE))
			<< "target-map key out of range: 0x" << std::hex << key;
		EXPECT_NE(key, kDeadBlock)
			<< "removed block 0x04 must not be a key in the exported map";

		for (u32 t : succ)
		{
			EXPECT_NE(t, kDeadBlock)
				<< "exported edge from 0x" << std::hex << key
				<< " still points at removed block 0x04 (export ran before prune)";

			const bool in_range = (t < static_cast<u32>(SPU_LS_SIZE));
			const bool sentinel = (t == static_cast<u32>(SPU_LS_SIZE));
			EXPECT_TRUE(in_range || sentinel)
				<< "exported edge from 0x" << std::hex << key
				<< " has bad successor 0x" << t;
		}
	}

	g_cfg.core.spu_block_size.set(saved);
}

// FIX #2 (POSITIVE walk - does NOT fire the guard): a giga function whose body is
// a multi-block loop, so the loop-body walk advances block_pc through cond_next
// fall-through hops. The guard is belt-and-suspenders and cannot trigger on valid
// input (the prune keeps dead edges out of block.targets). Asserts only that
// analyse() completes and returns a sane program.
TEST(SpuAnalyserGiga, MultiBlockLoopBodyWalkCompletes)
{
	const auto saved = g_cfg.core.spu_block_size.get();
	g_cfg.core.spu_block_size.set(spu_block_size_type::giga);

	auto rec = spu_recompiler_base::make_asmjit_recompiler();
	ASSERT_TRUE(rec);

	std::array<be_t<u32>, SPU_LS_SIZE / 4> ls{};
	const auto w = [&](u32 addr, u32 instr) { ls[addr / 4] = instr; };

	w(0x00, enc2_il(3, 4));            // r3 = 4 (loop counter seed)
	w(0x04, enc2_il(4, 1));            // r4 = 1
	w(0x08, enc2_brnz(3, 0x08, 0x10)); // if r3 != 0 -> 0x10, else fall to 0x0c
	w(0x0c, enc_bi_lr());              // exit path: return
	w(0x10, enc2_il(5, 9));            // body block B @0x10
	w(0x14, enc2_brnz(4, 0x14, 0x1c)); // -> 0x1c, else fall to 0x18
	w(0x18, enc2_il(6, 2));            // body block C @0x18 (fall-through chain)
	w(0x1c, enc2_br(0x1c, 0x00));      // back-edge to loop head

	std::map<u32, std::vector<u32>> targets;
	const spu_program prog = rec->analyse(ls.data(), 0x00, &targets);

	EXPECT_EQ(prog.entry_point, 0x00u);
	EXPECT_FALSE(prog.data.empty());

	for (const auto& [key, succ] : targets)
	{
		EXPECT_LT(key, static_cast<u32>(SPU_LS_SIZE));
		for (u32 t : succ)
		{
			EXPECT_TRUE(t < static_cast<u32>(SPU_LS_SIZE)
				|| t == static_cast<u32>(SPU_LS_SIZE));
		}
	}

	g_cfg.core.spu_block_size.set(saved);
}

// FIX #3 (POSITIVE, NON-DISCRIMINATING): the /41 -> /4 typo lives in the giga
// secondary "Fill more block info" re-decode loop and only corrupts internal
// bb.reg_const / bb.reg_val32 (-> stack_sub / func.good) - none of which reaches
// analyse()'s public outputs (spu_program / out_target_list), and BI/BISL targets
// come from the untouched first pass. So this can only exercise the re-decode loop
// over a multi-instruction block + assert completion + data round-trip; it PASSES
// under both /4 and /41. Discriminating coverage needs a test accessor on the
// internal block state (flagged in the PR notes).
TEST(SpuAnalyserGiga, ReDecodeConstPropCompletes)
{
	const auto saved = g_cfg.core.spu_block_size.get();
	g_cfg.core.spu_block_size.set(spu_block_size_type::giga);

	auto rec = spu_recompiler_base::make_asmjit_recompiler();
	ASSERT_TRUE(rec);

	std::array<be_t<u32>, SPU_LS_SIZE / 4> ls{};
	const auto w = [&](u32 addr, u32 instr) { ls[addr / 4] = instr; };

	w(0x00, enc2_il(3, 0x1234));   // index 0 : const into r3
	w(0x04, enc2_ila(4, 0x208));   // index 1 : const into r4 (distinct word)
	w(0x08, enc2_il(5, 0x22));     // index 2 : const into r5
	w(0x0c, enc2_ila(6, 0x108));   // index 3 : const into r6
	w(0x10, enc2_il(7, 0x33));     // index 4 : const into r7
	w(0x14, enc_bi_lr());          // return ($lr)

	std::map<u32, std::vector<u32>> targets;
	const spu_program prog = rec->analyse(ls.data(), 0x00, &targets);

	EXPECT_EQ(prog.entry_point, 0x00u);
	EXPECT_FALSE(prog.data.empty());

	// data stores std::bit_cast<u32>(ls_word); compare underlying u32 forms.
	ASSERT_GE(prog.data.size(), (0x10u / 4) + 1);
	EXPECT_EQ(prog.data[0x00 / 4], std::bit_cast<u32>(be_t<u32>{enc2_il(3, 0x1234)}));
	EXPECT_EQ(prog.data[0x10 / 4], std::bit_cast<u32>(be_t<u32>{enc2_il(7, 0x33)}));

	g_cfg.core.spu_block_size.set(saved);
}
