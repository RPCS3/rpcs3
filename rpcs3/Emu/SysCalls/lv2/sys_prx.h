#pragma once

namespace vm { using namespace ps3; }

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

struct sys_stub
{
	u8 s_size; // = 0x2c
	u8 s_unk0;
	be_t<u16> s_version; // = 0x1
	be_t<u16> s_unk1; // = 0x9 // flags?
	be_t<u16> s_imports;
	be_t<u32> s_unk2; // = 0x0
	be_t<u32> s_unk3; // = 0x0
	vm::bcptr<char> s_modulename;
	vm::bptr<u32> s_nid;
	vm::bptr<u32> s_text;
	be_t<u32> s_unk4; // = 0x0
	be_t<u32> s_unk5; // = 0x0
	be_t<u32> s_unk6; // = 0x0
	be_t<u32> s_unk7; // = 0x0
};

struct sys_proc_prx_param
{
	be_t<u32> size;
	be_t<u32> magic;
	be_t<u32> version;
	be_t<u32> pad0;
	be_t<u32> libentstart;
	be_t<u32> libentend;
	vm::bptr<sys_stub> libstubstart;
	vm::bptr<sys_stub> libstubend;
	be_t<u16> ver;
	be_t<u16> pad1;
	be_t<u32> pad2;
};

// Information about imported or exported libraries in PRX modules
struct sys_prx_library_info_t
{
	u8 size;
	u8 unk0;
	be_t<u16> version;
	be_t<u16> attributes;
	be_t<u16> num_func;
	be_t<u16> num_var;
	be_t<u16> num_tlsvar;
	u8 info_hash;
	u8 info_tlshash;
	u8 unk1[2];
	be_t<u32> name_addr;
	be_t<u32> fnid_addr;
	be_t<u32> fstub_addr;
	be_t<u32> unk4;
	be_t<u32> unk5;
	be_t<u32> unk6;
	be_t<u32> unk7;
};

// ELF file headers
struct sys_prx_param_t
{
	be_t<u32> size;
	be_t<u32> magic;
	be_t<u32> version;
	be_t<u32> unk0;
	be_t<u32> libentstart;
	be_t<u32> libentend;
	vm::bptr<sys_prx_library_info_t> libstubstart;
	vm::bptr<sys_prx_library_info_t> libstubend;
	be_t<u16> ver;
	be_t<u16> unk1;
	be_t<u32> unk2;
};

// PRX file headers
struct sys_prx_module_info_t
{
	be_t<u16> attributes;
	be_t<u16> version;
	char name[28];
	be_t<u32> toc;
	vm::bptr<sys_prx_library_info_t> exports_start;
	vm::bptr<sys_prx_library_info_t> exports_end;
	be_t<u32> imports_start;
	be_t<u32> imports_end;
};

// Relocation information of the SCE_PPURELA segment
struct sys_prx_relocation_info_t
{
	be_t<u64> offset;
	be_t<u16> unk0;
	u8 index_value;
	u8 index_addr;
	be_t<u32> type;
	vm::bptr<void, u64> ptr;
};


// Data types
struct sys_prx_load_module_option_t
{
	be_t<u64> size;
	vm::bptr<void> base_addr;
};

struct sys_prx_start_module_option_t
{
	be_t<u64> size;
	be_t<u64> put;
	vm::bptr<s32(int argc, vm::ptr<void> argv), u64> entry_point;
};

struct sys_prx_stop_module_option_t
{
	be_t<u64> size;
	be_t<u64> put;
	vm::bptr<s32(int argc, vm::ptr<void> argv), u64> entry_point;
};

struct sys_prx_unload_module_option_t
{
	be_t<u64> size;
};

// Auxiliary data types
struct lv2_prx_t
{
	const u32 id;

	bool is_started = false;

	vm::ptr<s32(int argc, vm::ptr<void> argv)> start = vm::null;
	vm::ptr<s32(int argc, vm::ptr<void> argv)> stop = vm::null;
	vm::ptr<s32()> exit = vm::null;

	lv2_prx_t();
};

REG_ID_TYPE(lv2_prx_t, 0x23); // SYS_PRX_OBJECT

// SysCalls
s32 sys_prx_load_module(vm::cptr<char> path, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt);
s32 sys_prx_load_module_list(s32 count, vm::cpptr<char> path_list, u64 flags, vm::ptr<sys_prx_load_module_option_t> pOpt, vm::ptr<u32> id_list);
s32 sys_prx_load_module_on_memcontainer();
s32 sys_prx_load_module_by_fd();
s32 sys_prx_load_module_on_memcontainer_by_fd();
s32 sys_prx_start_module(s32 id, u64 flags, vm::ptr<sys_prx_start_module_option_t> pOpt);
s32 sys_prx_stop_module(s32 id, u64 flags, vm::ptr<sys_prx_stop_module_option_t> pOpt);
s32 sys_prx_unload_module(s32 id, u64 flags, vm::ptr<sys_prx_unload_module_option_t> pOpt);
s32 sys_prx_get_module_list();
s32 sys_prx_get_my_module_id();
s32 sys_prx_get_module_id_by_address();
s32 sys_prx_get_module_id_by_name();
s32 sys_prx_get_module_info();
s32 sys_prx_register_library(vm::ptr<void> library);
s32 sys_prx_unregister_library(vm::ptr<void> library);
s32 sys_prx_get_ppu_guid();
s32 sys_prx_register_module();
s32 sys_prx_query_module();
s32 sys_prx_link_library();
s32 sys_prx_unlink_library();
s32 sys_prx_query_library();
s32 sys_prx_start();
s32 sys_prx_stop();
