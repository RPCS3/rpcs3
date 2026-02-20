#pragma once

#include "Emu/Cell/SPURecompiler.h"
#include "Emu/Cell/SPUAnalyser.h"

#include <array>

namespace spu_optimizer
{
	using history_index = u32;

	// History indices 0-127 represent initial GPR state, 128+ represent instructions
	static constexpr history_index INSTRUCTION_BASE = 128;

	// Helper to check if a history index refers to an actual instruction (not initial GPR state)
	static constexpr bool is_instruction(history_index h)
	{
		return h >= INSTRUCTION_BASE;
	}

	// Helper to get the program.data index from a history index
	static constexpr u32 to_data_index(history_index h)
	{
		return h - INSTRUCTION_BASE;
	}

	struct raw_inst
	{
		raw_inst(spu_opcode_t op, history_index ra = 0, history_index rb = 0, history_index rc = 0);
		spu_itype::type itype;
		history_index ra;
		history_index rb;
		history_index rc;
		u32 ref_count = 0; // Number of instructions that reference this instruction's result
	};

	class spu_optimizer
	{
	public:
		spu_optimizer(spu_program& program);

		// Increment ref_count of all instructions referenced by the given instruction
		void add_refs(const raw_inst& inst);

		// Decrement ref_count of all instructions referenced by the given instruction
		void release_refs(const raw_inst& inst);

		// Check if an instruction can be safely removed (ref_count == 0)
		bool can_remove(history_index h) const;

		// Try to replace an instruction with NOP. Returns true if successful.
		// Releases refs of the instruction first, then checks if it can be removed.
		bool try_remove(spu_program& program, history_index h);

		void STOP(spu_opcode_t op);
		void LNOP(spu_opcode_t op);
		void SYNC(spu_opcode_t op);
		void DSYNC(spu_opcode_t op);
		void MFSPR(spu_opcode_t op);
		void RDCH(spu_opcode_t op);
		void RCHCNT(spu_opcode_t op);
		void SF(spu_opcode_t op);
		void OR(spu_opcode_t op);
		void BG(spu_opcode_t op);
		void SFH(spu_opcode_t op);
		void NOR(spu_opcode_t op);
		void ABSDB(spu_opcode_t op);
		void ROT(spu_opcode_t op);
		void ROTM(spu_opcode_t op);
		void ROTMA(spu_opcode_t op);
		void SHL(spu_opcode_t op);
		void ROTH(spu_opcode_t op);
		void ROTHM(spu_opcode_t op);
		void ROTMAH(spu_opcode_t op);
		void SHLH(spu_opcode_t op);
		void ROTI(spu_opcode_t op);
		void ROTMI(spu_opcode_t op);
		void ROTMAI(spu_opcode_t op);
		void SHLI(spu_opcode_t op);
		void ROTHI(spu_opcode_t op);
		void ROTHMI(spu_opcode_t op);
		void ROTMAHI(spu_opcode_t op);
		void SHLHI(spu_opcode_t op);
		void A(spu_opcode_t op);
		void AND(spu_opcode_t op);
		void CG(spu_opcode_t op);
		void AH(spu_opcode_t op);
		void NAND(spu_opcode_t op);
		void AVGB(spu_opcode_t op);
		void MTSPR(spu_opcode_t op);
		void WRCH(spu_opcode_t op);
		void BIZ(spu_opcode_t op);
		void BINZ(spu_opcode_t op);
		void BIHZ(spu_opcode_t op);
		void BIHNZ(spu_opcode_t op);
		void STOPD(spu_opcode_t op);
		void STQX(spu_opcode_t op);
		void BI(spu_opcode_t op);
		void BISL(spu_opcode_t op);
		void IRET(spu_opcode_t op);
		void BISLED(spu_opcode_t op);
		void HBR(spu_opcode_t op);
		void GB(spu_opcode_t op);
		void GBH(spu_opcode_t op);
		void GBB(spu_opcode_t op);
		void FSM(spu_opcode_t op);
		void FSMH(spu_opcode_t op);
		void FSMB(spu_opcode_t op);
		void FREST(spu_opcode_t op);
		void FRSQEST(spu_opcode_t op);
		void LQX(spu_opcode_t op);
		void ROTQBYBI(spu_opcode_t op);
		void ROTQMBYBI(spu_opcode_t op);
		void SHLQBYBI(spu_opcode_t op);
		void CBX(spu_opcode_t op);
		void CHX(spu_opcode_t op);
		void CWX(spu_opcode_t op);
		void CDX(spu_opcode_t op);
		void ROTQBI(spu_opcode_t op);
		void ROTQMBI(spu_opcode_t op);
		void SHLQBI(spu_opcode_t op);
		void ROTQBY(spu_opcode_t op);
		void ROTQMBY(spu_opcode_t op);
		void SHLQBY(spu_opcode_t op);
		void ORX(spu_opcode_t op);
		void CBD(spu_opcode_t op);
		void CHD(spu_opcode_t op);
		void CWD(spu_opcode_t op);
		void CDD(spu_opcode_t op);
		void ROTQBII(spu_opcode_t op);
		void ROTQMBII(spu_opcode_t op);
		void SHLQBII(spu_opcode_t op);
		void ROTQBYI(spu_opcode_t op);
		void ROTQMBYI(spu_opcode_t op);
		void SHLQBYI(spu_opcode_t op);
		void NOP(spu_opcode_t op);
		void CGT(spu_opcode_t op);
		void XOR(spu_opcode_t op);
		void CGTH(spu_opcode_t op);
		void EQV(spu_opcode_t op);
		void CGTB(spu_opcode_t op);
		void SUMB(spu_opcode_t op);
		void HGT(spu_opcode_t op);
		void CLZ(spu_opcode_t op);
		void XSWD(spu_opcode_t op);
		void XSHW(spu_opcode_t op);
		void CNTB(spu_opcode_t op);
		void XSBH(spu_opcode_t op);
		void CLGT(spu_opcode_t op);
		void ANDC(spu_opcode_t op);
		void FCGT(spu_opcode_t op);
		void DFCGT(spu_opcode_t op);
		void FA(spu_opcode_t op);
		void FS(spu_opcode_t op);
		void FM(spu_opcode_t op);
		void CLGTH(spu_opcode_t op);
		void ORC(spu_opcode_t op);
		void FCMGT(spu_opcode_t op);
		void DFCMGT(spu_opcode_t op);
		void DFA(spu_opcode_t op);
		void DFS(spu_opcode_t op);
		void DFM(spu_opcode_t op);
		void CLGTB(spu_opcode_t op);
		void HLGT(spu_opcode_t op);
		void DFMA(spu_opcode_t op);
		void DFMS(spu_opcode_t op);
		void DFNMS(spu_opcode_t op);
		void DFNMA(spu_opcode_t op);
		void CEQ(spu_opcode_t op);
		void MPYHHU(spu_opcode_t op);
		void ADDX(spu_opcode_t op);
		void SFX(spu_opcode_t op);
		void CGX(spu_opcode_t op);
		void BGX(spu_opcode_t op);
		void MPYHHA(spu_opcode_t op);
		void MPYHHAU(spu_opcode_t op);
		void FSCRRD(spu_opcode_t op);
		void FESD(spu_opcode_t op);
		void FRDS(spu_opcode_t op);
		void FSCRWR(spu_opcode_t op);
		void DFTSV(spu_opcode_t op);
		void FCEQ(spu_opcode_t op);
		void DFCEQ(spu_opcode_t op);
		void MPY(spu_opcode_t op);
		void MPYH(spu_opcode_t op);
		void MPYHH(spu_opcode_t op);
		void MPYS(spu_opcode_t op);
		void CEQH(spu_opcode_t op);
		void FCMEQ(spu_opcode_t op);
		void DFCMEQ(spu_opcode_t op);
		void MPYU(spu_opcode_t op);
		void CEQB(spu_opcode_t op);
		void FI(spu_opcode_t op);
		void HEQ(spu_opcode_t op);
		void CFLTS(spu_opcode_t op);
		void CFLTU(spu_opcode_t op);
		void CSFLT(spu_opcode_t op);
		void CUFLT(spu_opcode_t op);
		void BRZ(spu_opcode_t op);
		void STQA(spu_opcode_t op);
		void BRNZ(spu_opcode_t op);
		void BRHZ(spu_opcode_t op);
		void BRHNZ(spu_opcode_t op);
		void STQR(spu_opcode_t op);
		void BRA(spu_opcode_t op);
		void LQA(spu_opcode_t op);
		void BRASL(spu_opcode_t op);
		void BR(spu_opcode_t op);
		void FSMBI(spu_opcode_t op);
		void BRSL(spu_opcode_t op);
		void LQR(spu_opcode_t op);
		void IL(spu_opcode_t op);
		void ILHU(spu_opcode_t op);
		void ILH(spu_opcode_t op);
		void IOHL(spu_opcode_t op);
		void ORI(spu_opcode_t op);
		void ORHI(spu_opcode_t op);
		void ORBI(spu_opcode_t op);
		void SFI(spu_opcode_t op);
		void SFHI(spu_opcode_t op);
		void ANDI(spu_opcode_t op);
		void ANDHI(spu_opcode_t op);
		void ANDBI(spu_opcode_t op);
		void AI(spu_opcode_t op);
		void AHI(spu_opcode_t op);
		void STQD(spu_opcode_t op);
		void LQD(spu_opcode_t op);
		void XORI(spu_opcode_t op);
		void XORHI(spu_opcode_t op);
		void XORBI(spu_opcode_t op);
		void CGTI(spu_opcode_t op);
		void CGTHI(spu_opcode_t op);
		void CGTBI(spu_opcode_t op);
		void HGTI(spu_opcode_t op);
		void CLGTI(spu_opcode_t op);
		void CLGTHI(spu_opcode_t op);
		void CLGTBI(spu_opcode_t op);
		void HLGTI(spu_opcode_t op);
		void MPYI(spu_opcode_t op);
		void MPYUI(spu_opcode_t op);
		void CEQI(spu_opcode_t op);
		void CEQHI(spu_opcode_t op);
		void CEQBI(spu_opcode_t op);
		void HEQI(spu_opcode_t op);
		void HBRA(spu_opcode_t op);
		void HBRR(spu_opcode_t op);
		void ILA(spu_opcode_t op);
		void SELB(spu_opcode_t op);
		void SHUFB(spu_opcode_t op);
		void MPYA(spu_opcode_t op);
		void FNMS(spu_opcode_t op);
		void FMA(spu_opcode_t op);
		void FMS(spu_opcode_t op);

		void UNK(spu_opcode_t op);
		void RPCS3_OPTIMIZER(spu_opcode_t op);

	private:
		void optimize(spu_program& program);
		bool check_accurate_reciprocal_pattern(spu_program& program, u32 fma_history_idx);
		bool check_sqrt_pattern(spu_program& program, u32 fma_history_idx);
		bool check_division_pattern(spu_program& program, u32 fma_history_idx);
		// Check if h points to a 1.0f or 1.0f+1ULP constant. Sets variant (0=1.0f, 1=1.0f+1ULP).
		// If the constant uses two instructions (IOHL+ILHU), second_h is set to the ILHU index.
		std::pair<bool, history_index> check_float_constant(spu_program& program, history_index h);
		bool check_half_constant(spu_program& program, history_index h);
		decltype(&spu_optimizer::UNK) decode(u32 op);

	private:
		usz cur_history_ctr = 128;
		std::vector<raw_inst> history;

		std::array<history_index, 128> gpr_history = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127};
	};

	extern spu_optimizer g_spu_optimizer;
} // namespace spu_optimizer
