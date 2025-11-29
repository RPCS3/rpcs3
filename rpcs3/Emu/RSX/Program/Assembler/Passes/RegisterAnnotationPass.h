#pragma once

#include "../CFG.h"

namespace rsx::assembler
{
	// The annotation pass annotates each basic block with 2 pieces of information:
	// 1. The "input" register list for a block.
	// 2. The "output" register list for a block (clobber list).
	// The information can be used by other passes to set up prologue/epilogue on each block.
	class RegisterAnnotationPass : public CFGPass
	{
	public:
		void run(FlowGraph& graph) override;
	};
}
