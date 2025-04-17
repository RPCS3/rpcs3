#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Memory/vm_reservation.h"
#include "Emu/Memory/vm_locking.h"

#include "Emu/Cell/PPUFunction.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "sys_sync.h"
#include "sys_lwmutex.h"
#include "sys_lwcond.h"
#include "sys_mutex.h"
#include "sys_cond.h"
#include "sys_event.h"
#include "sys_event_flag.h"
#include "sys_game.h"
#include "sys_interrupt.h"
#include "sys_memory.h"
#include "sys_mmapper.h"
#include "sys_net.h"
#include "sys_overlay.h"
#include "sys_ppu_thread.h"
#include "sys_process.h"
#include "sys_prx.h"
#include "sys_rsx.h"
#include "sys_rwlock.h"
#include "sys_semaphore.h"
#include "sys_spu.h"
#include "sys_time.h"
#include "sys_timer.h"
#include "sys_trace.h"
#include "sys_tty.h"
#include "sys_usbd.h"
#include "sys_vm.h"
#include "sys_fs.h"
#include "sys_dbg.h"
#include "sys_gamepad.h"
#include "sys_ss.h"
#include "sys_gpio.h"
#include "sys_config.h"
#include "sys_bdemu.h"
#include "sys_btsetting.h"
#include "sys_console.h"
#include "sys_hid.h"
#include "sys_io.h"
#include "sys_rsxaudio.h"
#include "sys_sm.h"
#include "sys_storage.h"
#include "sys_uart.h"
#include "sys_crypto_engine.h"

#include <algorithm>
#include <optional>
#include <deque>
#include <thread>
#include "util/tsc.hpp"
#include "util/sysinfo.hpp"
#include "util/init_mutex.hpp"

#if defined(ARCH_X64)
#ifdef _MSC_VER
#include <intrin.h>
#include <immintrin.h>
#else
#include <x86intrin.h>
#endif
#endif


extern std::string ppu_get_syscall_name(u64 code);

namespace rsx
{
	void set_rsx_yield_flag() noexcept;
}

using spu_rdata_t = decltype(spu_thread::rdata);
extern u32 compute_rdata_hash32(const spu_rdata_t& _src);

template <>
void fmt_class_string<ppu_syscall_code>::format(std::string& out, u64 arg)
{
	out += ppu_get_syscall_name(arg);
}

template <>
void fmt_class_string<lv2_protocol>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case SYS_SYNC_FIFO: return "FIFO";
		case SYS_SYNC_PRIORITY: return "PRIO";
		case SYS_SYNC_PRIORITY_INHERIT: return "PRIO-INHER";
		case SYS_SYNC_RETRY: return "RETRY";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<lv2_obj::name_64>::format(std::string& out, u64 arg)
{
	out += lv2_obj::name64(get_object(arg).data);
}

static void null_func_(ppu_thread& ppu, ppu_opcode_t, be_t<u32>* this_op, ppu_intrp_func*)
{
	ppu_log.todo("Unimplemented syscall %s -> CELL_OK (r3=0x%llx, r4=0x%llx, r5=0x%llx, r6=0x%llx, r7=0x%llx, r8=0x%llx, r9=0x%llx, r10=0x%llx)", ppu_syscall_code(ppu.gpr[11]),
		ppu.gpr[3], ppu.gpr[4], ppu.gpr[5], ppu.gpr[6], ppu.gpr[7], ppu.gpr[8], ppu.gpr[9], ppu.gpr[10]);

	ppu.gpr[3] = 0;
	ppu.cia = vm::get_addr(this_op) + 4;
}

static void uns_func_(ppu_thread& ppu, ppu_opcode_t, be_t<u32>* this_op, ppu_intrp_func*)
{
	ppu_log.trace("Unused syscall %d -> ENOSYS", ppu.gpr[11]);
	ppu.gpr[3] = CELL_ENOSYS;
	ppu.cia = vm::get_addr(this_op) + 4;
}

// Bind Syscall
#define BIND_SYSC(func) {BIND_FUNC(func), #func}
#define NULL_FUNC(name) {null_func_, #name}

constexpr std::pair<ppu_intrp_func_t, std::string_view> null_func{null_func_, ""};
constexpr std::pair<ppu_intrp_func_t, std::string_view> uns_func{uns_func_, ""};

// UNS = Unused
// ROOT = Root
// DBG = Debug
// DEX..DECR = Unavailable on retail consoles
// PM = Product Mode
// AuthID = Authentication ID
const std::array<std::pair<ppu_intrp_func_t, std::string_view>, 1024> g_ppu_syscall_table
{
	null_func,
	BIND_SYSC(sys_process_getpid),                          //1   (0x001)
	BIND_SYSC(sys_process_wait_for_child),                  //2   (0x002)  ROOT
	BIND_SYSC(sys_process_exit3),                           //3   (0x003)
	BIND_SYSC(sys_process_get_status),                      //4   (0x004)  DBG
	BIND_SYSC(sys_process_detach_child),                    //5   (0x005)  DBG

	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //6-11  UNS

	BIND_SYSC(sys_process_get_number_of_object),            //12  (0x00C)
	BIND_SYSC(sys_process_get_id),                          //13  (0x00D)
	BIND_SYSC(sys_process_is_spu_lock_line_reservation_address), //14  (0x00E)

	uns_func, uns_func, uns_func,                           //15-17  UNS

	BIND_SYSC(sys_process_getppid),                         //18  (0x012)
	BIND_SYSC(sys_process_kill),                            //19  (0x013)
	uns_func,                                               //20  (0x014)  UNS
	NULL_FUNC(_sys_process_spawn),                          //21  (0x015)  DBG
	BIND_SYSC(_sys_process_exit),                           //22  (0x016)
	BIND_SYSC(sys_process_wait_for_child2),                 //23  (0x017)  DBG
	null_func,//BIND_SYSC(),                                //24  (0x018)  DBG
	BIND_SYSC(sys_process_get_sdk_version),                 //25  (0x019)
	BIND_SYSC(_sys_process_exit2),                          //26  (0x01A)
	BIND_SYSC(sys_process_spawns_a_self2),                  //27  (0x01B)  DBG
	NULL_FUNC(_sys_process_get_number_of_object),           //28  (0x01C)  ROOT
	BIND_SYSC(sys_process_get_id2),                         //29  (0x01D)  ROOT
	BIND_SYSC(_sys_process_get_paramsfo),                   //30  (0x01E)
	NULL_FUNC(sys_process_get_ppu_guid),                    //31  (0x01F)

	uns_func, uns_func ,uns_func , uns_func ,uns_func, uns_func ,uns_func, uns_func ,uns_func, //32-40  UNS

	BIND_SYSC(_sys_ppu_thread_exit),                        //41  (0x029)
	uns_func,                                               //42  (0x02A)  UNS
	BIND_SYSC(sys_ppu_thread_yield),                        //43  (0x02B)
	BIND_SYSC(sys_ppu_thread_join),                         //44  (0x02C)
	BIND_SYSC(sys_ppu_thread_detach),                       //45  (0x02D)
	BIND_SYSC(sys_ppu_thread_get_join_state),               //46  (0x02E)
	BIND_SYSC(sys_ppu_thread_set_priority),                 //47  (0x02F)  DBG
	BIND_SYSC(sys_ppu_thread_get_priority),                 //48  (0x030)
	BIND_SYSC(sys_ppu_thread_get_stack_information),        //49  (0x031)
	BIND_SYSC(sys_ppu_thread_stop),                         //50  (0x032)  ROOT
	BIND_SYSC(sys_ppu_thread_restart),                      //51  (0x033)  ROOT
	BIND_SYSC(_sys_ppu_thread_create),                      //52  (0x034)  DBG
	BIND_SYSC(sys_ppu_thread_start),                        //53  (0x035)
	null_func,//BIND_SYSC(sys_ppu_...),                     //54  (0x036)  ROOT
	null_func,//BIND_SYSC(sys_ppu_...),                     //55  (0x037)  ROOT
	BIND_SYSC(sys_ppu_thread_rename),                       //56  (0x038)
	BIND_SYSC(sys_ppu_thread_recover_page_fault),           //57  (0x039)
	BIND_SYSC(sys_ppu_thread_get_page_fault_context),       //58  (0x03A)
	uns_func,                                               //59  (0x03B)  UNS
	BIND_SYSC(sys_trace_create),                            //60  (0x03C)
	BIND_SYSC(sys_trace_start),                             //61  (0x03D)
	BIND_SYSC(sys_trace_stop),                              //62  (0x03E)
	BIND_SYSC(sys_trace_update_top_index),                  //63  (0x03F)
	BIND_SYSC(sys_trace_destroy),                           //64  (0x040)
	BIND_SYSC(sys_trace_drain),                             //65  (0x041)
	BIND_SYSC(sys_trace_attach_process),                    //66  (0x042)
	BIND_SYSC(sys_trace_allocate_buffer),                   //67  (0x043)
	BIND_SYSC(sys_trace_free_buffer),                       //68  (0x044)
	BIND_SYSC(sys_trace_create2),                           //69  (0x045)
	BIND_SYSC(sys_timer_create),                            //70  (0x046)
	BIND_SYSC(sys_timer_destroy),                           //71  (0x047)
	BIND_SYSC(sys_timer_get_information),                   //72  (0x048)
	BIND_SYSC(_sys_timer_start),                            //73  (0x049)
	BIND_SYSC(sys_timer_stop),                              //74  (0x04A)
	BIND_SYSC(sys_timer_connect_event_queue),               //75  (0x04B)
	BIND_SYSC(sys_timer_disconnect_event_queue),            //76  (0x04C)
	NULL_FUNC(sys_trace_create2_in_cbepm),                  //77  (0x04D)
	null_func,//BIND_SYSC(sys_trace_...),                   //78  (0x04E)
	uns_func,                                               //79  (0x04F)  UNS
	NULL_FUNC(sys_interrupt_tag_create),                    //80  (0x050)
	BIND_SYSC(sys_interrupt_tag_destroy),                   //81  (0x051)
	BIND_SYSC(sys_event_flag_create),                       //82  (0x052)
	BIND_SYSC(sys_event_flag_destroy),                      //83  (0x053)
	BIND_SYSC(_sys_interrupt_thread_establish),             //84  (0x054)
	BIND_SYSC(sys_event_flag_wait),                         //85  (0x055)
	BIND_SYSC(sys_event_flag_trywait),                      //86  (0x056)
	BIND_SYSC(sys_event_flag_set),                          //87  (0x057)
	BIND_SYSC(sys_interrupt_thread_eoi),                    //88  (0x058)
	BIND_SYSC(_sys_interrupt_thread_disestablish),          //89  (0x059)
	BIND_SYSC(sys_semaphore_create),                        //90  (0x05A)
	BIND_SYSC(sys_semaphore_destroy),                       //91  (0x05B)
	BIND_SYSC(sys_semaphore_wait),                          //92  (0x05C)
	BIND_SYSC(sys_semaphore_trywait),                       //93  (0x05D)
	BIND_SYSC(sys_semaphore_post),                          //94  (0x05E)
	BIND_SYSC(_sys_lwmutex_create),                         //95  (0x05F)
	BIND_SYSC(_sys_lwmutex_destroy),                        //96  (0x060)
	BIND_SYSC(_sys_lwmutex_lock),                           //97  (0x061)
	BIND_SYSC(_sys_lwmutex_unlock),                         //98  (0x062)
	BIND_SYSC(_sys_lwmutex_trylock),                        //99  (0x063)
	BIND_SYSC(sys_mutex_create),                            //100 (0x064)
	BIND_SYSC(sys_mutex_destroy),                           //101 (0x065)
	BIND_SYSC(sys_mutex_lock),                              //102 (0x066)
	BIND_SYSC(sys_mutex_trylock),                           //103 (0x067)
	BIND_SYSC(sys_mutex_unlock),                            //104 (0x068)
	BIND_SYSC(sys_cond_create),                             //105 (0x069)
	BIND_SYSC(sys_cond_destroy),                            //106 (0x06A)
	BIND_SYSC(sys_cond_wait),                               //107 (0x06B)
	BIND_SYSC(sys_cond_signal),                             //108 (0x06C)
	BIND_SYSC(sys_cond_signal_all),                         //109 (0x06D)
	BIND_SYSC(sys_cond_signal_to),                          //110 (0x06E)
	BIND_SYSC(_sys_lwcond_create),                          //111 (0x06F)
	BIND_SYSC(_sys_lwcond_destroy),                         //112 (0x070)
	BIND_SYSC(_sys_lwcond_queue_wait),                      //113 (0x071)
	BIND_SYSC(sys_semaphore_get_value),                     //114 (0x072)
	BIND_SYSC(_sys_lwcond_signal),                          //115 (0x073)
	BIND_SYSC(_sys_lwcond_signal_all),                      //116 (0x074)
	BIND_SYSC(_sys_lwmutex_unlock2),                        //117 (0x075)
	BIND_SYSC(sys_event_flag_clear),                        //118 (0x076)
	BIND_SYSC(sys_time_get_rtc),                            //119 (0x077)  ROOT
	BIND_SYSC(sys_rwlock_create),                           //120 (0x078)
	BIND_SYSC(sys_rwlock_destroy),                          //121 (0x079)
	BIND_SYSC(sys_rwlock_rlock),                            //122 (0x07A)
	BIND_SYSC(sys_rwlock_tryrlock),                         //123 (0x07B)
	BIND_SYSC(sys_rwlock_runlock),                          //124 (0x07C)
	BIND_SYSC(sys_rwlock_wlock),                            //125 (0x07D)
	BIND_SYSC(sys_rwlock_trywlock),                         //126 (0x07E)
	BIND_SYSC(sys_rwlock_wunlock),                          //127 (0x07F)
	BIND_SYSC(sys_event_queue_create),                      //128 (0x080)
	BIND_SYSC(sys_event_queue_destroy),                     //129 (0x081)
	BIND_SYSC(sys_event_queue_receive),                     //130 (0x082)
	BIND_SYSC(sys_event_queue_tryreceive),                  //131 (0x083)
	BIND_SYSC(sys_event_flag_cancel),                       //132 (0x084)
	BIND_SYSC(sys_event_queue_drain),                       //133 (0x085)
	BIND_SYSC(sys_event_port_create),                       //134 (0x086)
	BIND_SYSC(sys_event_port_destroy),                      //135 (0x087)
	BIND_SYSC(sys_event_port_connect_local),                //136 (0x088)
	BIND_SYSC(sys_event_port_disconnect),                   //137 (0x089)
	BIND_SYSC(sys_event_port_send),                         //138 (0x08A)
	BIND_SYSC(sys_event_flag_get),                          //139 (0x08B)
	BIND_SYSC(sys_event_port_connect_ipc),                  //140 (0x08C)
	BIND_SYSC(sys_timer_usleep),                            //141 (0x08D)
	BIND_SYSC(sys_timer_sleep),                             //142 (0x08E)
	BIND_SYSC(sys_time_set_timezone),                       //143 (0x08F)  ROOT
	BIND_SYSC(sys_time_get_timezone),                       //144 (0x090)
	BIND_SYSC(sys_time_get_current_time),                   //145 (0x091)
	BIND_SYSC(sys_time_set_current_time),                   //146 (0x092)  ROOT
	BIND_SYSC(sys_time_get_timebase_frequency),             //147 (0x093)
	BIND_SYSC(_sys_rwlock_trywlock),                        //148 (0x094)
	NULL_FUNC(sys_time_get_system_time),                    //149 (0x095)
	BIND_SYSC(sys_raw_spu_create_interrupt_tag),            //150 (0x096)
	BIND_SYSC(sys_raw_spu_set_int_mask),                    //151 (0x097)
	BIND_SYSC(sys_raw_spu_get_int_mask),                    //152 (0x098)
	BIND_SYSC(sys_raw_spu_set_int_stat),                    //153 (0x099)
	BIND_SYSC(sys_raw_spu_get_int_stat),                    //154 (0x09A)
	BIND_SYSC(_sys_spu_image_get_information),              //155 (0x09B)
	BIND_SYSC(sys_spu_image_open),                          //156 (0x09C)
	BIND_SYSC(_sys_spu_image_import),                       //157 (0x09D)
	BIND_SYSC(_sys_spu_image_close),                        //158 (0x09E)
	BIND_SYSC(_sys_spu_image_get_segments),                 //159 (0x09F)
	BIND_SYSC(sys_raw_spu_create),                          //160 (0x0A0)
	BIND_SYSC(sys_raw_spu_destroy),                         //161 (0x0A1)
	uns_func,                                               //162 (0x0A2)  UNS
	BIND_SYSC(sys_raw_spu_read_puint_mb),                   //163 (0x0A3)
	uns_func,                                               //164 (0x0A4)  UNS
	BIND_SYSC(sys_spu_thread_get_exit_status),              //165 (0x0A5)
	BIND_SYSC(sys_spu_thread_set_argument),                 //166 (0x0A6)
	NULL_FUNC(sys_spu_thread_group_start_on_exit),          //167 (0x0A7)
	uns_func,                                               //168 (0x0A8)  UNS
	BIND_SYSC(sys_spu_initialize),                          //169 (0x0A9)
	BIND_SYSC(sys_spu_thread_group_create),                 //170 (0x0AA)
	BIND_SYSC(sys_spu_thread_group_destroy),                //171 (0x0AB)
	BIND_SYSC(sys_spu_thread_initialize),                   //172 (0x0AC)
	BIND_SYSC(sys_spu_thread_group_start),                  //173 (0x0AD)
	BIND_SYSC(sys_spu_thread_group_suspend),                //174 (0x0AE)
	BIND_SYSC(sys_spu_thread_group_resume),                 //175 (0x0AF)
	BIND_SYSC(sys_spu_thread_group_yield),                  //176 (0x0B0)
	BIND_SYSC(sys_spu_thread_group_terminate),              //177 (0x0B1)
	BIND_SYSC(sys_spu_thread_group_join),                   //178 (0x0B2)
	BIND_SYSC(sys_spu_thread_group_set_priority),           //179 (0x0B3)
	BIND_SYSC(sys_spu_thread_group_get_priority),           //180 (0x0B4)
	BIND_SYSC(sys_spu_thread_write_ls),                     //181 (0x0B5)
	BIND_SYSC(sys_spu_thread_read_ls),                      //182 (0x0B6)
	uns_func,                                               //183 (0x0B7)  UNS
	BIND_SYSC(sys_spu_thread_write_snr),                    //184 (0x0B8)
	BIND_SYSC(sys_spu_thread_group_connect_event),          //185 (0x0B9)
	BIND_SYSC(sys_spu_thread_group_disconnect_event),       //186 (0x0BA)
	BIND_SYSC(sys_spu_thread_set_spu_cfg),                  //187 (0x0BB)
	BIND_SYSC(sys_spu_thread_get_spu_cfg),                  //188 (0x0BC)
	uns_func,                                               //189 (0x0BD)  UNS
	BIND_SYSC(sys_spu_thread_write_spu_mb),                 //190 (0x0BE)
	BIND_SYSC(sys_spu_thread_connect_event),                //191 (0x0BF)
	BIND_SYSC(sys_spu_thread_disconnect_event),             //192 (0x0C0)
	BIND_SYSC(sys_spu_thread_bind_queue),                   //193 (0x0C1)
	BIND_SYSC(sys_spu_thread_unbind_queue),                 //194 (0x0C2)
	uns_func,                                               //195 (0x0C3)  UNS
	BIND_SYSC(sys_raw_spu_set_spu_cfg),                     //196 (0x0C4)
	BIND_SYSC(sys_raw_spu_get_spu_cfg),                     //197 (0x0C5)
	BIND_SYSC(sys_spu_thread_recover_page_fault),           //198 (0x0C6)
	BIND_SYSC(sys_raw_spu_recover_page_fault),              //199 (0x0C7)

	null_func, null_func, null_func, null_func, null_func,  //204  UNS?
	null_func, null_func, null_func, null_func, null_func,  //209  UNS?
	null_func, null_func, null_func,                        //212  UNS?
	BIND_SYSC(sys_console_write2),                          //213 (0x0D5)
	null_func,                                              //214  UNS?

	NULL_FUNC(sys_dbg_mat_set_condition),                   //215 (0x0D7)
	NULL_FUNC(sys_dbg_mat_get_condition),                   //216 (0x0D8)
	uns_func,//BIND_SYSC(sys_dbg_...),                      //217 (0x0D9) DBG  UNS?
	uns_func,//BIND_SYSC(sys_dbg_...),                      //218 (0x0DA) DBG  UNS?
	uns_func,//BIND_SYSC(sys_dbg_...),                      //219 (0x0DB) DBG  UNS?

	null_func, null_func, null_func, null_func, null_func,  //224  UNS
	null_func, null_func, null_func, null_func, null_func,  //229  UNS?

	BIND_SYSC(sys_isolated_spu_create),                     //230 (0x0E6)  ROOT
	BIND_SYSC(sys_isolated_spu_destroy),                    //231 (0x0E7)  ROOT
	BIND_SYSC(sys_isolated_spu_start),                      //232 (0x0E8)  ROOT
	BIND_SYSC(sys_isolated_spu_create_interrupt_tag),       //233 (0x0E9)  ROOT
	BIND_SYSC(sys_isolated_spu_set_int_mask),               //234 (0x0EA)  ROOT
	BIND_SYSC(sys_isolated_spu_get_int_mask),               //235 (0x0EB)  ROOT
	BIND_SYSC(sys_isolated_spu_set_int_stat),               //236 (0x0EC)  ROOT
	BIND_SYSC(sys_isolated_spu_get_int_stat),               //237 (0x0ED)  ROOT
	BIND_SYSC(sys_isolated_spu_set_spu_cfg),                //238 (0x0EE)  ROOT
	BIND_SYSC(sys_isolated_spu_get_spu_cfg),                //239 (0x0EF)  ROOT
	BIND_SYSC(sys_isolated_spu_read_puint_mb),              //240 (0x0F0)  ROOT
	uns_func, uns_func, uns_func,                           //241-243 ROOT  UNS
	NULL_FUNC(sys_spu_thread_group_system_set_next_group),  //244 (0x0F4)  ROOT
	NULL_FUNC(sys_spu_thread_group_system_unset_next_group),//245 (0x0F5)  ROOT
	NULL_FUNC(sys_spu_thread_group_system_set_switch_group),//246 (0x0F6)  ROOT
	NULL_FUNC(sys_spu_thread_group_system_unset_switch_group),//247 (0x0F7)  ROOT
	null_func,//BIND_SYSC(sys_spu_thread_group...),         //248 (0x0F8)  ROOT
	null_func,//BIND_SYSC(sys_spu_thread_group...),         //249 (0x0F9)  ROOT
	BIND_SYSC(sys_spu_thread_group_set_cooperative_victims),//250 (0x0FA)
	BIND_SYSC(sys_spu_thread_group_connect_event_all_threads), //251 (0x0FB)
	BIND_SYSC(sys_spu_thread_group_disconnect_event_all_threads), //252 (0x0FC)
	BIND_SYSC(sys_spu_thread_group_syscall_253),            //253 (0x0FD)
	BIND_SYSC(sys_spu_thread_group_log),                    //254 (0x0FE)

	uns_func, uns_func, uns_func, uns_func, uns_func,       //255-259  UNS

	BIND_SYSC(sys_spu_image_open_by_fd),                    //260 (0x104)

	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //261-269  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //270-279  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //280-289  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //290-299  UNS

	BIND_SYSC(sys_vm_memory_map),                           //300 (0x12C)
	BIND_SYSC(sys_vm_unmap),                                //301 (0x12D)
	BIND_SYSC(sys_vm_append_memory),                        //302 (0x12E)
	BIND_SYSC(sys_vm_return_memory),                        //303 (0x12F)
	BIND_SYSC(sys_vm_lock),                                 //304 (0x130)
	BIND_SYSC(sys_vm_unlock),                               //305 (0x131)
	BIND_SYSC(sys_vm_touch),                                //306 (0x132)
	BIND_SYSC(sys_vm_flush),                                //307 (0x133)
	BIND_SYSC(sys_vm_invalidate),                           //308 (0x134)
	BIND_SYSC(sys_vm_store),                                //309 (0x135)
	BIND_SYSC(sys_vm_sync),                                 //310 (0x136)
	BIND_SYSC(sys_vm_test),                                 //311 (0x137)
	BIND_SYSC(sys_vm_get_statistics),                       //312 (0x138)
	BIND_SYSC(sys_vm_memory_map_different),                 //313 (0x139)
	null_func,//BIND_SYSC(sys_...),                         //314 (0x13A)
	null_func,//BIND_SYSC(sys_...),                         //315 (0x13B)

	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //316-323  UNS

	BIND_SYSC(sys_memory_container_create),                 //324 (0x144)  DBG
	BIND_SYSC(sys_memory_container_destroy),                //325 (0x145)  DBG
	BIND_SYSC(sys_mmapper_allocate_fixed_address),          //326 (0x146)
	BIND_SYSC(sys_mmapper_enable_page_fault_notification),  //327 (0x147)
	BIND_SYSC(sys_mmapper_allocate_shared_memory_from_container_ext), //328 (0x148)
	BIND_SYSC(sys_mmapper_free_shared_memory),              //329 (0x149)
	BIND_SYSC(sys_mmapper_allocate_address),                //330 (0x14A)
	BIND_SYSC(sys_mmapper_free_address),                    //331 (0x14B)
	BIND_SYSC(sys_mmapper_allocate_shared_memory),          //332 (0x14C)
	NULL_FUNC(sys_mmapper_set_shared_memory_flag),          //333(0x14D)
	BIND_SYSC(sys_mmapper_map_shared_memory),               //334 (0x14E)
	BIND_SYSC(sys_mmapper_unmap_shared_memory),             //335 (0x14F)
	BIND_SYSC(sys_mmapper_change_address_access_right),     //336 (0x150)
	BIND_SYSC(sys_mmapper_search_and_map),                  //337 (0x151)
	NULL_FUNC(sys_mmapper_get_shared_memory_attribute),     //338 (0x152)
	BIND_SYSC(sys_mmapper_allocate_shared_memory_ext),      //339 (0x153)
	null_func,//BIND_SYSC(sys_...),                         //340 (0x154)
	BIND_SYSC(sys_memory_container_create),                 //341 (0x155)
	BIND_SYSC(sys_memory_container_destroy),                //342 (0x156)
	BIND_SYSC(sys_memory_container_get_size),               //343 (0x157)
	NULL_FUNC(sys_memory_budget_set),                       //344 (0x158)
	BIND_SYSC(sys_memory_container_destroy_parent_with_childs), //345 (0x159)
	null_func,//BIND_SYSC(sys_memory_...),                  //346 (0x15A)
	uns_func,                                               //347 (0x15B)  UNS
	BIND_SYSC(sys_memory_allocate),                         //348 (0x15C)
	BIND_SYSC(sys_memory_free),                             //349 (0x15D)
	BIND_SYSC(sys_memory_allocate_from_container),          //350 (0x15E)
	BIND_SYSC(sys_memory_get_page_attribute),               //351 (0x15F)
	BIND_SYSC(sys_memory_get_user_memory_size),             //352 (0x160)
	BIND_SYSC(sys_memory_get_user_memory_stat),             //353 (0x161)
	null_func,//BIND_SYSC(sys_memory_...),                  //354 (0x162)
	null_func,//BIND_SYSC(sys_memory_...),                  //355 (0x163)
	NULL_FUNC(sys_memory_allocate_colored),                 //356 (0x164)
	null_func,//BIND_SYSC(sys_memory_...),                  //357 (0x165)
	null_func,//BIND_SYSC(sys_memory_...),                  //358 (0x166)
	null_func,//BIND_SYSC(sys_memory_...),                  //359 (0x167)
	null_func,//BIND_SYSC(sys_memory_...),                  //360 (0x168)
	NULL_FUNC(sys_memory_allocate_from_container_colored),  //361 (0x169)
	BIND_SYSC(sys_mmapper_allocate_shared_memory_from_container),//362 (0x16A)
	null_func,//BIND_SYSC(sys_mmapper_...),                 //363 (0x16B)
	null_func,//BIND_SYSC(sys_mmapper_...),                 //364 (0x16C)
	uns_func, uns_func,                                     //366 (0x16E)  UNS
	BIND_SYSC(sys_uart_initialize),                         //367 (0x16F)  ROOT
	BIND_SYSC(sys_uart_receive),                            //368 (0x170)  ROOT
	BIND_SYSC(sys_uart_send),                               //369 (0x171)  ROOT
	BIND_SYSC(sys_uart_get_params),                         //370 (0x172)  ROOT
	uns_func,                                               //371 (0x173)  UNS
	BIND_SYSC(_sys_game_watchdog_start),                    //372 (0x174)
	BIND_SYSC(_sys_game_watchdog_stop),                     //373 (0x175)
	BIND_SYSC(_sys_game_watchdog_clear),                    //374 (0x176)
	BIND_SYSC(_sys_game_set_system_sw_version),             //375 (0x177)  ROOT
	BIND_SYSC(_sys_game_get_system_sw_version),             //376 (0x178)  ROOT
	BIND_SYSC(sys_sm_set_shop_mode),                        //377 (0x179)  ROOT
	BIND_SYSC(sys_sm_get_ext_event2),                       //378 (0x17A)  ROOT
	BIND_SYSC(sys_sm_shutdown),                             //379 (0x17B)  ROOT
	BIND_SYSC(sys_sm_get_params),                           //380 (0x17C)  DBG
	NULL_FUNC(sys_sm_get_inter_lpar_parameter),             //381 (0x17D)  ROOT
	NULL_FUNC(sys_sm_initialize),                           //382 (0x17E)  ROOT
	NULL_FUNC(sys_game_get_temperature),                    //383 (0x17F)  ROOT
	NULL_FUNC(sys_sm_get_tzpb),                             //384 (0x180)  ROOT
	NULL_FUNC(sys_sm_request_led),                          //385 (0x181)  ROOT
	BIND_SYSC(sys_sm_control_led),                          //386 (0x182)  ROOT
	NULL_FUNC(sys_sm_get_system_info),                      //387 (0x183)  DBG
	BIND_SYSC(sys_sm_ring_buzzer2),                         //388 (0x184)  ROOT
	NULL_FUNC(sys_sm_set_fan_policy),                       //389 (0x185)  PM
	NULL_FUNC(sys_sm_request_error_log),                    //390 (0x186)  ROOT
	NULL_FUNC(sys_sm_request_be_count),                     //391 (0x187)  ROOT
	BIND_SYSC(sys_sm_ring_buzzer),                          //392 (0x188)  ROOT
	NULL_FUNC(sys_sm_get_hw_config),                        //393 (0x189)  ROOT
	NULL_FUNC(sys_sm_request_scversion),                    //394 (0x18A)  ROOT
	NULL_FUNC(sys_sm_request_system_event_log),             //395 (0x18B)  PM
	NULL_FUNC(sys_sm_set_rtc_alarm),                        //396 (0x18C)  ROOT
	NULL_FUNC(sys_sm_get_rtc_alarm),                        //397 (0x18D)  ROOT
	BIND_SYSC(sys_console_write),                           //398 (0x18E)  ROOT
	uns_func,                                               //399 (0x18F)  UNS
	null_func,//BIND_SYSC(sys_sm_...),                      //400 (0x190)  PM
	null_func,//BIND_SYSC(sys_sm_...),                      //401 (0x191)  ROOT
	BIND_SYSC(sys_tty_read),                                //402 (0x192)
	BIND_SYSC(sys_tty_write),                               //403 (0x193)
	null_func,//BIND_SYSC(sys_...),                         //404 (0x194)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //405 (0x195)  PM
	null_func,//BIND_SYSC(sys_...),                         //406 (0x196)  PM
	null_func,//BIND_SYSC(sys_...),                         //407 (0x197)  PM
	NULL_FUNC(sys_sm_get_tzpb),                             //408 (0x198)  PM
	NULL_FUNC(sys_sm_get_fan_policy),                       //409 (0x199)  PM
	BIND_SYSC(_sys_game_board_storage_read),                //410 (0x19A)
	BIND_SYSC(_sys_game_board_storage_write),               //411 (0x19B)
	BIND_SYSC(_sys_game_get_rtc_status),                    //412 (0x19C)
	null_func,//BIND_SYSC(sys_...),                         //413 (0x19D)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //414 (0x19E)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //415 (0x19F)  ROOT

	uns_func, uns_func, uns_func, uns_func,                 //416-419  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //420-429  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //430-439  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //440-449  UNS

	BIND_SYSC(sys_overlay_load_module),                     //450 (0x1C2)
	BIND_SYSC(sys_overlay_unload_module),                   //451 (0x1C3)
	NULL_FUNC(sys_overlay_get_module_list),                 //452 (0x1C4)
	NULL_FUNC(sys_overlay_get_module_info),                 //453 (0x1C5)
	BIND_SYSC(sys_overlay_load_module_by_fd),               //454 (0x1C6)
	NULL_FUNC(sys_overlay_get_module_info2),                //455 (0x1C7)
	NULL_FUNC(sys_overlay_get_sdk_version),                 //456 (0x1C8)
	NULL_FUNC(sys_overlay_get_module_dbg_info),             //457 (0x1C9)
	NULL_FUNC(sys_overlay_get_module_dbg_info),             //458 (0x1CA)
	uns_func,                                               //459 (0x1CB)  UNS
	NULL_FUNC(sys_prx_dbg_get_module_id_list),              //460 (0x1CC)  ROOT
	BIND_SYSC(_sys_prx_get_module_id_by_address),           //461 (0x1CD)
	uns_func,                                               //462 (0x1CE)  DEX
	BIND_SYSC(_sys_prx_load_module_by_fd),                  //463 (0x1CF)
	BIND_SYSC(_sys_prx_load_module_on_memcontainer_by_fd),  //464 (0x1D0)
	BIND_SYSC(_sys_prx_load_module_list),                   //465 (0x1D1)
	BIND_SYSC(_sys_prx_load_module_list_on_memcontainer),   //466 (0x1D2)
	BIND_SYSC(sys_prx_get_ppu_guid),                        //467 (0x1D3)
	null_func,//BIND_SYSC(sys_...),                         //468 (0x1D4) ROOT
	uns_func,                                               //469 (0x1D5)  UNS
	NULL_FUNC(sys_npdrm_check_ekc),                         //470 (0x1D6)  ROOT
	NULL_FUNC(sys_npdrm_regist_ekc),                        //471 (0x1D7)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //472 (0x1D8)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //473 (0x1D9)
	null_func,//BIND_SYSC(sys_...),                         //474 (0x1DA)
	null_func,//BIND_SYSC(sys_...),                         //475 (0x1DB)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //476 (0x1DC)  ROOT

	uns_func, uns_func, uns_func,                           //477-479  UNS

	BIND_SYSC(_sys_prx_load_module),                        //480 (0x1E0)
	BIND_SYSC(_sys_prx_start_module),                       //481 (0x1E1)
	BIND_SYSC(_sys_prx_stop_module),                        //482 (0x1E2)
	BIND_SYSC(_sys_prx_unload_module),                      //483 (0x1E3)
	BIND_SYSC(_sys_prx_register_module),                    //484 (0x1E4)
	BIND_SYSC(_sys_prx_query_module),                       //485 (0x1E5)
	BIND_SYSC(_sys_prx_register_library),                   //486 (0x1E6)
	BIND_SYSC(_sys_prx_unregister_library),                 //487 (0x1E7)
	BIND_SYSC(_sys_prx_link_library),                       //488 (0x1E8)
	BIND_SYSC(_sys_prx_unlink_library),                     //489 (0x1E9)
	BIND_SYSC(_sys_prx_query_library),                      //490 (0x1EA)
	uns_func,                                               //491 (0x1EB)  UNS
	NULL_FUNC(sys_prx_dbg_get_module_list),                 //492 (0x1EC)  DBG
	NULL_FUNC(sys_prx_dbg_get_module_info),                 //493 (0x1ED)  DBG
	BIND_SYSC(_sys_prx_get_module_list),                    //494 (0x1EE)
	BIND_SYSC(_sys_prx_get_module_info),                    //495 (0x1EF)
	BIND_SYSC(_sys_prx_get_module_id_by_name),              //496 (0x1F0)
	BIND_SYSC(_sys_prx_load_module_on_memcontainer),        //497 (0x1F1)
	BIND_SYSC(_sys_prx_start),                              //498 (0x1F2)
	BIND_SYSC(_sys_prx_stop),                               //499 (0x1F3)
	BIND_SYSC(sys_hid_manager_open),                        //500 (0x1F4)
	NULL_FUNC(sys_hid_manager_close),                       //501 (0x1F5)
	BIND_SYSC(sys_hid_manager_read),                        //502 (0x1F6)  ROOT
	BIND_SYSC(sys_hid_manager_ioctl),                       //503 (0x1F7)
	NULL_FUNC(sys_hid_manager_map_logical_id_to_port_id),   //504 (0x1F8)  ROOT
	NULL_FUNC(sys_hid_manager_unmap_logical_id_to_port_id), //505 (0x1F9)  ROOT
	BIND_SYSC(sys_hid_manager_add_hot_key_observer),        //506 (0x1FA)  ROOT
	NULL_FUNC(sys_hid_manager_remove_hot_key_observer),     //507 (0x1FB)  ROOT
	NULL_FUNC(sys_hid_manager_grab_focus),                  //508 (0x1FC)  ROOT
	NULL_FUNC(sys_hid_manager_release_focus),               //509 (0x1FD)  ROOT
	BIND_SYSC(sys_hid_manager_check_focus),                 //510 (0x1FE)
	NULL_FUNC(sys_hid_manager_set_master_process),          //511 (0x1FF)  ROOT
	BIND_SYSC(sys_hid_manager_is_process_permission_root),  //512 (0x200)  ROOT
	BIND_SYSC(sys_hid_manager_513),                         //513 (0x201)
	BIND_SYSC(sys_hid_manager_514),                         //514 (0x202)
	uns_func,                                               //515 (0x203)  UNS
	BIND_SYSC(sys_config_open),                             //516 (0x204)
	BIND_SYSC(sys_config_close),                            //517 (0x205)
	BIND_SYSC(sys_config_get_service_event),                //518 (0x206)
	BIND_SYSC(sys_config_add_service_listener),             //519 (0x207)
	BIND_SYSC(sys_config_remove_service_listener),          //520 (0x208)
	BIND_SYSC(sys_config_register_service),                 //521 (0x209)
	BIND_SYSC(sys_config_unregister_service),               //522 (0x20A)
	BIND_SYSC(sys_config_get_io_event),                     //523 (0x20B)
	BIND_SYSC(sys_config_register_io_error_listener),       //524 (0x20C)
	BIND_SYSC(sys_config_unregister_io_error_listener),     //525 (0x20D)
	uns_func, uns_func, uns_func, uns_func,                 //526-529  UNS
	BIND_SYSC(sys_usbd_initialize),                         //530 (0x212)
	BIND_SYSC(sys_usbd_finalize),                           //531 (0x213)
	BIND_SYSC(sys_usbd_get_device_list),                    //532 (0x214)
	BIND_SYSC(sys_usbd_get_descriptor_size),                //533 (0x215)
	BIND_SYSC(sys_usbd_get_descriptor),                     //534 (0x216)
	BIND_SYSC(sys_usbd_register_ldd),                       //535 (0x217)
	BIND_SYSC(sys_usbd_unregister_ldd),                     //536 (0x218)
	BIND_SYSC(sys_usbd_open_pipe),                          //537 (0x219)
	BIND_SYSC(sys_usbd_open_default_pipe),                  //538 (0x21A)
	BIND_SYSC(sys_usbd_close_pipe),                         //539 (0x21B)
	BIND_SYSC(sys_usbd_receive_event),                      //540 (0x21C)
	BIND_SYSC(sys_usbd_detect_event),                       //541 (0x21D)
	BIND_SYSC(sys_usbd_attach),                             //542 (0x21E)
	BIND_SYSC(sys_usbd_transfer_data),                      //543 (0x21F)
	BIND_SYSC(sys_usbd_isochronous_transfer_data),          //544 (0x220)
	BIND_SYSC(sys_usbd_get_transfer_status),                //545 (0x221)
	BIND_SYSC(sys_usbd_get_isochronous_transfer_status),    //546 (0x222)
	BIND_SYSC(sys_usbd_get_device_location),                //547 (0x223)
	BIND_SYSC(sys_usbd_send_event),                         //548 (0x224)
	BIND_SYSC(sys_usbd_event_port_send),                    //549 (0x225)
	BIND_SYSC(sys_usbd_allocate_memory),                    //550 (0x226)
	BIND_SYSC(sys_usbd_free_memory),                        //551 (0x227)
	null_func,//BIND_SYSC(sys_usbd_...),                    //552 (0x228)
	null_func,//BIND_SYSC(sys_usbd_...),                    //553 (0x229)
	null_func,//BIND_SYSC(sys_usbd_...),                    //554 (0x22A)
	null_func,//BIND_SYSC(sys_usbd_...),                    //555 (0x22B)
	BIND_SYSC(sys_usbd_get_device_speed),                   //556 (0x22C)
	null_func,//BIND_SYSC(sys_usbd_...),                    //557 (0x22D)
	BIND_SYSC(sys_usbd_unregister_extra_ldd),               //558 (0x22E)
	BIND_SYSC(sys_usbd_register_extra_ldd),                 //559 (0x22F)
	null_func,//BIND_SYSC(sys_...),                         //560 (0x230)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //561 (0x231)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //562 (0x232)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //563 (0x233)
	null_func,//BIND_SYSC(sys_...),                         //564 (0x234)
	null_func,//BIND_SYSC(sys_...),                         //565 (0x235)
	null_func,//BIND_SYSC(sys_...),                         //566 (0x236)
	null_func,//BIND_SYSC(sys_...),                         //567 (0x237)
	null_func,//BIND_SYSC(sys_...),                         //568 (0x238)
	null_func,//BIND_SYSC(sys_...),                         //569 (0x239)
	NULL_FUNC(sys_pad_ldd_register_controller),             //570 (0x23A)
	NULL_FUNC(sys_pad_ldd_unregister_controller),           //571 (0x23B)
	NULL_FUNC(sys_pad_ldd_data_insert),                     //572 (0x23C)
	NULL_FUNC(sys_pad_dbg_ldd_set_data_insert_mode),        //573 (0x23D)
	NULL_FUNC(sys_pad_ldd_register_controller),             //574 (0x23E)
	NULL_FUNC(sys_pad_ldd_get_port_no),                     //575 (0x23F)
	uns_func,                                               //576 (0x240)  UNS
	null_func,//BIND_SYSC(sys_pad_manager_...),             //577 (0x241)  ROOT  PM
	null_func,//BIND_SYSC(sys_bluetooth_...),               //578 (0x242)
	null_func,//BIND_SYSC(sys_bluetooth_aud_serial_...),    //579 (0x243)
	null_func,//BIND_SYSC(sys_bluetooth_...),               //580 (0x244)  ROOT
	null_func,//BIND_SYSC(sys_bluetooth_...),               //581 (0x245)  ROOT
	null_func,//BIND_SYSC(sys_bluetooth_...),               //582 (0x246)  ROOT
	NULL_FUNC(sys_bt_read_firmware_version),                //583 (0x247)  ROOT
	NULL_FUNC(sys_bt_complete_wake_on_host),                //584 (0x248)  ROOT
	NULL_FUNC(sys_bt_disable_bluetooth),                    //585 (0x249)
	NULL_FUNC(sys_bt_enable_bluetooth),                     //586 (0x24A)
	NULL_FUNC(sys_bt_bccmd),                                //587 (0x24B)  ROOT
	NULL_FUNC(sys_bt_read_hq),                              //588 (0x24C)
	NULL_FUNC(sys_bt_hid_get_remote_status),                //589 (0x24D)
	NULL_FUNC(sys_bt_register_controller),                  //590 (0x24E)  ROOT
	NULL_FUNC(sys_bt_clear_registered_contoller),           //591 (0x24F)
	NULL_FUNC(sys_bt_connect_accept_controller),            //592 (0x250)
	NULL_FUNC(sys_bt_get_local_bdaddress),                  //593 (0x251)  ROOT
	NULL_FUNC(sys_bt_hid_get_data),                         //594 (0x252)
	NULL_FUNC(sys_bt_hid_set_report),                       //595 (0x253)
	NULL_FUNC(sys_bt_sched_log),                            //596 (0x254)
	NULL_FUNC(sys_bt_cancel_connect_accept_controller),     //597 (0x255)
	null_func,//BIND_SYSC(sys_bluetooth_...),               //598 (0x256)  ROOT
	null_func,//BIND_SYSC(sys_bluetooth_...),               //599 (0x257)  ROOT
	BIND_SYSC(sys_storage_open),                            //600 (0x258)  ROOT
	BIND_SYSC(sys_storage_close),                           //601 (0x259)
	BIND_SYSC(sys_storage_read),                            //602 (0x25A)
	BIND_SYSC(sys_storage_write),                           //603 (0x25B)
	BIND_SYSC(sys_storage_send_device_command),             //604 (0x25C)
	BIND_SYSC(sys_storage_async_configure),                 //605 (0x25D)
	BIND_SYSC(sys_storage_async_read),                      //606 (0x25E)
	BIND_SYSC(sys_storage_async_write),                     //607 (0x25F)
	BIND_SYSC(sys_storage_async_cancel),                    //608 (0x260)
	BIND_SYSC(sys_storage_get_device_info),                 //609 (0x261)  ROOT
	BIND_SYSC(sys_storage_get_device_config),               //610 (0x262)  ROOT
	BIND_SYSC(sys_storage_report_devices),                  //611 (0x263)  ROOT
	BIND_SYSC(sys_storage_configure_medium_event),          //612 (0x264)  ROOT
	BIND_SYSC(sys_storage_set_medium_polling_interval),     //613 (0x265)
	BIND_SYSC(sys_storage_create_region),                   //614 (0x266)
	BIND_SYSC(sys_storage_delete_region),                   //615 (0x267)
	BIND_SYSC(sys_storage_execute_device_command),          //616 (0x268)
	BIND_SYSC(sys_storage_check_region_acl),                //617 (0x269)
	BIND_SYSC(sys_storage_set_region_acl),                  //618 (0x26A)
	BIND_SYSC(sys_storage_async_send_device_command),       //619 (0x26B)
	null_func,//BIND_SYSC(sys_...),                         //620 (0x26C)  ROOT
	BIND_SYSC(sys_gamepad_ycon_if),                         //621 (0x26D)
	BIND_SYSC(sys_storage_get_region_offset),               //622 (0x26E)
	BIND_SYSC(sys_storage_set_emulated_speed),              //623 (0x26F)
	BIND_SYSC(sys_io_buffer_create),                        //624 (0x270)
	BIND_SYSC(sys_io_buffer_destroy),                       //625 (0x271)
	BIND_SYSC(sys_io_buffer_allocate),                      //626 (0x272)
	BIND_SYSC(sys_io_buffer_free),                          //627 (0x273)
	uns_func, uns_func,                                     //629 (0x275)  UNS
	BIND_SYSC(sys_gpio_set),                                //630 (0x276)
	BIND_SYSC(sys_gpio_get),                                //631 (0x277)
	uns_func,                                               //632 (0x278)  UNS
	NULL_FUNC(sys_fsw_connect_event),                       //633 (0x279)
	NULL_FUNC(sys_fsw_disconnect_event),                    //634 (0x27A)
	BIND_SYSC(sys_btsetting_if),                            //635 (0x27B)
	null_func,//BIND_SYSC(sys_...),                         //636 (0x27C)
	null_func,//BIND_SYSC(sys_...),                         //637 (0x27D)
	null_func,//BIND_SYSC(sys_...),                         //638 (0x27E)

	null_func,//BIND_SYSC(sys...),                          //639  DEPRECATED
	NULL_FUNC(sys_usbbtaudio_initialize),                   //640  DEPRECATED
	NULL_FUNC(sys_usbbtaudio_finalize),                     //641  DEPRECATED
	NULL_FUNC(sys_usbbtaudio_discovery),                    //642  DEPRECATED
	NULL_FUNC(sys_usbbtaudio_cancel_discovery),             //643  DEPRECATED
	NULL_FUNC(sys_usbbtaudio_pairing),                      //644  DEPRECATED
	NULL_FUNC(sys_usbbtaudio_set_passkey),                  //645  DEPRECATED
	NULL_FUNC(sys_usbbtaudio_connect),                      //646  DEPRECATED
	NULL_FUNC(sys_usbbtaudio_disconnect),                   //647  DEPRECATED
	null_func,//BIND_SYSC(sys_...),                         //648  DEPRECATED
	null_func,//BIND_SYSC(sys_...),                         //649  DEPRECATED

	BIND_SYSC(sys_rsxaudio_initialize),                     //650 (0x28A)
	BIND_SYSC(sys_rsxaudio_finalize),                       //651 (0x28B)
	BIND_SYSC(sys_rsxaudio_import_shared_memory),           //652 (0x28C)
	BIND_SYSC(sys_rsxaudio_unimport_shared_memory),         //653 (0x28D)
	BIND_SYSC(sys_rsxaudio_create_connection),              //654 (0x28E)
	BIND_SYSC(sys_rsxaudio_close_connection),               //655 (0x28F)
	BIND_SYSC(sys_rsxaudio_prepare_process),                //656 (0x290)
	BIND_SYSC(sys_rsxaudio_start_process),                  //657 (0x291)
	BIND_SYSC(sys_rsxaudio_stop_process),                   //658 (0x292)
	BIND_SYSC(sys_rsxaudio_get_dma_param),                  //659 (0x293)

	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //660-665  UNS

	BIND_SYSC(sys_rsx_device_open),                         //666 (0x29A)
	BIND_SYSC(sys_rsx_device_close),                        //667 (0x29B)
	BIND_SYSC(sys_rsx_memory_allocate),                     //668 (0x29C)
	BIND_SYSC(sys_rsx_memory_free),                         //669 (0x29D)
	BIND_SYSC(sys_rsx_context_allocate),                    //670 (0x29E)
	BIND_SYSC(sys_rsx_context_free),                        //671 (0x29F)
	BIND_SYSC(sys_rsx_context_iomap),                       //672 (0x2A0)
	BIND_SYSC(sys_rsx_context_iounmap),                     //673 (0x2A1)
	BIND_SYSC(sys_rsx_context_attribute),                   //674 (0x2A2)
	BIND_SYSC(sys_rsx_device_map),                          //675 (0x2A3)
	BIND_SYSC(sys_rsx_device_unmap),                        //676 (0x2A4)
	BIND_SYSC(sys_rsx_attribute),                           //677 (0x2A5)
	null_func,//BIND_SYSC(sys_...),                         //678 (0x2A6)
	null_func,//BIND_SYSC(sys_...),                         //679 (0x2A7)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //680 (0x2A8)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //681 (0x2A9)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //682 (0x2AA)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //683 (0x2AB)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //684 (0x2AC)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //685 (0x2AD)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //686 (0x2AE)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //687 (0x2AF)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //688 (0x2B0)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //689 (0x2B1)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //690 (0x2B2)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //691 (0x2B3)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //692 (0x2B4)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //693 (0x2B5)  ROOT
	null_func,//BIND_SYSC(sys_...),                         //694 (0x2B6)  DEPRECATED
	null_func,//BIND_SYSC(sys_...),                         //695 (0x2B7)  DEPRECATED
	null_func,//BIND_SYSC(sys_...),                         //696 (0x2B8)  ROOT
	uns_func,//BIND_SYSC(sys_...),                          //697 (0x2B9)  UNS
	uns_func,//BIND_SYSC(sys_...),                          //698 (0x2BA)  UNS
	BIND_SYSC(sys_bdemu_send_command),                      //699 (0x2BB)
	BIND_SYSC(sys_net_bnet_accept),                         //700 (0x2BC)
	BIND_SYSC(sys_net_bnet_bind),                           //701 (0x2BD)
	BIND_SYSC(sys_net_bnet_connect),                        //702 (0x2BE)
	BIND_SYSC(sys_net_bnet_getpeername),                    //703 (0x2BF)
	BIND_SYSC(sys_net_bnet_getsockname),                    //704 (0x2C0)
	BIND_SYSC(sys_net_bnet_getsockopt),                     //705 (0x2C1)
	BIND_SYSC(sys_net_bnet_listen),                         //706 (0x2C2)
	BIND_SYSC(sys_net_bnet_recvfrom),                       //707 (0x2C3)
	BIND_SYSC(sys_net_bnet_recvmsg),                        //708 (0x2C4)
	BIND_SYSC(sys_net_bnet_sendmsg),                        //709 (0x2C5)
	BIND_SYSC(sys_net_bnet_sendto),                         //710 (0x2C6)
	BIND_SYSC(sys_net_bnet_setsockopt),                     //711 (0x2C7)
	BIND_SYSC(sys_net_bnet_shutdown),                       //712 (0x2C8)
	BIND_SYSC(sys_net_bnet_socket),                         //713 (0x2C9)
	BIND_SYSC(sys_net_bnet_close),                          //714 (0x2CA)
	BIND_SYSC(sys_net_bnet_poll),                           //715 (0x2CB)
	BIND_SYSC(sys_net_bnet_select),                         //716 (0x2CC)
	BIND_SYSC(_sys_net_open_dump),                          //717 (0x2CD)
	BIND_SYSC(_sys_net_read_dump),                          //718 (0x2CE)
	BIND_SYSC(_sys_net_close_dump),                         //719 (0x2CF)
	BIND_SYSC(_sys_net_write_dump),                         //720 (0x2D0)
	BIND_SYSC(sys_net_abort),                               //721 (0x2D1)
	BIND_SYSC(sys_net_infoctl),                             //722 (0x2D2)
	BIND_SYSC(sys_net_control),                             //723 (0x2D3)
	BIND_SYSC(sys_net_bnet_ioctl),                          //724 (0x2D4)
	BIND_SYSC(sys_net_bnet_sysctl),                         //725 (0x2D5)
	BIND_SYSC(sys_net_eurus_post_command),                  //726 (0x2D6)

	uns_func, uns_func, uns_func,                           //727-729  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //730-739  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //740-749  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //750-759  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //760-769  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //770-779  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //780-789  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //790-799  UNS

	BIND_SYSC(sys_fs_test),                                 //800 (0x320)
	BIND_SYSC(sys_fs_open),                                 //801 (0x321)
	BIND_SYSC(sys_fs_read),                                 //802 (0x322)
	BIND_SYSC(sys_fs_write),                                //803 (0x323)
	BIND_SYSC(sys_fs_close),                                //804 (0x324)
	BIND_SYSC(sys_fs_opendir),                              //805 (0x325)
	BIND_SYSC(sys_fs_readdir),                              //806 (0x326)
	BIND_SYSC(sys_fs_closedir),                             //807 (0x327)
	BIND_SYSC(sys_fs_stat),                                 //808 (0x328)
	BIND_SYSC(sys_fs_fstat),                                //809 (0x329)
	BIND_SYSC(sys_fs_link),                                 //810 (0x32A)
	BIND_SYSC(sys_fs_mkdir),                                //811 (0x32B)
	BIND_SYSC(sys_fs_rename),                               //812 (0x32C)
	BIND_SYSC(sys_fs_rmdir),                                //813 (0x32D)
	BIND_SYSC(sys_fs_unlink),                               //814 (0x32E)
	BIND_SYSC(sys_fs_utime),                                //815 (0x32F)
	BIND_SYSC(sys_fs_access),                               //816 (0x330)
	BIND_SYSC(sys_fs_fcntl),                                //817 (0x331)
	BIND_SYSC(sys_fs_lseek),                                //818 (0x332)
	BIND_SYSC(sys_fs_fdatasync),                            //819 (0x333)
	BIND_SYSC(sys_fs_fsync),                                //820 (0x334)
	BIND_SYSC(sys_fs_fget_block_size),                      //821 (0x335)
	BIND_SYSC(sys_fs_get_block_size),                       //822 (0x336)
	BIND_SYSC(sys_fs_acl_read),                             //823 (0x337)
	BIND_SYSC(sys_fs_acl_write),                            //824 (0x338)
	BIND_SYSC(sys_fs_lsn_get_cda_size),                     //825 (0x339)
	BIND_SYSC(sys_fs_lsn_get_cda),                          //826 (0x33A)
	BIND_SYSC(sys_fs_lsn_lock),                             //827 (0x33B)
	BIND_SYSC(sys_fs_lsn_unlock),                           //828 (0x33C)
	BIND_SYSC(sys_fs_lsn_read),                             //829 (0x33D)
	BIND_SYSC(sys_fs_lsn_write),                            //830 (0x33E)
	BIND_SYSC(sys_fs_truncate),                             //831 (0x33F)
	BIND_SYSC(sys_fs_ftruncate),                            //832 (0x340)
	BIND_SYSC(sys_fs_symbolic_link),                        //833 (0x341)
	BIND_SYSC(sys_fs_chmod),                                //834 (0x342)
	BIND_SYSC(sys_fs_chown),                                //835 (0x343)
	BIND_SYSC(sys_fs_newfs),                                //836 (0x344)
	BIND_SYSC(sys_fs_mount),                                //837 (0x345)
	BIND_SYSC(sys_fs_unmount),                              //838 (0x346)
	NULL_FUNC(sys_fs_sync),                                 //839 (0x347)
	BIND_SYSC(sys_fs_disk_free),                            //840 (0x348)
	BIND_SYSC(sys_fs_get_mount_info_size),                  //841 (0x349)
	BIND_SYSC(sys_fs_get_mount_info),                       //842 (0x34A)
	NULL_FUNC(sys_fs_get_fs_info_size),                     //843 (0x34B)
	NULL_FUNC(sys_fs_get_fs_info),                          //844 (0x34C)
	BIND_SYSC(sys_fs_mapped_allocate),                      //845 (0x34D)
	BIND_SYSC(sys_fs_mapped_free),                          //846 (0x34E)
	BIND_SYSC(sys_fs_truncate2),                            //847 (0x34F)

	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //848-853  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //854-859  UNS

	NULL_FUNC(sys_ss_get_cache_of_analog_sunset_flag),      //860 (0x35C)  AUTHID
	NULL_FUNC(sys_ss_protected_file_db),                    //861  ROOT
	BIND_SYSC(sys_ss_virtual_trm_manager),                  //862  ROOT
	BIND_SYSC(sys_ss_update_manager),                       //863  ROOT
	NULL_FUNC(sys_ss_sec_hw_framework),                     //864  DBG
	BIND_SYSC(sys_ss_random_number_generator),              //865 (0x361)
	BIND_SYSC(sys_ss_secure_rtc),                           //866  ROOT
	BIND_SYSC(sys_ss_appliance_info_manager),               //867  ROOT
	BIND_SYSC(sys_ss_individual_info_manager),              //868  ROOT / DBG  AUTHID
	NULL_FUNC(sys_ss_factory_data_manager),                 //869  ROOT
	BIND_SYSC(sys_ss_get_console_id),                       //870 (0x366)
	BIND_SYSC(sys_ss_access_control_engine),                //871 (0x367)  DBG
	BIND_SYSC(sys_ss_get_open_psid),                        //872 (0x368)
	BIND_SYSC(sys_ss_get_cache_of_product_mode),            //873 (0x369)
	BIND_SYSC(sys_ss_get_cache_of_flash_ext_flag),          //874 (0x36A)
	BIND_SYSC(sys_ss_get_boot_device),                      //875 (0x36B)
	NULL_FUNC(sys_ss_disc_access_control),                  //876 (0x36C)
	null_func, //BIND_SYSC(sys_ss_~utoken_if),              //877 (0x36D)  ROOT
	NULL_FUNC(sys_ss_ad_sign),                              //878 (0x36E)
	NULL_FUNC(sys_ss_media_id),                             //879 (0x36F)
	NULL_FUNC(sys_deci3_open),                              //880 (0x370)
	NULL_FUNC(sys_deci3_create_event_path),                 //881 (0x371)
	NULL_FUNC(sys_deci3_close),                             //882 (0x372)
	NULL_FUNC(sys_deci3_send),                              //883 (0x373)
	NULL_FUNC(sys_deci3_receive),                           //884 (0x374)
	NULL_FUNC(sys_deci3_open2),                             //885 (0x375)
	uns_func, uns_func, uns_func,                           //886-888  UNS
	null_func,//BIND_SYSC(sys_...),                         //889 (0x379)  ROOT
	NULL_FUNC(sys_deci3_initialize),                        //890 (0x37A)
	NULL_FUNC(sys_deci3_terminate),                         //891 (0x37B)
	NULL_FUNC(sys_deci3_debug_mode),                        //892 (0x37C)
	NULL_FUNC(sys_deci3_show_status),                       //893 (0x37D)
	NULL_FUNC(sys_deci3_echo_test),                         //894 (0x37E)
	NULL_FUNC(sys_deci3_send_dcmp_packet),                  //895 (0x37F)
	NULL_FUNC(sys_deci3_dump_cp_register),                  //896 (0x380)
	NULL_FUNC(sys_deci3_dump_cp_buffer),                    //897 (0x381)
	uns_func,                                               //898 (0x382)  UNS
	NULL_FUNC(sys_deci3_test),                              //899 (0x383)
	NULL_FUNC(sys_dbg_stop_processes),                      //900 (0x384)
	NULL_FUNC(sys_dbg_continue_processes),                  //901 (0x385)
	NULL_FUNC(sys_dbg_stop_threads),                        //902 (0x386)
	NULL_FUNC(sys_dbg_continue_threads),                    //903 (0x387)
	BIND_SYSC(sys_dbg_read_process_memory),                 //904 (0x388)
	BIND_SYSC(sys_dbg_write_process_memory),                //905 (0x389)
	NULL_FUNC(sys_dbg_read_thread_register),                //906 (0x38A)
	NULL_FUNC(sys_dbg_write_thread_register),               //907 (0x38B)
	NULL_FUNC(sys_dbg_get_process_list),                    //908 (0x38C)
	NULL_FUNC(sys_dbg_get_thread_list),                     //909 (0x38D)
	NULL_FUNC(sys_dbg_get_thread_info),                     //910 (0x38E)
	NULL_FUNC(sys_dbg_spu_thread_read_from_ls),             //911 (0x38F)
	NULL_FUNC(sys_dbg_spu_thread_write_to_ls),              //912 (0x390)
	NULL_FUNC(sys_dbg_kill_process),                        //913 (0x391)
	NULL_FUNC(sys_dbg_get_process_info),                    //914 (0x392)
	NULL_FUNC(sys_dbg_set_run_control_bit_to_spu),          //915 (0x393)
	NULL_FUNC(sys_dbg_spu_thread_get_exception_cause),      //916 (0x394)
	NULL_FUNC(sys_dbg_create_kernel_event_queue),           //917 (0x395)
	NULL_FUNC(sys_dbg_read_kernel_event_queue),             //918 (0x396)
	NULL_FUNC(sys_dbg_destroy_kernel_event_queue),          //919 (0x397)
	NULL_FUNC(sys_dbg_get_process_event_ctrl_flag),         //920 (0x398)
	NULL_FUNC(sys_dbg_set_process_event_cntl_flag),         //921 (0x399)
	NULL_FUNC(sys_dbg_get_spu_thread_group_event_cntl_flag),//922 (0x39A)
	NULL_FUNC(sys_dbg_set_spu_thread_group_event_cntl_flag),//923 (0x39B)
	NULL_FUNC(sys_dbg_get_module_list),                     //924 (0x39C)
	NULL_FUNC(sys_dbg_get_raw_spu_list),                    //925 (0x39D)
	NULL_FUNC(sys_dbg_initialize_scratch_executable_area),  //926 (0x39E)
	NULL_FUNC(sys_dbg_terminate_scratch_executable_area),   //927 (0x3A0)
	NULL_FUNC(sys_dbg_initialize_scratch_data_area),        //928 (0x3A1)
	NULL_FUNC(sys_dbg_terminate_scratch_data_area),         //929 (0x3A2)
	NULL_FUNC(sys_dbg_get_user_memory_stat),                //930 (0x3A3)
	NULL_FUNC(sys_dbg_get_shared_memory_attribute_list),    //931 (0x3A4)
	NULL_FUNC(sys_dbg_get_mutex_list),                      //932 (0x3A4)
	NULL_FUNC(sys_dbg_get_mutex_information),               //933 (0x3A5)
	NULL_FUNC(sys_dbg_get_cond_list),                       //934 (0x3A6)
	NULL_FUNC(sys_dbg_get_cond_information),                //935 (0x3A7)
	NULL_FUNC(sys_dbg_get_rwlock_list),                     //936 (0x3A8)
	NULL_FUNC(sys_dbg_get_rwlock_information),              //937 (0x3A9)
	NULL_FUNC(sys_dbg_get_lwmutex_list),                    //938 (0x3AA)
	NULL_FUNC(sys_dbg_get_address_from_dabr),               //939 (0x3AB)
	NULL_FUNC(sys_dbg_set_address_to_dabr),                 //940 (0x3AC)
	NULL_FUNC(sys_dbg_get_lwmutex_information),             //941 (0x3AD)
	NULL_FUNC(sys_dbg_get_event_queue_list),                //942 (0x3AE)
	NULL_FUNC(sys_dbg_get_event_queue_information),         //943 (0x3AF)
	NULL_FUNC(sys_dbg_initialize_ppu_exception_handler),    //944 (0x3B0)
	NULL_FUNC(sys_dbg_finalize_ppu_exception_handler),      //945 (0x3B1)  DBG
	NULL_FUNC(sys_dbg_get_semaphore_list),                  //946 (0x3B2)
	NULL_FUNC(sys_dbg_get_semaphore_information),           //947 (0x3B3)
	NULL_FUNC(sys_dbg_get_kernel_thread_list),              //948 (0x3B4)
	NULL_FUNC(sys_dbg_get_kernel_thread_info),              //949 (0x3B5)
	NULL_FUNC(sys_dbg_get_lwcond_list),                     //950 (0x3B6)
	NULL_FUNC(sys_dbg_get_lwcond_information),              //951 (0x3B7)
	NULL_FUNC(sys_dbg_create_scratch_data_area_ext),        //952 (0x3B8)
	NULL_FUNC(sys_dbg_vm_get_page_information),             //953 (0x3B9)
	NULL_FUNC(sys_dbg_vm_get_info),                         //954 (0x3BA)
	NULL_FUNC(sys_dbg_enable_floating_point_enabled_exception),//955 (0x3BB)
	NULL_FUNC(sys_dbg_disable_floating_point_enabled_exception),//956 (0x3BC)
	NULL_FUNC(sys_dbg_get_process_memory_container_information),//957 (0x3BD)
	uns_func,                                               //958 (0x3BE)  UNS
	null_func,//BIND_SYSC(sys_dbg_...),                     //959 (0x3BF)
	NULL_FUNC(sys_control_performance_monitor),             //960 (0x3C0)
	NULL_FUNC(sys_performance_monitor_hidden),              //961 (0x3C1)
	NULL_FUNC(sys_performance_monitor_bookmark),            //962 (0x3C2)
	NULL_FUNC(sys_lv1_pc_trace_create),                     //963 (0x3C3)
	NULL_FUNC(sys_lv1_pc_trace_start),                      //964 (0x3C4)
	NULL_FUNC(sys_lv1_pc_trace_stop),                       //965 (0x3C5)
	NULL_FUNC(sys_lv1_pc_trace_get_status),                 //966 (0x3C6)
	NULL_FUNC(sys_lv1_pc_trace_destroy),                    //967 (0x3C7)
	NULL_FUNC(sys_rsx_trace_ioctl),                         //968 (0x3C8)
	null_func,//BIND_SYSC(sys_dbg_...),                     //969 (0x3C9)
	NULL_FUNC(sys_dbg_get_event_flag_list),                 //970 (0x3CA)
	NULL_FUNC(sys_dbg_get_event_flag_information),          //971 (0x3CB)
	null_func,//BIND_SYSC(sys_dbg_...),                     //972 (0x3CC)
	uns_func,//BIND_SYSC(sys_dbg_...),                      //973 (0x3CD)
	null_func,//BIND_SYSC(sys_dbg_...),                     //974 (0x3CE)
	NULL_FUNC(sys_dbg_read_spu_thread_context2),            //975 (0x3CF)
	BIND_SYSC(sys_crypto_engine_create),                    //976 (0x3D0)
	BIND_SYSC(sys_crypto_engine_destroy),                   //977 (0x3D1)
	NULL_FUNC(sys_crypto_engine_hasher_prepare),            //978 (0x3D2)  ROOT
	NULL_FUNC(sys_crypto_engine_hasher_run),                //979 (0x3D3)
	NULL_FUNC(sys_crypto_engine_hasher_get_hash),           //980 (0x3D4)
	NULL_FUNC(sys_crypto_engine_cipher_prepare),            //981 (0x3D5)  ROOT
	NULL_FUNC(sys_crypto_engine_cipher_run),                //982 (0x3D6)
	NULL_FUNC(sys_crypto_engine_cipher_get_hash),           //983 (0x3D7)
	BIND_SYSC(sys_crypto_engine_random_generate),           //984 (0x3D8)
	NULL_FUNC(sys_dbg_get_console_type),                    //985 (0x3D9)  ROOT
	null_func,//BIND_SYSC(sys_dbg_...),                     //986 (0x3DA)  ROOT  DBG
	null_func,//BIND_SYSC(sys_dbg_...),                     //987 (0x3DB)  ROOT
	null_func,//BIND_SYSC(sys_dbg_..._ppu_exception_handler) //988 (0x3DC)
	null_func,//BIND_SYSC(sys_dbg_...),                     //989 (0x3DD)

	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //990-998  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //999-1007  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //1008-1016  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //1020-1023  UNS
};

#undef BIND_SYSC
#undef NULL_FUNC

// TODO: more enums
enum CellAdecError : u32;
enum CellAtracError : u32;
enum CellAtracMultiError : u32;
enum CellAudioError : u32;
enum CellAudioOutError : u32;
enum CellAudioInError : u32;

enum CellVideoOutError : u32;

enum CellSpursCoreError : u32;
enum CellSpursPolicyModuleError : u32;
enum CellSpursTaskError : u32;
enum CellSpursJobError : u32;
enum CellSyncError : u32;

enum CellGameError : u32;
enum CellGameDataError : u32;
enum CellDiscGameError : u32;
enum CellHddGameError : u32;

enum SceNpTrophyError : u32;
enum SceNpError : u32;

template <u64 EnumMin, typename E>
constexpr auto formatter_of = std::make_pair(EnumMin, &fmt_class_string<E>::format);

const std::map<u64, void(*)(std::string&, u64)> s_error_codes_formatting_by_type
{
	formatter_of<0x80610000, CellAdecError>,
	formatter_of<0x80612100, CellAdecError>,
	formatter_of<0x80610300, CellAtracError>,
	formatter_of<0x80610b00, CellAtracMultiError>,
	formatter_of<0x80310700, CellAudioError>,
	formatter_of<0x8002b240, CellAudioOutError>,
	formatter_of<0x8002b260, CellAudioInError>,
	formatter_of<0x8002b220, CellVideoOutError>,

	formatter_of<0x80410100, CellSyncError>,
	formatter_of<0x80410700, CellSpursCoreError>,
	formatter_of<0x80410800, CellSpursPolicyModuleError>,
	formatter_of<0x80410900, CellSpursTaskError>,
	formatter_of<0x80410A00, CellSpursJobError>,

	formatter_of<0x8002cb00, CellGameError>,
	formatter_of<0x8002b600, CellGameDataError>,
	formatter_of<0x8002bd00, CellDiscGameError>,
	formatter_of<0x8002ba00, CellHddGameError>,

	formatter_of<0x80022900, SceNpTrophyError>,
	formatter_of<0x80029500, SceNpError>,
};

template<>
void fmt_class_string<CellError>::format(std::string& out, u64 arg)
{
	// Test if can be formatted by this formatter
	const bool lv2_cell_error = (arg >> 8) == 0x800100u;

	if (!lv2_cell_error)
	{
		// Format by external enum formatters
		auto upper = s_error_codes_formatting_by_type.upper_bound(arg);

		if (upper == s_error_codes_formatting_by_type.begin())
		{
			// Format as unknown by another enum formatter
			upper->second(out, arg);
			return;
		}

		// Find the formatter whose base is the highest that is not more than arg
		const auto found = std::prev(upper);
		found->second(out, arg);
		return;
	}

	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_EAGAIN);
		STR_CASE(CELL_EINVAL);
		STR_CASE(CELL_ENOSYS);
		STR_CASE(CELL_ENOMEM);
		STR_CASE(CELL_ESRCH);
		STR_CASE(CELL_ENOENT);
		STR_CASE(CELL_ENOEXEC);
		STR_CASE(CELL_EDEADLK);
		STR_CASE(CELL_EPERM);
		STR_CASE(CELL_EBUSY);
		STR_CASE(CELL_ETIMEDOUT);
		STR_CASE(CELL_EABORT);
		STR_CASE(CELL_EFAULT);
		STR_CASE(CELL_ENOCHILD);
		STR_CASE(CELL_ESTAT);
		STR_CASE(CELL_EALIGN);
		STR_CASE(CELL_EKRESOURCE);
		STR_CASE(CELL_EISDIR);
		STR_CASE(CELL_ECANCELED);
		STR_CASE(CELL_EEXIST);
		STR_CASE(CELL_EISCONN);
		STR_CASE(CELL_ENOTCONN);
		STR_CASE(CELL_EAUTHFAIL);
		STR_CASE(CELL_ENOTMSELF);
		STR_CASE(CELL_ESYSVER);
		STR_CASE(CELL_EAUTHFATAL);
		STR_CASE(CELL_EDOM);
		STR_CASE(CELL_ERANGE);
		STR_CASE(CELL_EILSEQ);
		STR_CASE(CELL_EFPOS);
		STR_CASE(CELL_EINTR);
		STR_CASE(CELL_EFBIG);
		STR_CASE(CELL_EMLINK);
		STR_CASE(CELL_ENFILE);
		STR_CASE(CELL_ENOSPC);
		STR_CASE(CELL_ENOTTY);
		STR_CASE(CELL_EPIPE);
		STR_CASE(CELL_EROFS);
		STR_CASE(CELL_ESPIPE);
		STR_CASE(CELL_E2BIG);
		STR_CASE(CELL_EACCES);
		STR_CASE(CELL_EBADF);
		STR_CASE(CELL_EIO);
		STR_CASE(CELL_EMFILE);
		STR_CASE(CELL_ENODEV);
		STR_CASE(CELL_ENOTDIR);
		STR_CASE(CELL_ENXIO);
		STR_CASE(CELL_EXDEV);
		STR_CASE(CELL_EBADMSG);
		STR_CASE(CELL_EINPROGRESS);
		STR_CASE(CELL_EMSGSIZE);
		STR_CASE(CELL_ENAMETOOLONG);
		STR_CASE(CELL_ENOLCK);
		STR_CASE(CELL_ENOTEMPTY);
		STR_CASE(CELL_ENOTSUP);
		STR_CASE(CELL_EFSSPECIFIC);
		STR_CASE(CELL_EOVERFLOW);
		STR_CASE(CELL_ENOTMOUNTED);
		STR_CASE(CELL_ENOTSDATA);
		STR_CASE(CELL_ESDKVER);
		STR_CASE(CELL_ENOLICDISC);
		STR_CASE(CELL_ENOLICENT);
		}

		return unknown;
	});
}

stx::init_lock acquire_lock(stx::init_mutex& mtx, ppu_thread* ppu)
{
	if (!ppu)
	{
		ppu = ensure(cpu_thread::get_current<ppu_thread>());
	}

	return mtx.init([](int invoke_count, const stx::init_lock&, ppu_thread* ppu)
	{
		if (!invoke_count)
		{
			// Sleep before waiting on lock
			lv2_obj::sleep(*ppu);
		}
		else
		{
			// Wake up after acquistion or failure to acquire
			ppu->check_state();
		}
	}, ppu);
}

stx::access_lock acquire_access_lock(stx::init_mutex& mtx, ppu_thread* ppu)
{
	if (!ppu)
	{
		ppu = ensure(cpu_thread::get_current<ppu_thread>());
	}

	// TODO: Check if needs to wait
	return mtx.access();
}

stx::reset_lock acquire_reset_lock(stx::init_mutex& mtx, ppu_thread* ppu)
{
	if (!ppu)
	{
		ppu = ensure(cpu_thread::get_current<ppu_thread>());
	}

	return mtx.reset([](int invoke_count, const stx::init_lock&, ppu_thread* ppu)
	{
		if (!invoke_count)
		{
			// Sleep before waiting on lock
			lv2_obj::sleep(*ppu);
		}
		else
		{
			// Wake up after acquistion or failure to acquire
			ppu->check_state();
		}
	}, ppu);
}

class ppu_syscall_usage
{
	// Internal buffer
	std::string m_stats;
	u64 m_old_stat[1024]{};

public:
	// Public info collection buffers
	atomic_t<u64> stat[1024]{};

	void print_stats(bool force_print) noexcept
	{
		std::multimap<u64, u64, std::greater<u64>> usage;

		for (u32 i = 0; i < 1024; i++)
		{
			if (u64 v = stat[i]; m_old_stat[i] != v || (force_print && v))
			{
				// Only add syscalls with non-zero usage counter and only if caught new calls since last print
				usage.emplace(v, i);
				m_old_stat[i] = v;
			}
		}

		m_stats.clear();

		for (auto&& pair : usage)
		{
			fmt::append(m_stats, u8"\n\t⁂ %s [%u]", ppu_get_syscall_name(pair.second), pair.first);
		}

		if (!m_stats.empty())
		{
			ppu_log.notice("PPU Syscall Usage Stats:%s", m_stats);
		}
	}

	void operator()()
	{
		bool was_paused = false;
		u64 sleep_until = get_system_time();

		for (u32 i = 1; thread_ctrl::state() != thread_state::aborting; i++)
		{
			thread_ctrl::wait_until(&sleep_until, 1'000'000);

			const bool is_paused = Emu.IsPaused();

			// Force-print all if paused
			const bool force_print = is_paused && !was_paused;

			if (force_print || i % 10 == 0)
			{
				was_paused = is_paused;
				print_stats(force_print);
			}
		}
	}

	~ppu_syscall_usage()
	{
		print_stats(true);
	}

	static constexpr auto thread_name = "PPU Syscall Usage Thread"sv;
};

extern void ppu_execute_syscall(ppu_thread& ppu, u64 code)
{
	if (g_cfg.core.ppu_decoder == ppu_decoder_type::llvm)
	{
		code = ppu.gpr[11];
	}

	if (code < g_ppu_syscall_table.size())
	{
		g_fxo->get<named_thread<ppu_syscall_usage>>().stat[code]++;

		if (const auto func = g_ppu_syscall_table[code].first)
		{
#ifdef __APPLE__
			pthread_jit_write_protect_np(false);
#endif
			func(ppu, {}, vm::_ptr<u32>(ppu.cia), nullptr);
			ppu_log.trace("Syscall '%s' (%llu) finished, r3=0x%llx", ppu_syscall_code(code), code, ppu.gpr[3]);

#ifdef __APPLE__
			pthread_jit_write_protect_np(true);
			// No need to flush cache lines after a syscall, since we didn't generate any code.
#endif
			return;
		}
	}

	fmt::throw_exception("Invalid syscall number (%llu)", code);
}

extern ppu_intrp_func_t ppu_get_syscall(u64 code)
{
	if (code < g_ppu_syscall_table.size())
	{
		return g_ppu_syscall_table[code].first;
	}

	return nullptr;
}

std::string ppu_get_syscall_name(u64 code)
{
	if (code < g_ppu_syscall_table.size() && !g_ppu_syscall_table[code].second.empty())
	{
		return std::string(g_ppu_syscall_table[code].second);
	}

	return fmt::format("syscall_%u", code);
}

DECLARE(lv2_obj::g_mutex);
DECLARE(lv2_obj::g_ppu){};
DECLARE(lv2_obj::g_pending){};
DECLARE(lv2_obj::g_priority_order_tag){};

thread_local DECLARE(lv2_obj::g_to_notify){};
thread_local DECLARE(lv2_obj::g_postpone_notify_barrier){};
thread_local DECLARE(lv2_obj::g_to_awake);

// Scheduler queue for timeouts (wait until -> thread)
static std::deque<std::pair<u64, class cpu_thread*>> g_waiting;

// Threads which must call lv2_obj::sleep before the scheduler starts
static std::deque<class cpu_thread*> g_to_sleep;
static atomic_t<bool> g_scheduler_ready = false;
static atomic_t<u64> s_yield_frequency = 0;
static atomic_t<u64> s_max_allowed_yield_tsc = 0;
static u64 s_last_yield_tsc = 0;
atomic_t<u32> g_lv2_preempts_taken = 0;

namespace cpu_counter
{
	void remove(cpu_thread*) noexcept;
}

std::string lv2_obj::name64(u64 name_u64)
{
	const auto ptr = reinterpret_cast<const char*>(&name_u64);

	// NTS string, ignore invalid/newline characters
	// Example: "lv2\n\0tx" will be printed as "lv2"
	std::string str{ptr, std::find(ptr, ptr + 7, '\0')};
	str.erase(std::remove_if(str.begin(), str.end(), [](uchar c){ return !std::isprint(c); }), str.end());

	return str;
}

bool lv2_obj::sleep(cpu_thread& cpu, const u64 timeout)
{
	// Should already be performed when using this flag
	if (!g_postpone_notify_barrier)
	{
		prepare_for_sleep(cpu);
	}

	if (cpu.get_class() == thread_class::ppu)
	{
		if (u32 addr = static_cast<ppu_thread&>(cpu).res_notify)
		{
			static_cast<ppu_thread&>(cpu).res_notify = 0;

			if (static_cast<ppu_thread&>(cpu).res_notify_time != vm::reservation_notifier_count_index(addr).second)
			{
				// Ignore outdated notification request
			}
			else if (auto it = std::find(g_to_notify, std::end(g_to_notify), std::add_pointer_t<const void>{}); it != std::end(g_to_notify))
			{
				*it++ = vm::reservation_notifier_notify(addr, true);

				if (it < std::end(g_to_notify))
				{
					// Null-terminate the list if it ends before last slot
					*it = nullptr;
				}
			}
			else
			{
				vm::reservation_notifier_notify(addr);
			}
		}
	}

	bool result = false;
	const u64 current_time = get_guest_system_time();
	{
		std::lock_guard lock{g_mutex};
		result = sleep_unlocked(cpu, timeout, current_time);

		if (!g_to_awake.empty())
		{
			// Schedule pending entries
			awake_unlocked({});
		}

		schedule_all(current_time);
	}

	if (!g_postpone_notify_barrier)
	{
		notify_all();
	}

	g_to_awake.clear();
	return result;
}

bool lv2_obj::awake(cpu_thread* thread, s32 prio)
{
	if (ppu_thread* ppu = cpu_thread::get_current<ppu_thread>())
	{
		if (u32 addr = ppu->res_notify)
		{
			ppu->res_notify = 0;

			if (ppu->res_notify_time != vm::reservation_notifier_count_index(addr).second)
			{
				// Ignore outdated notification request
			}
			else if (auto it = std::find(g_to_notify, std::end(g_to_notify), std::add_pointer_t<const void>{}); it != std::end(g_to_notify))
			{
				*it++ = vm::reservation_notifier_notify(addr, true);

				if (it < std::end(g_to_notify))
				{
					// Null-terminate the list if it ends before last slot
					*it = nullptr;
				}
			}
			else
			{
				vm::reservation_notifier_notify(addr);
			}
		}
	}

	bool result = false;
	{
		std::lock_guard lock(g_mutex);
		result = awake_unlocked(thread, prio);
		schedule_all();
	}

	if (result)
	{
		if (auto cpu = cpu_thread::get_current(); cpu && cpu->is_paused())
		{
			vm::temporary_unlock();
		}
	}

	if (!g_postpone_notify_barrier)
	{
		notify_all();
	}

	return result;
}

bool lv2_obj::yield(cpu_thread& thread)
{
	if (auto ppu = thread.try_get<ppu_thread>())
	{
		ppu->raddr = 0; // Clear reservation

		if (!atomic_storage<ppu_thread*>::load(ppu->next_ppu))
		{
			// Nothing to do
			return false;
		}
	}

	return awake(&thread, yield_cmd);
}

bool lv2_obj::sleep_unlocked(cpu_thread& thread, u64 timeout, u64 current_time)
{
	const u64 start_time = current_time;

	auto on_to_sleep_update = [&]()
	{
		if (g_to_sleep.size() > 5u)
		{
			ppu_log.warning("Threads (%d)", g_to_sleep.size());
		}
		else if (!g_to_sleep.empty())
		{
			// In case there is a deadlock (PPU threads not sleeping)
			// Print-out their IDs for further inspection (focus at 5 at max for now to avoid log spam)
			std::string out = fmt::format("Threads (%d):", g_to_sleep.size());
			for (auto thread : g_to_sleep)
			{
				fmt::append(out, " 0x%x,", thread->id);
			}

			out.resize(out.size() - 1);

			ppu_log.warning("%s", out);
		}
		else
		{
			ppu_log.warning("Final Thread");

			// All threads are ready, wake threads
			Emu.CallFromMainThread([]
			{
				if (Emu.IsStarting())
				{
					// It uses lv2_obj::g_mutex, run it on main thread
					Emu.FinalizeRunRequest();
				}
			});
		}
	};

	bool return_val = true;

	if (auto ppu = thread.try_get<ppu_thread>())
	{
		ppu_log.trace("sleep() - waiting (%zu)", g_pending);

		if (ppu->ack_suspend)
		{
			ppu->ack_suspend = false;
			g_pending--;
		}

		if (std::exchange(ppu->cancel_sleep, 0) == 2)
		{
			// Signal that the underlying LV2 operation has been cancelled and replaced with a short yield
			return_val = false;
		}

		const auto [_, ok] = ppu->state.fetch_op([&](bs_t<cpu_flag>& val)
		{
			if (!(val & cpu_flag::signal))
			{
				val += cpu_flag::suspend;

				// Flag used for forced timeout notification
				ensure(!timeout || !(val & cpu_flag::notify));
				return true;
			}

			return false;
		});

		if (!ok)
		{
			ppu_log.fatal("sleep() failed (signaled) (%s)", ppu->current_function);
			return false;
		}

		// Find and remove the thread
		if (!unqueue(g_ppu, ppu, &ppu_thread::next_ppu))
		{
			if (auto it = std::find(g_to_sleep.begin(), g_to_sleep.end(), ppu); it != g_to_sleep.end())
			{
				g_to_sleep.erase(it);
				ppu->start_time = start_time;
				on_to_sleep_update();
				return true;
			}

			// Already sleeping
			ppu_log.trace("sleep(): called on already sleeping thread.");
			return false;
		}

		ppu->raddr = 0; // Clear reservation
		ppu->start_time = start_time;
		ppu->end_time = timeout ? start_time + std::min<u64>(timeout, ~start_time) : u64{umax};
	}
	else if (auto spu = thread.try_get<spu_thread>())
	{
		if (auto it = std::find(g_to_sleep.begin(), g_to_sleep.end(), spu); it != g_to_sleep.end())
		{
			g_to_sleep.erase(it);
			on_to_sleep_update();
			return true;
		}

		return false;
	}

	if (timeout)
	{
		const u64 wait_until = start_time + std::min<u64>(timeout, ~start_time);

		// Register timeout if necessary
		for (auto it = g_waiting.cbegin(), end = g_waiting.cend();; it++)
		{
			if (it == end || it->first > wait_until)
			{
				g_waiting.emplace(it, wait_until, &thread);
				break;
			}
		}
	}

	return return_val;
}

bool lv2_obj::awake_unlocked(cpu_thread* cpu, s32 prio)
{
	// Check thread type
	AUDIT(!cpu || cpu->get_class() == thread_class::ppu);

	bool push_first = false;

	switch (prio)
	{
	default:
	{
		// Priority set
		auto set_prio = [](atomic_t<ppu_thread::ppu_prio_t>& prio, s32 value, bool increment_order_last, bool increment_order_first)
		{
			s64 tag = 0;

			if (increment_order_first || increment_order_last)
			{
				tag = ++g_priority_order_tag;
			}

			prio.atomic_op([&](ppu_thread::ppu_prio_t& prio)
			{
				prio.prio = value;

				if (increment_order_first)
				{
					prio.order = ~tag;
				}
				else if (increment_order_last)
				{
					prio.order = tag;
				}
			});
		};

		const s64 old_prio = static_cast<ppu_thread*>(cpu)->prio.load().prio;

		// If priority is the same, push ONPROC/RUNNABLE thread to the back of the priority list if it is not the current thread
		if (old_prio == prio && cpu == cpu_thread::get_current())
		{
			set_prio(static_cast<ppu_thread*>(cpu)->prio, prio, false, false);
			return true;
		}

		if (!unqueue(g_ppu, static_cast<ppu_thread*>(cpu), &ppu_thread::next_ppu))
		{
			set_prio(static_cast<ppu_thread*>(cpu)->prio, prio, old_prio > prio, old_prio < prio);
			return true;
		}

		set_prio(static_cast<ppu_thread*>(cpu)->prio, prio, false, false);
		break;
	}
	case yield_cmd:
	{
		usz i = 0;

		// Yield command
		for (auto ppu_next = &g_ppu;; i++)
		{
			const auto ppu = +*ppu_next;

			if (!ppu)
			{
				return false;
			}

			if (ppu == cpu)
			{
				auto ppu2 = ppu->next_ppu;

				if (!ppu2 || ppu2->prio.load().prio != ppu->prio.load().prio)
				{
					// Empty 'same prio' threads list
					return false;
				}

				for (i++;; i++)
				{
					const auto next = ppu2->next_ppu;

					if (!next || next->prio.load().prio != ppu->prio.load().prio)
					{
						break;
					}

					ppu2 = next;
				}

				// Rotate current thread to the last position of the 'same prio' threads list
				// Exchange forward pointers
				*ppu_next = std::exchange(ppu->next_ppu, std::exchange(ppu2->next_ppu, ppu));

				if (i < g_cfg.core.ppu_threads + 0u)
				{
					// Threads were rotated, but no context switch was made
					return false;
				}

				ppu->start_time = get_guest_system_time();
				break;
			}

			ppu_next = &ppu->next_ppu;
		}

		break;
	}
	case enqueue_cmd:
	{
		break;
	}
	}

	const auto emplace_thread = [push_first](cpu_thread* const cpu)
	{
		for (auto it = &g_ppu;;)
		{
			const auto next = +*it;

			if (next == cpu)
			{
				ppu_log.trace("sleep() - suspended (p=%zu)", g_pending);

				if (static_cast<ppu_thread*>(cpu)->cancel_sleep == 1)
				{
					// The next sleep call of the thread is cancelled
					static_cast<ppu_thread*>(cpu)->cancel_sleep = 2;
				}

				return false;
			}

			// Use priority, also preserve FIFO order
			if (!next || (push_first ? next->prio.load().prio >= static_cast<ppu_thread*>(cpu)->prio.load().prio : next->prio.load().prio > static_cast<ppu_thread*>(cpu)->prio.load().prio))
			{
				atomic_storage<ppu_thread*>::release(static_cast<ppu_thread*>(cpu)->next_ppu, next);
				atomic_storage<ppu_thread*>::release(*it, static_cast<ppu_thread*>(cpu));
				break;
			}

			it = &next->next_ppu;
		}

		// Unregister timeout if necessary
		for (auto it = g_waiting.cbegin(), end = g_waiting.cend(); it != end; it++)
		{
			if (it->second == cpu)
			{
				g_waiting.erase(it);
				break;
			}
		}

		ppu_log.trace("awake(): %s", cpu->id);
		return true;
	};

	// Yield changed the queue before
	bool changed_queue = prio == yield_cmd;

	s32 lowest_new_priority = smax;
	const bool has_free_hw_thread_space = count_non_sleeping_threads().onproc_count < g_cfg.core.ppu_threads + 0u;

	if (cpu && prio != yield_cmd)
	{
		// Emplace current thread
		if (emplace_thread(cpu))
		{
			changed_queue = true;
			lowest_new_priority = std::min<s32>(static_cast<ppu_thread*>(cpu)->prio.load().prio, lowest_new_priority);
		}
	}
	else for (const auto _cpu : g_to_awake)
	{
		// Emplace threads from list
		if (emplace_thread(_cpu))
		{
			changed_queue = true;
			lowest_new_priority = std::min<s32>(static_cast<ppu_thread*>(_cpu)->prio.load().prio, lowest_new_priority);
		}
	}

	auto target = +g_ppu;
	usz i = 0;

	// Suspend threads if necessary
	for (usz thread_count = g_cfg.core.ppu_threads; target; target = target->next_ppu, i++)
	{
		if (i >= thread_count && cpu_flag::suspend - target->state)
		{
			ppu_log.trace("suspend(): %s", target->id);
			target->ack_suspend = true;
			g_pending++;
			ensure(!target->state.test_and_set(cpu_flag::suspend));

			if (is_paused(target->state - cpu_flag::suspend))
			{
				target->state.notify_one();
			}
		}
	}

	const auto current_ppu = cpu_thread::get_current<ppu_thread>();

	// Remove pending if necessary
	if (current_ppu)
	{
		if (std::exchange(current_ppu->ack_suspend, false))
		{
			ensure(g_pending)--;
		}
	}

	// In real PS3 (it seems), when a thread with a higher priority than the caller is signaled and -
	// - that there is available space on the running queue for the other hardware thread to start
	// It prioritizes signaled thread - caller's hardware thread switches instantly to the new thread code
	// While signaling to the other hardware thread to execute the caller's code.
	// Resulting in a delay to the caller after such thread is signaled

	if (current_ppu && changed_queue && has_free_hw_thread_space)
	{
		if (current_ppu->prio.load().prio > lowest_new_priority)
		{
			const bool is_create_thread = current_ppu->gpr[11] == 0x35;

			// When not being set to All timers - activate only for sys_ppu_thread_start
			if (is_create_thread || g_cfg.core.sleep_timers_accuracy == sleep_timers_accuracy_level::_all_timers)
			{
				if (!current_ppu->state.test_and_set(cpu_flag::yield) || current_ppu->hw_sleep_time != 0)
				{
					current_ppu->hw_sleep_time += (is_create_thread ? 51 : 35);
				}
				else
				{
					current_ppu->hw_sleep_time = 30000; // In addition to another flag's use (TODO: Refactor and clean this)
				}
			}
		}
	}

	return changed_queue;
}

void lv2_obj::cleanup()
{
	g_ppu = nullptr;
	g_scheduler_ready = false;
	g_to_sleep.clear();
	g_waiting.clear();
	g_pending = 0;
	s_yield_frequency = 0;
}

void lv2_obj::schedule_all(u64 current_time)
{
	auto it = std::find(g_to_notify, std::end(g_to_notify), std::add_pointer_t<const void>{});

	if (!g_pending && g_scheduler_ready)
	{
		auto target = +g_ppu;

		// Wake up threads
		for (usz x = g_cfg.core.ppu_threads; target && x; target = target->next_ppu, x--)
		{
			if (target->state & cpu_flag::suspend)
			{
				ppu_log.trace("schedule(): %s", target->id);

				// Remove yield if it was sleeping until now
				const bs_t<cpu_flag> remove_yield = target->start_time == 0 ? +cpu_flag::suspend : (cpu_flag::yield + cpu_flag::preempt);

				target->start_time = 0;

				if ((target->state.fetch_op(AOFN(x += cpu_flag::signal, x -= cpu_flag::suspend, x-= remove_yield, void())) & (cpu_flag::wait + cpu_flag::signal)) != cpu_flag::wait)
				{
					continue;
				}

				if (it == std::end(g_to_notify))
				{
					// Out of notification slots, notify locally (resizable container is not worth it)
					target->state.notify_one();
				}
				else
				{
					*it++ = &target->state;
				}
			}
		}
	}

	// Check registered timeouts
	while (!g_waiting.empty())
	{
		const auto pair = &g_waiting.front();

		if (!current_time)
		{
			current_time = get_guest_system_time();
		}

		if (pair->first <= current_time)
		{
			const auto target = pair->second;
			g_waiting.pop_front();

			if (target != cpu_thread::get_current())
			{
				// Change cpu_thread::state for the lightweight notification to work
				ensure(!target->state.test_and_set(cpu_flag::notify));

				// Otherwise notify it to wake itself
				if (it == std::end(g_to_notify))
				{
					// Out of notification slots, notify locally (resizable container is not worth it)
					target->state.notify_one();
				}
				else
				{
					*it++ = &target->state;
				}
			}
		}
		else
		{
			// The list is sorted so assume no more timeouts
			break;
		}
	}

	if (it < std::end(g_to_notify))
	{
		// Null-terminate the list if it ends before last slot
		*it = nullptr;
	}

	if (const u64 freq = s_yield_frequency)
	{
		const u64 tsc = utils::get_tsc();
		const u64 last_tsc = s_last_yield_tsc;

		if (tsc >= last_tsc && tsc <= s_max_allowed_yield_tsc && tsc - last_tsc >= freq)
		{
			auto target = +g_ppu;
			cpu_thread* cpu = nullptr;

			for (usz x = g_cfg.core.ppu_threads;; target = target->next_ppu, x--)
			{
				if (!target || !x)
				{
					if (g_ppu && cpu_flag::preempt - g_ppu->state)
					{
						// Don't be picky, pick up any running PPU thread even it has a wait flag
						cpu = g_ppu;
					}
					// TODO: If this case is common enough it may be valuable to iterate over all CPU threads to find a perfect candidate (one without a wait or suspend flag)
					else if (auto current = cpu_thread::get_current(); current && cpu_flag::suspend - current->state)
					{
						// May be an SPU or RSX thread, use them as a last resort
						cpu = current;
					}

					break;
				}

				if (target->state.none_of(cpu_flag::preempt + cpu_flag::wait))
				{
					cpu = target;
					break;
				}
			}

			if (cpu && cpu_flag::preempt - cpu->state && !cpu->state.test_and_set(cpu_flag::preempt))
			{
				s_last_yield_tsc = tsc;
				g_lv2_preempts_taken.release(g_lv2_preempts_taken.load() + 1); // Has a minor race but performance is more important
				rsx::set_rsx_yield_flag();
			}
		}
	}
}

void lv2_obj::make_scheduler_ready()
{
	g_scheduler_ready.release(true);
	lv2_obj::awake_all();
}

std::pair<ppu_thread_status, u32> lv2_obj::ppu_state(ppu_thread* ppu, bool lock_idm, bool lock_lv2)
{
	std::optional<reader_lock> opt_lock[2];

	if (lock_idm)
	{
		opt_lock[0].emplace(id_manager::g_mutex);
	}

	if (!Emu.IsReady() ? ppu->state.all_of(cpu_flag::stop) : ppu->stop_flag_removal_protection)
	{
		return { PPU_THREAD_STATUS_IDLE, 0};
	}

	switch (ppu->joiner)
	{
	case ppu_join_status::zombie: return { PPU_THREAD_STATUS_ZOMBIE, 0};
	case ppu_join_status::exited: return { PPU_THREAD_STATUS_DELETED, 0};
	default: break;
	}

	if (lock_lv2)
	{
		opt_lock[1].emplace(lv2_obj::g_mutex);
	}

	u32 pos = umax;
	u32 i = 0;

	for (auto target = +g_ppu; target; target = target->next_ppu, i++)
	{
		if (target == ppu)
		{
			pos = i;
			break;
		}
	}

	if (pos == umax)
	{
		if (!ppu->interrupt_thread_executing)
		{
			return { PPU_THREAD_STATUS_STOP, 0};
		}

		return { PPU_THREAD_STATUS_SLEEP, 0 };
	}

	if (pos >= g_cfg.core.ppu_threads + 0u)
	{
		return { PPU_THREAD_STATUS_RUNNABLE, pos };
	}

	return { PPU_THREAD_STATUS_ONPROC, pos};
}

void lv2_obj::set_future_sleep(cpu_thread* cpu)
{
	g_to_sleep.emplace_back(cpu);
}

bool lv2_obj::is_scheduler_ready()
{
	reader_lock lock(g_mutex);
	return g_to_sleep.empty();
}

ppu_non_sleeping_count_t lv2_obj::count_non_sleeping_threads()
{
	ppu_non_sleeping_count_t total{};

	auto target = atomic_storage<ppu_thread*>::load(g_ppu);

	for (usz thread_count = g_cfg.core.ppu_threads; target; target = atomic_storage<ppu_thread*>::load(target->next_ppu))
	{
		if (total.onproc_count == thread_count)
		{
			total.has_running = true;
			break;
		}

		total.onproc_count++;
	}

	return total;
}

void lv2_obj::set_yield_frequency(u64 freq, u64 max_allowed_tsc)
{
	s_yield_frequency.release(freq);
	s_max_allowed_yield_tsc.release(max_allowed_tsc);
	g_lv2_preempts_taken.release(0);
}

#if defined(_MSC_VER)
#define mwaitx_func
#define waitpkg_func
#else
#define mwaitx_func __attribute__((__target__("mwaitx")))
#define waitpkg_func __attribute__((__target__("waitpkg")))
#endif

#if defined(ARCH_X64)
// Waits for a number of TSC clock cycles in power optimized state
// Cstate is represented in bits [7:4]+1 cstate. So C0 requires bits [7:4] to be set to 0xf, C1 requires bits [7:4] to be set to 0.
mwaitx_func static void __mwaitx(u32 cycles, u32 cstate)
{
	constexpr u32 timer_enable = 0x2;

	// monitorx will wake if the cache line is written to. We don't want this, so place the monitor value on it's own cache line.
	alignas(64) u64 monitor_var{};
	_mm_monitorx(&monitor_var, 0, 0);
	_mm_mwaitx(timer_enable, cstate, cycles);
}

// First bit indicates cstate, 0x0 for C.02 state (lower power) or 0x1 for C.01 state (higher power)
waitpkg_func static void __tpause(u32 cycles, u32 cstate)
{
	const u64 tsc = utils::get_tsc() + cycles;
	_tpause(cstate, tsc);
}
#endif

bool lv2_obj::wait_timeout(u64 usec, ppu_thread* cpu, bool scale, bool is_usleep)
{
	static_assert(u64{umax} / max_timeout >= 100, "max timeout is not valid for scaling");

	const u64 start_time = get_system_time();

	if (cpu)
	{
		if (u64 end_time = cpu->end_time; end_time != umax)
		{
			const u64 guest_start = get_guest_system_time(start_time);

			if (end_time <= guest_start)
			{
				return true;
			}

			usec = end_time - guest_start;
			scale = true;
		}
	}

	if (scale)
	{
		// Scale time
		usec = std::min<u64>(usec, u64{umax} / 100) * 100 / g_cfg.core.clocks_scale;
	}

	// Clamp
	usec = std::min<u64>(usec, max_timeout);

	u64 passed = 0;

	atomic_bs_t<cpu_flag> dummy{};
	const auto& state = cpu ? cpu->state : dummy;
	auto old_state = +state;

	auto wait_for = [&](u64 timeout)
	{
		thread_ctrl::wait_on(state, old_state, timeout);
	};

	for (;; old_state = state)
	{
		if (old_state & cpu_flag::notify)
		{
			// Timeout notification has been forced
			break;
		}

		if (old_state & cpu_flag::signal)
		{
			return false;
		}

		if (::is_stopped(old_state) || thread_ctrl::state() == thread_state::aborting)
		{
			return passed >= usec;
		}

		if (passed >= usec)
		{
			break;
		}

		u64 remaining = usec - passed;
#ifdef __linux__
		// NOTE: Assumption that timer initialization has succeeded
		constexpr u64 host_min_quantum = 10;
#else
		// Host scheduler quantum for windows (worst case)
		// NOTE: On ps3 this function has very high accuracy
		constexpr u64 host_min_quantum = 500;
#endif
		// TODO: Tune for other non windows operating sytems

		if (g_cfg.core.sleep_timers_accuracy < (is_usleep ? sleep_timers_accuracy_level::_usleep : sleep_timers_accuracy_level::_all_timers))
		{
			wait_for(remaining);
		}
		else
		{
			if (remaining > host_min_quantum)
			{
#ifdef __linux__
				// With timerslack set low, Linux is precise for all values above
				wait_for(remaining);
#else
				// Wait on multiple of min quantum for large durations to avoid overloading low thread cpus
				wait_for(remaining - (remaining % host_min_quantum));
#endif
			}
			// TODO: Determine best value for yield delay
#if defined(ARCH_X64)
			else if (utils::has_appropriate_um_wait())
			{
				const u32 us_in_tsc_clocks = ::narrow<u32>(remaining * (utils::get_tsc_freq() / 1000000ULL));

				if (utils::has_waitpkg())
				{
					__tpause(us_in_tsc_clocks, 0x1);
				}
				else
				{
					__mwaitx(us_in_tsc_clocks, 0xf0);
				}
			}
#endif
			else
			{
				// Try yielding. May cause long wake latency but helps weaker CPUs a lot by alleviating resource pressure
				std::this_thread::yield();
			}
		}

		passed = get_system_time() - start_time;
	}

	return true;
}

void lv2_obj::prepare_for_sleep(cpu_thread& cpu)
{
	vm::temporary_unlock(cpu);
	cpu_counter::remove(&cpu);
}

void lv2_obj::notify_all() noexcept
{
	for (auto cpu : g_to_notify)
	{
		if (!cpu)
		{
			break;
		}

		if (cpu != &g_to_notify)
		{
			const auto res_start = vm::reservation_notifier(0).second;
			const auto res_end = vm::reservation_notifier(umax).second;

			if (cpu >= res_start && cpu <= res_end)
			{
				atomic_wait_engine::notify_all(cpu);
			}
			else
			{
				// Note: by the time of notification the thread could have been deallocated which is why the direct function is used
				atomic_wait_engine::notify_one(cpu);
			}
		}
	}

	g_to_notify[0] = nullptr;
	g_postpone_notify_barrier = false;

	const auto cpu = cpu_thread::get_current();

	if (!cpu)
	{
		return;
	}

	if (cpu->get_class() != thread_class::spu && cpu->state.none_of(cpu_flag::suspend))
	{
		return;
	}

	std::optional<vm::writer_lock> lock;

	constexpr usz total_waiters = std::size(spu_thread::g_spu_waiters_by_value);

	u32 notifies[total_waiters]{};

	// There may be 6 waiters, but checking them all may be performance expensive 
	// Instead, check 2 at max, but use the CPU ID index to tell which index to start checking so the work would be distributed across all threads

	atomic_t<u64, 64>* range_lock = nullptr;

	for (usz i = 0, checked = 0; checked < 3 && i < total_waiters; i++)
	{
		auto& waiter = spu_thread::g_spu_waiters_by_value[(i + cpu->id) % total_waiters];
		const u64 value = waiter.load();
		u32 raddr = static_cast<u32>(value) & -128;

		if (vm::check_addr(raddr))
		{
			if (((raddr >> 28) < 2 || (raddr >> 28) == 0xd))
			{
				checked++;

				if (compute_rdata_hash32(*vm::get_super_ptr<spu_rdata_t>(raddr)) != static_cast<u32>(value >> 32))
				{
					// Clear address to avoid a race, keep waiter counter
					if (waiter.fetch_op([&](u64& x)
					{
						if ((x & -128) == (value & -128))
						{
							x &= 127;
							return true;
						}

						return false;
					}).second)
					{
						notifies[i] = raddr;
					}
				}

				continue;
			}

			if (!range_lock)
			{
				range_lock = vm::alloc_range_lock();
			}

			checked++;

			if (spu_thread::reservation_check(raddr, static_cast<u32>(value >> 32), range_lock))
			{
				// Clear address to avoid a race, keep waiter counter
				if (waiter.fetch_op([&](u64& x)
				{
					if ((x & -128) == (value & -128))
					{
						x &= 127;
						return true;
					}

					return false;
				}).second)
				{
					notifies[i] = raddr;
				}
			}
		}
	}

	if (range_lock)
	{
		vm::free_range_lock(range_lock);
	}

	for (u32 addr : notifies)
	{
		if (addr)
		{
			vm::reservation_notifier_notify(addr);
		}
	}
}
