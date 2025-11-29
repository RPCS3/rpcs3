#pragma once

#include "../../CFG.h"

struct RSXFragmentProgram;

namespace rsx::assembler::FP
{
	// The annotation pass annotates each basic block with 2 pieces of information:
	// 1. The "input" register list for a block.
	// 2. The "output" register list for a block (clobber list).
	// The information can be used by other passes to set up prologue/epilogue on each block.
	// The pass also populates register reference members of each instruction, such as the input and output lanes.
	class RegisterAnnotationPass : public CFGPass
	{
	public:
		RegisterAnnotationPass(RSXFragmentProgram& prog)
			: m_prog(prog)
		{}

		void run(FlowGraph& graph) override;

	private:
		const RSXFragmentProgram& m_prog;
	};
}
