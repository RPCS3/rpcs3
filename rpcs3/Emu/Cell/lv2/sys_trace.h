#pragma once

#include "util/types.hpp"

// SysCalls
s32 sys_trace_create();
s32 sys_trace_start();
s32 sys_trace_stop();
s32 sys_trace_update_top_index();
s32 sys_trace_destroy();
s32 sys_trace_drain();
s32 sys_trace_attach_process();
s32 sys_trace_allocate_buffer();
s32 sys_trace_free_buffer();
s32 sys_trace_create2();
