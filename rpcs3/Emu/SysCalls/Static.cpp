#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/Modules.h"
#include "Static.h"

void StaticFuncManager::StaticAnalyse(void* ptr, u32 size, u32 base)
{
	u32* data = (u32*)ptr; size /= 4;

	if(!Ini.HLEHookStFunc.GetValue())
		return;

	// TODO: optimize search
	for (u32 i = 0; i < size; i++)
	{
		for (u32 j = 0; j < m_static_funcs_list.size(); j++)
		{
			if ((data[i] & m_static_funcs_list[j]->ops[0].mask) == m_static_funcs_list[j]->ops[0].crc)
			{
				bool found = true;
				u32 can_skip = 0;
				for (u32 k = i, x = 0; x + 1 <= m_static_funcs_list[j]->ops.size(); k++, x++)
				{
					if (k >= size)
					{
						found = false;
						break;
					}

					// skip NOP
					if (data[k] == se32(0x60000000)) 
					{
						x--;
						continue;
					}

					const u32 mask = m_static_funcs_list[j]->ops[x].mask;
					const u32 crc = m_static_funcs_list[j]->ops[x].crc;

					if (!mask)
					{
						// TODO: define syntax
						if (crc < 4) // skip various number of instructions that don't match next pattern entry
						{
							can_skip += crc;
							k--; // process this position again
						}
						else if (data[k] != crc) // skippable pattern ("optional" instruction), no mask allowed
						{
							k--;
							if (can_skip) // cannot define this behaviour properly
							{
								LOG_WARNING(LOADER, "StaticAnalyse(): can_skip = %d (unchanged)", can_skip);
							}
						}
						else
						{
							if (can_skip) // cannot define this behaviour properly
							{
								LOG_WARNING(LOADER, "StaticAnalyse(): can_skip = %d (set to 0)", can_skip);
								can_skip = 0;
							}
						}
					}
					else if ((data[k] & mask) != crc) // masked pattern
					{
						if (can_skip)
						{
							can_skip--;
						}
						else
						{
							found = false;
							break;
						}
					}
					else
					{
						can_skip = 0;
					}
				}
				if (found)
				{
					LOG_NOTICE(LOADER, "Function '%s' hooked (addr=0x%x)", m_static_funcs_list[j]->name, i * 4 + base);
					m_static_funcs_list[j]->found++;
					data[i+0] = re32(0x39600000 | j); // li r11, j
					data[i+1] = se32(0x44000003); // sc 3
					data[i+2] = se32(0x4e800020); // blr
					i += 2; // skip modified code
				}
			}
		}
	}

	// check function groups
	for (u32 i = 0; i < m_static_funcs_list.size(); i++)
	{
		if (m_static_funcs_list[i]->found) // start from some group
		{
			const u64 group = m_static_funcs_list[i]->group;

			enum GroupSearchResult : u32
			{
				GSR_SUCCESS = 0, // every function from this group has been found once
				GSR_MISSING = 1, // (error) some function not found
				GSR_EXCESS = 2, // (error) some function found twice or more
			};
			u32 res = GSR_SUCCESS;

			// analyse
			for (u32 j = 0; j < m_static_funcs_list.size(); j++) if (m_static_funcs_list[j]->group == group)
			{
				u32 count = m_static_funcs_list[j]->found;

				if (count == 0) // not found
				{
					// check if this function has been found with different pattern
					for (u32 k = 0; k < m_static_funcs_list.size(); k++) if (m_static_funcs_list[k]->group == group)
					{
						if (k != j && m_static_funcs_list[k]->ptr == m_static_funcs_list[j]->ptr)
						{
							count += m_static_funcs_list[k]->found;
						}
					}
					if (count == 0)
					{
						res |= GSR_MISSING;
						LOG_ERROR(LOADER, "Function '%s' not found", m_static_funcs_list[j]->name);
					}
					else if (count > 1)
					{
						res |= GSR_EXCESS;
					}
				}
				else if (count == 1) // found
				{
					// ensure that this function has NOT been found with different pattern
					for (u32 k = 0; k < m_static_funcs_list.size(); k++) if (m_static_funcs_list[k]->group == group)
					{
						if (k != j && m_static_funcs_list[k]->ptr == m_static_funcs_list[j]->ptr)
						{
							if (m_static_funcs_list[k]->found)
							{
								res |= GSR_EXCESS;
								LOG_ERROR(LOADER, "Function '%s' hooked twice", m_static_funcs_list[j]->name);
							}
						}
					}
				}
				else
				{
					res |= GSR_EXCESS;
					LOG_ERROR(LOADER, "Function '%s' hooked twice", m_static_funcs_list[j]->name);
				}
			}

			// clear data
			for (u32 j = 0; j < m_static_funcs_list.size(); j++)
			{
				if (m_static_funcs_list[j]->group == group) m_static_funcs_list[j]->found = 0;
			}

			char name[9] = "????????";

			*(u64*)name = group;

			if (res == GSR_SUCCESS)
			{
				LOG_SUCCESS(LOADER, "Function group [%s] successfully hooked", std::string(name, 9).c_str());
			}
			else
			{
				LOG_ERROR(LOADER, "Function group [%s] failed:%s%s", std::string(name, 9).c_str(),
					(res & GSR_MISSING ? " missing;" : ""),
					(res & GSR_EXCESS ? " excess;" : ""));
			}
		}
	}
}

void StaticFuncManager::StaticExecute(u32 code)
{
	if (code < m_static_funcs_list.size())
	{
		(*m_static_funcs_list[code]->func)();
	}
	else
	{
		LOG_ERROR(LOADER, "StaticExecute(%d): unknown function or illegal opcode", code);
	}
}

void StaticFuncManager::StaticFinalize()
{
	m_static_funcs_list.clear();
}

void StaticFuncManager::push_back(SFunc *ele)
{
	m_static_funcs_list.push_back(ele);
}

SFunc *StaticFuncManager::operator[](size_t i)
{
	return m_static_funcs_list[i];
}

StaticFuncManager::~StaticFuncManager()
{
	for (SFunc *s : m_static_funcs_list)
	{
		delete s;
	}
}

