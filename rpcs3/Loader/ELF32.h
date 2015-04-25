#pragma once
#include "Loader.h"

struct vfsStream;

namespace loader
{
	namespace handlers
	{
		class elf32 : public handler
		{
		public:
			struct ehdr
			{
				u32 e_magic;
				u8  e_class;
				u8  e_data;
				u8  e_curver;
				u8  e_os_abi;

				union
				{
					struct
					{
						u64 e_abi_ver;
						u16 e_type;
						u16 e_machine;
						u32 e_version;
						u32 e_entry;
						u32 e_phoff;
						u32 e_shoff;
						u32 e_flags;
						u16 e_ehsize;
						u16 e_phentsize;
						u16 e_phnum;
						u16 e_shentsize;
						u16 e_shnum;
						u16 e_shstrndx;
					} data_le;

					struct
					{
						be_t<u64> e_abi_ver;
						be_t<u16> e_type;
						be_t<u16> e_machine;
						be_t<u32> e_version;
						be_t<u32> e_entry;
						be_t<u32> e_phoff;
						be_t<u32> e_shoff;
						be_t<u32> e_flags;
						be_t<u16> e_ehsize;
						be_t<u16> e_phentsize;
						be_t<u16> e_phnum;
						be_t<u16> e_shentsize;
						be_t<u16> e_shnum;
						be_t<u16> e_shstrndx;
					} data_be;
				};

				bool is_le() const { return e_data == 1; }
				bool check() const { return e_magic == 0x464C457F; }
			};

			struct shdr
			{
				union
				{
					struct
					{
						u32 sh_name;
						u32 sh_type;
						u32 sh_flags;
						u32 sh_addr;
						u32 sh_offset;
						u32 sh_size;
						u32 sh_link;
						u32 sh_info;
						u32 sh_addralign;
						u32 sh_entsize;
					} data_le;

					struct
					{
						be_t<u32> sh_name;
						be_t<u32> sh_type;
						be_t<u32> sh_flags;
						be_t<u32> sh_addr;
						be_t<u32> sh_offset;
						be_t<u32> sh_size;
						be_t<u32> sh_link;
						be_t<u32> sh_info;
						be_t<u32> sh_addralign;
						be_t<u32> sh_entsize;
					} data_be;
				};
			};

			struct phdr
			{
				union
				{
					struct
					{
						u32 p_type;
						u32 p_offset;
						u32 p_vaddr;
						u32 p_paddr;
						u32 p_filesz;
						u32 p_memsz;
						u32 p_flags;
						u32 p_align;
					} data_le;

					struct
					{
						be_t<u32> p_type;
						be_t<u32> p_offset;
						be_t<u32> p_vaddr;
						be_t<u32> p_paddr;
						be_t<u32> p_filesz;
						be_t<u32> p_memsz;
						be_t<u32> p_flags;
						be_t<u32> p_align;
					} data_be;
				};
			};

			ehdr m_ehdr;
			std::vector<phdr> m_phdrs;
			std::vector<shdr> m_shdrs;

			error_code init(vfsStream& stream) override;
			error_code load() override;
			error_code load_data(u32 offset, bool skip_writeable = false);

			virtual ~elf32() = default;
		};
	}
}
