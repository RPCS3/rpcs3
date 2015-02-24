#include "stdafx.h"
#include "Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "Crypto/sha1.h"
#include "ModuleManager.h"
#include "Emu/Cell/PPUInstrTable.h"

std::vector<ModuleFunc> g_ppu_func_list;
std::vector<StaticFunc> g_ppu_func_subs;

u32 add_ppu_func(ModuleFunc func)
{
	for (auto& f : g_ppu_func_list)
	{
		assert(f.id != func.id);
	}

	g_ppu_func_list.push_back(func);
	return (u32)g_ppu_func_list.size() - 1;
}

u32 add_ppu_func_sub(StaticFunc func)
{
	g_ppu_func_subs.push_back(func);
	return func.index;
}

u32 add_ppu_func_sub(const char group[8], const u64 ops[], const char* name, Module* module, ppu_func_caller func)
{
	StaticFunc sf;
	sf.index = add_ppu_func(ModuleFunc(get_function_id(name), 0, module, func));
	sf.name = name;
	sf.group = *(u64*)group;
	sf.found = 0;

	// TODO: check for self-inclusions, use CRC
	for (u32 i = 0; ops[i]; i++)
	{
		SFuncOp op;
		op.mask = re32((u32)(ops[i] >> 32));
		op.crc = re32((u32)(ops[i]));
		if (op.mask) op.crc &= op.mask;
		sf.ops.push_back(op);
	}

	return add_ppu_func_sub(sf);
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

void execute_ppu_func_by_index(PPUThread& CPU, u32 index)
{
	if (auto func = get_ppu_func_by_index(index))
	{
		auto old_last_syscall = CPU.m_last_syscall;
		CPU.m_last_syscall = func->id;

		if (index & EIF_SAVE_RTOC)
		{
			// save RTOC if necessary
			vm::write64(vm::cast(CPU.GPR[1] + 0x28), CPU.GPR[2]);
		}

		if (func->lle_func && !(func->flags & MFF_FORCED_HLE))
		{
			// call LLE function if available
			func->lle_func(CPU);
		}
		else if (func->func)
		{
			func->func(CPU);
		}
		else
		{
			LOG_ERROR(HLE, "Unimplemented function: %s", SysCalls::GetHLEFuncName(func->id));
			CPU.GPR[3] = 0;
		}

		if (index & EIF_PERFORM_BLR)
		{
			// return if necessary
			CPU.SetBranch(vm::cast(CPU.LR & ~3), true);
		}

		CPU.m_last_syscall = old_last_syscall;
	}
	else
	{
		throw "Invalid function index";
	}
}

void clear_ppu_functions()
{
	g_ppu_func_list.clear();
	g_ppu_func_subs.clear();
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

void hook_ppu_funcs(u32* base, u32 size)
{
	size /= 4;

	if (!Ini.HLEHookStFunc.GetValue())
		return;

	// TODO: optimize search
	for (u32 i = 0; i < size; i++)
	{
		for (u32 j = 0; j < g_ppu_func_subs.size(); j++)
		{
			if ((base[i] & g_ppu_func_subs[j].ops[0].mask) == g_ppu_func_subs[j].ops[0].crc)
			{
				bool found = true;
				u32 can_skip = 0;
				for (u32 k = i, x = 0; x + 1 <= g_ppu_func_subs[j].ops.size(); k++, x++)
				{
					if (k >= size)
					{
						found = false;
						break;
					}

					// skip NOP
					if (base[k] == se32(0x60000000))
					{
						x--;
						continue;
					}

					const u32 mask = g_ppu_func_subs[j].ops[x].mask;
					const u32 crc = g_ppu_func_subs[j].ops[x].crc;

					if (!mask)
					{
						// TODO: define syntax
						if (crc < 4) // skip various number of instructions that don't match next pattern entry
						{
							can_skip += crc;
							k--; // process this position again
						}
						else if (base[k] != crc) // skippable pattern ("optional" instruction), no mask allowed
						{
							k--;
							if (can_skip) // cannot define this behaviour properly
							{
								LOG_WARNING(LOADER, "hook_ppu_funcs(): can_skip = %d (unchanged)", can_skip);
							}
						}
						else
						{
							if (can_skip) // cannot define this behaviour properly
							{
								LOG_WARNING(LOADER, "hook_ppu_funcs(): can_skip = %d (set to 0)", can_skip);
								can_skip = 0;
							}
						}
					}
					else if ((base[k] & mask) != crc) // masked pattern
					{
						if (can_skip)
						{
							can_skip--;
						}
						else
						{
							found = false;
							break;
						}
					}
					else
					{
						can_skip = 0;
					}
				}
				if (found)
				{
					LOG_NOTICE(LOADER, "Function '%s' hooked (addr=0x%x)", g_ppu_func_subs[j].name, vm::get_addr(base + i * 4));
					g_ppu_func_subs[j].found++;
					base[i] = re32(0x04000000 | g_ppu_func_subs[j].index | EIF_PERFORM_BLR); // hack
				}
			}
		}
	}

	// check function groups
	for (u32 i = 0; i < g_ppu_func_subs.size(); i++)
	{
		if (g_ppu_func_subs[i].found) // start from some group
		{
			const u64 group = g_ppu_func_subs[i].group;

			enum GroupSearchResult : u32
			{
				GSR_SUCCESS = 0, // every function from this group has been found once
				GSR_MISSING = 1, // (error) some function not found
				GSR_EXCESS = 2, // (error) some function found twice or more
			};
			u32 res = GSR_SUCCESS;

			// analyse
			for (u32 j = 0; j < g_ppu_func_subs.size(); j++) if (g_ppu_func_subs[j].group == group)
			{
				u32 count = g_ppu_func_subs[j].found;

				if (count == 0) // not found
				{
					// check if this function has been found with different pattern
					for (u32 k = 0; k < g_ppu_func_subs.size(); k++) if (g_ppu_func_subs[k].group == group)
					{
						if (k != j && g_ppu_func_subs[k].index == g_ppu_func_subs[j].index)
						{
							count += g_ppu_func_subs[k].found;
						}
					}
					if (count == 0)
					{
						res |= GSR_MISSING;
						LOG_ERROR(LOADER, "Function '%s' not found", g_ppu_func_subs[j].name);
					}
					else if (count > 1)
					{
						res |= GSR_EXCESS;
					}
				}
				else if (count == 1) // found
				{
					// ensure that this function has NOT been found with different pattern
					for (u32 k = 0; k < g_ppu_func_subs.size(); k++) if (g_ppu_func_subs[k].group == group)
					{
						if (k != j && g_ppu_func_subs[k].index == g_ppu_func_subs[j].index)
						{
							if (g_ppu_func_subs[k].found)
							{
								res |= GSR_EXCESS;
								LOG_ERROR(LOADER, "Function '%s' hooked twice", g_ppu_func_subs[j].name);
							}
						}
					}
				}
				else
				{
					res |= GSR_EXCESS;
					LOG_ERROR(LOADER, "Function '%s' hooked twice", g_ppu_func_subs[j].name);
				}
			}

			// clear data
			for (u32 j = 0; j < g_ppu_func_subs.size(); j++)
			{
				if (g_ppu_func_subs[j].group == group) g_ppu_func_subs[j].found = 0;
			}

			char name[9] = "????????";

			*(u64*)name = group;

			if (res == GSR_SUCCESS)
			{
				LOG_SUCCESS(LOADER, "Function group [%s] successfully hooked", std::string(name, 9).c_str());
			}
			else
			{
				LOG_ERROR(LOADER, "Function group [%s] failed:%s%s", std::string(name, 9).c_str(),
					(res & GSR_MISSING ? " missing;" : ""),
					(res & GSR_EXCESS ? " excess;" : ""));
			}
		}
	}
}

Module::Module(const char* name, void(*init)())
	: m_is_loaded(false)
	, m_name(name)
	, m_init(init)
{
}

Module::~Module()
{
}

void Module::Init()
{
	m_init();
}

void Module::Load()
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

void Module::Unload()
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

void Module::SetLoaded(bool loaded)
{
	m_is_loaded = loaded;
}

bool Module::IsLoaded() const
{
	return m_is_loaded;
}

const std::string& Module::GetName() const
{
	return m_name;
}

void Module::SetName(const std::string& name)
{
	m_name = name;
}

bool Module::CheckID(u32 id) const
{
	return Emu.GetIdManager().CheckID(id) && Emu.GetIdManager().GetID(id).GetName() == GetName();
}

bool Module::CheckID(u32 id, ID*& _id) const
{
	return Emu.GetIdManager().CheckID(id) && (_id = &Emu.GetIdManager().GetID(id))->GetName() == GetName();
}

bool Module::RemoveId(u32 id)
{
	return Emu.GetIdManager().RemoveID(id);
}

IdManager& Module::GetIdManager() const
{
	return Emu.GetIdManager();
}
