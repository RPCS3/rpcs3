#pragma once

u16 swap16(u16 i);
u32 swap32(u32 i);
u64 swap64(u64 i);
u64 hex_to_u64(const char* hex_str);
void hex_to_bytes(unsigned char *data, const char *hex_str);