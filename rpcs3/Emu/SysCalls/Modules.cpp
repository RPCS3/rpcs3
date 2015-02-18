#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/Static.h"
#include "Crypto/sha1.h"
#include "ModuleManager.h"
#include "Emu/Cell/PPUInstrTable.h"

std::vector<ModuleFunc> g_ps3_func_list;

u32 add_ps3_func(ModuleFunc& func)
{
	for (auto& f : g_ps3_func_list)
	{
		if (f.id == func.id)
		{
			// partial update

			if (func.func)
			{
				f.func = func.func;
			}

			if (func.lle_func)
			{
				f.lle_func = func.lle_func;
			}

			return (u32)(&f - g_ps3_func_list.data());
		}
	}

	g_ps3_func_list.push_back(func);
	return (u32)g_ps3_func_list.size() - 1;
}

ModuleFunc* get_ps3_func_by_nid(u32 nid, u32* out_index)
{
	for (auto& f : g_ps3_func_list)
	{
		if (f.id == nid)
		{
			if (out_index)
			{
				*out_index = (u32)(&f - g_ps3_func_list.data());
			}

			return &f;
		}
	}

	return nullptr;
}

ModuleFunc* get_ps3_func_by_index(u32 index)
{
	if (index >= g_ps3_func_list.size())
	{
		return nullptr;
	}

	return &g_ps3_func_list[index];
}

void execute_ps3_func_by_index(PPUThread& CPU, u32 index)
{
	if (auto func = get_ps3_func_by_index(index))
	{
		// save RTOC
		vm::write64(vm::cast(CPU.GPR[1] + 0x28), CPU.GPR[2]);

		auto old_last_syscall = CPU.m_last_syscall;
		CPU.m_last_syscall = func->id;

		if (func->lle_func)
		{
			func->lle_func(CPU);
		}
		else if (func->func)
		{
			(*func->func)(CPU);
		}
		else
		{
			throw "Unimplemented function";
		}

		CPU.m_last_syscall = old_last_syscall;
	}
	else
	{
		throw "Invalid function index";
	}
}

void clear_ps3_functions()
{
	g_ps3_func_list.clear();
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

void Module::PushNewFuncSub(SFunc* func)
{
	Emu.GetSFuncManager().push_back(func);
}

void fix_import(Module* module, u32 nid, u32 addr)
{
	using namespace PPU_instr;
	
	vm::ptr<u32> ptr = vm::ptr<u32>::make(addr);

	u32 index;

	if (auto func = get_ps3_func_by_nid(nid, &index))
	{
		*ptr++ = HACK(index);
		*ptr++ = BLR();
	}
	else
	{
		module->Error("Unimplemented function 0x%x (0x%x)", nid, addr);
	}

	//*ptr++ = ADDIS(11, 0, func >> 16);
	//*ptr++ = ORI(11, 11, func & 0xffff);
	//*ptr++ = NOP();
	//++ptr;
	//*ptr++ = SC(0);
	//*ptr++ = BLR();
	//*ptr++ = NOP();
	//*ptr++ = NOP();
}

void fix_relocs(Module* module, u32 lib, u32 start, u32 end, u32 seg2)
{
	// start of table:
	// addr = (u64) addr - seg2, (u32) 1, (u32) 1, (u64) ptr
	// addr = (u64) addr - seg2, (u32) 0x101, (u32) 1, (u64) ptr - seg2 (???)
	// addr = (u64) addr, (u32) 0x100, (u32) 1, (u64) ptr - seg2 (???)
	// addr = (u64) addr, (u32) 0, (u32) 1, (u64) ptr (???)

	for (u32 i = lib + start; i < lib + end; i += 24)
	{
		u64 addr = vm::read64(i) + lib;
		const u64 flag = vm::read64(i + 8);

		if ((u32)addr != addr || (u32)(addr + seg2) != (addr + seg2))
		{
			module->Error("fix_relocs(): invalid address (0x%llx)", addr);
		}
		else if (flag == 0x10100000001ull)
		{
			addr = addr + seg2;
			u32 value = vm::read32((u32)addr);
			assert(value == vm::read64(i + 16) + seg2);
			vm::write32((u32)addr, value + lib);
		}
		else if (flag == 0x100000001ull)
		{
			addr = addr + seg2;
			u32 value = vm::read32((u32)addr);
			assert(value == vm::read64(i + 16));
			vm::write32((u32)addr, value + lib);
		}
		else if (flag == 0x10000000001ull)
		{
			u32 value = vm::read32((u32)addr);
			assert(value == vm::read64(i + 16) + seg2);
			vm::write32((u32)addr, value + lib);
		}
		else if (flag == 1)
		{
			u32 value = vm::read32((u32)addr);
			assert(value == vm::read64(i + 16));
			vm::write32((u32)addr, value + lib);
		}
		else if (flag == 0x10000000004ull || flag == 0x10000000006ull)
		{
			// seems to be instruction modifiers for imports (done in other way in FIX_IMPORT)
		}
		else
		{
			module->Notice("fix_relocs(): 0x%x : 0x%llx", i - lib, flag);
		}
	}
}
