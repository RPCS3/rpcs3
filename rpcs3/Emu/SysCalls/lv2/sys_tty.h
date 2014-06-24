#pragma once

s32 sys_tty_read(u32 ch, u64 buf_addr, u32 len, u64 preadlen_addr);
s32 sys_tty_write(u32 ch, u64 buf_addr, u32 len, u64 pwritelen_addr);
