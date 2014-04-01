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

static func_caller* sc_table[1024] = 
{
	null_func, bind_func(sys_process_getpid), null_func, bind_func(sys_process_exit), null_func, //4
	null_func, null_func, null_func, null_func, null_func, //9
	null_func, null_func, bind_func(sys_process_get_number_of_object), bind_func(sys_process_get_id), null_func, //14
	null_func, null_func, null_func, bind_func(sys_process_getppid), null_func, //19
	null_func, null_func, bind_func(sys_process_exit), null_func, null_func, //24
	null_func, null_func, null_func, null_func, null_func, //29
	bind_func(sys_process_get_paramsfo), null_func, null_func, null_func, null_func, //34
	null_func, null_func, null_func, null_func, null_func, //39
	null_func,												//40  (0x028)
	bind_func(sys_ppu_thread_exit),							//41  (0x029)
	null_func,												//42  (0x02A)
	bind_func(sys_ppu_thread_yield),						//43  (0x02B)
	bind_func(sys_ppu_thread_join),							//44  (0x02C)
	bind_func(sys_ppu_thread_detach),						//45  (0x02D)
	bind_func(sys_ppu_thread_get_join_state),				//46  (0x02E)
	bind_func(sys_ppu_thread_set_priority),					//47  (0x02F)
	bind_func(sys_ppu_thread_get_priority),					//48  (0x030)
	bind_func(sys_ppu_thread_get_stack_information),		//49  (0x031)
	bind_func(sys_ppu_thread_stop),							//50  (0x032)
	bind_func(sys_ppu_thread_restart),						//51  (0x033)
	bind_func(sys_ppu_thread_create),						//52  (0x034)
	null_func,												//53  (0x035)
	null_func,												//54  (0x036)
	null_func, null_func, null_func, null_func, null_func, //59
	bind_func(sys_trace_create),							//60  (0x03C)
	bind_func(sys_trace_start),								//61  (0x03D)
	bind_func(sys_trace_stop),								//62  (0x03E)
	bind_func(sys_trace_update_top_index),					//63  (0x03F)
	bind_func(sys_trace_destroy), 							//64  (0x040)
	bind_func(sys_trace_drain), 							//65  (0x041)
	bind_func(sys_trace_attach_process),					//66  (0x042)
	bind_func(sys_trace_allocate_buffer), 					//67  (0x043)
	bind_func(sys_trace_free_buffer), 						//68  (0x044)
	bind_func(sys_trace_create2), 							//69  (0x045)
	bind_func(sys_timer_create),							//70  (0x046)
	bind_func(sys_timer_destroy),							//71  (0x047)
	bind_func(sys_timer_get_information),					//72  (0x048)
	bind_func(sys_timer_start),								//73  (0x049)
	bind_func(sys_timer_stop),								//74  (0x04A)
	bind_func(sys_timer_connect_event_queue),				//75  (0x04B)
	bind_func(sys_timer_disconnect_event_queue),			//76  (0x04C)
	null_func,												//77  (0x04D)
	null_func,												//78  (0x04E)
	null_func,												//79  (0x04F)
	null_func, null_func, bind_func(sys_event_flag_create), bind_func(sys_event_flag_destroy), null_func, //84
	bind_func(sys_event_flag_wait), bind_func(sys_event_flag_trywait), bind_func(sys_event_flag_set), null_func, null_func, //89
	bind_func(sys_semaphore_create),						//90  (0x05A)
	bind_func(sys_semaphore_destroy),						//91  (0x05B)
	bind_func(sys_semaphore_wait),							//92  (0x05C)
	bind_func(sys_semaphore_trywait),						//93  (0x05D)
	bind_func(sys_semaphore_post),							//94  (0x05E)
	bind_func(sys_lwmutex_create),							//95  (0x05F)
	bind_func(sys_lwmutex_destroy),							//96  (0x060)
	bind_func(sys_lwmutex_lock),							//97  (0x061)
	bind_func(sys_lwmutex_trylock),							//98  (0x062)
	bind_func(sys_lwmutex_unlock),							//99  (0x063)
	bind_func(sys_mutex_create),							//100 (0x064)
	bind_func(sys_mutex_destroy),							//101 (0x065)
	bind_func(sys_mutex_lock),								//102 (0x066)
	bind_func(sys_mutex_trylock),							//103 (0x067)
	bind_func(sys_mutex_unlock),							//104 (0x068)
	bind_func(sys_cond_create),								//105 (0x069)
	bind_func(sys_cond_destroy),							//106 (0x06A)
	bind_func(sys_cond_wait),								//107 (0x06B)
	bind_func(sys_cond_signal),								//108 (0x06C)
	bind_func(sys_cond_signal_all),							//109 (0x06D)
	bind_func(sys_cond_signal_to), null_func, null_func, null_func,             //113 (0x071)
	bind_func(sys_semaphore_get_value),                     //114 (0x072)
	null_func, null_func, null_func, bind_func(sys_event_flag_clear), null_func,  //119 (0x077)
	bind_func(sys_rwlock_create),							//120 (0x078)
	bind_func(sys_rwlock_destroy),							//121 (0x079)
	bind_func(sys_rwlock_rlock),							//122 (0x07A)
	bind_func(sys_rwlock_tryrlock),							//123 (0x07B)
	bind_func(sys_rwlock_runlock),							//124 (0x07C)
	bind_func(sys_rwlock_wlock),							//125 (0x07D)
	bind_func(sys_rwlock_trywlock),							//126 (0x07E)
	bind_func(sys_rwlock_wunlock),							//127 (0x07F)
	bind_func(sys_event_queue_create),						//128 (0x080)
	bind_func(sys_event_queue_destroy),						//129 (0x081)
	bind_func(sys_event_queue_receive), bind_func(sys_event_queue_tryreceive), // 131
	bind_func(sys_event_flag_cancel), bind_func(sys_event_queue_drain), bind_func(sys_event_port_create), //134
	bind_func(sys_event_port_destroy), bind_func(sys_event_port_connect_local), //136
	bind_func(sys_event_port_disconnect), bind_func(sys_event_port_send), bind_func(sys_event_flag_get), //139
	null_func, bind_func(sys_timer_usleep), bind_func(sys_timer_sleep), null_func, bind_func(sys_time_get_timezone), //144
	bind_func(sys_time_get_current_time), bind_func(sys_time_get_system_time), bind_func(sys_time_get_timebase_frequency), null_func, null_func, //149
	null_func, null_func, null_func, null_func, null_func, //154
	null_func, bind_func(sys_spu_image_open), null_func, null_func, null_func, //159
	bind_func(sys_raw_spu_create), null_func, null_func, null_func, null_func, //164
	bind_func(sys_spu_thread_get_exit_status), bind_func(sys_spu_thread_set_argument), null_func, null_func, bind_func(sys_spu_initialize), //169
	bind_func(sys_spu_thread_group_create), bind_func(sys_spu_thread_group_destroy), bind_func(sys_spu_thread_initialize), //172
	bind_func(sys_spu_thread_group_start), bind_func(sys_spu_thread_group_suspend), //174
	bind_func(sys_spu_thread_group_resume), null_func, null_func, bind_func(sys_spu_thread_group_join), null_func, //179
	null_func, bind_func(sys_spu_thread_write_ls), bind_func(sys_spu_thread_read_ls), null_func, bind_func(sys_spu_thread_write_snr), //184
	bind_func(sys_spu_thread_group_connect_event), bind_func(sys_spu_thread_group_disconnect_event), //186
	bind_func(sys_spu_thread_set_spu_cfg), bind_func(sys_spu_thread_get_spu_cfg), null_func, //189
	bind_func(sys_spu_thread_write_spu_mb), bind_func(sys_spu_thread_connect_event), bind_func(sys_spu_thread_disconnect_event), //192
	bind_func(sys_spu_thread_bind_queue), bind_func(sys_spu_thread_unbind_queue), //194
	null_func, null_func, null_func, null_func, null_func, //199
	null_func, null_func, null_func, null_func, null_func, //204
	null_func, null_func, null_func, null_func, null_func, //209
	null_func, null_func, null_func, null_func, null_func, //214
	null_func, null_func, null_func, null_func, null_func, //219
	null_func, null_func, null_func, null_func, null_func, //224
	null_func, null_func, null_func, null_func, null_func, //229
	null_func, null_func, null_func, null_func, null_func, //234
	null_func, null_func, null_func, null_func, null_func, //239
	null_func, null_func, null_func, null_func, null_func, //244
	null_func, null_func, null_func, null_func, null_func, //249
	null_func, bind_func(sys_spu_thread_group_connect_event_all_threads), bind_func(sys_spu_thread_group_disconnect_event_all_threads), null_func, null_func, //254
	null_func, null_func, null_func, null_func, null_func, //259
	null_func, null_func, null_func, null_func, null_func, //264
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
	null_func, null_func, //314
	null_func, null_func, null_func, null_func, null_func, //319
	null_func, null_func, null_func, null_func, //323
	bind_func(sys_memory_container_create),                 //324
	bind_func(sys_memory_container_destroy),                //325
	bind_func(sys_mmapper_allocate_fixed_address),          //326
	bind_func(sys_mmapper_enable_page_fault_notification),  //327
	null_func, null_func, //329
	bind_func(sys_mmapper_allocate_address),                //330
	bind_func(sys_mmapper_free_address),                    //331
	null_func, null_func, null_func, null_func, //335
	bind_func(sys_mmapper_change_address_access_right),     //336
	bind_func(sys_mmapper_search_and_map),                  //337
	null_func, null_func, null_func, //340
	bind_func(sys_memory_container_create),                 //341
	bind_func(sys_memory_container_destroy),                //342
	bind_func(sys_memory_container_get_size),				//343
	null_func, null_func, null_func, null_func, //347
	bind_func(sys_memory_allocate),							//348
	bind_func(sys_memory_free),								//349
	bind_func(sys_memory_allocate_from_container),			//350
	bind_func(sys_memory_get_page_attribute),				//351
	bind_func(sys_memory_get_user_memory_size),				//352
	null_func, null_func, //354
	null_func, null_func, null_func, null_func, null_func, //359
	null_func, null_func, null_func, null_func, null_func, //364
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
	null_func,												//665 (0x299)
	bind_func(sys_rsx_device_open),							//666 (0x29A)
	bind_func(sys_rsx_device_close),						//667 (0x29B)
	bind_func(sys_rsx_memory_allocate),						//668 (0x29C)
	bind_func(sys_rsx_memory_free),							//669 (0x29D)
	bind_func(sys_rsx_context_allocate),					//670 (0x29E)
	bind_func(sys_rsx_context_free), 						//671 (0x29F)
	bind_func(sys_rsx_context_iomap), 						//672 (0x2A0)
	bind_func(sys_rsx_context_iounmap), 					//673 (0x2A1)
	bind_func(sys_rsx_context_attribute),					//674 (0x2A2)
	bind_func(sys_rsx_device_map), 							//675 (0x2A3)
	bind_func(sys_rsx_device_unmap), 						//676 (0x2A4)
	bind_func(sys_rsx_attribute), 							//677 (0x2A5)
	null_func,												//678 (0x2A6)
	null_func,												//679 (0x2A7)
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
	null_func,												//800 (0x320)
	bind_func(cellFsOpen),									//801 (0x321)
	bind_func(cellFsRead),									//802 (0x322)
	bind_func(cellFsWrite),									//803 (0x323)
	bind_func(cellFsClose),									//804 (0x324)
	bind_func(cellFsOpendir),								//805 (0x325)
	bind_func(cellFsReaddir),								//806 (0x326)
	bind_func(cellFsClosedir),								//807 (0x327)
	bind_func(cellFsStat),									//808 (0x328)
	bind_func(cellFsFstat),									//809 (0x329)
	null_func,												//810 (0x32A)
	bind_func(cellFsMkdir),									//811 (0x32B)
	bind_func(cellFsRename),								//812 (0x32C)
	bind_func(cellFsRmdir),									//813 (0x32D)
	bind_func(cellFsUnlink),								//814 (0x32E)
	null_func, null_func, null_func, bind_func(cellFsLseek), null_func, //819
	null_func, null_func, null_func, null_func, null_func, //824
	null_func, null_func, null_func, null_func, null_func, //829
	null_func, null_func, null_func, null_func, null_func, //834
	null_func, null_func, null_func, null_func, null_func, //839
	null_func, null_func, null_func, null_func, null_func, //844
	null_func, null_func, null_func, null_func, null_func, //849
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
	null_func, null_func, null_func, bind_func(cellGcmCallback),    //1024
};

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
