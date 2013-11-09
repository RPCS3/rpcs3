#pragma once

struct sys_rwlock_attribute_t
{
	u32 attr_protocol;	//sys_protocol_t
	u32 attr_pshared;	//sys_process_shared_t
	u64 key;			//sys_ipc_key_t
	s32 flags;
	u8 name[8];
};

#pragma pack()