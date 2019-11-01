#include "stdafx.h"
#include "sys_ss.h"

#include "sys_process.h"
#include "Emu/Cell/PPUThread.h"


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



LOG_CHANNEL(sys_ss);

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

error_code sys_ss_access_control_engine(u64 pkg_id, u64 a2, u64 a3)
{
	sys_ss.todo("sys_ss_access_control_engine(pkg_id=0x%llx, a2=0x%llx, a3=0x%llx)", pkg_id, a2, a3);

	const u64 authid = g_ps3_process_info.self_info.valid ? 
		g_ps3_process_info.self_info.app_info.authid : 0;

	switch (pkg_id)
	{
	case 0x1:
	{
		if (!g_ps3_process_info.debug_or_root())
		{
			return CELL_ENOSYS;
		}

		if (!a2)
		{
			return CELL_ESRCH;
		}

		verify(HERE), a2 == process_getpid();
		vm::_ref<u64>(vm::cast(a3)) = authid;
		break;
	}
	case 0x2:
	{
		vm::_ref<u64>(vm::cast(a2)) = authid;
		break;
	}
	case 0x3:
	{
		if (!g_ps3_process_info.debug_or_root())
		{
			return CELL_ENOSYS;
		}

		break;
	}
	default:
		return 0x8001051du;
	}

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
