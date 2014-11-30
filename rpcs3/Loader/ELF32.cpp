#include "stdafx.h"
#include "Utilities/Log.h"
#include "Utilities/rFile.h"
#include "Emu/FS/vfsStream.h"
#include "Emu/Memory/Memory.h"
#include "ELF32.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/ARMv7/ARMv7Thread.h"
#include "Emu/System.h"

namespace loader
{
	namespace handlers
	{
		handler::error_code elf32::init(vfsStream& stream)
		{
			error_code res = handler::init(stream);

			if (res != ok)
				return res;

			m_stream->Read(&m_ehdr, sizeof(ehdr));

			if (!m_ehdr.check())
			{
				return bad_file;
			}

			if (m_ehdr.data_le.e_phnum && (m_ehdr.is_le() ? m_ehdr.data_le.e_phentsize : m_ehdr.data_be.e_phentsize) != sizeof(phdr))
			{
				return broken_file;
			}

			if (m_ehdr.data_le.e_shnum && (m_ehdr.is_le() ? m_ehdr.data_le.e_shentsize : m_ehdr.data_be.e_shentsize) != sizeof(shdr))
			{
				return broken_file;
			}

			LOG_WARNING(LOADER, "m_ehdr.e_type = 0x%x", (u16)(m_ehdr.is_le() ? m_ehdr.data_le.e_type : m_ehdr.data_be.e_type));

			if (m_ehdr.data_le.e_phnum)
			{
				m_phdrs.resize(m_ehdr.is_le() ? m_ehdr.data_le.e_phnum : m_ehdr.data_be.e_phnum);
				m_stream->Seek(handler::get_stream_offset() + (m_ehdr.is_le() ? m_ehdr.data_le.e_phoff : m_ehdr.data_be.e_phoff));
				size_t size = (m_ehdr.is_le() ? m_ehdr.data_le.e_phnum : m_ehdr.data_be.e_phnum) * sizeof(phdr);

				if (m_stream->Read(m_phdrs.data(), size) != size)
					return broken_file;
			}
			else
				m_phdrs.clear();

			if (m_ehdr.data_le.e_shnum)
			{
				m_shdrs.resize(m_ehdr.is_le() ? m_ehdr.data_le.e_shnum : m_ehdr.data_be.e_shnum);
				m_stream->Seek(handler::get_stream_offset() + (m_ehdr.is_le() ? m_ehdr.data_le.e_shoff : m_ehdr.data_be.e_shoff));
				size_t size = (m_ehdr.is_le() ? m_ehdr.data_le.e_shnum : m_ehdr.data_be.e_shnum) * sizeof(phdr);

				if (m_stream->Read(m_shdrs.data(), size) != size)
					return broken_file;
			}
			else
				m_shdrs.clear();

			return ok;
		}

		handler::error_code elf32::load()
		{
			Elf_Machine machine;
			switch (machine = (Elf_Machine)(u16)(m_ehdr.is_le() ? m_ehdr.data_le.e_machine : m_ehdr.data_be.e_machine))
			{
			case MACHINE_MIPS: vm::psp::init(); break;
			case MACHINE_ARM: vm::psv::init(); break;
			case MACHINE_SPU: vm::ps3::init(); break;

			default:
				return bad_version;
			}

			error_code res = load_data(0);

			if (res != ok)
				return res;

			switch (machine)
			{
			case MACHINE_MIPS: break;
			case MACHINE_ARM:
			{
				auto armv7_thr_stop_data = vm::psv::ptr<u32>::make(Memory.PSV.RAM.AllocAlign(3 * 4));
				armv7_thr_stop_data[0] = 0xf870; // HACK 
				armv7_thr_stop_data[1] = 0x0001; // index 1
				Emu.SetCPUThreadExit(armv7_thr_stop_data.addr());

				u32 entry = m_ehdr.data_le.e_entry + (u32)Memory.PSV.RAM.GetStartAddr();

				auto code = vm::psv::ptr<const u32>::make(entry & ~3);

				// very rough way to find entry point in .sceModuleInfo.rodata
				while (code[0] != 0xffffffffu)
				{
					entry = code[0] + 0x81000000;
					code++;
				}

				arm7_thread(entry & ~1 /* TODO: Thumb/ARM encoding selection */, "main_thread").args({ Emu.GetPath()/*, "-emu"*/ }).run();
				break;
			}
			case MACHINE_SPU: spu_thread(m_ehdr.is_le() ? m_ehdr.data_le.e_entry : m_ehdr.data_be.e_entry, "main_thread").args({ Emu.GetPath()/*, "-emu"*/ }).run(); break;
			}

			return ok;
		}

		handler::error_code elf32::load_data(u32 offset)
		{
			Elf_Machine machine = (Elf_Machine)(u16)(m_ehdr.is_le() ? m_ehdr.data_le.e_machine : m_ehdr.data_be.e_machine);

			for (auto &phdr : m_phdrs)
			{
				u32 memsz = m_ehdr.is_le() ? phdr.data_le.p_memsz : phdr.data_be.p_memsz;
				u32 filesz = m_ehdr.is_le() ? phdr.data_le.p_filesz : phdr.data_be.p_filesz;
				u32 vaddr = offset + (m_ehdr.is_le() ? phdr.data_le.p_vaddr : phdr.data_be.p_vaddr);
				u32 offset = m_ehdr.is_le() ? phdr.data_le.p_offset : phdr.data_be.p_offset;

				switch (m_ehdr.is_le() ? phdr.data_le.p_type : phdr.data_be.p_type)
				{
				case 0x00000001: //LOAD
					if (phdr.data_le.p_memsz)
					{
						if (machine == MACHINE_ARM && !Memory.PSV.RAM.AllocFixed(vaddr, memsz))
						{
							LOG_ERROR(LOADER, "%s(): AllocFixed(0x%llx, 0x%x) failed", __FUNCTION__, vaddr, memsz);

							return loading_error;
						}

						if (filesz)
						{
							m_stream->Seek(handler::get_stream_offset() + offset);
							m_stream->Read(vm::get_ptr(vaddr), filesz);
						}
					}
					break;
				}
			}

			return ok;
		}
	}
}