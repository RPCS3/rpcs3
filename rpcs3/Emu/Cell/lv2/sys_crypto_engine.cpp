#include "stdafx.h"

#include "Emu/Cell/ErrorCodes.h"

#include "sys_crypto_engine.h"

LOG_CHANNEL(sys_crypto_engine);

error_code sys_crypto_engine_create(vm::ptr<u32> id)
{
	sys_crypto_engine.todo("sys_crypto_engine_create(id=*0x%x)", id);

	return CELL_OK;
}

error_code sys_crypto_engine_destroy(u32 id)
{
	sys_crypto_engine.todo("sys_crypto_engine_destroy(id=0x%x)", id);

	return CELL_OK;
}

error_code sys_crypto_engine_random_generate(vm::ptr<void> buffer, u64 buffer_size)
{
	sys_crypto_engine.todo("sys_crypto_engine_random_generate(buffer=*0x%x, buffer_size=0x%x", buffer, buffer_size);

	return CELL_OK;
}
