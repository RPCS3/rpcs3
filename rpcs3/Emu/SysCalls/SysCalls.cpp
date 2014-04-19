#include "stdafx.h"
#include "SysCalls.h"
#include "Modules.h"
#include "SC_FUNC.h"

namespace detail{
	template<> bool CheckId(u32 id, ID*& _id,const std::string &name)
	{
		return Emu.GetIdManager().CheckID(id) && (_id = &Emu.GetIdManager().GetID(id))->m_name == name;
	}
}

void default_syscall();
static func_caller *null_func = bind_func(default_syscall);

static const int kSyscallTableLength = 1024;

static func_caller* sc_table[kSyscallTableLength] =
{
	null_func,
	bind_func(sys_process_getpid),                          //1   (0x001)
	null_func,//bind_func(sys_process_wait_for_child),      //2   (0x002)  ROOT
	bind_func(sys_process_exit),                            //3   (0x003)
	null_func,//bind_func(sys_process_get_status),          //4   (0x004)  DBG
	null_func,//bind_func(sys_process_detach_child),        //5   (0x005)  DBG

	// Unused: 6-11
	null_func, null_func, null_func, null_func, null_func, null_func,

	bind_func(sys_process_get_number_of_object),            //12  (0x00B)
	bind_func(sys_process_get_id),                          //13  (0x00C)
	null_func,//bind_func(sys_process_is_spu_lock_line_reservation_address),  //14  (0x00D)

	// Unused: 15-17
	null_func, null_func, null_func,
	
	bind_func(sys_process_getppid),                         //18  (0x012)
	null_func,//bind_func(sys_process_kill),                //19  (0x013)
	null_func,                                              //
	null_func,//bind_func(_sys_process_spawn),              //21  (0x015)  DBG
	bind_func(sys_process_exit),                            //22  (0x016)
	null_func,//bind_func(sys_process_wait_for_child2),     //23  (0x017)  DBG
	null_func,//bind_func(),                                //24  (0x018)  DBG
	null_func,//bind_func(sys_process_get_sdk_version),     //25  (0x019)
	null_func,//bind_func(_sys_process_exit),               //26  (0x01A)
	null_func,//bind_func(),                                //27  (0x01B)  DBG
	null_func,//bind_func(_sys_process_get_number_of_object)//28  (0x01C)  ROOT
	null_func,//bind_func(sys_process_get_id),              //29  (0x01D)  ROOT
	bind_func(sys_process_get_paramsfo),                    //30  (0x01E)
	null_func,//bind_func(sys_process_get_ppu_guid),        //31  (0x01F)
	
	// Unused: 32-40
	null_func, null_func, null_func, null_func, null_func, null_func, null_func, null_func, null_func,

	bind_func(sys_ppu_thread_exit),                         //41  (0x029)
	null_func,                                              //
	bind_func(sys_ppu_thread_yield),                        //43  (0x02B)
	bind_func(sys_ppu_thread_join),                         //44  (0x02C)
	bind_func(sys_ppu_thread_detach),                       //45  (0x02D)
	bind_func(sys_ppu_thread_get_join_state),               //46  (0x02E)
	bind_func(sys_ppu_thread_set_priority),                 //47  (0x02F)  DBG
	bind_func(sys_ppu_thread_get_priority),                 //48  (0x030)
	bind_func(sys_ppu_thread_get_stack_information),        //49  (0x031)
	bind_func(sys_ppu_thread_stop),                         //50  (0x032)  ROOT
	bind_func(sys_ppu_thread_restart),                      //51  (0x033)  ROOT
	bind_func(sys_ppu_thread_create),                       //52  (0x034)  DBG
	null_func,//bind_func(sys_ppu_thread_start),            //53  (0x035)
	null_func,//bind_func(),                                //54  (0x036)  ROOT
	null_func,//bind_func(),                                //55  (0x037)  ROOT
	null_func,//bind_func(sys_ppu_thread_rename),           //56  (0x038)
	null_func,//bind_func(sys_ppu_thread_recover_page_fault)//57  (0x039)
	null_func,//bind_func(sys_ppu_thread_get_page_fault_context),//58 (0x03A)
	
	// Unused: 59
	null_func,

	bind_func(sys_trace_create),                            //60  (0x03C)
	bind_func(sys_trace_start),                             //61  (0x03D)
	bind_func(sys_trace_stop),                              //62  (0x03E)
	bind_func(sys_trace_update_top_index),                  //63  (0x03F)
	bind_func(sys_trace_destroy),                           //64  (0x040)
	bind_func(sys_trace_drain),                             //65  (0x041)
	bind_func(sys_trace_attach_process),                    //66  (0x042)
	bind_func(sys_trace_allocate_buffer),                   //67  (0x043)
	bind_func(sys_trace_free_buffer),                       //68  (0x044)
	bind_func(sys_trace_create2),                           //69  (0x045)
	bind_func(sys_timer_create),                            //70  (0x046)
	bind_func(sys_timer_destroy),                           //71  (0x047)
	bind_func(sys_timer_get_information),                   //72  (0x048)
	bind_func(sys_timer_start),                             //73  (0x049)
	bind_func(sys_timer_stop),                              //74  (0x04A)
	bind_func(sys_timer_connect_event_queue),               //75  (0x04B)
	bind_func(sys_timer_disconnect_event_queue),            //76  (0x04C)
	null_func,//bind_func(sys_trace_create2_in_cbepm),      //77  (0x04D)
	null_func,//bind_func()                                 //78  (0x04E)

	// Unused: 79
	null_func,

	null_func,//bind_func(sys_interrupt_tag_create)         //80  (0x050)
	null_func,//bind_func(sys_interrupt_tag_destroy)        //81  (0x051)
	bind_func(sys_event_flag_create),                       //82  (0x052)
	bind_func(sys_event_flag_destroy),                      //83  (0x053)
	null_func,//bind_func(sys_interrupt_thread_establish)   //84  (0x054)
	bind_func(sys_event_flag_wait),                         //85  (0x055)
	bind_func(sys_event_flag_trywait),                      //86  (0x056)
	bind_func(sys_event_flag_set),                          //87  (0x057)
	null_func,//bind_func(sys_interrupt_thread_eoi)         //88  (0x058)
	null_func,//bind_func(sys_interrupt_thread_disestablish)//89  (0x059)
	bind_func(sys_semaphore_create),                        //90  (0x05A)
	bind_func(sys_semaphore_destroy),                       //91  (0x05B)
	bind_func(sys_semaphore_wait),                          //92  (0x05C)
	bind_func(sys_semaphore_trywait),                       //93  (0x05D)
	bind_func(sys_semaphore_post),                          //94  (0x05E)
	bind_func(sys_lwmutex_create),                          //95  (0x05F)
	bind_func(sys_lwmutex_destroy),                         //96  (0x060)
	bind_func(sys_lwmutex_lock),                            //97  (0x061)
	bind_func(sys_lwmutex_trylock),                         //98  (0x062)
	bind_func(sys_lwmutex_unlock),                          //99  (0x063)
	bind_func(sys_mutex_create),                            //100 (0x064)
	bind_func(sys_mutex_destroy),                           //101 (0x065)
	bind_func(sys_mutex_lock),                              //102 (0x066)
	bind_func(sys_mutex_trylock),                           //103 (0x067)
	bind_func(sys_mutex_unlock),                            //104 (0x068)
	bind_func(sys_cond_create),                             //105 (0x069)
	bind_func(sys_cond_destroy),                            //106 (0x06A)
	bind_func(sys_cond_wait),                               //107 (0x06B)
	bind_func(sys_cond_signal),                             //108 (0x06C)
	bind_func(sys_cond_signal_all),                         //109 (0x06D)
	bind_func(sys_cond_signal_to),                          //110 (0x06E)
	null_func,//bind_func(sys_lwcond_create)                //111 (0x06F)
	null_func,//bind_func(sys_lwcond_destroy)               //112 (0x070)
	null_func,//bind_func(sys_lwcond_queue_wait)            //113 (0x071)
	bind_func(sys_semaphore_get_value),                     //114 (0x072)
	null_func,//bind_func()                                 //115 (0x073)
	null_func,//bind_func()                                 //116 (0x074)
	null_func,//bind_func()                                 //117 (0x075)
	bind_func(sys_event_flag_clear),                        //118 (0x076)
	null_func,//bind_func()                                 //119 (0x077)  ROOT
	bind_func(sys_rwlock_create),                           //120 (0x078)
	bind_func(sys_rwlock_destroy),                          //121 (0x079)
	bind_func(sys_rwlock_rlock),                            //122 (0x07A)
	bind_func(sys_rwlock_tryrlock),                         //123 (0x07B)
	bind_func(sys_rwlock_runlock),                          //124 (0x07C)
	bind_func(sys_rwlock_wlock),                            //125 (0x07D)
	bind_func(sys_rwlock_trywlock),                         //126 (0x07E)
	bind_func(sys_rwlock_wunlock),                          //127 (0x07F)
	bind_func(sys_event_queue_create),                      //128 (0x080)
	bind_func(sys_event_queue_destroy),                     //129 (0x081)
	bind_func(sys_event_queue_receive),                     //130 (0x082)
	bind_func(sys_event_queue_tryreceive),                  //131 (0x083)
	bind_func(sys_event_flag_cancel),                       //132 (0x084)
	bind_func(sys_event_queue_drain),                       //133 (0x085)
	bind_func(sys_event_port_create),                       //134 (0x086)
	bind_func(sys_event_port_destroy),                      //135 (0x087)
	bind_func(sys_event_port_connect_local),                //136 (0x088)
	bind_func(sys_event_port_disconnect),                   //137 (0x089)
	bind_func(sys_event_port_send),                         //138 (0x08A)
	bind_func(sys_event_flag_get),                          //139 (0x08B)
	null_func,//bind_func(sys_event_port_connect_ipc)       //140 (0x08C)
	bind_func(sys_timer_usleep),                            //141 (0x08D)
	bind_func(sys_timer_sleep),                             //142 (0x08E)
	null_func,//bind_func(sys_time_set_timezone)            //143 (0x08F)  ROOT
	bind_func(sys_time_get_timezone),                       //144 (0x090)
	bind_func(sys_time_get_current_time),                   //145 (0x091)
	bind_func(sys_time_get_system_time),                    //146 (0x092)  ROOT
	bind_func(sys_time_get_timebase_frequency),             //147 (0x093)
	null_func,//bind_func(sys_rwlock_trywlock)              //148 (0x094)
	
	// Unused: 149
	null_func,

	null_func,//bind_func(sys_raw_spu_create_interrupt_tag) //150 (0x096)
	null_func,//bind_func(sys_raw_spu_set_int_mask)         //151 (0x097)
	null_func,//bind_func(sys_raw_spu_get_int_mask)         //152 (0x098)
	null_func,//bind_func(sys_raw_spu_set_int_stat)         //153 (0x099)
	null_func,//bind_func(sys_raw_spu_get_int_stat)         //154 (0x09A)
	null_func,//bind_func(sys_spu_image_get_information?)   //155 (0x09B)
	bind_func(sys_spu_image_open),                          //156 (0x09C)
	null_func,//bind_func(sys_spu_image_import)             //157 (0x09D)
	null_func,//bind_func(sys_spu_image_close)              //158 (0x09E)
	null_func,//bind_func(sys_raw_spu_load)                 //159 (0x09F)
	bind_func(sys_raw_spu_create),                          //160 (0x0A0)
	null_func,//bind_func(sys_raw_spu_destroy)              //161 (0x0A1)
	null_func,                                              //
	null_func,//bind_func(sys_raw_spu_read_puint_mb)        //163 (0x0A3)
	null_func,                                              //
	bind_func(sys_spu_thread_get_exit_status),              //165 (0x0A5)
	bind_func(sys_spu_thread_set_argument),                 //166 (0x0A6)
	null_func,//bind_func(sys_spu_thread_group_start_on_exit)//167(0x0A7)
	null_func,                                              //
	bind_func(sys_spu_initialize),                          //169 (0x0A9)
	bind_func(sys_spu_thread_group_create),                 //170 (0x0AA)
	bind_func(sys_spu_thread_group_destroy),                //171 (0x0AB)
	bind_func(sys_spu_thread_initialize),                   //172 (0x0AC)
	bind_func(sys_spu_thread_group_start),                  //173 (0x0AD)
	bind_func(sys_spu_thread_group_suspend),                //174 (0x0AE)
	bind_func(sys_spu_thread_group_resume),                 //175 (0x0AF)
	null_func,//bind_func(sys_spu_thread_group_yield)       //176 (0x0B0)
	null_func,//bind_func(sys_spu_thread_group_terminate)   //177 (0x0B1)
	bind_func(sys_spu_thread_group_join),                   //178 (0x0B2)
	null_func,//bind_func(sys_spu_thread_group_set_priority)//179 (0x0B3)
	null_func,//bind_func(sys_spu_thread_group_get_priority)//180 (0x0B4)
	bind_func(sys_spu_thread_write_ls),                     //181 (0x0B5)
	bind_func(sys_spu_thread_read_ls),                      //182 (0x0B6)
	null_func,                                              //
	bind_func(sys_spu_thread_write_snr),                    //184 (0x0B8)
	bind_func(sys_spu_thread_group_connect_event),          //185 (0x0B9)
	bind_func(sys_spu_thread_group_disconnect_event),       //186 (0x0BA)
	bind_func(sys_spu_thread_set_spu_cfg),                  //187 (0x0BB)
	bind_func(sys_spu_thread_get_spu_cfg),                  //188 (0x0BC)
	null_func,                                              //
	bind_func(sys_spu_thread_write_spu_mb),                 //190 (0x0BE)
	bind_func(sys_spu_thread_connect_event),                //191 (0x0BF)
	bind_func(sys_spu_thread_disconnect_event),             //192 (0x0C0)
	bind_func(sys_spu_thread_bind_queue),                   //193 (0x0C1)
	bind_func(sys_spu_thread_unbind_queue),                 //194 (0x0C2)
	null_func,                                              //
	null_func,//bind_func(sys_raw_spu_set_spu_cfg)          //196 (0x0C4)
	null_func,//bind_func(sys_raw_spu_get_spu_cfg)          //197 (0x0C5)
	null_func,//bind_func(sys_spu_thread_recover_page_fault)//198 (0x0C6)
	null_func,//bind_func(sys_raw_spu_recover_page_fault)   //199 (0x0C7)

	null_func, null_func, null_func, null_func, null_func, //204(0x104)
	null_func, null_func, null_func, null_func, null_func, //209
	null_func, null_func, null_func, null_func, null_func, //214
	null_func, null_func, null_func, null_func, null_func, //219
	null_func, null_func, null_func, null_func, null_func, //224
	null_func, null_func, null_func, null_func, null_func, //229
	null_func, null_func, null_func, null_func, null_func, //234
	null_func, null_func, null_func, null_func, null_func, //239
	null_func, null_func, null_func, null_func, null_func, //244
	null_func, null_func, null_func, null_func, null_func, //249
	null_func,                                             //250

	bind_func(sys_spu_thread_group_connect_event_all_threads),//251 (0x0FB)
	bind_func(sys_spu_thread_group_disconnect_event_all_threads),//252 (0x0FC)
	null_func,//bind_func()                                 //253 (0x0FD)
	null_func,//bind_func(sys_spu_thread_group_log)         //254 (0x0FE)

	// Unused: 255-259
	null_func, null_func, null_func, null_func, null_func,

	null_func,//bind_func(sys_spu_image_open_by_fd)         //260 (0x104)
	
	null_func, null_func, null_func, null_func,            //264
	null_func, null_func, null_func, null_func, null_func, //269
	null_func, null_func, null_func, null_func, null_func, //274
	null_func, null_func, null_func, null_func, null_func, //279
	null_func, null_func, null_func, null_func, null_func, //284
	null_func, null_func, null_func, null_func, null_func, //289
	null_func, null_func, null_func, null_func, null_func, //294
	null_func, null_func, null_func, null_func, null_func, //299

	bind_func(sys_vm_memory_map),                           //300 (0x12C)
	bind_func(sys_vm_unmap),                                //301 (0x12D)
	bind_func(sys_vm_append_memory),                        //302 (0x12E)
	bind_func(sys_vm_return_memory),                        //303 (0x12F)
	bind_func(sys_vm_lock),                                 //304 (0x130)
	bind_func(sys_vm_unlock),                               //305 (0x131)
	bind_func(sys_vm_touch),                                //306 (0x132)
	bind_func(sys_vm_flush),                                //307 (0x133)
	bind_func(sys_vm_invalidate),                           //308 (0x134)
	bind_func(sys_vm_store),                                //309 (0x135)
	bind_func(sys_vm_sync),                                 //310 (0x136)
	bind_func(sys_vm_test),                                 //311 (0x137)
	bind_func(sys_vm_get_statistics),                       //312 (0x138)
	null_func,//bind_func()                                 //313 (0x139)
	null_func,//bind_func()                                 //314 (0x13A)
	null_func,//bind_func()                                 //315 (0x13B)
	
	// Unused: 316-323
	null_func, null_func, null_func, null_func, null_func, null_func, null_func, null_func,

	bind_func(sys_memory_container_create),                 //324 (0x144)
	bind_func(sys_memory_container_destroy),                //325 (0x145)
	bind_func(sys_mmapper_allocate_fixed_address),          //326 (0x146)
	bind_func(sys_mmapper_enable_page_fault_notification),  //327 (0x147)
	null_func,//bind_func()                                 //328 (0x148)
	null_func,//bind_func(sys_mmapper_free_shared_memory)   //329 (0x149)
	bind_func(sys_mmapper_allocate_address),                //330 (0x14A)
	bind_func(sys_mmapper_free_address),                    //331 (0x14B)
	null_func,//bind_func(sys_mmapper_allocate_shared_memory)//332(0x14C)
	null_func,//bind_func(sys_mmapper_set_shared_memory_flag)//333(0x14D)
	null_func,//bind_func(sys_mmapper_map_shared_memory)    //334 (0x14E)
	null_func,//bind_func(sys_mmapper_unmap_shared_memory)  //335 (0x14F)
	bind_func(sys_mmapper_change_address_access_right),     //336 (0x150)
	bind_func(sys_mmapper_search_and_map),                  //337 (0x151)
	null_func,//bind_func(sys_mmapper_get_shared_memory_attribute) //338 (0x152)
	null_func,//bind_func()                                 //339 (0x153)
	null_func,//bind_func()                                 //340 (0x154)
	bind_func(sys_memory_container_create),                 //341 (0x155)
	bind_func(sys_memory_container_destroy),                //342 (0x156)
	bind_func(sys_memory_container_get_size),               //343 (0x157)
	null_func,//bind_func(sys_memory_budget_set)            //344 (0x158)
	null_func,//bind_func()                                 //345 (0x159)
	null_func,//bind_func()                                 //346 (0x15A)
	null_func,                                              //
	bind_func(sys_memory_allocate),                         //348 (0x15C)
	bind_func(sys_memory_free),                             //349 (0x15D)
	bind_func(sys_memory_allocate_from_container),          //350 (0x15E)
	bind_func(sys_memory_get_page_attribute),               //351 (0x15F)
	bind_func(sys_memory_get_user_memory_size),             //352 (0x160)
	null_func,//bind_func(sys_memory_get_user_memory_stat)  //353 (0x161)
	null_func,//bind_func()                                 //354 (0x162)
	null_func,//bind_func()                                 //355 (0x163)
	null_func,//bind_func(sys_memory_allocate_colored)      //356 (0x164)
	null_func,//bind_func()                                 //357 (0x165)
	null_func,//bind_func()                                 //358 (0x166)
	null_func,//bind_func()                                 //359 (0x167)
	null_func,//bind_func()                                 //360 (0x168)
	null_func,//bind_func(sys_memory_allocate_from_container_colored) //361 (0x169)
	null_func,//bind_func(sys_mmapper_allocate_memory_from_container) //362 (0x16A)
	null_func,//bind_func()                                 //363 (0x16B)
	null_func,//bind_func()                                 //364 (0x16C)


	null_func, null_func, null_func, null_func, null_func, //369
	null_func, null_func, null_func, null_func, null_func, //374
	null_func, null_func, null_func, null_func, null_func, //379
	null_func, null_func, null_func, null_func, null_func, //384
	null_func, null_func, null_func, null_func, null_func, //389
	null_func, null_func, null_func, null_func, null_func, //394
	null_func, null_func, null_func, null_func, null_func, //399
	null_func, null_func, bind_func(sys_tty_read), bind_func(sys_tty_write), null_func, //404
	null_func, null_func, null_func, null_func, null_func, //409
	null_func, null_func, null_func, null_func, null_func, //414
	null_func, null_func, null_func, null_func, null_func, //419
	null_func, null_func, null_func, null_func, null_func, //424
	null_func, null_func, null_func, null_func, null_func, //429
	null_func, null_func, null_func, null_func, null_func, //434
	null_func, null_func, null_func, null_func, null_func, //439
	null_func, null_func, null_func, null_func, null_func, //444
	null_func, null_func, null_func, null_func, null_func, //449
	null_func, null_func, null_func, null_func, null_func, //454
	null_func, null_func, null_func, null_func, null_func, //459
	null_func, null_func, null_func, null_func, null_func, //464
	null_func, null_func, null_func, null_func, null_func, //469
	null_func, null_func, null_func, null_func, null_func, //474
	null_func, null_func, null_func, null_func, null_func, //479
	null_func, null_func, null_func, null_func, null_func, //484
	null_func, null_func, null_func, null_func, null_func, //489
	null_func, null_func, null_func, null_func, null_func, //494
	null_func, null_func, null_func, null_func, null_func, //499
	null_func, null_func, null_func, null_func, null_func, //504
	null_func, null_func, null_func, null_func, null_func, //509
	null_func, null_func, null_func, null_func, null_func, //514
	null_func, null_func, null_func, null_func, null_func, //519
	null_func, null_func, null_func, null_func, null_func, //524
	null_func, null_func, null_func, null_func, null_func, //529
	null_func, null_func, null_func, null_func, null_func, //534
	null_func, null_func, null_func, null_func, null_func, //539
	null_func, null_func, null_func, null_func, null_func, //544
	null_func, null_func, null_func, null_func, null_func, //549
	null_func, null_func, null_func, null_func, null_func, //554
	null_func, null_func, null_func, null_func, null_func, //559
	null_func, null_func, null_func, null_func, null_func, //564
	null_func, null_func, null_func, null_func, null_func, //569
	null_func, null_func, null_func, null_func, null_func, //574
	null_func, null_func, null_func, null_func, null_func, //579
	null_func, null_func, null_func, null_func, null_func, //584
	null_func, null_func, null_func, null_func, null_func, //589
	null_func, null_func, null_func, null_func, null_func, //594
	null_func, null_func, null_func, null_func, null_func, //599
	null_func, null_func, null_func, null_func, null_func, //604
	null_func, null_func, null_func, null_func, null_func, //609
	null_func, null_func, null_func, null_func, null_func, //614
	null_func, null_func, null_func, null_func, null_func, //619
	null_func, null_func, null_func, null_func, null_func, //624
	null_func, null_func, null_func, null_func, null_func, //629
	null_func, null_func, null_func, null_func, null_func, //634
	null_func, null_func, null_func, null_func, null_func, //639
	null_func, null_func, null_func, null_func, null_func, //644
	null_func, null_func, null_func, null_func, null_func, //649
	null_func, null_func, null_func, null_func, null_func, //654
	null_func, null_func, null_func, null_func, null_func, //659
	null_func, null_func, null_func, null_func, null_func, //664
	null_func,                                             //665

	bind_func(sys_rsx_device_open),                         //666 (0x29A)
	bind_func(sys_rsx_device_close),                        //667 (0x29B)
	bind_func(sys_rsx_memory_allocate),                     //668 (0x29C)
	bind_func(sys_rsx_memory_free),                         //669 (0x29D)
	bind_func(sys_rsx_context_allocate),                    //670 (0x29E)
	bind_func(sys_rsx_context_free),                        //671 (0x29F)
	bind_func(sys_rsx_context_iomap),                       //672 (0x2A0)
	bind_func(sys_rsx_context_iounmap),                     //673 (0x2A1)
	bind_func(sys_rsx_context_attribute),                   //674 (0x2A2)
	bind_func(sys_rsx_device_map),                          //675 (0x2A3)
	bind_func(sys_rsx_device_unmap),                        //676 (0x2A4)
	bind_func(sys_rsx_attribute),                           //677 (0x2A5)
	null_func,                                              //678 (0x2A6)
	null_func,                                              //679 (0x2A7)  ROOT

	null_func, null_func, null_func, null_func, null_func, //684
	null_func, null_func, null_func, null_func, null_func, //689
	null_func, null_func, null_func, null_func, null_func, //694
	null_func, null_func, null_func, null_func, null_func, //699
	null_func, null_func, null_func, null_func, null_func, //704
	null_func, null_func, null_func, null_func, null_func, //709
	null_func, null_func, null_func, null_func, null_func, //714
	null_func, null_func, null_func, null_func, null_func, //719
	null_func, null_func, null_func, null_func, null_func, //724
	null_func, null_func, null_func, null_func, null_func, //729
	null_func, null_func, null_func, null_func, null_func, //734
	null_func, null_func, null_func, null_func, null_func, //739
	null_func, null_func, null_func, null_func, null_func, //744
	null_func, null_func, null_func, null_func, null_func, //749
	null_func, null_func, null_func, null_func, null_func, //754
	null_func, null_func, null_func, null_func, null_func, //759
	null_func, null_func, null_func, null_func, null_func, //764
	null_func, null_func, null_func, null_func, null_func, //769
	null_func, null_func, null_func, null_func, null_func, //774
	null_func, null_func, null_func, null_func, null_func, //779
	null_func, null_func, null_func, null_func, null_func, //784
	null_func, null_func, null_func, null_func, null_func, //789
	null_func, null_func, null_func, null_func, null_func, //794
	null_func, null_func, null_func, null_func, null_func, //799

	null_func,//bind_func(sys_fs_test),                     //800 (0x320)
	bind_func(cellFsOpen),                                  //801 (0x321)
	bind_func(cellFsRead),                                  //802 (0x322)
	bind_func(cellFsWrite),                                 //803 (0x323)
	bind_func(cellFsClose),                                 //804 (0x324)
	bind_func(cellFsOpendir),                               //805 (0x325)
	bind_func(cellFsReaddir),                               //806 (0x326)
	bind_func(cellFsClosedir),                              //807 (0x327)
	bind_func(cellFsStat),                                  //808 (0x328)
	bind_func(cellFsFstat),                                 //809 (0x329)
	null_func,//bind_func(sys_fs_link),                     //810 (0x32A)
	bind_func(cellFsMkdir),                                 //811 (0x32B)
	bind_func(cellFsRename),                                //812 (0x32C)
	bind_func(cellFsRmdir),                                 //813 (0x32D)
	bind_func(cellFsUnlink),                                //814 (0x32E)
	null_func,//bind_func(cellFsUtime),                     //815 (0x32F)
	null_func,//bind_func(sys_fs_access),                   //816 (0x330)
	null_func,//bind_func(sys_fs_fcntl),                    //817 (0x331)
	bind_func(cellFsLseek),                                 //818 (0x332)
	null_func,//bind_func(sys_fs_fdatasync),                //819 (0x333)
	null_func,//bind_func(cellFsFsync),                     //820 (0x334)
	bind_func(cellFsFGetBlockSize),                         //821 (0x335)
	bind_func(cellFsGetBlockSize),                          //822 (0x336)
	null_func,//bind_func(sys_fs_acl_read),                 //823 (0x337)
	null_func,//bind_func(sys_fs_acl_write),                //824 (0x338)
	null_func,//bind_func(sys_fs_lsn_get_cda_size),         //825 (0x339)
	null_func,//bind_func(sys_fs_lsn_get_cda),              //826 (0x33A)
	null_func,//bind_func(sys_fs_lsn_lock),                 //827 (0x33B)
	null_func,//bind_func(sys_fs_lsn_unlock),               //828 (0x33C)
	null_func,//bind_func(sys_fs_lsn_read),                 //829 (0x33D)
	null_func,//bind_func(sys_fs_lsn_write),                //830 (0x33E)
	bind_func(cellFsTruncate),                              //831 (0x33F)
	bind_func(cellFsFtruncate),                             //832 (0x340)
	null_func,//bind_func(sys_fs_symbolic_link),            //833 (0x341)
	null_func,//bind_func(cellFsChmod),                     //834 (0x342)
	null_func,//bind_func(sys_fs_chown),                    //835 (0x343)
	null_func,//bind_func(sys_fs_newfs),                    //836 (0x344)
	null_func,//bind_func(sys_fs_mount),                    //837 (0x345)
	null_func,//bind_func(sys_fs_unmount),                  //838 (0x346)
	null_func,//bind_func(sys_fs_sync),                     //839 (0x347)
	null_func,//bind_func(sys_fs_disk_free),                //840 (0x348)
	null_func,//bind_func(sys_fs_get_mount_info_size),      //841 (0x349)
	null_func,//bind_func(sys_fs_get_mount_info),           //842 (0x34A)
	null_func,//bind_func(sys_fs_get_fs_info_size),         //843 (0x34B)
	null_func,//bind_func(sys_fs_get_fs_info),              //844 (0x34C)
	null_func,//bind_func(sys_fs_mapped_allocate),          //845 (0x34D)
	null_func,//bind_func(sys_fs_mapped_free),              //846 (0x34E)
	null_func,//bind_func(sys_fs_truncate2),                //847 (0x34F)
	
	null_func, null_func, //849
	null_func, null_func, null_func, null_func, null_func, //854
	null_func, null_func, null_func, null_func, null_func, //859
	null_func, null_func, null_func, null_func, null_func, //864
	null_func, null_func, null_func, null_func, null_func, //869
	null_func, null_func, null_func, null_func, null_func, //874
	null_func, null_func, null_func, null_func, null_func, //879
	null_func, null_func, null_func, null_func, null_func, //884
	null_func, null_func, null_func, null_func, null_func, //889
	null_func, null_func, null_func, null_func, null_func, //894
	null_func, null_func, null_func, null_func, null_func, //899
	null_func, null_func, null_func, null_func, null_func, //904
	null_func, null_func, null_func, null_func, null_func, //909
	null_func, null_func, null_func, null_func, null_func, //914
	null_func, null_func, null_func, null_func, null_func, //919
	null_func, null_func, null_func, null_func, null_func, //924
	null_func, null_func, null_func, null_func, null_func, //929
	null_func, null_func, null_func, null_func, null_func, //934
	null_func, null_func, null_func, null_func, null_func, //939
	null_func, null_func, null_func, null_func, null_func, //944
	null_func, null_func, null_func, null_func, null_func, //949
	null_func, null_func, null_func, null_func, null_func, //954
	null_func, null_func, null_func, null_func, null_func, //959
	null_func, null_func, null_func, null_func, null_func, //964
	null_func, null_func, null_func, null_func, null_func, //969
	null_func, null_func, null_func, null_func, null_func, //974
	null_func, null_func, null_func, null_func, null_func, //979
	null_func, null_func, null_func, null_func, null_func, //984
	null_func, null_func, null_func, null_func, null_func, //989
	null_func, null_func, null_func, null_func, null_func, //994
	null_func, null_func, null_func, null_func, null_func, //999
	null_func, null_func, null_func, null_func, null_func, //1004
	null_func, null_func, null_func, null_func, null_func, //1009
	null_func, null_func, null_func, null_func, null_func, //1014
	null_func, null_func, null_func, null_func, null_func, //1019
	null_func, null_func, null_func, bind_func(cellGcmCallback), //1024
};

/** HACK: Used to delete func_caller objects that get allocated and stored in sc_table (above).
* The destructor of this static object gets called when the program shuts down.
*/
struct SyscallTableCleaner_t
{
	SyscallTableCleaner_t() {}
	~SyscallTableCleaner_t()
	{
		for (int i = 0; i < kSyscallTableLength; ++i)
		{
			if (sc_table[i] != null_func)
				delete sc_table[i];
		}

		delete null_func;
	}
} SyscallTableCleaner_t;

void default_syscall()
{
	declCPU();
	u32 code = CPU.GPR[11];
	//TODO: remove this
	switch(code)
	{
		/*
		//process
		case 2: RESULT(lv2ProcessWaitForChild(CPU)); return;
		case 4: RESULT(lv2ProcessGetStatus(CPU)); return;
		case 5: RESULT(lv2ProcessDetachChild(CPU)); return;
		case 12: RESULT(lv2ProcessGetNumberOfObject(CPU)); return;
		case 13: RESULT(lv2ProcessGetId(CPU)); return;
		case 18: RESULT(lv2ProcessGetPpid(CPU)); return;
		case 19: RESULT(lv2ProcessKill(CPU)); return;
		case 23: RESULT(lv2ProcessWaitForChild2(CPU)); return;
		case 25: RESULT(lv2ProcessGetSdkVersion(CPU)); return;
		*/
		//tty
		case 988:
			ConLog.Warning("SysCall 988! r3: 0x%llx, r4: 0x%llx, pc: 0x%llx",
				CPU.GPR[3], CPU.GPR[4], CPU.PC);
			RESULT(0);
		return;

		case 999:
			dump_enable = !dump_enable;
			Emu.Pause();
			ConLog.Warning("Dump %s", (dump_enable ? "enabled" : "disabled"));
		return;

		case 1000:
			Ini.HLELogging.SetValue(!Ini.HLELogging.GetValue());
			ConLog.Warning("Log %s", (Ini.HLELogging.GetValue() ? "enabled" : "disabled"));
		return;
	}

	ConLog.Error("Unknown syscall: %d - %08x", code, code);
	RESULT(0);
	return;
}

void SysCalls::DoSyscall(u32 code)
{
	if(code < 1024)
	{
		(*sc_table[code])();
		return;
	}
	
	if(CallFunc(code))
	{
		return;
	}
	//ConLog.Error("Unknown function 0x%08x", code);
	//return 0;

	//TODO: remove this
	declCPU();
	RESULT(DoFunc(code));
}
