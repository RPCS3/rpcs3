#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/DbgCommand.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/ARMv7/ARMv7Thread.h"
#include "CPUThreadManager.h"

CPUThreadManager::CPUThreadManager()
{
}

CPUThreadManager::~CPUThreadManager()
{
}

void CPUThreadManager::Close()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& x : m_raw_spu)
	{
		x.reset();
	}
}

std::vector<std::shared_ptr<CPUThread>> CPUThreadManager::GetAllThreads() const
{
	std::vector<std::shared_ptr<CPUThread>> result;

	for (auto& v : Emu.GetIdManager().get_data<PPUThread>())
	{
		result.emplace_back(std::static_pointer_cast<CPUThread>(v.data));
	}

	for (auto& v : Emu.GetIdManager().get_data<SPUThread>())
	{
		result.emplace_back(std::static_pointer_cast<CPUThread>(v.data));
	}

	for (auto& v : Emu.GetIdManager().get_data<RawSPUThread>())
	{
		result.emplace_back(std::static_pointer_cast<CPUThread>(v.data));
	}

	for (auto& v : Emu.GetIdManager().get_data<ARMv7Thread>())
	{
		result.emplace_back(std::static_pointer_cast<CPUThread>(v.data));
	}

	return result;
}

void CPUThreadManager::Exec() const
{
	for (auto& v : Emu.GetIdManager().get_data<PPUThread>())
	{
		static_cast<CPUThread*>(v.data.get())->Exec();
	}

	for (auto& v : Emu.GetIdManager().get_data<ARMv7Thread>())
	{
		static_cast<CPUThread*>(v.data.get())->Exec();
	}
}

std::shared_ptr<RawSPUThread> CPUThreadManager::NewRawSPUThread()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	std::shared_ptr<RawSPUThread> result;

	for (u32 i = 0; i < m_raw_spu.size(); i++)
	{
		if (m_raw_spu[i].expired())
		{
			m_raw_spu[i] = result = Emu.GetIdManager().make_ptr<RawSPUThread>("RawSPU " + std::to_string(i), i);
			break;
		}
	}

	return result;
}

std::shared_ptr<RawSPUThread> CPUThreadManager::GetRawSPUThread(u32 index)
{
	if (index >= m_raw_spu.size())
	{
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(m_mutex);

	return m_raw_spu[index].lock();
}
