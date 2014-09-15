#pragma once

// Return codes
enum
{
	CELL_PRX_ERROR_ERROR                       = 0x80011001, // Error state
	CELL_PRX_ERROR_ILLEGAL_PERM                = 0x800110d1, // No permission to execute API
	CELL_PRX_ERROR_UNKNOWN_MODULE              = 0x8001112e, // Specified PRX could not be found
	CELL_PRX_ERROR_ALREADY_STARTED             = 0x80011133, // Specified PRX is already started
	CELL_PRX_ERROR_NOT_STARTED                 = 0x80011134, // Specified PRX is not started
	CELL_PRX_ERROR_ALREADY_STOPPED             = 0x80011135, // Specified PRX is already stopped
	CELL_PRX_ERROR_CAN_NOT_STOP                = 0x80011136, // Specified PRX must not be stopped
	CELL_PRX_ERROR_NOT_REMOVABLE               = 0x80011138, // Specified PRX must not be deleted
	CELL_PRX_ERROR_LIBRARY_NOT_YET_LINKED      = 0x8001113a, // Called unlinked function
	CELL_PRX_ERROR_LIBRARY_FOUND               = 0x8001113b, // Specified library is already registered
	CELL_PRX_ERROR_LIBRARY_NOTFOUND            = 0x8001113c, // Specified library is not registered
	CELL_PRX_ERROR_ILLEGAL_LIBRARY             = 0x8001113d, // Library structure is invalid
	CELL_PRX_ERROR_LIBRARY_INUSE               = 0x8001113e, // Library cannot be deleted because it is linked
	CELL_PRX_ERROR_ALREADY_STOPPING            = 0x8001113f, // Specified PRX is in the process of stopping
	CELL_PRX_ERROR_UNSUPPORTED_PRX_TYPE        = 0x80011148, // Specified PRX format is invalid and cannot be loaded
	CELL_PRX_ERROR_INVAL                       = 0x80011324, // Argument value is invalid
	CELL_PRX_ERROR_ILLEGAL_PROCESS             = 0x80011801, // Specified process does not exist
	CELL_PRX_ERROR_NO_LIBLV2                   = 0x80011881, // liblv2.sprx does not exist
	CELL_PRX_ERROR_UNSUPPORTED_ELF_TYPE        = 0x80011901, // ELF type of specified file is not supported
	CELL_PRX_ERROR_UNSUPPORTED_ELF_CLASS       = 0x80011902, // ELF class of specified file is not supported
	CELL_PRX_ERROR_UNDEFINED_SYMBOL            = 0x80011904, // References undefined symbols
	CELL_PRX_ERROR_UNSUPPORTED_RELOCATION_TYPE = 0x80011905, // Uses unsupported relocation type
	CELL_PRX_ERROR_ELF_IS_REGISTERED           = 0x80011910, // Fixed ELF is already registered
};

// Data types
struct sys_prx_load_module_option_t
{
	be_t<u64> size;
	be_t<u32> base_addr; // void*
};

struct sys_prx_start_module_option_t
{
	be_t<u64> size;
};

struct sys_prx_stop_module_option_t
{
	be_t<u64> size;
};

struct sys_prx_unload_module_option_t
{
	be_t<u64> size;
};

// Auxiliary data types
struct sys_prx_t
{
	u32 size;
	u32 address;
	std::string path;
	bool isStarted;

	sys_prx_t()
		: isStarted(false)
	{
	}
};

// SysCalls
s32 sys_prx_load_module(vm::ptr<const char> path, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt);
s32 sys_prx_load_module_on_memcontainer();
s32 sys_prx_load_module_by_fd();
s32 sys_prx_load_module_on_memcontainer_by_fd();
s32 sys_prx_start_module(s32 id, u32 args, u32 argp_addr, vm::ptr<be_t<u32>> modres, u64 flags, vm::ptr<sys_prx_start_module_option_t> pOpt);
s32 sys_prx_stop_module(s32 id, u32 args, u32 argp_addr, vm::ptr<be_t<u32>> modres, u64 flags, vm::ptr<sys_prx_stop_module_option_t> pOpt);
s32 sys_prx_unload_module(s32 id, u64 flags, vm::ptr<sys_prx_unload_module_option_t> pOpt);
s32 sys_prx_get_module_list();
s32 sys_prx_get_my_module_id();
s32 sys_prx_get_module_id_by_address();
s32 sys_prx_get_module_id_by_name();
s32 sys_prx_get_module_info();
s32 sys_prx_register_library(u32 lib_addr);
s32 sys_prx_unregister_library();
s32 sys_prx_get_ppu_guid();
s32 sys_prx_register_module();
s32 sys_prx_query_module();
s32 sys_prx_link_library();
s32 sys_prx_unlink_library();
s32 sys_prx_query_library();
s32 sys_prx_start();
s32 sys_prx_stop();
