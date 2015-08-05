#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/ModuleManager.h"
#include "Emu/Cell/PPUInstrTable.h"

#include "Emu/FS/VFS.h"
#include "Emu/FS/vfsFile.h"
#include "Crypto/unself.h"
#include "Loader/ELF64.h"
#include "sys_prx.h"

SysCallBase sys_prx("sys_prx");

extern void fill_ppu_exec_map(u32 addr, u32 size);

lv2_prx_t::lv2_prx_t()
	: id(idm::get_current_id())
{
}

s32 prx_load_module(std::string path, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sys_prx.Warning("prx_load_module(path='%s', flags=0x%llx, pOpt=*0x%x)", path.c_str(), flags, pOpt);

	loader::handlers::elf64 loader;

	vfsFile f(path);
	if (!f.IsOpened())
	{
		return CELL_PRX_ERROR_UNKNOWN_MODULE;
	}

	if (loader.init(f) != loader::handler::error_code::ok || !loader.is_sprx())
	{
		return CELL_PRX_ERROR_ILLEGAL_LIBRARY;
	}

	loader::handlers::elf64::sprx_info info;
	loader.load_sprx(info);

	auto prx = idm::make_ptr<lv2_prx_t>();

	auto meta = info.modules[""];
	prx->start.set(meta.exports[0xBC9A0086]);
	prx->stop.set(meta.exports[0xAB779874]);
	prx->exit.set(meta.exports[0x3AB9A95E]);

	for (auto &module_ : info.modules)
	{
		if (module_.first == "")
			continue;

		Module* module = Emu.GetModuleManager().GetModuleByName(module_.first.c_str());

		if (!module)
		{
			sys_prx.Error("Unknown module '%s'", module_.first.c_str());
		}

		for (auto& f : module_.second.exports)
		{
			const u32 nid = f.first;
			const u32 addr = f.second;

			u32 index;

			auto func = get_ppu_func_by_nid(nid, &index);

			if (!func)
			{
				index = add_ppu_func(ModuleFunc(nid, 0, module, nullptr, nullptr, vm::ptr<void()>::make(addr)));
			}
			else
			{
				func->lle_func.set(addr);

				if (func->flags & MFF_FORCED_HLE)
				{
					u32 i_addr = 0;

					if (!vm::check_addr(addr, 8) || !vm::check_addr(i_addr = vm::read32(addr), 4))
					{
						sys_prx.Error("Failed to inject code for exported function '%s' (opd=0x%x, 0x%x)", SysCalls::GetFuncName(nid), addr, i_addr);
					}
					else
					{
						vm::write32(i_addr, PPU_instr::HACK(index | EIF_PERFORM_BLR));
					}
				}
			}
		}

		for (auto &import : module_.second.imports)
		{
			u32 nid = import.first;
			u32 addr = import.second;

			u32 index;

			auto func = get_ppu_func_by_nid(nid, &index);

			if (!func)
			{
				sys_prx.Error("Unknown function '%s' in '%s' module (0x%x)", SysCalls::GetFuncName(nid), module_.first);

				index = add_ppu_func(ModuleFunc(nid, 0, module, nullptr, nullptr));
			}
			else
			{
				const bool is_lle = func->lle_func && !(func->flags & MFF_FORCED_HLE);

				sys_prx.Error("Imported %sfunction '%s' in '%s' module (0x%x)", (is_lle ? "LLE " : ""), SysCalls::GetFuncName(nid), module_.first, addr);
			}

			if (!patch_ppu_import(addr, index))
			{
				sys_prx.Error("Failed to inject code at address 0x%x", addr);
			}
		}
	}

	for (auto& seg : info.segments)
	{
		const u32 addr = seg.begin.addr();
		const u32 size = align(seg.size, 4096);

		if (vm::check_addr(addr, size))
		{
			fill_ppu_exec_map(addr, size);
		}
		else
		{
			sys_prx.Error("Failed to process executable area (addr=0x%x, size=0x%x)", addr, size);
		}
	}

	return prx->id;
}

s32 sys_prx_load_module(vm::cptr<char> path, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt)
{
	sys_prx.Warning("sys_prx_load_module(path=*0x%x, flags=0x%llx, pOpt=*0x%x)", path, flags, pOpt);

	return prx_load_module(path.get_ptr(), flags, pOpt);
}

s32 sys_prx_load_module_list(s32 count, vm::cpptr<char> path_list, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt, vm::ptr<u32> id_list)
{
	sys_prx.Warning("sys_prx_load_module_list(count=%d, path_list=*0x%x, flags=0x%llx, pOpt=*0x%x, id_list=*0x%x)", count, path_list, flags, pOpt, id_list);

	for (s32 i = 0; i < count; ++i)
	{
		auto path = path_list[i];
		std::string name = path.get_ptr();
		s32 result = prx_load_module(name, flags, pOpt);

		if (result < 0)
			return result;

		id_list[i] = result;
	}

	return CELL_OK;
}

s32 sys_prx_load_module_on_memcontainer()
{
	sys_prx.Todo("sys_prx_load_module_on_memcontainer()");
	return CELL_OK;
}

s32 sys_prx_load_module_by_fd()
{
	sys_prx.Todo("sys_prx_load_module_by_fd()");
	return CELL_OK;
}

s32 sys_prx_load_module_on_memcontainer_by_fd()
{
	sys_prx.Todo("sys_prx_load_module_on_memcontainer_by_fd()");
	return CELL_OK;
}

s32 sys_prx_start_module(s32 id, u64 flags, vm::ptr<sys_prx_start_module_option_t> pOpt)
{
	sys_prx.Warning("sys_prx_start_module(id=0x%x, flags=0x%llx, pOpt=*0x%x)", id, flags, pOpt);

	const auto prx = idm::get<lv2_prx_t>(id);

	if (!prx)
	{
		return CELL_ESRCH;
	}

	//if (prx->is_started)
	//	return CELL_PRX_ERROR_ALREADY_STARTED;

	//prx->is_started = true;
	pOpt->entry_point.set(prx->start ? prx->start.addr() : ~0ull);

	return CELL_OK;
}

s32 sys_prx_stop_module(s32 id, u64 flags, vm::ptr<sys_prx_stop_module_option_t> pOpt)
{
	sys_prx.Warning("sys_prx_stop_module(id=0x%x, flags=0x%llx, pOpt=*0x%x)", id, flags, pOpt);

	const auto prx = idm::get<lv2_prx_t>(id);

	if (!prx)
	{
		return CELL_ESRCH;
	}

	//if (!prx->is_started)
	//	return CELL_PRX_ERROR_ALREADY_STOPPED;

	//prx->is_started = false;
	pOpt->entry_point.set(prx->stop ? prx->stop.addr() : -1);

	return CELL_OK;
}

s32 sys_prx_unload_module(s32 id, u64 flags, vm::ptr<sys_prx_unload_module_option_t> pOpt)
{
	sys_prx.Warning("sys_prx_unload_module(id=0x%x, flags=0x%llx, pOpt=*0x%x)", id, flags, pOpt);

	// Get the PRX, free the used memory and delete the object and its ID
	const auto prx = idm::get<lv2_prx_t>(id);

	if (!prx)
	{
		return CELL_ESRCH;
	}

	//Memory.Free(prx->address);

	//s32 result = prx->exit ? prx->exit() : CELL_OK;
	idm::remove<lv2_prx_t>(id);
	
	return CELL_OK;
}

s32 sys_prx_get_module_list(u64 flags, vm::ptr<sys_prx_get_module_list_t> pInfo)
{
	sys_prx.Todo("sys_prx_get_module_list(flags=%d, pInfo=*0x%x)", flags, pInfo);
	return CELL_OK;
}

s32 sys_prx_get_my_module_id()
{
	sys_prx.Todo("sys_prx_get_my_module_id()");
	return CELL_OK;
}

s32 sys_prx_get_module_id_by_address()
{
	sys_prx.Todo("sys_prx_get_module_id_by_address()");
	return CELL_OK;
}

s32 sys_prx_get_module_id_by_name(vm::cptr<char> name, u64 flags, vm::ptr<sys_prx_get_module_id_by_name_option_t> pOpt)
{
	const char *realName = name.get_ptr();
	sys_prx.Todo("sys_prx_get_module_id_by_name(name=%s, flags=%d, pOpt=*0x%x)", realName, flags, pOpt);

	//if (realName == "?") ...

	return CELL_PRX_ERROR_UNKNOWN_MODULE;
}

s32 sys_prx_get_module_info(s32 id, u64 flags, vm::ptr<sys_prx_module_info_t> info)
{
	sys_prx.Todo("sys_prx_get_module_info(id=%d, flags=%d, info=*0x%x)", id, flags, info);
	return CELL_OK;
}

s32 sys_prx_register_library(vm::ptr<void> library)
{
	sys_prx.Todo("sys_prx_register_library(library=*0x%x)", library);
	return CELL_OK;
}

s32 sys_prx_unregister_library(vm::ptr<void> library)
{
	sys_prx.Todo("sys_prx_unregister_library(library=*0x%x)", library);
	return CELL_OK;
}

s32 sys_prx_get_ppu_guid()
{
	sys_prx.Todo("sys_prx_get_ppu_guid()");
	return CELL_OK;
}

s32 sys_prx_register_module()
{
	sys_prx.Todo("sys_prx_register_module()");
	return CELL_OK;
}

s32 sys_prx_query_module()
{
	sys_prx.Todo("sys_prx_query_module()");
	return CELL_OK;
}

s32 sys_prx_link_library()
{
	sys_prx.Todo("sys_prx_link_library()");
	return CELL_OK;
}

s32 sys_prx_unlink_library()
{
	sys_prx.Todo("sys_prx_unlink_library()");
	return CELL_OK;
}

s32 sys_prx_query_library()
{
	sys_prx.Todo("sys_prx_query_library()");
	return CELL_OK;
}

s32 sys_prx_start()
{
	sys_prx.Todo("sys_prx_start()");
	return CELL_OK;
}

s32 sys_prx_stop()
{
	sys_prx.Todo("sys_prx_stop()");
	return CELL_OK;
}
