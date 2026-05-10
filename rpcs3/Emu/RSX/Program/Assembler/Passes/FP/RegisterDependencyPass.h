#pragma once

#include "../../CFG.h"

namespace rsx::assembler::FP
{
	// The register dependency pass identifies data hazards for each basic block and injects barrier instructions.
	// Real PS3 does not have explicit barriers, but does instead often use delay slots or fence instructions to stall until a specific hardware unit clears the fence to advance.
	// For decompiled shaders, we have the problem that aliasing is not real and is instead simulated. We do not have access to unions on the GPU without really nasty tricks.
	class RegisterDependencyPass : public CFGPass
	{
	public:
		bool run(FlowGraph& graph) override;
	};
}
