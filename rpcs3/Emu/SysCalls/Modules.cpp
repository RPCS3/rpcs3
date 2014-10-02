#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/Static.h"
#include "Crypto/sha1.h"
#include "ModuleManager.h"
#include "Emu/Cell/PPUInstrTable.h"

u32 getFunctionId(const char* name)
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

Module::Module(u16 id, const char* name, void(*load)(), void(*unload)())
	: m_is_loaded(false)
	, m_name(name)
	, m_id(id)
	, m_load_func(load)
	, m_unload_func(unload)
{
	Emu.GetModuleManager().SetModule(m_id, this);
}

Module::Module(Module &&other)
	: m_is_loaded(false)
	, m_id(0)
	, m_load_func(nullptr)
	, m_unload_func(nullptr)
{
	std::swap(this->m_name,other.m_name);
	std::swap(this->m_id, other.m_id);
	std::swap(this->m_is_loaded, other.m_is_loaded);
	std::swap(this->m_load_func, other.m_load_func);
	std::swap(this->m_unload_func, other.m_unload_func);
	std::swap(this->m_funcs_list, other.m_funcs_list);
}

Module &Module::operator =(Module &&other)
{
	std::swap(this->m_name, other.m_name);
	std::swap(this->m_id, other.m_id);
	std::swap(this->m_is_loaded, other.m_is_loaded);
	std::swap(this->m_load_func, other.m_load_func);
	std::swap(this->m_unload_func, other.m_unload_func);
	std::swap(this->m_funcs_list, other.m_funcs_list);
	return *this;
}

Module::~Module()
{
	UnLoad();

	for (int i = 0; i < m_funcs_list.size(); i++)
	{
		delete m_funcs_list[i];
	}
}

void Module::Load()
{
	if(IsLoaded())
		return;

	if(m_load_func) m_load_func();

	for(u32 i=0; i<m_funcs_list.size(); ++i)
	{
		Emu.GetModuleManager().AddFunc(m_funcs_list[i]);
	}

	SetLoaded(true);
}

void Module::UnLoad()
{
	if(!IsLoaded())
		return;

	if(m_unload_func) m_unload_func();

	// TODO: Re-enable this when needed
	// This was disabled because some functions would get unloaded and
	// some games tried to use them, thus only printing a TODO message
	//for(u32 i=0; i<m_funcs_list.size(); ++i)
	//{
	//	Emu.GetModuleManager().UnloadFunc(m_funcs_list[i]->id);
	//}

	SetLoaded(false);
}

bool Module::Load(u32 id)
{
	if(Emu.GetModuleManager().IsLoadedFunc(id)) 
		return false;

	for(u32 i=0; i<m_funcs_list.size(); ++i)
	{
		if(m_funcs_list[i]->id == id)
		{
			Emu.GetModuleManager().AddFunc(m_funcs_list[i]);
			return true;
		}
	}

	return false;
}

bool Module::UnLoad(u32 id)
{
	return Emu.GetModuleManager().UnloadFunc(id);
}

void Module::SetLoaded(bool loaded)
{
	m_is_loaded = loaded;
}

bool Module::IsLoaded() const
{
	return m_is_loaded;
}

u16 Module::GetID() const
{
	return m_id;
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

void fix_import(Module* module, u32 func, u32 addr)
{
	using namespace PPU_instr;
	
	vm::ptr<u32>& ptr = (vm::ptr<u32>&)addr;

	*ptr++ = LIS(11, func >> 16);
	*ptr++ = ORI(11, 11, func & 0xffff);
	*ptr++ = NOP();
	++ptr;
	*ptr++ = SC(2);
	*ptr++ = BLR();
	*ptr++ = NOP();
	*ptr++ = NOP();

	module->Load(func);
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
		u64 addr = vm::read64(i);
		const u64 flag = vm::read64(i + 8);

		if (flag == 0x10100000001ull)
		{
			addr = addr + seg2 + lib;
			u32 value = vm::read32(addr);
			assert(value == vm::read64(i + 16) + seg2);
			vm::write32(addr, value + lib);
		}
		else if (flag == 0x100000001ull)
		{
			addr = addr + seg2 + lib;
			u32 value = vm::read32(addr);
			assert(value == vm::read64(i + 16));
			vm::write32(addr, value + lib);
		}
		else if (flag == 0x10000000001ull)
		{
			addr = addr + lib;
			u32 value = vm::read32(addr);
			assert(value == vm::read64(i + 16) + seg2);
			vm::write32(addr, value + lib);
		}
		else if (flag == 1)
		{
			addr = addr + lib;
			u32 value = vm::read32(addr);
			assert(value == vm::read64(i + 16));
			vm::write32(addr, value + lib);
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