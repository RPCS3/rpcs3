#include "stdafx.h"
#include "SysCalls.h"
#include "Modules.h"
#include "SC_FUNC.h"

void default_syscall();
static func_caller *null_func = bind_func(default_syscall);

static func_caller* sc_table[1024] = 
{
	null_func, bind_func(sys_process_getpid), null_func, bind_func(sys_process_exit), null_func, //4
	null_func, null_func, null_func, null_func, null_func, //9
	null_func, null_func, null_func, null_func, null_func, //14
	null_func, null_func, null_func, null_func, null_func, //19
	null_func, null_func, bind_func(sys_process_exit), null_func, null_func, //24
	null_func, null_func, null_func, null_func, null_func, //29
	null_func, null_func, null_func, null_func, null_func, //34
	null_func, null_func, null_func, null_func, null_func, //39
	null_func, bind_func(sys_ppu_thread_exit), null_func, bind_func(sys_ppu_thread_yield), bind_func(sys_ppu_thread_join), //44
	bind_func(sys_ppu_thread_detach), bind_func(sys_ppu_thread_get_join_state), bind_func(sys_ppu_thread_set_priority), bind_func(sys_ppu_thread_get_priority), bind_func(sys_ppu_thread_get_stack_information), //49
	bind_func(sys_ppu_thread_stop), bind_func(sys_ppu_thread_restart), bind_func(sys_ppu_thread_create), null_func, null_func, //54
	null_func, null_func, null_func, null_func, null_func, //59
	null_func, null_func, null_func, null_func, null_func, //64
	null_func, null_func, null_func, null_func, null_func, //69
	null_func, null_func, null_func, null_func, null_func, //74
	null_func, null_func, null_func, null_func, null_func, //49
	null_func, null_func, null_func, null_func, null_func, //84
	null_func, null_func, null_func, null_func, null_func, //89
	bind_func(sys_semaphore_create), bind_func(sys_semaphore_destroy), bind_func(sys_semaphore_wait), bind_func(sys_semaphore_trywait), bind_func(sys_semaphore_post), //94
	bind_func(sys_lwmutex_create), bind_func(sys_lwmutex_destroy), bind_func(sys_lwmutex_lock), bind_func(sys_lwmutex_trylock), bind_func(sys_lwmutex_unlock), //99
	bind_func(sys_mutex_create), bind_func(sys_mutex_destroy), bind_func(sys_mutex_lock), bind_func(sys_mutex_trylock), bind_func(sys_mutex_unlock), //104
	bind_func(sys_cond_create), bind_func(sys_cond_destroy), bind_func(sys_cond_wait), bind_func(sys_cond_signal), bind_func(sys_cond_signal_all), //109
	null_func, null_func, null_func, null_func, null_func, //114
	null_func, null_func, null_func, null_func, null_func, //119
	null_func, null_func, null_func, null_func, null_func, //124
	null_func, null_func, null_func, bind_func(sys_event_queue_create), null_func, //129
	bind_func(sys_event_queue_receive), null_func, null_func, null_func, bind_func(sys_event_port_create), //134
	null_func, bind_func(sys_event_port_connect_local), null_func, bind_func(sys_event_port_send), null_func, //139
	null_func, null_func, null_func, null_func, null_func, //144
	bind_func(sys_time_get_current_time), bind_func(sys_time_get_system_time), bind_func(sys_time_get_timebase_frequency), null_func, null_func, //149
	null_func, null_func, null_func, null_func, null_func, //154
	null_func, bind_func(sys_spu_image_open), null_func, null_func, null_func, //159
	bind_func(sys_raw_spu_create), null_func, null_func, null_func, null_func, //164
	null_func, null_func, null_func, null_func, bind_func(sys_spu_initialize), //169
	bind_func(sys_spu_thread_group_create), bind_func(sys_spu_thread_set_argument), bind_func(sys_spu_thread_initialize), bind_func(sys_spu_thread_group_start), null_func, //174
	null_func, null_func, null_func, null_func, null_func, //179
	null_func, bind_func(sys_spu_thread_write_ls), bind_func(sys_spu_thread_read_ls), null_func, null_func, //184
	null_func, null_func, null_func, null_func, null_func, //189
	bind_func(sys_spu_thread_write_spu_mb), null_func, null_func, null_func, null_func, //194
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
	null_func, bind_func(sys_spu_thread_group_connect_event_all_threads), null_func, null_func, null_func, //254
	null_func, null_func, null_func, null_func, null_func, //259
	null_func, null_func, null_func, null_func, null_func, //264
	null_func, null_func, null_func, null_func, null_func, //269
	null_func, null_func, null_func, null_func, null_func, //274
	null_func, null_func, null_func, null_func, null_func, //279
	null_func, null_func, null_func, null_func, null_func, //284
	null_func, null_func, null_func, null_func, null_func, //289
	null_func, null_func, null_func, null_func, null_func, //294
	null_func, null_func, null_func, null_func, null_func, //299
	null_func, null_func, null_func, null_func, null_func, //304
	null_func, null_func, null_func, null_func, null_func, //309
	null_func, null_func, null_func, null_func, null_func, //314
	null_func, null_func, null_func, null_func, null_func, //319
	null_func, null_func, null_func, null_func, bind_func(sys_memory_container_create), //324
	bind_func(sys_memory_container_destroy), null_func, null_func, null_func, null_func, //329
	bind_func(sys_mmapper_allocate_address), null_func, null_func, null_func, null_func, //334
	null_func, null_func, null_func, null_func, null_func, //339
	null_func, null_func, null_func, null_func, null_func, //344
	null_func, null_func, null_func, bind_func(sys_memory_allocate), bind_func(sys_memory_free), //349
	null_func, null_func, bind_func(sys_memory_get_user_memory_size), null_func, null_func, //354
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
	null_func, null_func, null_func, null_func, null_func, //669
	null_func, null_func, null_func, null_func, null_func, //674
	null_func, null_func, null_func, null_func, null_func, //679
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
	null_func, bind_func(cellFsOpen), bind_func(cellFsRead), bind_func(cellFsWrite), bind_func(cellFsClose), //804
	bind_func(cellFsOpendir), bind_func(cellFsReaddir), bind_func(cellFsClosedir), null_func, bind_func(cellFsFstat), //809
	null_func, bind_func(cellFsMkdir), bind_func(cellFsRename), bind_func(cellFsRmdir), null_func, //814
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

bool enable_log = false;

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
		//timer
		case 141:
		case 142:
			std::this_thread::sleep_for(std::chrono::nanoseconds(SC_ARGS_1));
			RESULT(0);
		return;

		//tty
		case 988:
			ConLog.Warning("SysCall 988! r3: 0x%llx, r4: 0x%llx, pc: 0x%llx",
				CPU.GPR[3], CPU.GPR[4], CPU.PC);
			RESULT(0);
		return;

		case 999:
			dump_enable = !dump_enable;
			Emu.Pause();
			ConLog.Warning("Dump %s", dump_enable ? "enabled" : "disabled");
		return;

		case 1000:
			enable_log = !enable_log;
			ConLog.Warning("Log %s", enable_log ? "enabled" : "disabled");
		return;
	}

	ConLog.Error("Unknown syscall: %d - %08x", code, code);
	RESULT(0);
	return;
}

SysCalls::SysCalls(PPUThread& cpu) : CPU(cpu)
{
}

SysCalls::~SysCalls()
{
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
	RESULT(DoFunc(code));
}