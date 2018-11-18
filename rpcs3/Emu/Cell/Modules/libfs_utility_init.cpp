#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"


LOG_CHANNEL(libfs_utility_init);

error_code fs_utility_init_1F3CD9F1()
{
	libfs_utility_init.todo("fs_utility_init_1F3CD9F1()");
	return CELL_OK;
}

error_code fs_utility_init_263172B8(u32 arg1)
{
	libfs_utility_init.todo("fs_utility_init_263172B8(0x%0x)", arg1);
	// arg1 usually 0x3 ??

	// This method seems to call fsck on the various partitions, among other checks
	// Negative numbers indicate an error
	// Some positive numbers are deemed illegal, others (including 0) are accepted as valid
	return CELL_OK;
}

error_code fs_utility_init_4E949DA4()
{
	libfs_utility_init.todo("fs_utility_init_4E949DA4()");
	return CELL_OK;
}

error_code fs_utility_init_665DF255()
{
	libfs_utility_init.todo("fs_utility_init_665DF255()");
	return CELL_OK;
}

error_code fs_utility_init_6B5896B0(vm::ptr<u64> dest)
{
	libfs_utility_init.todo("fs_utility_init_6B5896B0(dest=*0x%0x)", dest);

	// This method writes the number of partitions to the address pointed to by dest
	*dest = 2;
	
	return CELL_OK;
}


error_code fs_utility_init_A9B04535(u32 arg1)
{
	libfs_utility_init.todo("fs_utility_init_A9B04535(0x%0x)", arg1);
	// This method seems to call fsck on the various partitions, among other checks
	// Negative numbers indicate an error
	// Some positive numbers are deemed illegal, others (including 0) are accepted as valid
	return CELL_OK;
}


error_code fs_utility_init_E7563CE6()
{
	libfs_utility_init.todo("fs_utility_init_E7563CE6()");
	return CELL_OK;
}


error_code fs_utility_init_F691D443()
{
	libfs_utility_init.todo("fs_utility_init_F691D443()");
	return CELL_OK;
}




DECLARE(ppu_module_manager::libfs_utility_init)("fs_utility_init", []()
{
	REG_FNID(fs_utility_init, 0x1F3CD9F1, fs_utility_init_1F3CD9F1);
	REG_FNID(fs_utility_init, 0x263172B8, fs_utility_init_263172B8);
	REG_FNID(fs_utility_init, 0x4E949DA4, fs_utility_init_4E949DA4);
	REG_FNID(fs_utility_init, 0x665DF255, fs_utility_init_665DF255);
	REG_FNID(fs_utility_init, 0x6B5896B0, fs_utility_init_6B5896B0);
	REG_FNID(fs_utility_init, 0xA9B04535, fs_utility_init_A9B04535);
	REG_FNID(fs_utility_init, 0xE7563CE6, fs_utility_init_E7563CE6);
	REG_FNID(fs_utility_init, 0xF691D443, fs_utility_init_F691D443);
});
