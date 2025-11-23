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

		BasicBlock* push(BasicBlock* parent = nullptr, u32 pc = 0, EdgeType edge_type = EdgeType::NONE)
		{
			if (!parent && !blocks.empty())
			{
				parent = &blocks.back();
			}

			blocks.push_back({});
			BasicBlock* new_block = &blocks.back();

			if (parent)
			{
				parent->insert_succ(new_block, edge_type);
				new_block->insert_pred(parent, edge_type);
			}

			new_block->id = pc;
			return new_block;
		}
	};

	FlowGraph deconstruct_fragment_program(const RSXFragmentProgram& prog);
}

