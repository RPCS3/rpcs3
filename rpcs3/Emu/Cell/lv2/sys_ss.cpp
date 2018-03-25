#include "stdafx.h"
#include "Emu/Cell/PPUThread.h"

#include "sys_ss.h"

#ifdef _WIN32
#include <Windows.h>
#include <wincrypt.h>

const HCRYPTPROV s_crypto_provider = []() -> HCRYPTPROV
{
	HCRYPTPROV result;

	if (!CryptAcquireContextW(&result, nullptr, nullptr, PROV_RSA_FULL, 0) && !CryptAcquireContextW(&result, nullptr, nullptr, PROV_RSA_FULL, CRYPT_NEWKEYSET))
	{
		return 0;
	}

	::atexit([]()
	{
		if (s_crypto_provider)
		{
			CryptReleaseContext(s_crypto_provider, 0);
		}
	});

	return result;
}();
#endif



logs::channel sys_ss("sys_ss");

error_code sys_ss_random_number_generator(u32 arg1, vm::ptr<void> buf, u64 size)
{
	sys_ss.warning("sys_ss_random_number_generator(arg1=%u, buf=*0x%x, size=0x%x)", arg1, buf, size);

	if (arg1 != 2)
	{
		return 0x80010509;
	}

	if (size > 0x10000000)
	{
		return 0x80010501;
	}

#ifdef _WIN32
	if (!s_crypto_provider || !CryptGenRandom(s_crypto_provider, size, (BYTE*)buf.get_ptr()))
	{
		return CELL_EABORT;
	}
#else
	fs::file rnd{"/dev/urandom"};

	if (!rnd || rnd.read(buf.get_ptr(), size) != size)
	{
		return CELL_EABORT;
	}
#endif

	return CELL_OK;
}

s32 sys_ss_get_console_id(vm::ptr<u8> buf)
{
	sys_ss.todo("sys_ss_get_console_id(buf=*0x%x)", buf);

	// TODO: Return some actual IDPS?
	*buf = 0;

	return CELL_OK;
}

s32 sys_ss_get_open_psid(vm::ptr<CellSsOpenPSID> psid)
{
	sys_ss.warning("sys_ss_get_open_psid(psid=*0x%x)", psid);

	psid->high = 0;
	psid->low = 0;

	return CELL_OK;
}
