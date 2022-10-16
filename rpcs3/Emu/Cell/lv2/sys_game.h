#pragma once

error_code _sys_game_watchdog_start(u32 timeout);
error_code _sys_game_watchdog_stop();
error_code _sys_game_watchdog_clear();
u64 _sys_game_get_system_sw_version();
error_code _sys_game_board_storage_read(vm::ptr<u8> buffer1, vm::ptr<u8> buffer2);
