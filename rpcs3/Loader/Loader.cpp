#include "stdafx.h"
#include "Utilities/Log.h"
#include "Loader.h"
#include "PSF.h"
#include "Emu/FS/vfsLocalFile.h"

namespace loader
{
	bool loader::load(vfsStream& stream)
	{
		for (auto i : m_handlers)
		{
			i->set_status(i->init(stream));
			if (i->get_status() == handler::ok)
			{
				i->set_status(i->load());
				if (i->get_status() == handler::ok)
				{
					return true;
				}

				LOG_NOTICE(LOADER, "loader::load() failed: %s", i->get_error_code().c_str());
			}
			else
			{
				LOG_NOTICE(LOADER, "loader::init() failed: %s", i->get_error_code().c_str());
				stream.Seek(i->get_stream_offset());
			}
		}

		return false;
	}

	handler::error_code handler::init(vfsStream& stream)
	{
		m_stream_offset = stream.Tell();
		m_stream = &stream;

		return ok;
	}
};

static const u64 g_spu_offset = 0x10000;

const std::string Ehdr_DataToString(const u8 data)
{
	if(data > 1) return fmt::Format("%d's complement, big endian", data);
	if(data < 1) return "Data is not found";

	return fmt::Format("%d's complement, little endian", data);
}

const std::string Ehdr_TypeToString(const u16 type)
{
	switch(type)
	{
	case 0: return "NULL";
	case 2: return "EXEC (Executable file)";
	};

	return fmt::Format("Unknown (%d)", type);
}

const std::string Ehdr_OS_ABIToString(const u8 os_abi)
{
	switch(os_abi)
	{
	case 0x0 : return "UNIX System V";
	case 0x66: return "Cell OS LV-2";
	};

	return fmt::Format("Unknown (0x%x)", os_abi);
}

const std::string Ehdr_MachineToString(const u16 machine)
{
	switch(machine)
	{
	case MACHINE_MIPS:	return "MIPS";
	case MACHINE_PPC64:	return "PowerPC64";
	case MACHINE_SPU:	return "SPU";
	case MACHINE_ARM:	return "ARM";
	};

	return fmt::Format("Unknown (0x%x)", machine);
}

const std::string Phdr_FlagsToString(u32 flags)
{
	enum {ppu_R = 0x1, ppu_W = 0x2, ppu_E = 0x4};
	enum {spu_E = 0x1, spu_W = 0x2, spu_R = 0x4};
	enum {rsx_R = 0x1, rsx_W = 0x2, rsx_E = 0x4};

#define FLAGS_TO_STRING(f) \
	std::string(f & f##_R ? "R" : "-") + \
	std::string(f & f##_W ? "W" : "-") + \
	std::string(f & f##_E ? "E" : "-")

	const u8 ppu = flags & 0xf;
	const u8 spu = (flags >> 0x14) & 0xf;
	const u8 rsx = (flags >> 0x18) & 0xf;

	std::string ret;
	ret += fmt::Format("[0x%x] ", flags);

	flags &= ~ppu;
	flags &= ~spu << 0x14;
	flags &= ~rsx << 0x18;

	if(flags != 0) return fmt::Format("Unknown %s PPU[0x%x] SPU[0x%x] RSX[0x%x]", ret.c_str(), ppu, spu, rsx);

	ret += "PPU[" + FLAGS_TO_STRING(ppu) + "] ";
	ret += "SPU[" + FLAGS_TO_STRING(spu) + "] ";
	ret += "RSX[" + FLAGS_TO_STRING(rsx) + "]";

	return ret;
}

const std::string Phdr_TypeToString(const u32 type)
{
	switch(type)
	{
	case 0x00000001: return "LOAD";
	case 0x00000004: return "NOTE";
	case 0x00000007: return "TLS";
	case 0x60000001: return "LOOS+1";
	case 0x60000002: return "LOOS+2";
	};

	return fmt::Format("Unknown (0x%x)", type);
}
