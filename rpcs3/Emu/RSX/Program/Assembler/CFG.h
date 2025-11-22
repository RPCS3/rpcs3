#pragma once

#include <util/asm.hpp>
#include "IR.h"

#include <list>

struct RSXFragmentProgram;

namespace rsx::assembler
{
	struct FlowGraph
	{
		std::list<BasicBlock> blocks;

		BasicBlock* push(BasicBlock* parent = nullptr, u32 pc = 0)
		{
			if (!parent && !blocks.empty())
			{
				parent = &blocks.back();
			}

			blocks.push_back({});
			BasicBlock* new_block = &blocks.back();

			if (parent)
			{
				parent->insert_succ(new_block);
				new_block->insert_pred(parent);
			}

			new_block->id = pc;
			return new_block;
		}
	};

	FlowGraph deconstruct_fragment_program(const RSXFragmentProgram& prog);
}

