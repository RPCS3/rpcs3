#pragma once

#include "../../CFG.h"

struct RSXFragmentProgram;

namespace rsx::assembler::FP
{
	struct RegisterAnnotationPassOptions
	{
		bool skip_delay_slots = false; // When enabled, detect delay slots and ignore annotating them.
	};

	// The annotation pass annotates each basic block with 2 pieces of information:
	// 1. The "input" register list for a block.
	// 2. The "output" register list for a block (clobber list).
	// The information can be used by other passes to set up prologue/epilogue on each block.
	// The pass also populates register reference members of each instruction, such as the input and output lanes.
	class RegisterAnnotationPass : public CFGPass
	{
	public:
		RegisterAnnotationPass(
			const RSXFragmentProgram& prog,
			const RegisterAnnotationPassOptions& options = {})
			: m_prog(prog), m_config(options)
		{}

		void run(FlowGraph& graph) override;

	private:
		const RSXFragmentProgram& m_prog;
		RegisterAnnotationPassOptions m_config;
	};
}
