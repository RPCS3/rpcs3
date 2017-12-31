#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Crypto/unself.h"
#include "Crypto/unedat.h"
#include "Loader/ELF.h"
#include "Emu/Cell/ErrorCodes.h"

#include "sys_overlay.h"


namespace vm { using namespace ps3; }

extern u32 ppu_load_overlay_exec(const ppu_exec_object&, const std::string& path);
extern void sys_initialize_tls(ppu_thread&, u64, u32, u32, u32);

extern u32 g_ps3_sdk_version;
extern std::vector<std::string> g_ppu_function_names;

logs::channel sys_overlay("sys_overlay");

error_code sys_overlay_load_module(vm::ps3::ptr<sys_overlay_t> ovlmid, vm::ps3::cptr<char> path2, uint64_t flags, vm::ps3::ptr<u32> entry)
{
	sys_overlay.warning("sys_overlay_load_module(ovlmid= 0x%x, *ovlmid=0x%x, path = 0x%x, pathstr = %s, flags = 0x%x, entry = 0x%x)", ovlmid, (*ovlmid).id, path2, path2.get_ptr(), flags, entry);
	const std::string path = path2.get_ptr();
	const auto name = path.substr(path.find_last_of('/') + 1);
	const auto loadedkeys = fxm::get_always<LoadedNpdrmKeys_t>();
	const ppu_exec_object obj = decrypt_self(fs::file(vfs::get(path)), loadedkeys->devKlic.data());

	//unknown error codes
	if (obj != elf_error::ok)
	{
		return CELL_ENOEXEC;
	}

	sys_overlay.success("Loaded overlay: %s", path);

	*entry = ppu_load_overlay_exec(obj, path);
	(*ovlmid).id = idm::last_id();
	return not_an_error(idm::last_id());
}

error_code sys_overlay_unload_module(u32 ovlmid) {
	sys_overlay.warning("sys_overlay_unload_module(ovlmid=0x%x)", ovlmid);
	const auto _main = idm::withdraw<lv2_obj, ppu_overlay_module>(ovlmid);

	for (auto& seg : _main->segs)
	{
		vm::dealloc(seg.addr);
	}

	return CELL_OK;
}
