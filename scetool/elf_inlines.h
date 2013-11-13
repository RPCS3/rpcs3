/*
* Copyright (c) 2011-2013 by naehrwert
* This file is released under the GPLv2.
*/

#ifndef _ELF_INLINES_H_
#define _ELF_INLINES_H_

#include <string.h>

#include "types.h"
#include "elf.h"

static inline void _es_elf32_ehdr(Elf32_Ehdr *h)
{
	h->e_type = _Es16(h->e_type);
	h->e_machine = _Es16(h->e_machine);
	h->e_version = _Es32(h->e_version);
	h->e_entry = _Es32(h->e_entry);
	h->e_phoff = _Es32(h->e_phoff);
	h->e_shoff = _Es32(h->e_shoff);
	h->e_flags = _Es32(h->e_flags);
	h->e_ehsize = _Es16(h->e_ehsize);
	h->e_phentsize = _Es16(h->e_phentsize);
	h->e_phnum = _Es16(h->e_phnum);
	h->e_shentsize = _Es16(h->e_shentsize);
	h->e_shnum = _Es16(h->e_shnum);
	h->e_shstrndx = _Es16(h->e_shstrndx);
}

static inline void _copy_es_elf32_ehdr(Elf32_Ehdr *dst, Elf32_Ehdr *src)
{
	memcpy(dst, src, sizeof(Elf32_Ehdr)); 
	_es_elf32_ehdr(dst);
}

static inline void _es_elf64_ehdr(Elf64_Ehdr *h)
{
	h->e_type = _Es16(h->e_type);
	h->e_machine = _Es16(h->e_machine);
	h->e_version = _Es32(h->e_version);
	h->e_entry = _Es64(h->e_entry);
	h->e_phoff = _Es64(h->e_phoff);
	h->e_shoff = _Es64(h->e_shoff);
	h->e_flags = _Es32(h->e_flags);
	h->e_ehsize = _Es16(h->e_ehsize);
	h->e_phentsize = _Es16(h->e_phentsize);
	h->e_phnum = _Es16(h->e_phnum);
	h->e_shentsize = _Es16(h->e_shentsize);
	h->e_shnum = _Es16(h->e_shnum);
	h->e_shstrndx = _Es16(h->e_shstrndx);
}

static inline void _copy_es_elf64_ehdr(Elf64_Ehdr *dst, Elf64_Ehdr *src)
{
	memcpy(dst, src, sizeof(Elf64_Ehdr)); 
	_es_elf64_ehdr(dst);
}

static inline void _es_elf32_shdr(Elf32_Shdr *h)
{
	h->sh_name = _Es32(h->sh_name);
	h->sh_type = _Es32(h->sh_type);
	h->sh_flags = _Es32(h->sh_flags);
	h->sh_addr = _Es32(h->sh_addr);
	h->sh_offset = _Es32(h->sh_offset);
	h->sh_size = _Es32(h->sh_size);
	h->sh_link = _Es32(h->sh_link);
	h->sh_info = _Es32(h->sh_info);
	h->sh_addralign = _Es32(h->sh_addralign);
	h->sh_entsize = _Es32(h->sh_entsize);
}

static inline void _copy_es_elf32_shdr(Elf32_Shdr *dst, Elf32_Shdr *src)
{
	memcpy(dst, src, sizeof(Elf32_Shdr)); 
	_es_elf32_shdr(dst);
}

static inline void _es_elf64_shdr(Elf64_Shdr *h)
{
	h->sh_name = _Es32(h->sh_name);
	h->sh_type = _Es32(h->sh_type);
	h->sh_flags = _Es64(h->sh_flags);
	h->sh_addr = _Es64(h->sh_addr);
	h->sh_offset = _Es64(h->sh_offset);
	h->sh_size = _Es64(h->sh_size);
	h->sh_link = _Es32(h->sh_link);
	h->sh_info = _Es32(h->sh_info);
	h->sh_addralign = _Es64(h->sh_addralign);
	h->sh_entsize = _Es64(h->sh_entsize);
}

static inline void _copy_es_elf64_shdr(Elf64_Shdr *dst, Elf64_Shdr *src)
{
	memcpy(dst, src, sizeof(Elf64_Shdr)); 
	_es_elf64_shdr(dst);
}

static inline void _es_elf32_phdr(Elf32_Phdr *h)
{
	h->p_type = _Es32(h->p_type);
	h->p_offset = _Es32(h->p_offset);
	h->p_vaddr = _Es32(h->p_vaddr);
	h->p_paddr = _Es32(h->p_paddr);
	h->p_filesz = _Es32(h->p_filesz);
	h->p_memsz = _Es32(h->p_memsz);
	h->p_flags = _Es32(h->p_flags);
	h->p_align = _Es32(h->p_align);
}

static inline void _copy_es_elf32_phdr(Elf32_Phdr *dst, Elf32_Phdr *src)
{
	memcpy(dst, src, sizeof(Elf32_Phdr)); 
	_es_elf32_phdr(dst);
}

static inline void _es_elf64_phdr(Elf64_Phdr *h)
{
	h->p_type = _Es32(h->p_type);
	h->p_flags = _Es32(h->p_flags);
	h->p_offset = _Es64(h->p_offset);
	h->p_vaddr = _Es64(h->p_vaddr);
	h->p_paddr = _Es64(h->p_paddr);
	h->p_filesz = _Es64(h->p_filesz);
	h->p_memsz = _Es64(h->p_memsz);
	h->p_align = _Es64(h->p_align);
}

static inline void _copy_es_elf64_phdr(Elf64_Phdr *dst, Elf64_Phdr *src)
{
	memcpy(dst, src, sizeof(Elf64_Phdr)); 
	_es_elf64_phdr(dst);
}

#endif
