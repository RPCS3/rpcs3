#include "stdafx.h"
#include "SPUAnalyser.h"
#include "SPURecompiler.h"
#include "SPUOpcodes.h"

const spu_decoder<spu_itype> s_spu_itype;

std::shared_ptr<spu_function_t> SPUDatabase::find(const be_t<u32>* data, u64 key, u32 max_size)
{
	for (auto found = m_db.equal_range(key); found.first != found.second; found.first++)
	{
		const auto& func = found.first->second;

		// Compare binary data explicitly (TODO: optimize)
		if (LIKELY(func->size <= max_size) && std::memcmp(func->data.data(), data, func->size) == 0)
		{
			return func;
		}
	}

	return nullptr;
}

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
	// Check arguments (bounds and alignment)
	if (max_limit > 0x40000 || entry >= max_limit || entry % 4 || max_limit % 4)
	{
		throw EXCEPTION("Invalid arguments (entry=0x%05x, limit=0x%05x)", entry, max_limit);
	}

	// Key for multimap
	const u64 key = entry | u64{ ls[entry / 4] } << 32;

	{
		reader_lock lock(m_mutex);

		// Try to find existing function in the database
		if (auto func = find(ls + entry / 4, key, max_limit - entry))
		{
			return func;
		}
	}

	writer_lock lock(m_mutex);

	// Double-check
	if (auto func = find(ls + entry / 4, key, max_limit - entry))
	{
		return func;
	}

	// Initialize block entries with the function entry point
	std::set<u32> blocks{ entry };

	// Entries of adjacent functions; jump table entries
	std::set<u32> adjacent, jt;

	// Set initial limit which will be narrowed later
	u32 limit = max_limit;

	// Minimal position of ila $SP,* instruction
	u32 ila_sp_pos = max_limit;

	// Find preliminary set of possible block entries (first pass), `start` is the current block address
	for (u32 start = entry, pos = entry; pos < limit; pos += 4)
	{
		const spu_opcode_t op{ ls[pos / 4] };

		const auto type = s_spu_itype.decode(op.opcode);

		// Find existing function
		if (pos != entry && find(ls + pos / 4, pos | u64{ op.opcode } << 32, limit - pos))
		{
			limit = pos;
			break;
		}

		// Additional analysis at the beginning of the block
		if (start != entry && start == pos)
		{
			std::vector<u32> jt_abs;
			std::vector<u32> jt_rel;

			for (; pos < limit; pos += 4)
			{
				const u32 target = ls[pos / 4];

				if (target % 4)
				{
					// Address cannot be misaligned: abort jt scan
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

		if (op.opcode == 0) // Hack: special case (STOP 0)
		{
			limit = pos + 4;
			break;
		}
		
		if (type == BI || type == IRET) // Branch Indirect
		{
			if (type == IRET) LOG_ERROR(SPU, "[0x%05x] Interrupt Return", pos);

			blocks.emplace(start);
			start = pos + 4;
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

			blocks.emplace(start);
			start = pos + 4;
		}
		else if (type == BRSL || type == BRASL) // Branch Relative/Absolute and Set Link
		{
			const u32 target = spu_branch_target(type == BRSL ? pos : 0, op.i16);

			if (target == pos + 4)
			{
				// Branch to the next instruction and set link ("get next instruction address" idiom)

				if (op.rt == 0) LOG_ERROR(SPU, "[0x%05x] Branch-to-next with $LR", pos);
			}
			else
			{
				// Add adjacent function
				adjacent.emplace(target);

				if (target > entry)
				{
					limit = std::min<u32>(limit, target);
				}

				if (op.rt != 0) LOG_ERROR(SPU, "[0x%05x] Function call without $LR", pos);
			}
		}
		else if (type == BISL || type == BISLED) // Branch Indirect and Set Link
		{
			if (op.rt != 0) LOG_ERROR(SPU, "[0x%05x] Indirect function call without $LR", pos);
		}
		else if (type == BRNZ || type == BRZ || type == BRHNZ || type == BRHZ) // Branch Relative if (Not) Zero (Half)word
		{
			const u32 target = spu_branch_target(pos, op.i16);

			// Add adjacent function because it always could be
			adjacent.emplace(target);

			if (target > entry)
			{
				blocks.emplace(target);
			}
		}
		else // Other instructions (writing rt reg)
		{
			const u32 rt = type & spu_itype::_quadrop ? +op.rt4 : +op.rt;

			// Analyse link register access
			if (rt == 0)
			{
			}

			// Analyse stack pointer access
			if (rt == 1)
			{
				if (type == ILA && pos < ila_sp_pos)
				{
					// set minimal ila $SP,* instruction position
					ila_sp_pos = pos;
				}
			}
		}
	}

	// Find more function calls (second pass, questionable)
	for (u32 pos = 0; pos < 0x40000; pos += 4)
	{
		const spu_opcode_t op{ ls[pos / 4] };

		const auto type = s_spu_itype.decode(op.opcode);

		if (type == BRSL || type == BRASL) // Branch Relative/Absolute and Set Link
		{
			const u32 target = spu_branch_target(type == BRSL ? pos : 0, op.i16);

			if (target != pos + 4 && target > entry && limit > target)
			{
				// Narrow the limit
				limit = target;
			}
		}
		else if (!type) // Invalid instruction
		{
			break;
		}
	}

	if (limit <= entry)
	{
		LOG_ERROR(SPU, "Function not found [0x%05x]", entry);
		return nullptr;
	}

	// Prepare new function (set addr and size)
	auto func = std::make_shared<spu_function_t>(entry, limit - entry);

	// Copy function contents
	func->data = { ls + entry / 4, ls + limit / 4 };

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

	// Set whether the function can reset stack
	func->does_reset_stack = ila_sp_pos < limit;

	// Add function to the database
	m_db.emplace(key, func);

	LOG_SUCCESS(SPU, "Function detected [0x%05x-0x%05x] (size=0x%x)", func->addr, func->addr + func->size, func->size);

	return func;
}
