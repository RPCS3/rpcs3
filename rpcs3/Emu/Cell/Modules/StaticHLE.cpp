#include "stdafx.h"
#include "StaticHLE.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/IdManager.h"

LOG_CHANNEL(static_hle);

// for future use
DECLARE(ppu_module_manager::static_hle) ("static_hle", []()
{
});

std::vector<std::array<std::string, 6>> shle_patterns_list
{
	{ "2BA5000778630020788400207C6B1B78419D00702C2500004D82002028A5000F", "FF", "36D0", "05C4", "sys_libc", "memcpy" },
	{ "2BA5000778630020788400207C6B1B78419D00702C2500004D82002028A5000F", "5C", "87A0", "05C4", "sys_libc", "memcpy" },
	{ "2B8500077CA32A14788406207C6A1B78409D009C3903000198830000788B45E4", "B4", "1453", "00D4", "sys_libc", "memset" },
	{ "280500087C661B7840800020280500004D8200207CA903A69886000038C60001", "F8", "F182", "0118", "sys_libc", "memset" },
	{ "2B8500077CA32A14788406207C6A1B78409D009C3903000198830000788B45E4", "70", "DFDA", "00D4", "sys_libc", "memset" },
	{ "7F832000FB61FFD8FBE1FFF8FB81FFE0FBA1FFE8FBC1FFF07C7B1B787C9F2378", "FF", "25B5", "12D4", "sys_libc", "memmove" },
	{ "2B850007409D00B07C6923785520077E2F800000409E00ACE8030000E9440000", "FF", "71F1", "0158", "sys_libc", "memcmp" },
	{ "280500007CE32050788B0760418200E028850100786A07607C2A580040840210", "FF", "87F2", "0470", "sys_libc", "memcmp" },
	{ "2B850007409D00B07C6923785520077E2F800000409E00ACE8030000E9440000", "68", "EF18", "0158", "sys_libc", "memcmp" },
};

statichle_handler::statichle_handler(int)
{
	load_patterns();
}

statichle_handler::~statichle_handler()
{
}

bool statichle_handler::load_patterns()
{
	for (u32 i = 0; i < shle_patterns_list.size(); i++)
	{
		auto& pattern = shle_patterns_list[i];

		if (pattern[0].size() != 64)
		{
			static_hle.error("[%d]:Start pattern length != 64", i);
			continue;
		}
		if (pattern[1].size() != 2)
		{
			static_hle.error("[%d]:Crc16_length != 2", i);
			continue;
		}
		if (pattern[2].size() != 4)
		{
			static_hle.error("[d]:Crc16 length != 4", i);
			continue;
		}
		if (pattern[3].size() != 4)
		{
			static_hle.error("[d]:Total length != 4", i);
			continue;
		}

		shle_pattern dapat;

		auto char_to_u8 = [&](u8 char1, u8 char2) -> u16
		{
			if (char1 == '.' && char2 == '.')
				return 0xFFFF;

			if (char1 == '.' || char2 == '.')
			{
				static_hle.error("[%d]:Broken byte pattern", i);
				return -1;
			}

			const u8 hv = char1 > '9' ? char1 - 'A' + 10 : char1 - '0';
			const u8 lv = char2 > '9' ? char2 - 'A' + 10 : char2 - '0';

			return (hv << 4) | lv;
		};

		for (u32 j = 0; j < 32; j++)
			dapat.start_pattern[j] = char_to_u8(pattern[0][j * 2], pattern[0][(j * 2) + 1]);

		dapat.crc16_length = ::narrow<u8>(char_to_u8(pattern[1][0], pattern[1][1]));
		dapat.crc16        = (char_to_u8(pattern[2][0], pattern[2][1]) << 8) | char_to_u8(pattern[2][2], pattern[2][3]);
		dapat.total_length = (char_to_u8(pattern[3][0], pattern[3][1]) << 8) | char_to_u8(pattern[3][2], pattern[3][3]);
		dapat._module      = pattern[4];
		dapat.name         = pattern[5];

		dapat.fnid = ppu_generate_id(dapat.name.c_str());

		static_hle.notice("Added a pattern for %s(id:0x%X)", dapat.name, dapat.fnid);
		hle_patterns.push_back(std::move(dapat));
	}

	return true;
}

#define POLY 0x8408

u16 statichle_handler::gen_CRC16(const u8* data_p, usz length)
{
	unsigned int data;

	if (length == 0)
		return 0;
	unsigned int crc = 0xFFFF;
	do
	{
		data = *data_p++;
		for (unsigned char i = 0; i < 8; i++)
		{
			if ((crc ^ data) & 1)
				crc = (crc >> 1) ^ POLY;
			else
				crc >>= 1;
			data >>= 1;
		}
	} while (--length != 0);

	crc  = ~crc;
	data = crc;
	crc  = (crc << 8) | ((data >> 8) & 0xff);
	return static_cast<u16>(crc);
}

bool statichle_handler::check_against_patterns(vm::cptr<u8>& data, u32 size, u32 addr)
{
	for (auto& pat : hle_patterns)
	{
		if (size < pat.total_length)
			continue;

		// check start pattern
		int i = 0;
		for (i = 0; i < 32; i++)
		{
			if (pat.start_pattern[i] == 0xFFFF)
				continue;
			if (data[i] != pat.start_pattern[i])
				break;
		}

		if (i != 32)
			continue;

		// start pattern ok, checking middle part
		if (pat.crc16_length != 0)
			if (gen_CRC16(&data[32], pat.crc16_length) != pat.crc16)
				continue;

		// we got a match!
		static_hle.success("Found function %s at 0x%x", pat.name, addr);

		// patch the code
		const auto smodule = ppu_module_manager::get_module(pat._module);

		if (smodule == nullptr)
		{
			static_hle.error("Couldn't find module: %s", pat._module);
			return false;
		}

		const auto sfunc   = &::at32(smodule->functions, pat.fnid);
		const u32 target   = g_fxo->get<ppu_function_manager>().func_addr(sfunc->index, true);

		// write stub
		vm::write32(addr, ppu_instructions::LIS(0, (target&0xFFFF0000)>>16));
		vm::write32(addr+4, ppu_instructions::ORI(0, 0, target&0xFFFF));
		vm::write32(addr+8, ppu_instructions::MTCTR(0));
		vm::write32(addr+12, ppu_instructions::BCTR());

		return true;
	}

	return false;
}
