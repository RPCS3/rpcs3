#pragma once
#include "Loader.h"
#include <unordered_map>

struct vfsStream;
class rFile;

namespace loader
{
	namespace handlers
	{
		class elf64 : public handler
		{
		public:
			struct ehdr
			{
				be_t<u32> e_magic;
				u8 e_class;
				u8 e_data;
				u8 e_curver;
				u8 e_os_abi;
				be_t<u64> e_abi_ver;
				be_t<u16> e_type;
				be_t<u16> e_machine;
				be_t<u32> e_version;
				be_t<u64> e_entry;
				be_t<u64> e_phoff;
				be_t<u64> e_shoff;
				be_t<u32> e_flags;
				be_t<u16> e_ehsize;
				be_t<u16> e_phentsize;
				be_t<u16> e_phnum;
				be_t<u16> e_shentsize;
				be_t<u16> e_shnum;
				be_t<u16> e_shstrndx;

				bool check() const { return e_magic.ToBE() == se32(0x7F454C46); }
			} m_ehdr;

			struct phdr
			{
				be_t<u32> p_type;
				be_t<u32> p_flags;
				be_t<u64> p_offset;
				bptr<void, 1, u64> p_vaddr;
				bptr<void, 1, u64> p_paddr;
				be_t<u64> p_filesz;
				be_t<u64> p_memsz;
				be_t<u64> p_align;
			};

			struct shdr
			{
				be_t<u32> sh_name;
				be_t<u32> sh_type;
				be_t<u64> sh_flags;
				bptr<void, 1, u64> sh_addr;
				be_t<u64> sh_offset;
				be_t<u64> sh_size;
				be_t<u32> sh_link;
				be_t<u32> sh_info;
				be_t<u64> sh_addralign;
				be_t<u64> sh_entsize;
			};

			struct sprx_module_info
			{
				be_t<u16> attr;
				u8 version[2];
				char name[28];
				be_t<u32> toc_addr;
				be_t<u32> export_start;
				be_t<u32> export_end;
				be_t<u32> import_start;
				be_t<u32> import_end;
			} m_sprx_module_info;

			struct sprx_export_info
			{
				u8 size;
				u8 padding;
				be_t<u16> version;
				be_t<u16> attr;
				be_t<u16> func_count;
				be_t<u16> vars_count;
				be_t<u16> tls_vars_count;
				be_t<u16> hash_info;
				be_t<u16> tls_hash_info;
				u8 reserved[2];
				be_t<u32> lib_name_offset;
				be_t<u32> nid_offset;
				be_t<u32> stub_offset;
			};

			struct sprx_import_info
			{
				u8 size;
				u8 unused;
				be_t<u16> version;
				be_t<u16> attr;
				be_t<u16> func_count;
				be_t<u16> vars_count;
				be_t<u16> tls_vars_count;
				u8 reserved[4];
				be_t<u32> lib_name_offset;
				be_t<u32> nid_offset;
				be_t<u32> stub_offset;
				//...
			};

			struct sprx_function_info
			{
				be_t<u32> name_table_offset;
				be_t<u32> entry_table_offset;
				be_t<u32> padding;
			} m_sprx_function_info;

			struct sprx_lib_info
			{
				std::string name;
			};

			struct sprx_segment_info
			{
				vm::ptr<void> begin;
				u32 size;
				u32 size_file;
				vm::ptr<void> initial_addr;
				std::vector<sprx_module_info> modules;
			};

			struct sprx_info
			{
				std::string name;
				u32 rtoc;

				struct module_info
				{
					std::unordered_map<u32, u32> exports;
					std::unordered_map<u32, u32> imports;
				};

				std::unordered_map<std::string, module_info> modules;
				std::vector<sprx_segment_info> segments;
			};

			std::vector<phdr> m_phdrs;
			std::vector<shdr> m_shdrs;

			std::vector<sprx_segment_info> m_sprx_segments_info;
			std::vector<sprx_import_info> m_sprx_import_info;
			std::vector<sprx_export_info> m_sprx_export_info;

		public:
			virtual ~elf64() = default;

			error_code init(vfsStream& stream) override;
			error_code load() override;
			error_code load_data(u64 offset);
			error_code load_sprx(sprx_info& info);
			bool is_sprx() const { return m_ehdr.e_type == 0xffa4; }
			std::string sprx_get_module_name() const { return m_sprx_module_info.name; }
		};
	}
}