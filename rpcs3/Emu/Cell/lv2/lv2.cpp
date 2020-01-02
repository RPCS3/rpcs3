#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/vm_ptr.h"

#include "Emu/Cell/PPUFunction.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/MFC.h"
#include "sys_sync.h"
#include "sys_lwmutex.h"
#include "sys_lwcond.h"
#include "sys_mutex.h"
#include "sys_cond.h"
#include "sys_event.h"
#include "sys_event_flag.h"
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

extern std::string ppu_get_syscall_name(u64 code);

template <>
void fmt_class_string<ppu_syscall_code>::format(std::string& out, u64 arg)
{
	out += ppu_get_syscall_name(arg);
}

static bool null_func(ppu_thread& ppu)
{
	LOG_TODO(HLE, "Unimplemented syscall %s -> CELL_OK", ppu_syscall_code(ppu.gpr[11]));
	ppu.gpr[3] = 0;
	ppu.cia += 4;
	return false;
}

static bool uns_func(ppu_thread& ppu)
{
	LOG_TRACE(HLE, "Unused syscall %d -> ENOSYS", ppu.gpr[11]);
	ppu.gpr[3] = CELL_ENOSYS;
	ppu.cia += 4;
	return false;
}

std::array<ppu_function_t, 1024> g_ppu_syscall_table{};

// UNS = Unused
// ROOT = Root
// DBG = Debug
// DEX..DECR = Unavailable on retail consoles
// PM = Product Mode
// AuthID = Authentication ID
const std::array<ppu_function_t, 1024> s_ppu_syscall_table
{
	null_func,
	BIND_FUNC(sys_process_getpid),                          //1   (0x001)
	BIND_FUNC(sys_process_wait_for_child),                  //2   (0x002)  ROOT
	null_func,//BIND_FUNC(sys_process_exit),                //3   (0x003)
	BIND_FUNC(sys_process_get_status),                      //4   (0x004)  DBG
	BIND_FUNC(sys_process_detach_child),                    //5   (0x005)  DBG

	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //6-11  UNS

	BIND_FUNC(sys_process_get_number_of_object),            //12  (0x00C)
	BIND_FUNC(sys_process_get_id),                          //13  (0x00D)
	BIND_FUNC(sys_process_is_spu_lock_line_reservation_address), //14  (0x00E)

	uns_func, uns_func, uns_func,                           //15-17  UNS

	BIND_FUNC(sys_process_getppid),                         //18  (0x012)
	BIND_FUNC(sys_process_kill),                            //19  (0x013)
	uns_func,                                               //20  (0x014)  UNS
	null_func,//BIND_FUNC(_sys_process_spawn),              //21  (0x015)  DBG
	BIND_FUNC(_sys_process_exit),                           //22  (0x016)
	BIND_FUNC(sys_process_wait_for_child2),                 //23  (0x017)  DBG
	null_func,//BIND_FUNC(),                                //24  (0x018)  DBG
	BIND_FUNC(sys_process_get_sdk_version),                 //25  (0x019)
	BIND_FUNC(_sys_process_exit2),                          //26  (0x01A)
	null_func,//BIND_FUNC(),                                //27  (0x01B)  DBG
	null_func,//BIND_FUNC(_sys_process_get_number_of_object)//28  (0x01C)  ROOT
	BIND_FUNC(sys_process_get_id2),                          //29  (0x01D)  ROOT
	BIND_FUNC(_sys_process_get_paramsfo),                   //30  (0x01E)
	null_func,//BIND_FUNC(sys_process_get_ppu_guid),        //31  (0x01F)

	uns_func, uns_func ,uns_func , uns_func ,uns_func, uns_func ,uns_func, uns_func ,uns_func, //32-40  UNS

	BIND_FUNC(_sys_ppu_thread_exit),                        //41  (0x029)
	uns_func,                                               //42  (0x02A)  UNS
	BIND_FUNC(sys_ppu_thread_yield),                        //43  (0x02B)
	BIND_FUNC(sys_ppu_thread_join),                         //44  (0x02C)
	BIND_FUNC(sys_ppu_thread_detach),                       //45  (0x02D)
	BIND_FUNC(sys_ppu_thread_get_join_state),               //46  (0x02E)
	BIND_FUNC(sys_ppu_thread_set_priority),                 //47  (0x02F)  DBG
	BIND_FUNC(sys_ppu_thread_get_priority),                 //48  (0x030)
	BIND_FUNC(sys_ppu_thread_get_stack_information),        //49  (0x031)
	null_func,//BIND_FUNC(sys_ppu_thread_stop),             //50  (0x032)  ROOT
	null_func,//BIND_FUNC(sys_ppu_thread_restart),          //51  (0x033)  ROOT
	BIND_FUNC(_sys_ppu_thread_create),                      //52  (0x034)  DBG
	BIND_FUNC(sys_ppu_thread_start),                        //53  (0x035)
	null_func,//BIND_FUNC(sys_ppu_...),                     //54  (0x036)  ROOT
	null_func,//BIND_FUNC(sys_ppu_...),                     //55  (0x037)  ROOT
	BIND_FUNC(sys_ppu_thread_rename),                       //56  (0x038)
	BIND_FUNC(sys_ppu_thread_recover_page_fault),           //57  (0x039)
	BIND_FUNC(sys_ppu_thread_get_page_fault_context),       //58  (0x03A)
	uns_func,                                               //59  (0x03B)  UNS
	BIND_FUNC(sys_trace_create),                            //60  (0x03C)
	BIND_FUNC(sys_trace_start),                             //61  (0x03D)
	BIND_FUNC(sys_trace_stop),                              //62  (0x03E)
	BIND_FUNC(sys_trace_update_top_index),                  //63  (0x03F)
	BIND_FUNC(sys_trace_destroy),                           //64  (0x040)
	BIND_FUNC(sys_trace_drain),                             //65  (0x041)
	BIND_FUNC(sys_trace_attach_process),                    //66  (0x042)
	BIND_FUNC(sys_trace_allocate_buffer),                   //67  (0x043)
	BIND_FUNC(sys_trace_free_buffer),                       //68  (0x044)
	BIND_FUNC(sys_trace_create2),                           //69  (0x045)
	BIND_FUNC(sys_timer_create),                            //70  (0x046)
	BIND_FUNC(sys_timer_destroy),                           //71  (0x047)
	BIND_FUNC(sys_timer_get_information),                   //72  (0x048)
	BIND_FUNC(_sys_timer_start),                            //73  (0x049)
	BIND_FUNC(sys_timer_stop),                              //74  (0x04A)
	BIND_FUNC(sys_timer_connect_event_queue),               //75  (0x04B)
	BIND_FUNC(sys_timer_disconnect_event_queue),            //76  (0x04C)
	null_func,//BIND_FUNC(sys_trace_create2_in_cbepm),      //77  (0x04D)
	null_func,//BIND_FUNC(sys_trace_...)                    //78  (0x04E)
	uns_func,                                               //79  (0x04F)  UNS
	null_func,//BIND_FUNC(sys_interrupt_tag_create)         //80  (0x050)
	BIND_FUNC(sys_interrupt_tag_destroy),                   //81  (0x051)
	BIND_FUNC(sys_event_flag_create),                       //82  (0x052)
	BIND_FUNC(sys_event_flag_destroy),                      //83  (0x053)
	BIND_FUNC(_sys_interrupt_thread_establish),             //84  (0x054)
	BIND_FUNC(sys_event_flag_wait),                         //85  (0x055)
	BIND_FUNC(sys_event_flag_trywait),                      //86  (0x056)
	BIND_FUNC(sys_event_flag_set),                          //87  (0x057)
	BIND_FUNC(sys_interrupt_thread_eoi),                    //88  (0x058)
	BIND_FUNC(_sys_interrupt_thread_disestablish),          //89  (0x059)
	BIND_FUNC(sys_semaphore_create),                        //90  (0x05A)
	BIND_FUNC(sys_semaphore_destroy),                       //91  (0x05B)
	BIND_FUNC(sys_semaphore_wait),                          //92  (0x05C)
	BIND_FUNC(sys_semaphore_trywait),                       //93  (0x05D)
	BIND_FUNC(sys_semaphore_post),                          //94  (0x05E)
	BIND_FUNC(_sys_lwmutex_create),                         //95  (0x05F)
	BIND_FUNC(_sys_lwmutex_destroy),                        //96  (0x060)
	BIND_FUNC(_sys_lwmutex_lock),                           //97  (0x061)
	BIND_FUNC(_sys_lwmutex_unlock),                         //98  (0x062)
	BIND_FUNC(_sys_lwmutex_trylock),                        //99  (0x063)
	BIND_FUNC(sys_mutex_create),                            //100 (0x064)
	BIND_FUNC(sys_mutex_destroy),                           //101 (0x065)
	BIND_FUNC(sys_mutex_lock),                              //102 (0x066)
	BIND_FUNC(sys_mutex_trylock),                           //103 (0x067)
	BIND_FUNC(sys_mutex_unlock),                            //104 (0x068)
	BIND_FUNC(sys_cond_create),                             //105 (0x069)
	BIND_FUNC(sys_cond_destroy),                            //106 (0x06A)
	BIND_FUNC(sys_cond_wait),                               //107 (0x06B)
	BIND_FUNC(sys_cond_signal),                             //108 (0x06C)
	BIND_FUNC(sys_cond_signal_all),                         //109 (0x06D)
	BIND_FUNC(sys_cond_signal_to),                          //110 (0x06E)
	BIND_FUNC(_sys_lwcond_create),                          //111 (0x06F)
	BIND_FUNC(_sys_lwcond_destroy),                         //112 (0x070)
	BIND_FUNC(_sys_lwcond_queue_wait),                      //113 (0x071)
	BIND_FUNC(sys_semaphore_get_value),                     //114 (0x072)
	BIND_FUNC(_sys_lwcond_signal),                          //115 (0x073)
	BIND_FUNC(_sys_lwcond_signal_all),                      //116 (0x074)
	BIND_FUNC(_sys_lwmutex_unlock2),                        //117 (0x075)
	BIND_FUNC(sys_event_flag_clear),                        //118 (0x076)
	null_func,//BIND_FUNC(sys_time_get_rtc)                 //119 (0x077)  ROOT
	BIND_FUNC(sys_rwlock_create),                           //120 (0x078)
	BIND_FUNC(sys_rwlock_destroy),                          //121 (0x079)
	BIND_FUNC(sys_rwlock_rlock),                            //122 (0x07A)
	BIND_FUNC(sys_rwlock_tryrlock),                         //123 (0x07B)
	BIND_FUNC(sys_rwlock_runlock),                          //124 (0x07C)
	BIND_FUNC(sys_rwlock_wlock),                            //125 (0x07D)
	BIND_FUNC(sys_rwlock_trywlock),                         //126 (0x07E)
	BIND_FUNC(sys_rwlock_wunlock),                          //127 (0x07F)
	BIND_FUNC(sys_event_queue_create),                      //128 (0x080)
	BIND_FUNC(sys_event_queue_destroy),                     //129 (0x081)
	BIND_FUNC(sys_event_queue_receive),                     //130 (0x082)
	BIND_FUNC(sys_event_queue_tryreceive),                  //131 (0x083)
	BIND_FUNC(sys_event_flag_cancel),                       //132 (0x084)
	BIND_FUNC(sys_event_queue_drain),                       //133 (0x085)
	BIND_FUNC(sys_event_port_create),                       //134 (0x086)
	BIND_FUNC(sys_event_port_destroy),                      //135 (0x087)
	BIND_FUNC(sys_event_port_connect_local),                //136 (0x088)
	BIND_FUNC(sys_event_port_disconnect),                   //137 (0x089)
	BIND_FUNC(sys_event_port_send),                         //138 (0x08A)
	BIND_FUNC(sys_event_flag_get),                          //139 (0x08B)
	BIND_FUNC(sys_event_port_connect_ipc),                  //140 (0x08C)
	BIND_FUNC(sys_timer_usleep),                            //141 (0x08D)
	BIND_FUNC(sys_timer_sleep),                             //142 (0x08E)
	null_func,//BIND_FUNC(sys_time_set_timezone)            //143 (0x08F)  ROOT
	BIND_FUNC(sys_time_get_timezone),                       //144 (0x090)
	BIND_FUNC(sys_time_get_current_time),                   //145 (0x091)
	null_func,//BIND_FUNC(sys_time_get_system_time),        //146 (0x092)  ROOT
	BIND_FUNC(sys_time_get_timebase_frequency),             //147 (0x093)
	BIND_FUNC(_sys_rwlock_trywlock),                        //148 (0x094)
	uns_func,                                               //149 (0x095)  UNS
	BIND_FUNC(sys_raw_spu_create_interrupt_tag),            //150 (0x096)
	BIND_FUNC(sys_raw_spu_set_int_mask),                    //151 (0x097)
	BIND_FUNC(sys_raw_spu_get_int_mask),                    //152 (0x098)
	BIND_FUNC(sys_raw_spu_set_int_stat),                    //153 (0x099)
	BIND_FUNC(sys_raw_spu_get_int_stat),                    //154 (0x09A)
	BIND_FUNC(_sys_spu_image_get_information),              //155 (0x09B)
	BIND_FUNC(sys_spu_image_open),                          //156 (0x09C)
	BIND_FUNC(_sys_spu_image_import),                       //157 (0x09D)
	BIND_FUNC(_sys_spu_image_close),                        //158 (0x09E)
	BIND_FUNC(_sys_spu_image_get_segments),                 //159 (0x09F)
	BIND_FUNC(sys_raw_spu_create),                          //160 (0x0A0)
	BIND_FUNC(sys_raw_spu_destroy),                         //161 (0x0A1)
	uns_func,                                               //162 (0x0A2)  UNS
	BIND_FUNC(sys_raw_spu_read_puint_mb),                   //163 (0x0A3)
	uns_func,                                               //164 (0x0A4)  UNS
	BIND_FUNC(sys_spu_thread_get_exit_status),              //165 (0x0A5)
	BIND_FUNC(sys_spu_thread_set_argument),                 //166 (0x0A6)
	null_func,//BIND_FUNC(sys_spu_thread_group_start_on_exit)//167(0x0A7)
	uns_func,                                               //168 (0x0A8)  UNS
	BIND_FUNC(sys_spu_initialize),                          //169 (0x0A9)
	BIND_FUNC(sys_spu_thread_group_create),                 //170 (0x0AA)
	BIND_FUNC(sys_spu_thread_group_destroy),                //171 (0x0AB)
	BIND_FUNC(sys_spu_thread_initialize),                   //172 (0x0AC)
	BIND_FUNC(sys_spu_thread_group_start),                  //173 (0x0AD)
	BIND_FUNC(sys_spu_thread_group_suspend),                //174 (0x0AE)
	BIND_FUNC(sys_spu_thread_group_resume),                 //175 (0x0AF)
	BIND_FUNC(sys_spu_thread_group_yield),                  //176 (0x0B0)
	BIND_FUNC(sys_spu_thread_group_terminate),              //177 (0x0B1)
	BIND_FUNC(sys_spu_thread_group_join),                   //178 (0x0B2)
	BIND_FUNC(sys_spu_thread_group_set_priority),           //179 (0x0B3)
	BIND_FUNC(sys_spu_thread_group_get_priority),           //180 (0x0B4)
	BIND_FUNC(sys_spu_thread_write_ls),                     //181 (0x0B5)
	BIND_FUNC(sys_spu_thread_read_ls),                      //182 (0x0B6)
	uns_func,                                               //183 (0x0B7)  UNS
	BIND_FUNC(sys_spu_thread_write_snr),                    //184 (0x0B8)
	BIND_FUNC(sys_spu_thread_group_connect_event),          //185 (0x0B9)
	BIND_FUNC(sys_spu_thread_group_disconnect_event),       //186 (0x0BA)
	BIND_FUNC(sys_spu_thread_set_spu_cfg),                  //187 (0x0BB)
	BIND_FUNC(sys_spu_thread_get_spu_cfg),                  //188 (0x0BC)
	uns_func,                                               //189 (0x0BD)  UNS
	BIND_FUNC(sys_spu_thread_write_spu_mb),                 //190 (0x0BE)
	BIND_FUNC(sys_spu_thread_connect_event),                //191 (0x0BF)
	BIND_FUNC(sys_spu_thread_disconnect_event),             //192 (0x0C0)
	BIND_FUNC(sys_spu_thread_bind_queue),                   //193 (0x0C1)
	BIND_FUNC(sys_spu_thread_unbind_queue),                 //194 (0x0C2)
	uns_func,                                               //195 (0x0C3)  UNS
	BIND_FUNC(sys_raw_spu_set_spu_cfg),                     //196 (0x0C4)
	BIND_FUNC(sys_raw_spu_get_spu_cfg),                     //197 (0x0C5)
	BIND_FUNC(sys_spu_thread_recover_page_fault),           //198 (0x0C6)
	BIND_FUNC(sys_raw_spu_recover_page_fault),              //199 (0x0C7)

	null_func, null_func, null_func, null_func, null_func,  //204  UNS?
	null_func, null_func, null_func, null_func, null_func,  //209  UNS?
	null_func, null_func, null_func, null_func, null_func,  //214  UNS?

	null_func,//BIND_FUNC(sys_dbg_mat_set_condition)        //215 (0x0D7)
	null_func,//BIND_FUNC(sys_dbg_mat_get_condition)        //216 (0x0D8)
	uns_func,//BIND_FUNC(sys_dbg_...)                       //217 (0x0D9) DBG  UNS?
	uns_func,//BIND_FUNC(sys_dbg_...)                       //218 (0x0DA) DBG  UNS?
	uns_func,//BIND_FUNC(sys_dbg_...)                       //219 (0x0DB) DBG  UNS?

	null_func, null_func, null_func, null_func, null_func,  //224  UNS
	null_func, null_func, null_func, null_func, null_func,  //229  UNS?

	null_func,//BIND_FUNC(sys_isolated_spu_create)          //230 (0x0E6)  ROOT
	null_func,//BIND_FUNC(sys_isolated_spu_destroy)         //231 (0x0E7)  ROOT
	null_func,//BIND_FUNC(sys_isolated_spu_start)           //232 (0x0E8)  ROOT
	null_func,//BIND_FUNC(sys_isolated_spu_create_interrupt_tag) //233 (0x0E9)  ROOT
	null_func,//BIND_FUNC(sys_isolated_spu_set_int_mask)    //234 (0x0EA)  ROOT
	null_func,//BIND_FUNC(sys_isolated_spu_get_int_mask)    //235 (0x0EB)  ROOT
	null_func,//BIND_FUNC(sys_isolated_spu_set_int_stat)    //236 (0x0EC)  ROOT
	null_func,//BIND_FUNC(sys_isolated_spu_get_int_stat)    //237 (0x0ED)  ROOT
	null_func,//BIND_FUNC(sys_isolated_spu_set_spu_cfg)     //238 (0x0EE)  ROOT
	null_func,//BIND_FUNC(sys_isolated_spu_get_spu_cfg)     //239 (0x0EF)  ROOT
	null_func,//BIND_FUNC(sys_isolated_spu_read_puint_mb)   //240 (0x0F0)  ROOT
	uns_func, uns_func, uns_func,                           //241-243 ROOT  UNS
	null_func,//BIND_FUNC(sys_spu_thread_group_system_set_next_group) //244 (0x0F4)  ROOT
	null_func,//BIND_FUNC(sys_spu_thread_group_system_unset_next_group) //245 (0x0F5)  ROOT
	null_func,//BIND_FUNC(sys_spu_thread_group_system_set_switch_group) //246 (0x0F6)  ROOT
	null_func,//BIND_FUNC(sys_spu_thread_group_system_unset_switch_group) //247 (0x0F7)  ROOT
	null_func,//BIND_FUNC(sys_spu_thread_group...)          //248 (0x0F8)  ROOT
	null_func,//BIND_FUNC(sys_spu_thread_group...)          //249 (0x0F9)  ROOT
	null_func,//BIND_FUNC(sys_spu_thread_group_set_cooperative_victims) //250 (0x0FA)
	BIND_FUNC(sys_spu_thread_group_connect_event_all_threads), //251 (0x0FB)
	BIND_FUNC(sys_spu_thread_group_disconnect_event_all_threads), //252 (0x0FC)
	null_func,//BIND_FUNC(sys_spu_thread_group...)          //253 (0x0FD)
	BIND_FUNC(sys_spu_thread_group_log),                    //254 (0x0FE)

	uns_func, uns_func, uns_func, uns_func, uns_func,       //255-259  UNS

	null_func,//BIND_FUNC(sys_spu_image_open_by_fd)         //260 (0x104)

	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //261-269  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //270-279  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //280-289  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //290-299  UNS

	BIND_FUNC(sys_vm_memory_map),                           //300 (0x12C)
	BIND_FUNC(sys_vm_unmap),                                //301 (0x12D)
	BIND_FUNC(sys_vm_append_memory),                        //302 (0x12E)
	BIND_FUNC(sys_vm_return_memory),                        //303 (0x12F)
	BIND_FUNC(sys_vm_lock),                                 //304 (0x130)
	BIND_FUNC(sys_vm_unlock),                               //305 (0x131)
	BIND_FUNC(sys_vm_touch),                                //306 (0x132)
	BIND_FUNC(sys_vm_flush),                                //307 (0x133)
	BIND_FUNC(sys_vm_invalidate),                           //308 (0x134)
	BIND_FUNC(sys_vm_store),                                //309 (0x135)
	BIND_FUNC(sys_vm_sync),                                 //310 (0x136)
	BIND_FUNC(sys_vm_test),                                 //311 (0x137)
	BIND_FUNC(sys_vm_get_statistics),                       //312 (0x138)
	BIND_FUNC(sys_vm_memory_map_different),				    //313 (0x139) //BIND_FUNC(sys_vm_memory_map (different))
	null_func,//BIND_FUNC(sys_...)                          //314 (0x13A)
	null_func,//BIND_FUNC(sys_...)                          //315 (0x13B)

	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //316-323  UNS

	BIND_FUNC(sys_memory_container_create),                 //324 (0x144)  DBG
	BIND_FUNC(sys_memory_container_destroy),                //325 (0x145)  DBG
	BIND_FUNC(sys_mmapper_allocate_fixed_address),          //326 (0x146)
	BIND_FUNC(sys_mmapper_enable_page_fault_notification),  //327 (0x147)
	BIND_FUNC(sys_mmapper_allocate_shared_memory_from_container_ext), //328 (0x148)
	BIND_FUNC(sys_mmapper_free_shared_memory),              //329 (0x149)
	BIND_FUNC(sys_mmapper_allocate_address),                //330 (0x14A)
	BIND_FUNC(sys_mmapper_free_address),                    //331 (0x14B)
	BIND_FUNC(sys_mmapper_allocate_shared_memory),          //332 (0x14C)
	null_func,//BIND_FUNC(sys_mmapper_set_shared_memory_flag)//333(0x14D)
	BIND_FUNC(sys_mmapper_map_shared_memory),               //334 (0x14E)
	BIND_FUNC(sys_mmapper_unmap_shared_memory),             //335 (0x14F)
	BIND_FUNC(sys_mmapper_change_address_access_right),     //336 (0x150)
	BIND_FUNC(sys_mmapper_search_and_map),                  //337 (0x151)
	null_func,//BIND_FUNC(sys_mmapper_get_shared_memory_attribute) //338 (0x152)
	BIND_FUNC(sys_mmapper_allocate_shared_memory_ext),      //339 (0x153)
	null_func,//BIND_FUNC(sys_...)                          //340 (0x154)
	BIND_FUNC(sys_memory_container_create),                 //341 (0x155)
	BIND_FUNC(sys_memory_container_destroy),                //342 (0x156)
	BIND_FUNC(sys_memory_container_get_size),               //343 (0x157)
	null_func,//BIND_FUNC(sys_memory_budget_set)            //344 (0x158)
	null_func,//BIND_FUNC(sys_memory_...)                   //345 (0x159)
	null_func,//BIND_FUNC(sys_memory_...)                   //346 (0x15A)
	uns_func,                                               //347 (0x15B)  UNS
	BIND_FUNC(sys_memory_allocate),                         //348 (0x15C)
	BIND_FUNC(sys_memory_free),                             //349 (0x15D)
	BIND_FUNC(sys_memory_allocate_from_container),          //350 (0x15E)
	BIND_FUNC(sys_memory_get_page_attribute),               //351 (0x15F)
	BIND_FUNC(sys_memory_get_user_memory_size),             //352 (0x160)
	null_func,//BIND_FUNC(sys_memory_get_user_memory_stat)  //353 (0x161)
	null_func,//BIND_FUNC(sys_memory_...)                   //354 (0x162)
	null_func,//BIND_FUNC(sys_memory_...)                   //355 (0x163)
	null_func,//BIND_FUNC(sys_memory_allocate_colored)      //356 (0x164)
	null_func,//BIND_FUNC(sys_memory_...)                   //357 (0x165)
	null_func,//BIND_FUNC(sys_memory_...)                   //358 (0x166)
	null_func,//BIND_FUNC(sys_memory_...)                   //359 (0x167)
	null_func,//BIND_FUNC(sys_memory_...)                   //360 (0x168)
	null_func,//BIND_FUNC(sys_memory_allocate_from_container_colored) //361 (0x169)
	BIND_FUNC(sys_mmapper_allocate_shared_memory_from_container),//362 (0x16A)
	null_func,//BIND_FUNC(sys_mmapper_...)                  //363 (0x16B)
	null_func,//BIND_FUNC(sys_mmapper_...)                  //364 (0x16C)
	uns_func, uns_func,                                     //366 (0x16E)  UNS
	null_func,//BIND_FUNC(sys_uart_initialize)              //367 (0x16F)  ROOT
	null_func,//BIND_FUNC(sys_uart_receive)                 //368 (0x170)  ROOT
	null_func,//BIND_FUNC(sys_uart_send)                    //369 (0x171)  ROOT
	null_func,//BIND_FUNC(sys_uart_get_params)              //370 (0x172)  ROOT
	uns_func,                                               //371 (0x173)  UNS
	null_func,//BIND_FUNC(sys_game_watchdog_start)          //372 (0x174)
	null_func,//BIND_FUNC(sys_game_watchdog_stop)           //373 (0x175)
	null_func,//BIND_FUNC(sys_game_watchdog_clear)          //374 (0x176)
	null_func,//BIND_FUNC(sys_game_set_system_sw_version)   //375 (0x177)  ROOT
	null_func,//BIND_FUNC(sys_game_get_system_sw_version)   //376 (0x178)  ROOT
	null_func,//BIND_FUNC(sys_sm_set_shop_mode)             //377 (0x179)  ROOT
	null_func,//BIND_FUNC(sys_sm_get_ext_event2)            //378 (0x17A)  ROOT
	null_func,//BIND_FUNC(sys_sm_shutdown)                  //379 (0x17B)  ROOT
	null_func,//BIND_FUNC(sys_sm_get_params)                //380 (0x17C)  DBG
	null_func,//BIND_FUNC(sys_sm_get_inter_lpar_parameter)  //381 (0x17D)  ROOT
	null_func,//BIND_FUNC(sys_sm_initialize)                //382 (0x17E)  ROOT
	null_func,//BIND_FUNC(sys_game_get_temperature)         //383 (0x17F)  ROOT
	null_func,//BIND_FUNC(sys_sm_get_tzpb)                  //384 (0x180)  ROOT
	null_func,//BIND_FUNC(sys_sm_request_led)               //385 (0x181)  ROOT
	null_func,//BIND_FUNC(sys_sm_control_led)               //386 (0x182)  ROOT
	null_func,//BIND_FUNC(sys_sm_get_system_info)           //387 (0x183)  DBG
	null_func,//BIND_FUNC(sys_sm_ring_buzzer)               //388 (0x184)  ROOT
	null_func,//BIND_FUNC(sys_sm_set_fan_policy)            //389 (0x185)  PM
	null_func,//BIND_FUNC(sys_sm_request_error_log)         //390 (0x186)  ROOT
	null_func,//BIND_FUNC(sys_sm_request_be_count)          //391 (0x187)  ROOT
	null_func,//BIND_FUNC(sys_sm_ring_buzzer)               //392 (0x188)  ROOT
	null_func,//BIND_FUNC(sys_sm_get_hw_config)             //393 (0x189)  ROOT
	null_func,//BIND_FUNC(sys_sm_request_scversion)         //394 (0x18A)  ROOT
	null_func,//BIND_FUNC(sys_sm_request_system_event_log)  //395 (0x18B)  PM
	null_func,//BIND_FUNC(sys_sm_set_rtc_alarm)             //396 (0x18C)  ROOT
	null_func,//BIND_FUNC(sys_sm_get_rtc_alarm)             //397 (0x18D)  ROOT
	null_func,//BIND_FUNC(sys_console_write)                //398 (0x18E)  ROOT
	uns_func,                                               //399 (0x18F)  UNS
	null_func,//BIND_FUNC(sys_sm_...)                       //400 (0x190)  PM
	null_func,//BIND_FUNC(sys_sm_...)                       //401 (0x191)  ROOT
	BIND_FUNC(sys_tty_read),                                //402 (0x192)
	BIND_FUNC(sys_tty_write),                               //403 (0x193)
	null_func,//BIND_FUNC(sys_...)                          //404 (0x194)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //405 (0x195)  PM
	null_func,//BIND_FUNC(sys_...)                          //406 (0x196)  PM
	null_func,//BIND_FUNC(sys_...)                          //407 (0x197)  PM
	null_func,//BIND_FUNC(sys_sm_get_tzpb)                  //408 (0x198)  PM
	null_func,//BIND_FUNC(sys_sm_get_fan_policy)            //409 (0x199)  PM
	null_func,//BIND_FUNC(sys_game_board_storage_read)      //410 (0x19A)
	null_func,//BIND_FUNC(sys_game_board_storage_write)     //411 (0x19B)
	null_func,//BIND_FUNC(sys_game_get_rtc_status)          //412 (0x19C)
	null_func,//BIND_FUNC(sys_...)                          //413 (0x19D)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //414 (0x19E)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //415 (0x19F)  ROOT

	uns_func, uns_func, uns_func, uns_func,                 //416-419  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //420-429  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //430-439  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //440-449  UNS

	BIND_FUNC(sys_overlay_load_module),                     //450 (0x1C2)
	BIND_FUNC(sys_overlay_unload_module),                   //451 (0x1C3)
	null_func,//BIND_FUNC(sys_overlay_get_module_list)      //452 (0x1C4)
	null_func,//BIND_FUNC(sys_overlay_get_module_info)      //453 (0x1C5)
	BIND_FUNC(sys_overlay_load_module_by_fd),               //454 (0x1C6)
	null_func,//BIND_FUNC(sys_overlay_get_module_info2)     //455 (0x1C7)
	null_func,//BIND_FUNC(sys_overlay_get_sdk_version)      //456 (0x1C8)
	null_func,//BIND_FUNC(sys_overlay_get_module_dbg_info)  //457 (0x1C9)
	null_func,//BIND_FUNC(sys_overlay_get_module_dbg_info)  //458 (0x1CA)
	uns_func,                                               //459 (0x1CB)  UNS
	null_func,//BIND_FUNC(sys_prx_dbg_get_module_id_list)   //460 (0x1CC)  ROOT
	BIND_FUNC(_sys_prx_get_module_id_by_address),           //461 (0x1CD)
	uns_func,                                               //462 (0x1CE)  DEX
	BIND_FUNC(_sys_prx_load_module_by_fd),                  //463 (0x1CF)
	BIND_FUNC(_sys_prx_load_module_on_memcontainer_by_fd),  //464 (0x1D0)
	BIND_FUNC(_sys_prx_load_module_list),                   //465 (0x1D1)
	BIND_FUNC(_sys_prx_load_module_list_on_memcontainer),   //466 (0x1D2)
	BIND_FUNC(sys_prx_get_ppu_guid),                        //467 (0x1D3)
	null_func,//BIND_FUNC(sys_...)                          //468 (0x1D4) ROOT
	uns_func,                                               //469 (0x1D5)  UNS
	null_func,//BIND_FUNC(sys_npdrm_check_ekc)              //470 (0x1D6)  ROOT
	null_func,//BIND_FUNC(sys_npdrm_regist_ekc)             //471 (0x1D7)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //472 (0x1D8)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //473 (0x1D9)
	null_func,//BIND_FUNC(sys_...)                          //474 (0x1DA)
	null_func,//BIND_FUNC(sys_...)                          //475 (0x1DB)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //476 (0x1DC)  ROOT

	uns_func, uns_func, uns_func,                           //477-479  UNS

	BIND_FUNC(_sys_prx_load_module),                        //480 (0x1E0)
	BIND_FUNC(_sys_prx_start_module),                       //481 (0x1E1)
	BIND_FUNC(_sys_prx_stop_module),                        //482 (0x1E2)
	BIND_FUNC(_sys_prx_unload_module),                      //483 (0x1E3)
	BIND_FUNC(_sys_prx_register_module),                    //484 (0x1E4)
	BIND_FUNC(_sys_prx_query_module),                       //485 (0x1E5)
	BIND_FUNC(_sys_prx_register_library),                   //486 (0x1E6)
	BIND_FUNC(_sys_prx_unregister_library),                 //487 (0x1E7)
	BIND_FUNC(_sys_prx_link_library),                       //488 (0x1E8)
	BIND_FUNC(_sys_prx_unlink_library),                     //489 (0x1E9)
	BIND_FUNC(_sys_prx_query_library),                      //490 (0x1EA)
	uns_func,                                               //491 (0x1EB)  UNS
	null_func,//BIND_FUNC(sys_prx_dbg_get_module_list)      //492 (0x1EC)  DBG
	null_func,//BIND_FUNC(sys_prx_dbg_get_module_info)      //493 (0x1ED)  DBG
	BIND_FUNC(_sys_prx_get_module_list),                    //494 (0x1EE)
	BIND_FUNC(_sys_prx_get_module_info),                    //495 (0x1EF)
	BIND_FUNC(_sys_prx_get_module_id_by_name),              //496 (0x1F0)
	BIND_FUNC(_sys_prx_load_module_on_memcontainer),        //497 (0x1F1)
	BIND_FUNC(_sys_prx_start),                              //498 (0x1F2)
	BIND_FUNC(_sys_prx_stop),                               //499 (0x1F3)
	null_func,//BIND_FUNC(sys_hid_manager_open)             //500 (0x1F4)
	null_func,//BIND_FUNC(sys_hid_manager_close)            //501 (0x1F5)
	null_func,//BIND_FUNC(sys_hid_manager_read)             //502 (0x1F6)  ROOT
	null_func,//BIND_FUNC(sys_hid_manager_ioctl)            //503 (0x1F7)
	null_func,//BIND_FUNC(sys_hid_manager_map_logical_id_to_port_id) //504 (0x1F8)  ROOT
	null_func,//BIND_FUNC(sys_hid_manager_unmap_logical_id_to_port_id) //505 (0x1F9)  ROOT
	null_func,//BIND_FUNC(sys_hid_manager_add_hot_key_observer) //506 (0x1FA)  ROOT
	null_func,//BIND_FUNC(sys_hid_manager_remove_hot_key_observer) //507 (0x1FB)  ROOT
	null_func,//BIND_FUNC(sys_hid_manager_grab_focus)       //508 (0x1FC)  ROOT
	null_func,//BIND_FUNC(sys_hid_manager_release_focus)    //509 (0x1FD)  ROOT
	null_func,//BIND_FUNC(sys_hid_manager_check_focus)      //510 (0x1FE)
	null_func,//BIND_FUNC(sys_hid_manager_set_master_process) //511 (0x1FF)  ROOT
	null_func,//BIND_FUNC(sys_hid_manager_...)              //512 (0x200)  ROOT
	null_func,//BIND_FUNC(sys_hid_manager_...)              //513 (0x201)
	null_func,//BIND_FUNC(sys_hid_manager_...)              //514 (0x202)
	uns_func,                                               //515 (0x203)  UNS
	BIND_FUNC(sys_config_open),                             //516 (0x204)
	BIND_FUNC(sys_config_close),                            //517 (0x205)
	BIND_FUNC(sys_config_get_service_event),                //518 (0x206)
	BIND_FUNC(sys_config_add_service_listener),             //519 (0x207)
	BIND_FUNC(sys_config_remove_service_listener),          //520 (0x208)
	BIND_FUNC(sys_config_register_service),                 //521 (0x209)
	BIND_FUNC(sys_config_unregister_service),               //522 (0x20A)
	BIND_FUNC(sys_config_get_io_event),                     //523 (0x20B)
	BIND_FUNC(sys_config_register_io_error_listener),       //524 (0x20C)
	BIND_FUNC(sys_config_unregister_io_error_listener),     //525 (0x20D)
	uns_func, uns_func, uns_func, uns_func,                 //526-529  UNS
	BIND_FUNC(sys_usbd_initialize),                         //530 (0x212)
	BIND_FUNC(sys_usbd_finalize),                           //531 (0x213)
	BIND_FUNC(sys_usbd_get_device_list),                    //532 (0x214)
	BIND_FUNC(sys_usbd_get_descriptor_size),                //533 (0x215)
	BIND_FUNC(sys_usbd_get_descriptor),                     //534 (0x216)
	BIND_FUNC(sys_usbd_register_ldd),                       //535 (0x217)
	BIND_FUNC(sys_usbd_unregister_ldd),                     //536 (0x218)
	BIND_FUNC(sys_usbd_open_pipe),                          //537 (0x219)
	BIND_FUNC(sys_usbd_open_default_pipe),                  //538 (0x21A)
	BIND_FUNC(sys_usbd_close_pipe),                         //539 (0x21B)
	BIND_FUNC(sys_usbd_receive_event),                      //540 (0x21C)
	BIND_FUNC(sys_usbd_detect_event),                       //541 (0x21D)
	BIND_FUNC(sys_usbd_attach),                             //542 (0x21E)
	BIND_FUNC(sys_usbd_transfer_data),                      //543 (0x21F)
	BIND_FUNC(sys_usbd_isochronous_transfer_data),          //544 (0x220)
	BIND_FUNC(sys_usbd_get_transfer_status),                //545 (0x221)
	BIND_FUNC(sys_usbd_get_isochronous_transfer_status),    //546 (0x222)
	BIND_FUNC(sys_usbd_get_device_location),                //547 (0x223)
	BIND_FUNC(sys_usbd_send_event),                         //548 (0x224)
	BIND_FUNC(sys_usbd_event_port_send),                    //549 (0x225)
	BIND_FUNC(sys_usbd_allocate_memory),                    //550 (0x226)
	BIND_FUNC(sys_usbd_free_memory),                        //551 (0x227)
	null_func,//BIND_FUNC(sys_ubsd_...)                     //552 (0x228)
	null_func,//BIND_FUNC(sys_ubsd_...)                     //553 (0x229)
	null_func,//BIND_FUNC(sys_ubsd_...)                     //554 (0x22A)
	null_func,//BIND_FUNC(sys_ubsd_...)                     //555 (0x22B)
	BIND_FUNC(sys_usbd_get_device_speed),                   //556 (0x22C)
	null_func,//BIND_FUNC(sys_ubsd_...)                     //557 (0x22D)
	null_func,//BIND_FUNC(sys_ubsd_...)                     //558 (0x22E)
	BIND_FUNC(sys_usbd_register_extra_ldd),                 //559 (0x22F)
	null_func,//BIND_FUNC(sys_...)                          //560 (0x230)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //561 (0x231)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //562 (0x232)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //563 (0x233)
	null_func,//BIND_FUNC(sys_...)                          //564 (0x234)
	null_func,//BIND_FUNC(sys_...)                          //565 (0x235)
	null_func,//BIND_FUNC(sys_...)                          //566 (0x236)
	null_func,//BIND_FUNC(sys_...)                          //567 (0x237)
	null_func,//BIND_FUNC(sys_...)                          //568 (0x238)
	null_func,//BIND_FUNC(sys_...)                          //569 (0x239)
	null_func,//BIND_FUNC(sys_pad_ldd_register_controller)  //570 (0x23A)
	null_func,//BIND_FUNC(sys_pad_ldd_unregister_controller) //571 (0x23B)
	null_func,//BIND_FUNC(sys_pad_ldd_data_insert)          //572 (0x23C)
	null_func,//BIND_FUNC(sys_pad_dbg_ldd_set_data_insert_mode) //573 (0x23D)
	null_func,//BIND_FUNC(sys_pad_ldd_register_controller)  //574 (0x23E)
	null_func,//BIND_FUNC(sys_pad_ldd_get_port_no)          //575 (0x23F)
	uns_func,                                               //576 (0x240)  UNS
	null_func,//BIND_FUNC(sys_pad_manager_...)              //577 (0x241)  ROOT  PM
	null_func,//BIND_FUNC(sys_bluetooth_...)                //578 (0x242)
	null_func,//BIND_FUNC(sys_bluetooth_aud_serial_...)    //579 (0x243)
	null_func,//BIND_FUNC(sys_bluetooth_...)                //580 (0x244)  ROOT
	null_func,//BIND_FUNC(sys_bluetooth_...)                //581 (0x245)  ROOT
	null_func,//BIND_FUNC(sys_bluetooth_...)                //582 (0x246)  ROOT
	null_func,//BIND_FUNC(sys_bt_read_firmware_version)     //583 (0x247)  ROOT
	null_func,//BIND_FUNC(sys_bt_complete_wake_on_host)     //584 (0x248)  ROOT
	null_func,//BIND_FUNC(sys_bt_disable_bluetooth)         //585 (0x249)
	null_func,//BIND_FUNC(sys_bt_enable_bluetooth)          //586 (0x24A)
	null_func,//BIND_FUNC(sys_bt_bccmd)                     //587 (0x24B)  ROOT
	null_func,//BIND_FUNC(sys_bt_read_hq)                   //588 (0x24C)
	null_func,//BIND_FUNC(sys_bt_hid_get_remote_status)     //589 (0x24D)
	null_func,//BIND_FUNC(sys_bt_register_controller)       //590 (0x24E)  ROOT
	null_func,//BIND_FUNC(sys_bt_clear_registered_contoller) //591 (0x24F)
	null_func,//BIND_FUNC(sys_bt_connect_accept_controller) //592 (0x250)
	null_func,//BIND_FUNC(sys_bt_get_local_bdaddress)       //593 (0x251)  ROOT
	null_func,//BIND_FUNC(sys_bt_hid_get_data)              //594 (0x252)
	null_func,//BIND_FUNC(sys_bt_hid_set_report)            //595 (0x253)
	null_func,//BIND_FUNC(sys_bt_sched_log)                 //596 (0x254)
	null_func,//BIND_FUNC(sys_bt_cancel_connect_accept_controller) //597 (0x255)
	null_func,//BIND_FUNC(sys_bluetooth_...)                //598 (0x256)  ROOT
	null_func,//BIND_FUNC(sys_bluetooth_...)                //599 (0x257)  ROOT
	null_func,//BIND_FUNC(sys_storage_open)                 //600 (0x258)  ROOT
	null_func,//BIND_FUNC(sys_storage_close)                //601 (0x259)
	null_func,//BIND_FUNC(sys_storage_read)                 //602 (0x25A)
	null_func,//BIND_FUNC(sys_storage_write)                //603 (0x25B)
	null_func,//BIND_FUNC(sys_storage_send_device_command)  //604 (0x25C)
	null_func,//BIND_FUNC(sys_storage_async_configure)      //605 (0x25D)
	null_func,//BIND_FUNC(sys_storage_async_read)           //606 (0x25E)
	null_func,//BIND_FUNC(sys_storage_async_write)          //607 (0x25F)
	null_func,//BIND_FUNC(sys_storage_async_cancel)         //608 (0x260)
	null_func,//BIND_FUNC(sys_storage_get_device_info)      //609 (0x261)  ROOT
	null_func,//BIND_FUNC(sys_storage_get_device_config)    //610 (0x262)  ROOT
	null_func,//BIND_FUNC(sys_storage_report_devices)       //611 (0x263)  ROOT
	null_func,//BIND_FUNC(sys_storage_configure_medium_event) //612 (0x264)  ROOT
	null_func,//BIND_FUNC(sys_storage_set_medium_polling_interval) //613 (0x265)
	null_func,//BIND_FUNC(sys_storage_create_region)        //614 (0x266)
	null_func,//BIND_FUNC(sys_storage_delete_region)        //615 (0x267)
	null_func,//BIND_FUNC(sys_storage_execute_device_command) //616 (0x268)
	null_func,//BIND_FUNC(sys_storage_check_region_acl)     //617 (0x269)
	null_func,//BIND_FUNC(sys_storage_set_region_acl)       //618 (0x26A)
	null_func,//BIND_FUNC(sys_storage_async_send_device_command) //619 (0x26B)
	null_func,//BIND_FUNC(sys_...)                          //620 (0x26C)  ROOT
	BIND_FUNC(sys_gamepad_ycon_if),                         //621 (0x26D)
	null_func,//BIND_FUNC(sys_storage_get_region_offset)    //622 (0x26E)
	null_func,//BIND_FUNC(sys_storage_set_emulated_speed)   //623 (0x26F)
	null_func,//BIND_FUNC(sys_io_buffer_create)             //624 (0x270)
	null_func,//BIND_FUNC(sys_io_buffer_destroy)            //625 (0x271)
	null_func,//BIND_FUNC(sys_io_buffer_allocate)           //626 (0x272)
	null_func,//BIND_FUNC(sys_io_buffer_free)               //627 (0x273)
	uns_func, uns_func,                                     //629 (0x275)  UNS
	BIND_FUNC(sys_gpio_set),                                //630 (0x276)
	BIND_FUNC(sys_gpio_get),                                //631 (0x277)
	uns_func,                                               //632 (0x278)  UNS
	null_func,//BIND_FUNC(sys_fsw_connect_event)            //633 (0x279)
	null_func,//BIND_FUNC(sys_fsw_disconnect_event)         //634 (0x27A)
	null_func,//BIND_FUNC(sys_btsetting_if)                 //635 (0x27B)
	null_func,//BIND_FUNC(sys_...)                          //636 (0x27C)
	null_func,//BIND_FUNC(sys_...)                          //637 (0x27D)
	null_func,//BIND_FUNC(sys_...)                          //638 (0x27E)

	null_func,//BIND_FUNC(sys...)                           //639  DEPRECATED
	null_func,//BIND_FUNC(sys_usbbtaudio_initialize)        //640  DEPRECATED
	null_func,//BIND_FUNC(sys_usbbtaudio_finalize)          //641  DEPRECATED
	null_func,//BIND_FUNC(sys_usbbtaudio_discovery)         //642  DEPRECATED
	null_func,//BIND_FUNC(sys_usbbtaudio_cancel_discovery)  //643  DEPRECATED
	null_func,//BIND_FUNC(sys_usbbtaudio_pairing)           //644  DEPRECATED
	null_func,//BIND_FUNC(sys_usbbtaudio_set_passkey)       //645  DEPRECATED
	null_func,//BIND_FUNC(sys_usbbtaudio_connect)           //646  DEPRECATED
	null_func,//BIND_FUNC(sys_usbbtaudio_disconnect)        //647  DEPRECATED
	null_func,//BIND_FUNC(sys_...)                          //648  DEPRECATED
	null_func,//BIND_FUNC(sys_...)                          //649  DEPRECATED

	null_func,//BIND_FUNC(sys_rsxaudio_initialize)          //650 (0x28A)
	null_func,//BIND_FUNC(sys_rsxaudio_finalize)            //651 (0x28B)
	null_func,//BIND_FUNC(sys_rsxaudio_import_shared_memory) //652 (0x28C)
	null_func,//BIND_FUNC(sys_rsxaudio_unimport_shared_memory) //653 (0x28D)
	null_func,//BIND_FUNC(sys_rsxaudio_create_connection)   //654 (0x28E)
	null_func,//BIND_FUNC(sys_rsxaudio_close_connection)    //655 (0x28F)
	null_func,//BIND_FUNC(sys_rsxaudio_prepare_process)     //656 (0x290)
	null_func,//BIND_FUNC(sys_rsxaudio_start_process)       //657 (0x291)
	null_func,//BIND_FUNC(sys_rsxaudio_stop_process)        //658 (0x292)
	null_func,//BIND_FUNC(sys_rsxaudio_)                    //659 (0x293)

	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //660-665  UNS

	BIND_FUNC(sys_rsx_device_open),                         //666 (0x29A)
	BIND_FUNC(sys_rsx_device_close),                        //667 (0x29B)
	BIND_FUNC(sys_rsx_memory_allocate),                     //668 (0x29C)
	BIND_FUNC(sys_rsx_memory_free),                         //669 (0x29D)
	BIND_FUNC(sys_rsx_context_allocate),                    //670 (0x29E)
	BIND_FUNC(sys_rsx_context_free),                        //671 (0x29F)
	BIND_FUNC(sys_rsx_context_iomap),                       //672 (0x2A0)
	BIND_FUNC(sys_rsx_context_iounmap),                     //673 (0x2A1)
	BIND_FUNC(sys_rsx_context_attribute),                   //674 (0x2A2)
	BIND_FUNC(sys_rsx_device_map),                          //675 (0x2A3)
	BIND_FUNC(sys_rsx_device_unmap),                        //676 (0x2A4)
	BIND_FUNC(sys_rsx_attribute),                           //677 (0x2A5)
	null_func,//BIND_FUNC(sys_...)                          //678 (0x2A6)
	null_func,//BIND_FUNC(sys_...)                          //679 (0x2A7)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //680 (0x2A8)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //681 (0x2A9)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //682 (0x2AA)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //683 (0x2AB)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //684 (0x2AC)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //685 (0x2AD)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //686 (0x2AE)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //687 (0x2AF)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //688 (0x2B0)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //689 (0x2B1)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //690 (0x2B2)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //691 (0x2B3)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //692 (0x2B4)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //693 (0x2B5)  ROOT
	null_func,//BIND_FUNC(sys_...)                          //694 (0x2B6)  DEPRECATED
	null_func,//BIND_FUNC(sys_...)                          //695 (0x2B7)  DEPRECATED
	null_func,//BIND_FUNC(sys_...)                          //696 (0x2B8)  ROOT
	uns_func,//BIND_FUNC(sys_...)                           //697 (0x2B9)  UNS
	uns_func,//BIND_FUNC(sys_...)                           //698 (0x2BA)  UNS
	null_func,//BIND_FUNC(sys_bdemu_send_command)           //699 (0x2BB)
	BIND_FUNC(sys_net_bnet_accept),                         //700 (0x2BC)
	BIND_FUNC(sys_net_bnet_bind),                           //701 (0x2BD)
	BIND_FUNC(sys_net_bnet_connect),                        //702 (0x2BE)
	BIND_FUNC(sys_net_bnet_getpeername),                    //703 (0x2BF)
	BIND_FUNC(sys_net_bnet_getsockname),                    //704 (0x2C0)
	BIND_FUNC(sys_net_bnet_getsockopt),                     //705 (0x2C1)
	BIND_FUNC(sys_net_bnet_listen),                         //706 (0x2C2)
	BIND_FUNC(sys_net_bnet_recvfrom),                       //707 (0x2C3)
	BIND_FUNC(sys_net_bnet_recvmsg),                        //708 (0x2C4)
	BIND_FUNC(sys_net_bnet_sendmsg),                        //709 (0x2C5)
	BIND_FUNC(sys_net_bnet_sendto),                         //710 (0x2C6)
	BIND_FUNC(sys_net_bnet_setsockopt),                     //711 (0x2C7)
	BIND_FUNC(sys_net_bnet_shutdown),                       //712 (0x2C8)
	BIND_FUNC(sys_net_bnet_socket),                         //713 (0x2C9)
	BIND_FUNC(sys_net_bnet_close),                          //714 (0x2CA)
	BIND_FUNC(sys_net_bnet_poll),                           //715 (0x2CB)
	BIND_FUNC(sys_net_bnet_select),                         //716 (0x2CC)
	BIND_FUNC(_sys_net_open_dump),                          //717 (0x2CD)
	BIND_FUNC(_sys_net_read_dump),                          //718 (0x2CE)
	BIND_FUNC(_sys_net_close_dump),                         //719 (0x2CF)
	BIND_FUNC(_sys_net_write_dump),                         //720 (0x2D0)
	BIND_FUNC(sys_net_abort),                               //721 (0x2D1)
	BIND_FUNC(sys_net_infoctl),                             //722 (0x2D2)
	BIND_FUNC(sys_net_control),                             //723 (0x2D3)
	BIND_FUNC(sys_net_bnet_ioctl),                          //724 (0x2D4)
	BIND_FUNC(sys_net_bnet_sysctl),                         //725 (0x2D5)
	BIND_FUNC(sys_net_eurus_post_command),                  //726 (0x2D6)

	uns_func, uns_func, uns_func,                           //727-729  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //730-739  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //740-749  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //750-759  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //760-769  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //770-779  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //780-789  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //790-799  UNS

	BIND_FUNC(sys_fs_test),                                 //800 (0x320)
	BIND_FUNC(sys_fs_open),                                 //801 (0x321)
	BIND_FUNC(sys_fs_read),                                 //802 (0x322)
	BIND_FUNC(sys_fs_write),                                //803 (0x323)
	BIND_FUNC(sys_fs_close),                                //804 (0x324)
	BIND_FUNC(sys_fs_opendir),                              //805 (0x325)
	BIND_FUNC(sys_fs_readdir),                              //806 (0x326)
	BIND_FUNC(sys_fs_closedir),                             //807 (0x327)
	BIND_FUNC(sys_fs_stat),                                 //808 (0x328)
	BIND_FUNC(sys_fs_fstat),                                //809 (0x329)
	BIND_FUNC(sys_fs_link),                                 //810 (0x32A)
	BIND_FUNC(sys_fs_mkdir),                                //811 (0x32B)
	BIND_FUNC(sys_fs_rename),                               //812 (0x32C)
	BIND_FUNC(sys_fs_rmdir),                                //813 (0x32D)
	BIND_FUNC(sys_fs_unlink),                               //814 (0x32E)
	BIND_FUNC(sys_fs_utime),                                //815 (0x32F)
	BIND_FUNC(sys_fs_access),                               //816 (0x330)
	BIND_FUNC(sys_fs_fcntl),                                //817 (0x331)
	BIND_FUNC(sys_fs_lseek),                                //818 (0x332)
	BIND_FUNC(sys_fs_fdatasync),                            //819 (0x333)
	BIND_FUNC(sys_fs_fsync),                                //820 (0x334)
	BIND_FUNC(sys_fs_fget_block_size),                      //821 (0x335)
	BIND_FUNC(sys_fs_get_block_size),                       //822 (0x336)
	BIND_FUNC(sys_fs_acl_read),                             //823 (0x337)
	BIND_FUNC(sys_fs_acl_write),                            //824 (0x338)
	BIND_FUNC(sys_fs_lsn_get_cda_size),                     //825 (0x339)
	BIND_FUNC(sys_fs_lsn_get_cda),                          //826 (0x33A)
	BIND_FUNC(sys_fs_lsn_lock),                             //827 (0x33B)
	BIND_FUNC(sys_fs_lsn_unlock),                           //828 (0x33C)
	BIND_FUNC(sys_fs_lsn_read),                             //829 (0x33D)
	BIND_FUNC(sys_fs_lsn_write),                            //830 (0x33E)
	BIND_FUNC(sys_fs_truncate),                             //831 (0x33F)
	BIND_FUNC(sys_fs_ftruncate),                            //832 (0x340)
	BIND_FUNC(sys_fs_symbolic_link),                        //833 (0x341)
	BIND_FUNC(sys_fs_chmod),                                //834 (0x342)
	BIND_FUNC(sys_fs_chown),                                //835 (0x343)
	null_func,//BIND_FUNC(sys_fs_newfs),                    //836 (0x344)
	null_func,//BIND_FUNC(sys_fs_mount),                    //837 (0x345)
	null_func,//BIND_FUNC(sys_fs_unmount),                  //838 (0x346)
	null_func,//BIND_FUNC(sys_fs_sync),                     //839 (0x347)
	BIND_FUNC(sys_fs_disk_free),                            //840 (0x348)
	null_func,//BIND_FUNC(sys_fs_get_mount_info_size),      //841 (0x349)
	null_func,//BIND_FUNC(sys_fs_get_mount_info),           //842 (0x34A)
	null_func,//BIND_FUNC(sys_fs_get_fs_info_size),         //843 (0x34B)
	null_func,//BIND_FUNC(sys_fs_get_fs_info),              //844 (0x34C)
	BIND_FUNC(sys_fs_mapped_allocate),                      //845 (0x34D)
	BIND_FUNC(sys_fs_mapped_free),                          //846 (0x34E)
	BIND_FUNC(sys_fs_truncate2),                            //847 (0x34F)

	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //848-853  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //854-859  UNS

	null_func,//BIND_FUNC(syscall_sys_ss_get_cache_of_analog_sunset_flag), //860 (0x35C)  AUTHID
	null_func,//BIND_FUNC(sys_ss_protected_file_db)         //861  ROOT
	null_func,//BIND_FUNC(sys_ss_virtual_trm_manager)       //862  ROOT
	null_func,//BIND_FUNC(sys_ss_update_manager)            //863  ROOT
	null_func,//BIND_FUNC(sys_ss_sec_hw_framework)          //864  DBG
	BIND_FUNC(sys_ss_random_number_generator),              //865 (0x361)
	null_func,//BIND_FUNC(sys_ss_secure_rtc)                //866  ROOT
	null_func,//BIND_FUNC(sys_ss_appliance_info_manager)    //867  ROOT
	null_func,//BIND_FUNC(sys_ss_individual_info_manager)   //868  ROOT / DBG  AUTHID
	null_func,//BIND_FUNC(sys_ss_factory_data_manager)      //869  ROOT
	BIND_FUNC(sys_ss_get_console_id),                       //870 (0x366)
	BIND_FUNC(sys_ss_access_control_engine),                //871 (0x367)  DBG
	BIND_FUNC(sys_ss_get_open_psid),                        //872 (0x368)
	null_func,//BIND_FUNC(sys_ss_get_cache_of_product_mode), //873 (0x369)
	null_func,//BIND_FUNC(sys_ss_get_cache_of_flash_ext_flag), //874 (0x36A)
	null_func,//BIND_FUNC(sys_ss_get_boot_device)           //875 (0x36B)
	null_func,//BIND_FUNC(sys_ss_disc_access_control)       //876 (0x36C)
	null_func,//BIND_FUNC(sys_ss_~utoken_if)                //877 (0x36D)  ROOT
	null_func,//BIND_FUNC(sys_ss_ad_sign)                   //878 (0x36E)
	null_func,//BIND_FUNC(sys_ss_media_id)                  //879 (0x36F)
	null_func,//BIND_FUNC(sys_deci3_open)                   //880 (0x370)
	null_func,//BIND_FUNC(sys_deci3_create_event_path)      //881 (0x371)
	null_func,//BIND_FUNC(sys_deci3_close)                  //882 (0x372)
	null_func,//BIND_FUNC(sys_deci3_send)                   //883 (0x373)
	null_func,//BIND_FUNC(sys_deci3_receive)                //884 (0x374)
	null_func,//BIND_FUNC(sys_deci3_open2)                  //885 (0x375)
	uns_func, uns_func, uns_func,                           //886-888  UNS
	null_func,//BIND_FUNC(sys_...)                          //889 (0x379)  ROOT
	null_func,//BIND_FUNC(sys_deci3_initialize)             //890 (0x37A)
	null_func,//BIND_FUNC(sys_deci3_terminate)              //891 (0x37B)
	null_func,//BIND_FUNC(sys_deci3_debug_mode)             //892 (0x37C)
	null_func,//BIND_FUNC(sys_deci3_show_status)            //893 (0x37D)
	null_func,//BIND_FUNC(sys_deci3_echo_test)              //894 (0x37E)
	null_func,//BIND_FUNC(sys_deci3_send_dcmp_packet)       //895 (0x37F)
	null_func,//BIND_FUNC(sys_deci3_dump_cp_register)	    //896 (0x380)
	null_func,//BIND_FUNC(sys_deci3_dump_cp_buffer)         //897 (0x381)
	uns_func,                                               //898 (0x382)  UNS
	null_func,//BIND_FUNC(sys_deci3_test)                   //899 (0x383)
	null_func,//BIND_FUNC(sys_dbg_stop_processes)           //900 (0x384)
	null_func,//BIND_FUNC(sys_dbg_continue_processes)       //901 (0x385)
	null_func,//BIND_FUNC(sys_dbg_stop_threads)             //902 (0x386)
	null_func,//BIND_FUNC(sys_dbg_continue_threads)         //903 (0x387)
	BIND_FUNC(sys_dbg_read_process_memory),					//904 (0x388)
	BIND_FUNC(sys_dbg_write_process_memory),				//905 (0x389)
	null_func,//BIND_FUNC(sys_dbg_read_thread_register)     //906 (0x38A)
	null_func,//BIND_FUNC(sys_dbg_write_thread_register)    //907 (0x38B)
	null_func,//BIND_FUNC(sys_dbg_get_process_list)         //908 (0x38C)
	null_func,//BIND_FUNC(sys_dbg_get_thread_list)          //909 (0x38D)
	null_func,//BIND_FUNC(sys_dbg_get_thread_info)          //910 (0x38E)
	null_func,//BIND_FUNC(sys_dbg_spu_thread_read_from_ls)  //911 (0x38F)
	null_func,//BIND_FUNC(sys_dbg_spu_thread_write_to_ls)   //912 (0x390)
	null_func,//BIND_FUNC(sys_dbg_kill_process)             //913 (0x391)
	null_func,//BIND_FUNC(sys_dbg_get_process_info)         //914 (0x392)
	null_func,//BIND_FUNC(sys_dbg_set_run_control_bit_to_spu) //915 (0x393)
	null_func,//BIND_FUNC(sys_dbg_spu_thread_get_exception_cause) //916 (0x394)
	null_func,//BIND_FUNC(sys_dbg_create_kernel_event_queue) //917 (0x395)
	null_func,//BIND_FUNC(sys_dbg_read_kernel_event_queue)   //918 (0x396)
	null_func,//BIND_FUNC(sys_dbg_destroy_kernel_event_queue) //919 (0x397)
	null_func,//BIND_FUNC(sys_dbg_get_process_event_ctrl_flag) //920 (0x398)
	null_func,//BIND_FUNC(sys_dbg_set_process_event_cntl_flag) //921 (0x399)
	null_func,//BIND_FUNC(sys_dbg_get_spu_thread_group_event_cntl_flag) //922 (0x39A)
	null_func,//BIND_FUNC(sys_dbg_set_spu_thread_group_event_cntl_flag) //923 (0x39B)
	null_func,//BIND_FUNC(sys_dbg_get_module_list)          //924 (0x39C)
	null_func,//BIND_FUNC(sys_dbg_get_raw_spu_list)         //925 (0x39D)
	null_func,//BIND_FUNC(sys_dbg_initialize_scratch_executable_area) //926 (0x39E)
	null_func,//BIND_FUNC(sys_dbg_terminate_scratch_executable_area) //927 (0x3A0)
	null_func,//BIND_FUNC(sys_dbg_initialize_scratch_data_area) //928 (0x3A1)
	null_func,//BIND_FUNC(sys_dbg_terminate_scratch_data_area) //929 (0x3A2)
	null_func,//BIND_FUNC(sys_dbg_get_user_memory_stat)     //930 (0x3A3)
	null_func,//BIND_FUNC(sys_dbg_get_shared_memory_attribute_list) //931 (0x3A4)
	null_func,//BIND_FUNC(sys_dbg_get_mutex_list)           //932 (0x3A4)
	null_func,//BIND_FUNC(sys_dbg_get_mutex_information)    //933 (0x3A5)
	null_func,//BIND_FUNC(sys_dbg_get_cond_list)            //934 (0x3A6)
	null_func,//BIND_FUNC(sys_dbg_get_cond_information)     //935 (0x3A7)
	null_func,//BIND_FUNC(sys_dbg_get_rwlock_list)          //936 (0x3A8)
	null_func,//BIND_FUNC(sys_dbg_get_rwlock_information)   //937 (0x3A9)
	null_func,//BIND_FUNC(sys_dbg_get_lwmutex_list)         //938 (0x3AA)
	null_func,//BIND_FUNC(sys_dbg_get_address_from_dabr)    //939 (0x3AB)
	null_func,//BIND_FUNC(sys_dbg_set_address_to_dabr)      //940 (0x3AC)
	null_func,//BIND_FUNC(sys_dbg_get_lwmutex_information)  //941 (0x3AD)
	null_func,//BIND_FUNC(sys_dbg_get_event_queue_list)     //942 (0x3AE)
	null_func,//BIND_FUNC(sys_dbg_get_event_queue_information) //943 (0x3AF)
	null_func,//BIND_FUNC(sys_dbg_initialize_ppu_exception_handler) //944 (0x3B0)
	null_func,//BIND_FUNC(sys_dbg_finalize_ppu_exception_handler) //945 (0x3B1)  DBG
	null_func,//BIND_FUNC(sys_dbg_get_semaphore_list)       //946 (0x3B2)
	null_func,//BIND_FUNC(sys_dbg_get_semaphore_information) //947 (0x3B3)
	null_func,//BIND_FUNC(sys_dbg_get_kernel_thread_list)   //948 (0x3B4)
	null_func,//BIND_FUNC(sys_dbg_get_kernel_thread_info)   //949 (0x3B5)
	null_func,//BIND_FUNC(sys_dbg_get_lwcond_list)          //950 (0x3B6)
	null_func,//BIND_FUNC(sys_dbg_get_lwcond_information)   //951 (0x3B7)
	null_func,//BIND_FUNC(sys_dbg_create_scratch_data_area_ext) //952 (0x3B8)
	null_func,//BIND_FUNC(sys_dbg_vm_get_page_information)  //953 (0x3B9)
	null_func,//BIND_FUNC(sys_dbg_vm_get_info)              //954 (0x3BA)
	null_func,//BIND_FUNC(sys_dbg_enable_floating_point_enabled_exception) //955 (0x3BB)
	null_func,//BIND_FUNC(sys_dbg_disable_floating_point_enabled_exception) //956 (0x3BC)
	null_func,//BIND_FUNC(sys_dbg_get_process_memory_container_information) //957 (0x3BD)
	uns_func,                                               //958 (0x3BE)  UNS
	null_func,//BIND_FUNC(sys_dbg_...)                      //959 (0x3BF)
	null_func,//BIND_FUNC(sys_control_performance_monitor)  //960 (0x3C0)
	null_func,//BIND_FUNC(sys_performance_monitor_hidden)   //961 (0x3C1)
	null_func,//BIND_FUNC(sys_performance_monitor_bookmark) //962 (0x3C2)
	null_func,//BIND_FUNC(sys_lv1_pc_trace_create)          //963 (0x3C3)
	null_func,//BIND_FUNC(sys_lv1_pc_trace_start)           //964 (0x3C4)
	null_func,//BIND_FUNC(sys_lv1_pc_trace_stop)            //965 (0x3C5)
	null_func,//BIND_FUNC(sys_lv1_pc_trace_get_status)      //966 (0x3C6)
	null_func,//BIND_FUNC(sys_lv1_pc_trace_destroy)         //967 (0x3C7)
	null_func,//BIND_FUNC(sys_rsx_trace_ioctl)              //968 (0x3C8)
	null_func,//BIND_FUNC(sys_dbg_...)                      //969 (0x3C9)
	null_func,//BIND_FUNC(sys_dbg_get_event_flag_list)      //970 (0x3CA)
	null_func,//BIND_FUNC(sys_dbg_get_event_flag_information) //971 (0x3CB)
	null_func,//BIND_FUNC(sys_dbg_...)                      //972 (0x3CC)
	uns_func,//BIND_FUNC(sys_dbg_...)                       //973 (0x3CD)
	null_func,//BIND_FUNC(sys_dbg_...)                      //974 (0x3CE)
	null_func,//BIND_FUNC(sys_dbg_read_spu_thread_context2) //975 (0x3CF)
	null_func,//BIND_FUNC(sys_crypto_engine_create)         //976 (0x3D0)
	null_func,//BIND_FUNC(sys_crypto_engine_destroy)        //977 (0x3D1)
	null_func,//BIND_FUNC(sys_crypto_engine_hasher_prepare) //978 (0x3D2)  ROOT
	null_func,//BIND_FUNC(sys_crypto_engine_hasher_run)     //979 (0x3D3)
	null_func,//BIND_FUNC(sys_crypto_engine_hasher_get_hash) //980 (0x3D4)
	null_func,//BIND_FUNC(sys_crypto_engine_cipher_prepare) //981 (0x3D5)  ROOT
	null_func,//BIND_FUNC(sys_crypto_engine_cipher_run)     //982 (0x3D6)
	null_func,//BIND_FUNC(sys_crypto_engine_cipher_get_hash) //983 (0x3D7)
	null_func,//BIND_FUNC(sys_crypto_engine_random_generate) //984 (0x3D8)
	null_func,//BIND_FUNC(sys_dbg_get_console_type)         //985 (0x3D9)  ROOT
	null_func,//BIND_FUNC(sys_dbg_...)                      //986 (0x3DA)  ROOT  DBG
	null_func,//BIND_FUNC(sys_dbg_...)                      //987 (0x3DB)  ROOT
	null_func,//BIND_FUNC(sys_dbg_..._ppu_exception_handler) //988 (0x3DC)
	null_func,//BIND_FUNC(sys_dbg_...)                      //989 (0x3DD)

	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //990-998  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //999-1007  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //1008-1016  UNS
	uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, uns_func, //1020-1023  UNS
};

template<>
void fmt_class_string<CellError>::format(std::string& out, u64 arg)
{
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

extern void ppu_initialize_syscalls()
{
	g_ppu_syscall_table = s_ppu_syscall_table;
}

extern void ppu_execute_syscall(ppu_thread& ppu, u64 code)
{
	if (code < g_ppu_syscall_table.size())
	{
		if (auto func = g_ppu_syscall_table[code])
		{
			func(ppu);
			LOG_TRACE(PPU, "Syscall '%s' (%llu) finished, r3=0x%llx", ppu_syscall_code(code), code, ppu.gpr[3]);
			return;
		}
	}

	fmt::throw_exception("Invalid syscall number (%llu)", code);
}

extern ppu_function_t ppu_get_syscall(u64 code)
{
	if (code < g_ppu_syscall_table.size())
	{
		return g_ppu_syscall_table[code];
	}

	return nullptr;
}

extern u64 get_guest_system_time();

DECLARE(lv2_obj::g_mutex);
DECLARE(lv2_obj::g_ppu);
DECLARE(lv2_obj::g_pending);
DECLARE(lv2_obj::g_waiting);

thread_local DECLARE(lv2_obj::g_to_awake);

void lv2_obj::sleep_unlocked(cpu_thread& thread, u64 timeout)
{
	const u64 start_time = get_guest_system_time();

	if (auto ppu = static_cast<ppu_thread*>(thread.id_type() == 1 ? &thread : nullptr))
	{
		LOG_TRACE(PPU, "sleep() - waiting (%zu)", g_pending.size());

		const auto [_, ok] = ppu->state.fetch_op([&](bs_t<cpu_flag>& val)
		{
			if (!(val & cpu_flag::signal))
			{
				val += cpu_flag::suspend;
				return true;
			}

			return false;
		});

		if (!ok)
		{
			LOG_FATAL(PPU, "sleep() failed (signaled) (%s)", ppu->current_function);
			return;
		}

		// Find and remove the thread
		unqueue(g_ppu, ppu);
		unqueue(g_pending, ppu);

		ppu->start_time = start_time;
	}

	if (timeout)
	{
		const u64 wait_until = start_time + timeout;

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

	if (!g_to_awake.empty())
	{
		// Schedule pending entries
		awake_unlocked({});
	}
	else
	{
		schedule_all();
	}
}

void lv2_obj::awake_unlocked(cpu_thread* cpu, s32 prio)
{
	// Check thread type
	AUDIT(!cpu || cpu->id_type() == 1);

	switch (prio)
	{
	default:
	{
		// Priority set
		if (static_cast<ppu_thread*>(cpu)->prio.exchange(prio) == prio || !unqueue(g_ppu, cpu))
		{
			return;
		}

		break;
	}
	case yield_cmd:
	{
		// Yield command
		const u64 start_time = get_guest_system_time();

		for (std::size_t i = 0, pos = -1; i < g_ppu.size(); i++)
		{
			if (g_ppu[i] == cpu)
			{
				pos = i;
				prio = g_ppu[i]->prio;
			}
			else if (i == pos + 1 && prio != -4 && g_ppu[i]->prio != prio)
			{
				return;
			}
		}

		unqueue(g_ppu, cpu);
		unqueue(g_pending, cpu);

		static_cast<ppu_thread*>(cpu)->start_time = start_time;
	}
	case enqueue_cmd:
	{
		break;
	}
	}

	const auto emplace_thread = [](cpu_thread* const cpu)
	{
		for (auto it = g_ppu.cbegin(), end = g_ppu.cend();; it++)
		{
			if (it != end && *it == cpu)
			{
				LOG_TRACE(PPU, "sleep() - suspended (p=%zu)", g_pending.size());
				return;
			}

			// Use priority, also preserve FIFO order
			if (it == end || (*it)->prio > static_cast<ppu_thread*>(cpu)->prio)
			{
				g_ppu.insert(it, static_cast<ppu_thread*>(cpu));
				break;
			}
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

		LOG_TRACE(PPU, "awake(): %s", cpu->id);
	};

	if (cpu)
	{
		// Emplace current thread
		emplace_thread(cpu);
	}
	else for (const auto _cpu : g_to_awake)
	{
		// Emplace threads from list
		emplace_thread(_cpu);
	}

	// Remove pending if necessary
	if (!g_pending.empty() && cpu && cpu == get_current_cpu_thread())
	{
		unqueue(g_pending, cpu);
	}

	// Suspend threads if necessary
	for (std::size_t i = g_cfg.core.ppu_threads; i < g_ppu.size(); i++)
	{
		const auto target = g_ppu[i];

		if (!target->state.test_and_set(cpu_flag::suspend))
		{
			LOG_TRACE(PPU, "suspend(): %s", target->id);
			g_pending.emplace_back(target);
		}
	}

	schedule_all();
}

void lv2_obj::cleanup()
{
	g_ppu.clear();
	g_pending.clear();
	g_waiting.clear();
}

void lv2_obj::schedule_all()
{
	if (g_pending.empty())
	{
		// Wake up threads
		for (std::size_t i = 0, x = std::min<std::size_t>(g_cfg.core.ppu_threads, g_ppu.size()); i < x; i++)
		{
			const auto target = g_ppu[i];

			if (target->state & cpu_flag::suspend)
			{
				LOG_TRACE(PPU, "schedule(): %s", target->id);
				target->state ^= (cpu_flag::signal + cpu_flag::suspend);
				target->start_time = 0;

				if (target != get_current_cpu_thread())
				{
					target->notify();
				}
			}
		}
	}

	// Check registered timeouts
	while (!g_waiting.empty())
	{
		auto& pair = g_waiting.front();

		if (pair.first <= get_guest_system_time())
		{
			pair.second->notify();
			g_waiting.pop_front();
		}
		else
		{
			// The list is sorted so assume no more timeouts
			break;
		}
	}
}
