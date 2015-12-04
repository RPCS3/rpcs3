#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Crypto/sha1.h"
#include "ModuleManager.h"
#include "Emu/Cell/PPUInstrTable.h"

std::vector<ModuleFunc> g_ppu_func_list;
std::vector<StaticFunc> g_ppu_func_subs;
std::vector<ModuleVariable> g_ps3_var_list;

u32 add_ppu_func(ModuleFunc func)
{
	if (g_ppu_func_list.empty())
	{
		// prevent relocations if the array growths, must be sizeof(ModuleFunc) * 0x8000 ~~ about 1 MB of memory
		g_ppu_func_list.reserve(0x8000);
	}

	for (auto& f : g_ppu_func_list)
	{
		if (f.id == func.id)
		{
			// if NIDs overlap or if the same function is added twice
			throw EXCEPTION("FNID already exists: 0x%08x (%s)", f.id, f.name);
		}
	}

	g_ppu_func_list.emplace_back(std::move(func));
	return (u32)g_ppu_func_list.size() - 1;
}

void add_variable(u32 nid, Module<>* module, const char* name, u32(*addr)())
{
	if (g_ps3_var_list.empty())
	{
		g_ps3_var_list.reserve(0x4000); // as g_ppu_func_list
	}

	for (auto& v : g_ps3_var_list)
	{
		if (v.id == nid)
		{
			throw EXCEPTION("VNID already exists: 0x%08x (%s)", nid, name);
		}
	}

	g_ps3_var_list.emplace_back(ModuleVariable{ nid, module, name, addr });
}

ModuleVariable* get_variable_by_nid(u32 nid)
{
	for (auto& v : g_ps3_var_list)
	{
		if (v.id == nid)
		{
			return &v;
		}
	}

	return nullptr;
}

u32 add_ppu_func_sub(StaticFunc func)
{
	g_ppu_func_subs.emplace_back(func);
	return func.index;
}

u32 add_ppu_func_sub(const std::initializer_list<SearchPatternEntry>& ops, const char* name, Module<>* module, ppu_func_caller func)
{
	StaticFunc sf;
	sf.index = add_ppu_func(ModuleFunc(get_function_id(name), 0, module, name, func));
	sf.name = name;
	sf.found = 0;
	sf.ops = ops;

	return add_ppu_func_sub(std::move(sf));
}

ModuleFunc* get_ppu_func_by_nid(u32 nid, u32* out_index)
{
	for (auto& f : g_ppu_func_list)
	{
		if (f.id == nid)
		{
			if (out_index)
			{
				*out_index = (u32)(&f - g_ppu_func_list.data());
			}

			return &f;
		}
	}

	return nullptr;
}

ModuleFunc* get_ppu_func_by_index(u32 index)
{
	index &= ~EIF_FLAGS;

	if (index >= g_ppu_func_list.size())
	{
		return nullptr;
	}

	return &g_ppu_func_list[index];
}

void execute_ppu_func_by_index(PPUThread& ppu, u32 index)
{
	if (auto func = get_ppu_func_by_index(index))
	{
		// save RTOC if necessary
		if (index & EIF_SAVE_RTOC)
		{
			vm::write64(VM_CAST(ppu.GPR[1] + 0x28), ppu.GPR[2]);
		}

		// save old syscall/NID value
		const auto last_code = ppu.hle_code;

		// branch directly to the LLE function
		if (index & EIF_USE_BRANCH)
		{
			// for example, FastCall2 can't work with functions which do user level context switch

			if (last_code)
			{
				throw EXCEPTION("This function cannot be called from the callback: %s (0x%llx)", get_ps3_function_name(func->id), func->id);
			}

			if (!func->lle_func)
			{
				throw EXCEPTION("LLE function not set: %s (0x%llx)", get_ps3_function_name(func->id), func->id);
			}

			if (func->flags & MFF_FORCED_HLE)
			{
				throw EXCEPTION("Forced HLE enabled: %s (0x%llx)", get_ps3_function_name(func->id), func->id);
			}

			if (rpcs3::config.misc.log.hle_logging.value())
			{
				LOG_NOTICE(HLE, "Branch to LLE function: %s (0x%llx)", get_ps3_function_name(func->id), func->id);
			}

			if (index & EIF_PERFORM_BLR)
			{
				throw EXCEPTION("TODO: Branch with link: %s (0x%llx)", get_ps3_function_name(func->id), func->id);
				// CPU.LR = CPU.PC + 4;
			}

			const auto data = vm::_ptr<u32>(func->lle_func.addr());
			ppu.PC = data[0] - 4;
			ppu.GPR[2] = data[1]; // set rtoc

			return;
		}
		
		// change current syscall/NID value
		ppu.hle_code = func->id;

		if (func->lle_func && !(func->flags & MFF_FORCED_HLE))
		{
			// call LLE function if available

			const auto data = vm::_ptr<u32>(func->lle_func.addr());
			const u32 pc = data[0];
			const u32 rtoc = data[1];

			if (rpcs3::config.misc.log.hle_logging.value())
			{
				LOG_NOTICE(HLE, "LLE function called: %s", get_ps3_function_name(func->id));
			}
			
			ppu.fast_call(pc, rtoc);

			if (rpcs3::config.misc.log.hle_logging.value())
			{
				LOG_NOTICE(HLE, "LLE function finished: %s -> 0x%llx", get_ps3_function_name(func->id), ppu.GPR[3]);
			}
		}
		else if (func->func)
		{
			if (rpcs3::config.misc.log.hle_logging.value())
			{
				LOG_NOTICE(HLE, "HLE function called: %s", get_ps3_function_name(func->id));
			}

			func->func(ppu);

			if (rpcs3::config.misc.log.hle_logging.value())
			{
				LOG_NOTICE(HLE, "HLE function finished: %s -> 0x%llx", get_ps3_function_name(func->id), ppu.GPR[3]);
			}
		}
		else
		{
			LOG_ERROR(HLE, "Unimplemented function: %s -> CELL_OK", get_ps3_function_name(func->id));
			ppu.GPR[3] = 0;
		}

		if (index & EIF_PERFORM_BLR)
		{
			// return if necessary
			ppu.PC = VM_CAST(ppu.LR & ~3) - 4;
		}

		// execute module-specific error check
		if ((s64)ppu.GPR[3] < 0 && func->module && func->module->on_error)
		{
			func->module->on_error(ppu.GPR[3], func);
		}

		ppu.hle_code = last_code;
	}
	else
	{
		throw EXCEPTION("Invalid function index (0x%x)", index);
	}
}

void clear_ppu_functions()
{
	g_ppu_func_list.clear();
	g_ppu_func_subs.clear();
	g_ps3_var_list.clear();
}

u32 get_function_id(const char* name)
{
	const char* suffix = "\x67\x59\x65\x99\x04\x25\x04\x90\x56\x64\x27\x49\x94\x89\x74\x1A"; // Symbol name suffix
	u8 output[20];

	// Compute SHA-1 hash
	sha1_context ctx;

	sha1_starts(&ctx);
	sha1_update(&ctx, (const u8*)name, strlen(name));
	sha1_update(&ctx, (const u8*)suffix, strlen(suffix));
	sha1_finish(&ctx, output);

	return (u32&)output[0];
}

void hook_ppu_func(vm::ptr<u32> base, u32 pos, u32 size)
{
	using namespace PPU_instr;

	for (auto& sub : g_ppu_func_subs)
	{
		bool found = sub.ops.size() != 0;

		for (u32 k = pos, x = 0; x + 1 <= sub.ops.size(); k++, x++)
		{
			if (k >= size)
			{
				found = false;
				break;
			}

			// skip NOP
			if (base[k] == 0x60000000)
			{
				x--;
				continue;
			}

			const be_t<u32> data = sub.ops[x].data;
			const be_t<u32> mask = sub.ops[x].mask;

			const bool match = (base[k] & mask) == data;

			switch (sub.ops[x].type)
			{
			case SPET_MASKED_OPCODE:
			{
				// masked pattern
				if (!match)
				{
					found = false;
				}

				break;
			}
			case SPET_OPTIONAL_MASKED_OPCODE:
			{
				// optional masked pattern
				if (!match)
				{
					k--;
				}

				break;
			}
			case SPET_LABEL:
			{
				const u32 addr = (base + k--).addr();
				const u32 lnum = data;
				const auto label = sub.labels.find(lnum);

				if (label == sub.labels.end()) // register the label
				{
					sub.labels[lnum] = addr;
				}
				else if (label->second != addr) // or check registered label
				{
					found = false;
				}

				break;
			}
			case SPET_BRANCH_TO_LABEL:
			{
				if (!match)
				{
					found = false;
					break;
				}

				const auto addr = (base[k] & 2 ? 0 : (base + k).addr()) + ((s32)base[k] << cntlz32(mask) >> (cntlz32(mask) + 2));
				const auto lnum = sub.ops[x].num;
				const auto label = sub.labels.find(lnum);

				if (label == sub.labels.end()) // register the label
				{
					sub.labels[lnum] = addr;
				}
				else if (label->second != addr) // or check registered label
				{
					found = false;
				}

				break;
			}
			//case SPET_BRANCH_TO_FUNC:
			//{
			//	if (!match)
			//	{
			//		found = false;
			//		break;
			//	}

			//	const auto addr = (base[k] & 2 ? 0 : (base + k).addr()) + ((s32)base[k] << cntlz32(mask) >> (cntlz32(mask) + 2));
			//	const auto nid = sub.ops[x].num;
			//	// TODO: recursive call
			//}
			default:
			{
				throw EXCEPTION("Unknown search pattern type (%d)", sub.ops[x].type);
			}
			}

			if (!found)
			{
				break;
			}
		}

		if (found)
		{
			LOG_SUCCESS(LOADER, "Function '%s' hooked (addr=*0x%x)", sub.name, base + pos);
			sub.found++;
			base[pos] = HACK(sub.index | EIF_PERFORM_BLR);
		}

		if (sub.labels.size())
		{
			sub.labels.clear();
		}
	}
}

void hook_ppu_funcs(vm::ptr<u32> base, u32 size)
{
	using namespace PPU_instr;

	// TODO: optimize search
	for (u32 i = 0; i < size; i++)
	{
		// skip NOP
		if (base[i] == 0x60000000)
		{
			continue;
		}

		hook_ppu_func(base, i, size);
	}

	// check functions
	for (u32 i = 0; i < g_ppu_func_subs.size(); i++)
	{
		if (g_ppu_func_subs[i].found > 1)
		{
			LOG_ERROR(LOADER, "Function '%s' hooked %u times", g_ppu_func_subs[i].found);
		}
	}
}

bool patch_ppu_import(u32 addr, u32 index)
{
	const auto data = vm::cptr<u32>::make(addr);

	using namespace PPU_instr;

	if (index >= g_ppu_func_list.size())
	{
		return false;
	}

	const u32 imm = (g_ppu_func_list[index].flags & MFF_NO_RETURN) && !(g_ppu_func_list[index].flags & MFF_FORCED_HLE)
		? index | EIF_USE_BRANCH
		: index | EIF_PERFORM_BLR;

	// check different patterns:

	if (vm::check_addr(addr, 32) &&
		(data[0] & 0xffff0000) == LI_(r12, 0) &&
		(data[1] & 0xffff0000) == ORIS(r12, r12, 0) &&
		(data[2] & 0xffff0000) == LWZ(r12, r12, 0) &&
		data[3] == STD(r2, r1, 0x28) &&
		data[4] == LWZ(r0, r12, 0) &&
		data[5] == LWZ(r2, r12, 4) &&
		data[6] == MTCTR(r0) &&
		data[7] == BCTR())
	{
		vm::write32(addr, HACK(imm | EIF_SAVE_RTOC));
		return true;
	}

	if (vm::check_addr(addr, 12) &&
		(data[0] & 0xffff0000) == LI_(r0, 0) &&
		(data[1] & 0xffff0000) == ORIS(r0, r0, 0) &&
		(data[2] & 0xfc000003) == B(0, 0, 0))
	{
		const auto sub = vm::cptr<u32>::make(addr + 8 + ((s32)data[2] << 6 >> 8 << 2));

		if (vm::check_addr(sub.addr(), 60) &&
			sub[0x0] == STDU(r1, r1, -0x80) &&
			sub[0x1] == STD(r2, r1, 0x70) &&
			sub[0x2] == MR(r2, r0) &&
			sub[0x3] == MFLR(r0) &&
			sub[0x4] == STD(r0, r1, 0x90) &&
			sub[0x5] == LWZ(r2, r2, 0) &&
			sub[0x6] == LWZ(r0, r2, 0) &&
			sub[0x7] == LWZ(r2, r2, 4) &&
			sub[0x8] == MTCTR(r0) &&
			sub[0x9] == BCTRL() &&
			sub[0xa] == LD(r2, r1, 0x70) &&
			sub[0xb] == ADDI(r1, r1, 0x80) &&
			sub[0xc] == LD(r0, r1, 0x10) &&
			sub[0xd] == MTLR(r0) &&
			sub[0xe] == BLR())
		{
			vm::write32(addr, HACK(imm));
			return true;
		}
	}

	if (vm::check_addr(addr, 64) &&
		data[0x0] == MFLR(r0) &&
		data[0x1] == STD(r0, r1, 0x10) &&
		data[0x2] == STDU(r1, r1, -0x80) &&
		data[0x3] == STD(r2, r1, 0x70) &&
		(data[0x4] & 0xffff0000) == LI_(r2, 0) &&
		(data[0x5] & 0xffff0000) == ORIS(r2, r2, 0) &&
		data[0x6] == LWZ(r2, r2, 0) &&
		data[0x7] == LWZ(r0, r2, 0) &&
		data[0x8] == LWZ(r2, r2, 4) &&
		data[0x9] == MTCTR(r0) &&
		data[0xa] == BCTRL() &&
		data[0xb] == LD(r2, r1, 0x70) &&
		data[0xc] == ADDI(r1, r1, 0x80) &&
		data[0xd] == LD(r0, r1, 0x10) &&
		data[0xe] == MTLR(r0) &&
		data[0xf] == BLR())
	{
		vm::write32(addr, HACK(imm));
		return true;
	}

	if (vm::check_addr(addr, 64) &&
		data[0x0] == MFLR(r0) &&
		data[0x1] == STD(r0, r1, 0x10) &&
		data[0x2] == STDU(r1, r1, -0x80) &&
		data[0x3] == STD(r2, r1, 0x70) &&
		(data[0x4] & 0xffff0000) == LIS(r12, 0) &&
		(data[0x5] & 0xffff0000) == LWZ(r12, r12, 0) &&
		data[0x6] == LWZ(r0, r12, 0) &&
		data[0x7] == LWZ(r2, r12, 4) &&
		data[0x8] == MTCTR(r0) &&
		data[0x9] == BCTRL() &&
		data[0xa] == LD(r2, r1, 0x70) &&
		data[0xb] == ADDI(r1, r1, 0x80) &&
		data[0xc] == LD(r0, r1, 0x10) &&
		data[0xd] == MTLR(r0) &&
		data[0xe] == BLR())
	{
		vm::write32(addr, HACK(imm));
		return true;
	}

	if (vm::check_addr(addr, 56) &&
		(data[0x0] & 0xffff0000) == LI_(r12, 0) &&
		(data[0x1] & 0xffff0000) == ORIS(r12, r12, 0) &&
		(data[0x2] & 0xffff0000) == LWZ(r12, r12, 0) &&
		data[0x3] == STD(r2, r1, 0x28) &&
		data[0x4] == MFLR(r0) &&
		data[0x5] == STD(r0, r1, 0x20) &&
		data[0x6] == LWZ(r0, r12, 0) &&
		data[0x7] == LWZ(r2, r12, 4) &&
		data[0x8] == MTCTR(r0) &&
		data[0x9] == BCTRL() &&
		data[0xa] == LD(r0, r1, 0x20) &&
		data[0xb] == MTLR(r0) &&
		data[0xc] == LD(r2, r1, 0x28) &&
		data[0xd] == BLR())
	{
		vm::write32(addr, HACK(imm));
		return true;
	}

	//vm::write32(addr, HACK(imm));
	return false;
}

Module<>::Module(const char* name, void(*init)())
	: m_is_loaded(false)
	, m_name(name)
	, m_init(init)
{
}

Module<>::~Module()
{
}

void Module<>::Init()
{
	on_load = nullptr;
	on_unload = nullptr;
	on_stop = nullptr;
	on_error = nullptr;

	m_init();
}

void Module<>::Load()
{
	if (IsLoaded())
	{
		return;
	}

	if (on_load)
	{
		on_load();
	}

	SetLoaded(true);
}

void Module<>::Unload()
{
	if (!IsLoaded())
	{
		return;
	}

	if (on_unload)
	{
		on_unload();
	}

	SetLoaded(false);
}

void Module<>::SetLoaded(bool loaded)
{
	m_is_loaded = loaded;
}

bool Module<>::IsLoaded() const
{
	return m_is_loaded;
}

const std::string& Module<>::GetName() const
{
	return m_name;
}

void Module<>::SetName(const std::string& name)
{
	m_name = name;
}
