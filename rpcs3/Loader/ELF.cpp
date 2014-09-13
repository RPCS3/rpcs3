#include "stdafx.h"
#include "Emu/FS/vfsStream.h"
#include "ELF.h"
#include "ELF64.h"
#include "ELF32.h"

void Elf_Ehdr::Show()
{
}

void Elf_Ehdr::Load(vfsStream& f)
{
	e_magic = Read32(f);
	e_class = Read8(f);
}

ELFLoader::ELFLoader(vfsStream& f)
	: m_elf_file(f)
	, LoaderBase()
	, m_loader(nullptr)
{
}

bool ELFLoader::LoadInfo()
{
	if(!m_elf_file.IsOpened()) 
		return false;

	m_elf_file.Seek(0);
	ehdr.Load(m_elf_file);
	if(!ehdr.CheckMagic()) 
		return false;

	switch(ehdr.GetClass())
	{
	case CLASS_ELF32: 
		m_loader = new ELF32Loader(m_elf_file);
		break;
	case CLASS_ELF64: 
		m_loader = new ELF64Loader(m_elf_file);
		break;
	}

	if(!(m_loader && m_loader->LoadInfo())) 
		return false;

	entry = m_loader->GetEntry();
	machine = m_loader->GetMachine();

	return true;
}

bool ELFLoader::LoadData(u64 offset)
{
	return m_loader && m_loader->LoadData(offset);
}

bool ELFLoader::Close()
{
	delete m_loader;
	m_loader = nullptr;
	return m_elf_file.Close();
}
