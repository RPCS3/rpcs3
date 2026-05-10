#pragma once

#include "Emu/Cell/lv2/sys_mutex.h"
#include "Emu/Cell/lv2/sys_cond.h"
#include "Emu/Cell/lv2/sys_rwlock.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_semaphore.h"
#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_event_flag.h"

#include "Emu/Memory/vm_ptr.h"



// Error Codes
enum CellLv2DbgError : u32
{
	CELL_LV2DBG_ERROR_DEINVALIDPROCESSID = 0x80010401,
	CELL_LV2DBG_ERROR_DEINVALIDTHREADID = 0x80010402,
	CELL_LV2DBG_ERROR_DEILLEGALREGISTERTYPE = 0x80010403,
	CELL_LV2DBG_ERROR_DEILLEGALREGISTERNUMBER = 0x80010404,
	CELL_LV2DBG_ERROR_DEILLEGALTHREADSTATE = 0x80010405,
	CELL_LV2DBG_ERROR_DEINVALIDEFFECTIVEADDRESS = 0x80010406,
	CELL_LV2DBG_ERROR_DENOTFOUNDPROCESSID = 0x80010407,
	CELL_LV2DBG_ERROR_DENOMEM = 0x80010408,
	CELL_LV2DBG_ERROR_DEINVALIDARGUMENTS = 0x80010409,
	CELL_LV2DBG_ERROR_DENOTFOUNDFILE = 0x8001040a,
	CELL_LV2DBG_ERROR_DEINVALIDFILETYPE = 0x8001040b,
	CELL_LV2DBG_ERROR_DENOTFOUNDTHREADID = 0x8001040c,
	CELL_LV2DBG_ERROR_DEINVALIDTHREADSTATUS = 0x8001040d,
	CELL_LV2DBG_ERROR_DENOAVAILABLEPROCESSID = 0x8001040e,
	CELL_LV2DBG_ERROR_DENOTFOUNDEVENTHANDLER = 0x8001040f,
	CELL_LV2DBG_ERROR_DESPNOROOM = 0x80010410,
	CELL_LV2DBG_ERROR_DESPNOTFOUND = 0x80010411,
	CELL_LV2DBG_ERROR_DESPINPROCESS = 0x80010412,
	CELL_LV2DBG_ERROR_DEINVALIDPRIMARYSPUTHREADID = 0x80010413,
	CELL_LV2DBG_ERROR_DETHREADSTATEISNOTSTOPPED = 0x80010414,
	CELL_LV2DBG_ERROR_DEINVALIDTHREADTYPE = 0x80010415,
	CELL_LV2DBG_ERROR_DECONTINUEFAILED = 0x80010416,
	CELL_LV2DBG_ERROR_DESTOPFAILED = 0x80010417,
	CELL_LV2DBG_ERROR_DENOEXCEPTION = 0x80010418,
	CELL_LV2DBG_ERROR_DENOMOREEVENTQUE = 0x80010419,
	CELL_LV2DBG_ERROR_DEEVENTQUENOTCREATED = 0x8001041a,
	CELL_LV2DBG_ERROR_DEEVENTQUEOVERFLOWED = 0x8001041b,
	CELL_LV2DBG_ERROR_DENOTIMPLEMENTED = 0x8001041c,
	CELL_LV2DBG_ERROR_DEQUENOTREGISTERED = 0x8001041d,
	CELL_LV2DBG_ERROR_DENOMOREEVENTPROCESS = 0x8001041e,
	CELL_LV2DBG_ERROR_DEPROCESSNOTREGISTERED = 0x8001041f,
	CELL_LV2DBG_ERROR_DEEVENTDISCARDED = 0x80010420,
	CELL_LV2DBG_ERROR_DENOMORESYNCID = 0x80010421,
	CELL_LV2DBG_ERROR_DESYNCIDALREADYADDED = 0x80010422,
	CELL_LV2DBG_ERROR_DESYNCIDNOTFOUND = 0x80010423,
	CELL_LV2DBG_ERROR_DESYNCIDNOTACQUIRED = 0x80010424,
	CELL_LV2DBG_ERROR_DEPROCESSALREADYREGISTERED = 0x80010425,
	CELL_LV2DBG_ERROR_DEINVALIDLSADDRESS = 0x80010426,
	CELL_LV2DBG_ERROR_DEINVALIDOPERATION = 0x80010427,
	CELL_LV2DBG_ERROR_DEINVALIDMODULEID = 0x80010428,
	CELL_LV2DBG_ERROR_DEHANDLERALREADYREGISTERED = 0x80010429,
	CELL_LV2DBG_ERROR_DEINVALIDHANDLER = 0x8001042a,
	CELL_LV2DBG_ERROR_DEHANDLENOTREGISTERED = 0x8001042b,
	CELL_LV2DBG_ERROR_DEOPERATIONDENIED = 0x8001042c,
	CELL_LV2DBG_ERROR_DEHANDLERNOTINITIALIZED = 0x8001042d,
	CELL_LV2DBG_ERROR_DEHANDLERALREADYINITIALIZED = 0x8001042e,
	CELL_LV2DBG_ERROR_DEILLEGALCOREDUMPPARAMETER = 0x8001042f,
};

enum : u64
{
	SYS_DBG_PPU_THREAD_STOP = 0x0000000000000001ull,
	SYS_DBG_SPU_THREAD_GROUP_STOP = 0x0000000000000002ull,
	SYS_DBG_SYSTEM_PPU_THREAD_NOT_STOP = 0x0000000000000004ull,
	SYS_DBG_SYSTEM_SPU_THREAD_GROUP_NOT_STOP = 0x0000000000000008ull,
	SYS_DBG_NOT_EXE_CTRL_BY_COREDUMP_EVENT = 0x0000000000000010ull,
};

enum : u64
{
	SYS_DBG_PPU_EXCEPTION_TRAP = 0x0000000001000000ull,
	SYS_DBG_PPU_EXCEPTION_PREV_INST = 0x0000000002000000ull,
	SYS_DBG_PPU_EXCEPTION_ILL_INST = 0x0000000004000000ull,
	SYS_DBG_PPU_EXCEPTION_TEXT_HTAB_MISS = 0x0000000008000000ull,
	SYS_DBG_PPU_EXCEPTION_TEXT_SLB_MISS = 0x0000000010000000ull,
	SYS_DBG_PPU_EXCEPTION_DATA_HTAB_MISS = 0x0000000020000000ull,
	SYS_DBG_PPU_EXCEPTION_DATA_SLB_MISS = 0x0000000040000000ull,
	SYS_DBG_PPU_EXCEPTION_FLOAT = 0x0000000080000000ull,
	SYS_DBG_PPU_EXCEPTION_DABR_MATCH = 0x0000000100000000ull,
	SYS_DBG_PPU_EXCEPTION_ALIGNMENT = 0x0000000200000000ull,
	SYS_DBG_PPU_EXCEPTION_DATA_MAT = 0x0000002000000000ull,
};

enum : u64
{
	SYS_DBG_EVENT_CORE_DUMPED = 0x0000000000001000ull,
	SYS_DBG_EVENT_PPU_EXCEPTION_HANDLER_SIGNALED = 0x0000000000002000ull,
};

union sys_dbg_vr_t
{
	u8 byte[16];
	be_t<u16> halfword[8];
	be_t<u32> word[8];
	be_t<u64> dw[2];
};

struct sys_dbg_ppu_thread_context_t
{
	be_t<u64> gpr[32];
	be_t<u32> cr;
	be_t<u64> xer;
	be_t<u64> lr;
	be_t<u64> ctr;
	be_t<u64> pc;
	be_t<u64> fpr[32];
	be_t<u32> fpscr;
	sys_dbg_vr_t vr[32];
	sys_dbg_vr_t vscr;
};

union sys_dbg_spu_gpr_t
{
	u8 byte[16];
	be_t<u16> halfword[8];
	be_t<u32> word[4];
	be_t<u64> dw[2];
};

union sys_dbg_spu_fpscr_t
{
	u8 byte[16];
	be_t<u16> halfword[8];
	be_t<u32> word[4];
	be_t<u64> dw[2];
};

struct sys_dbg_spu_thread_context_t
{
	sys_dbg_spu_gpr_t gpr[128];
	be_t<u32> npc;
	be_t<u32> fpscr;
	be_t<u32> srr0;
	be_t<u32> spu_status;
	be_t<u64> spu_cfg;
	be_t<u32> mb_stat;
	be_t<u32> ppu_mb;
	be_t<u32> spu_mb[4];
	be_t<u32> decrementer;
	be_t<u64> mfc_cq_sr[96];
};

struct sys_dbg_spu_thread_context2_t
{
	sys_dbg_spu_gpr_t gpr[128];
	be_t<u32> npc;
	sys_dbg_spu_fpscr_t	fpscr;
	be_t<u32> srr0;
	be_t<u32> spu_status;
	be_t<u64> spu_cfg;
	be_t<u32> mb_stat;
	be_t<u32> ppu_mb;
	be_t<u32> spu_mb[4];
	be_t<u32> decrementer;
	be_t<u64> mfc_cq_sr[96];
};

struct sys_dbg_mutex_information_t
{
	sys_mutex_attribute_t attr;
	be_t<u64> owner;
	be_t<s32> lock_counter;
	be_t<s32> cond_ref_counter;
	be_t<u32> cond_id; // zero
	vm::bptr<u64> wait_id_list;
	be_t<u32> wait_threads_num;
	be_t<u32> wait_all_threads_num;
};

struct sys_dbg_cond_information_t
{
	sys_cond_attribute_t attr;
	be_t<u32> mutex_id;
	vm::bptr<u64> wait_id_list;
	be_t<u32> wait_threads_num;
	be_t<u32> wait_all_threads_num;
};

struct sys_dbg_rwlock_information_t
{
	sys_rwlock_attribute_t attr;
	be_t<u64> owner;
	vm::bptr<u64> r_wait_id_list;
	be_t<u32> r_wait_threads_num;
	be_t<u32> r_wait_all_threads_num;
	vm::bptr<u64> w_wait_id_list;
	be_t<u32> w_wait_threads_num;
	be_t<u32> w_wait_all_threads_num;
};

struct sys_dbg_event_queue_information_t
{
	sys_event_queue_attribute_t	attr;
	be_t<u64> event_queue_key;
	be_t<s32> queue_size;
	vm::bptr<u64> wait_id_list;
	be_t<u32> wait_threads_num;
	be_t<u32> wait_all_threads_num;
	vm::bptr<sys_event_t> equeue_list;
	be_t<u32> readable_equeue_num;
	be_t<u32> readable_all_equeue_num;
};

struct sys_dbg_semaphore_information_t
{
	sys_semaphore_attribute_t attr;
	be_t<s32> max_val;
	be_t<s32> cur_val;
	vm::bptr<u64> wait_id_list;
	be_t<u32> wait_threads_num;
	be_t<u32> wait_all_threads_num;
};

struct sys_dbg_lwmutex_information_t
{
	sys_lwmutex_attribute_t	attr;
	be_t<u64> owner;
	be_t<s32> lock_counter;
	vm::bptr<u64> wait_id_list;
	be_t<u32> wait_threads_num;
	be_t<u32> wait_all_threads_num;
};

struct sys_dbg_lwcond_information_t
{
	sys_lwcond_attribute_t attr;
	vm::bptr<sys_lwmutex_t> lwmutex;
	vm::bptr<u64> wait_id_list;
	be_t<u32> wait_threads_num;
	be_t<u32> wait_all_threads_num;
};

struct sys_dbg_event_flag_wait_information_t
{
	be_t<u64> bitptn;
	be_t<u32> mode;
};

struct sys_dbg_event_flag_information_t
{
	sys_event_flag_attribute_t attr;
	be_t<u64> cur_bitptn;
	vm::bptr<u64> wait_id_list;
	vm::bptr<sys_dbg_event_flag_wait_information_t> wait_info_list;
	be_t<u32> wait_threads_num;
	be_t<u32> wait_all_threads_num;
};

using dbg_exception_handler_t = void(u64 exception_type, u64 thread_id, u64 dar);

enum : u64
{
	SYS_VM_STATE_LOCKED = 0x0008ull,
	SYS_VM_STATE_DIRTY = 0x0010ull,
};

struct sys_vm_page_information_t
{
	be_t<u64> state;
};

enum : u64
{
	SYS_DBG_DABR_CTRL_READ = 0x0000000000000005ull,
	SYS_DBG_DABR_CTRL_WRITE = 0x0000000000000006ull,
	SYS_DBG_DABR_CTRL_CLEAR = 0x0000000000000000ull,
};

enum
{
	SYS_DBG_MAT_TRANSPARENT = 1,
	SYS_DBG_MAT_WRITE = 2,
	SYS_DBG_MAT_READ_WRITE = 4,
	SYS_MAT_GRANULARITY = 4096,
};

enum sys_dbg_coredump_parameter_t : s32
{
	SYS_DBG_COREDUMP_OFF,
	SYS_DBG_COREDUMP_ON_SAVE_TO_APP_HOME,
	SYS_DBG_COREDUMP_ON_SAVE_TO_DEV_MS,
	SYS_DBG_COREDUMP_ON_SAVE_TO_DEV_USB,
	SYS_DBG_COREDUMP_ON_SAVE_TO_DEV_HDD0,
};
