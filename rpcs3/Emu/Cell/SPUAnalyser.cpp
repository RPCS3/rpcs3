#include "stdafx.h"
#include "Utilities/Log.h"

#include "Crypto/sha1.h"
#include "SPURecompiler.h"
#include "SPUAnalyser.h"

const spu_opcode_table_t<spu_itype_t> g_spu_itype{ DEFINE_SPU_OPCODES(spu_itype::), spu_itype::UNK };

SPUDatabase::SPUDatabase()
{
	// TODO: load existing database associated with currently running executable

	LOG_SUCCESS(SPU, "SPU Database initialized...");
}

SPUDatabase::~SPUDatabase()
{
	// TODO: serialize database
}

std::shared_ptr<spu_function_t> SPUDatabase::analyse(const be_t<u32>* ls, u32 entry, u32 max_limit)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// Check arguments (bounds and alignment)
	if (max_limit > 0x40000 || entry >= max_limit || entry % 4 || max_limit % 4)
	{
		throw EXCEPTION("Invalid arguments (entry=0x%05x, limit=0x%05x)", entry, max_limit);
	}

	// Key for multimap
	const u64 key = entry | u64{ ls[entry / 4] } << 32;

	// Try to find existing function in the database
	for (auto found = m_db.find(key); found != m_db.end(); found++)
	{
		// Compare binary data explicitly (TODO: optimize)
		if (std::equal(found->second->data.begin(), found->second->data.end(), ls + entry / 4))
		{
			return found->second;
		}
	}

	// Initialize block entries with the function entry point
	std::set<u32> blocks{ entry };

	// Entries of adjacent functions; jump table entries
	std::set<u32> adjacent, jt;

	// Set initial limit which will be narrowed later
	u32 limit = max_limit;

	// Find preliminary set of possible block entries (first pass), `start` is the current block address
	for (u32 start = entry, pos = entry; pos < limit; pos += 4)
	{
		const spu_opcode_t op{ ls[pos / 4] };

		const spu_itype_t type = g_spu_itype[op.opcode];

		using namespace spu_itype;

		if (start == pos) // Additional analysis at the beginning of the block (questionable)
		{
			// Possible jump table
			std::vector<u32> jt_abs, jt_rel;

			for (; pos < limit; pos += 4)
			{
				const u32 target = ls[pos / 4];

				if (target % 4)
				{
					// Misaligned address: abort analysis
					break;
				}

				if (target >= entry && target < limit)
				{
					// Possible jump table entry (absolute)
					jt_abs.emplace_back(target);
				}

				if (target + start >= entry && target + start < limit)
				{
					// Possible jump table entry (relative)
					jt_rel.emplace_back(target + start);
				}

				if (std::max(jt_abs.size(), jt_rel.size()) * 4 + start <= pos)
				{
					break;
				}
			}

			if (pos - start >= 8)
			{
				// Register jump table block (at least 2 entries) as an ordinary block
				blocks.emplace(start);

				// Register jump table entries (absolute)
				if (jt_abs.size() * 4 == pos - start)
				{
					blocks.insert(jt_abs.begin(), jt_abs.end());

					jt.insert(jt_abs.begin(), jt_abs.end());
				}

				// Register jump table entries (relative)
				if (jt_rel.size() * 4 == pos - start)
				{
					blocks.insert(jt_rel.begin(), jt_rel.end());

					jt.insert(jt_rel.begin(), jt_rel.end());
				}

				// Fix pos value
				start = pos; pos = pos - 4;
				
				continue;
			}

			// Restore pos value
			pos = start;
		}

		if (!type || (start == pos && start > *blocks.rbegin())) // Invalid instruction or "unrelated" block started
		{
			// Discard current block and abort the operation
			limit = start;

			break;
		}
		else if (op.opcode == 0) // Hack: special case (STOP 0)
		{
			limit = pos + 4;

			break;
		}
		else if (type == BI) // Branch Indirect
		{
			blocks.emplace(start); start = pos + 4;
		}
		else if (type == BR || type == BRA) // Branch Relative/Absolute
		{
			const u32 target = spu_branch_target(type == BR ? pos : 0, op.i16);

			// Add adjacent function because it always could be
			adjacent.emplace(target);

			if (target > entry)
			{
				blocks.emplace(target);
			}

			blocks.emplace(start); start = pos + 4;
		}
		else if (type == BRSL || type == BRASL) // Branch Relative/Absolute and Set Link
		{
			const u32 target = spu_branch_target(type == BRSL ? pos : 0, op.i16);

			if (target == pos + 4)
			{
				// Branch to the next instruction and set link ("get next instruction address" idiom)

				if (op.rt == 0) LOG_ERROR(SPU, "Suspicious instruction at [0x%05x]", pos);
			}
			else
			{
				// Add adjacent function
				adjacent.emplace(target);

				if (target > entry)
				{
					limit = std::min<u32>(limit, target);
				}
			}
		}
		else if (type == BISL) // Branch Indirect and Set Link
		{
			// Nothing
		}
		else if (type == BRNZ || type == BRZ || type == BRHNZ || type == BRHZ) // Branch Relative if (Not) Zero Word/Halfword
		{
			const u32 target = spu_branch_target(pos, op.i16);

			// Add adjacent function because it always could be
			adjacent.emplace(target);

			if (target > entry)
			{
				blocks.emplace(target);
			}
		}
	}

	// Find more function calls (second pass, questionable)
	for (u32 pos = 0; pos < 0x40000; pos += 4)
	{
		const spu_opcode_t op{ ls[pos / 4] };

		const spu_itype_t type = g_spu_itype[op.opcode];

		using namespace spu_itype;

		if (!type) // Invalid instruction
		{
			break;
		}
		else if (type == BRSL || type == BRASL) // Branch Relative/Absolute and Set Link
		{
			const u32 target = spu_branch_target(type == BRSL ? pos : 0, op.i16);

			if (target != pos + 4 && target > entry && limit > target)
			{
				// Narrow the limit
				limit = target;
			}
		}
	}

	if (limit <= entry)
	{
		LOG_ERROR(SPU, "Function not found [0x%05x]", entry);
		return nullptr;
	}

	// Prepare new function (set addr and size)
	auto func = std::make_shared<spu_function_t>(entry, limit - entry);

	// Fill function block info
	for (auto i = blocks.crbegin(); i != blocks.crend(); i++)
	{
		if (limit > *i)
		{
			func->blocks.emplace_hint(func->blocks.begin(), *i);
		}
	}

	// Fill adjacent function info
	for (auto i = adjacent.crbegin(); i != adjacent.crend(); i++)
	{
		if (limit <= *i || entry >= *i)
		{
			func->adjacent.emplace_hint(func->adjacent.begin(), *i);
		}
	}

	// Fill jump table entries
	for (auto i = jt.crbegin(); i != jt.crend(); i++)
	{
		if (limit > *i)
		{
			func->jtable.emplace_hint(func->jtable.begin(), *i);
		}
	}

	// Copy function contents
	func->data = { ls + entry / 4, ls + limit / 4 };

	// Add function to the database
	m_db.emplace(key, func);

	LOG_SUCCESS(SPU, "Function detected [0x%05x-0x%05x] (size=0x%x)", func->addr, func->addr + func->size, func->size);

	return func;
}
