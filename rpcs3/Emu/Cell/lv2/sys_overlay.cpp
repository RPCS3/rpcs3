#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Crypto/unself.h"
#include "Crypto/unedat.h"
#include "Loader/ELF.h"

#include "sys_overlay.h"

extern std::shared_ptr<lv2_overlay> ppu_load_overlay(const ppu_exec_object&, const std::string& path);

extern void ppu_initialize(const ppu_module&);

LOG_CHANNEL(sys_overlay);

error_code sys_overlay_load_module(vm::ptr<u32> ovlmid, vm::cptr<char> path2, u64 flags, vm::ptr<u32> entry)
{
	sys_overlay.warning("sys_overlay_load_module(ovlmid=*0x%x, path=%s, flags=0x%x, entry=*0x%x)", ovlmid, path2, flags, entry);

	const std::string path = path2.get_ptr();
	const auto name = path.substr(path.find_last_of('/') + 1);

	const ppu_exec_object obj = decrypt_self(fs::file(vfs::get(path)), fxm::get_always<LoadedNpdrmKeys_t>()->devKlic.data());

	if (obj != elf_error::ok)
	{
		return {CELL_ENOEXEC, obj.operator elf_error()};
	}

	const auto ovlm = ppu_load_overlay(obj, path);

	ppu_initialize(*ovlm);

	sys_overlay.success("Loaded overlay: %s", path);

	*ovlmid = idm::last_id();
	*entry  = ovlm->entry;

	return CELL_OK;
}

error_code sys_overlay_unload_module(u32 ovlmid)
{
	sys_overlay.warning("sys_overlay_unload_module(ovlmid=0x%x)", ovlmid);

	const auto _main = idm::withdraw<lv2_obj, lv2_overlay>(ovlmid);

	if (!_main)
	{
		return CELL_ESRCH;
	}

	for (auto& seg : _main->segs)
	{
		vm::dealloc(seg.addr);
	}

	return CELL_OK;
}
