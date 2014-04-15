#include "stdafx.h"
#include "Modules.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

extern std::vector<SFunc*> g_static_funcs_list;

void StaticAnalyse(void* ptr, u32 size, u32 base)
{
	u32* data = (u32*)ptr; size /= 4;

	if(!Ini.HLEHookStFunc.GetValue())
		return;

	// TODO: optimize search
	for (u32 i = 0; i < size; i++)
	{
		for (u32 j = 0; j < g_static_funcs_list.size(); j++)
		{
			if ((data[i] & g_static_funcs_list[j]->ops[0].mask) == g_static_funcs_list[j]->ops[0].crc)
			{
				bool found = true;
				u32 can_skip = 0;
				for (u32 k = i, x = 0; x + 1 <= g_static_funcs_list[j]->ops.size(); k++, x++)
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

					const u32 mask = g_static_funcs_list[j]->ops[x].mask;
					const u32 crc = g_static_funcs_list[j]->ops[x].crc;

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
								ConLog.Warning("StaticAnalyse(): can_skip = %d (unchanged)", can_skip);
							}
						}
						else
						{
							if (can_skip) // cannot define this behaviour properly
							{
								ConLog.Warning("StaticAnalyse(): can_skip = %d (set to 0)", can_skip);
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
					ConLog.Write("Function '%s' hooked (addr=0x%x)", g_static_funcs_list[j]->name, i * 4 + base);
					g_static_funcs_list[j]->found++;
					data[i+0] = re32(0x39600000 | j); // li r11, j
					data[i+1] = se32(0x44000003); // sc 3
					data[i+2] = se32(0x4e800020); // blr
					i += 2; // skip modified code
				}
			}
		}
	}

	// check function groups
	for (u32 i = 0; i < g_static_funcs_list.size(); i++)
	{
		if (g_static_funcs_list[i]->found) // start from some group
		{
			const u64 group = g_static_funcs_list[i]->group;

			enum GroupSearchResult : u32
			{
				GSR_SUCCESS = 0, // every function from this group has been found once
				GSR_MISSING = 1, // (error) some function not found
				GSR_EXCESS = 2, // (error) some function found twice or more
			};
			u32 res = GSR_SUCCESS;

			// analyse
			for (u32 j = 0; j < g_static_funcs_list.size(); j++) if (g_static_funcs_list[j]->group == group)
			{
				u32 count = g_static_funcs_list[j]->found;

				if (count == 0) // not found
				{
					// check if this function has been found with different pattern
					for (u32 k = 0; k < g_static_funcs_list.size(); k++) if (g_static_funcs_list[k]->group == group)
					{
						if (k != j && g_static_funcs_list[k]->ptr == g_static_funcs_list[j]->ptr)
						{
							count += g_static_funcs_list[k]->found;
						}
					}
					if (count == 0)
					{
						res |= GSR_MISSING;
						ConLog.Error("Function '%s' not found", g_static_funcs_list[j]->name);
					}
					else if (count > 1)
					{
						res |= GSR_EXCESS;
					}
				}
				else if (count == 1) // found
				{
					// ensure that this function has NOT been found with different pattern
					for (u32 k = 0; k < g_static_funcs_list.size(); k++) if (g_static_funcs_list[k]->group == group)
					{
						if (k != j && g_static_funcs_list[k]->ptr == g_static_funcs_list[j]->ptr)
						{
							if (g_static_funcs_list[k]->found)
							{
								res |= GSR_EXCESS;
								ConLog.Error("Function '%s' hooked twice", g_static_funcs_list[j]->name);
							}
						}
					}
				}
				else
				{
					res |= GSR_EXCESS;
					ConLog.Error("Function '%s' hooked twice", g_static_funcs_list[j]->name);
				}
			}

			// clear data
			for (u32 j = 0; j < g_static_funcs_list.size(); j++)
			{
				if (g_static_funcs_list[j]->group == group) g_static_funcs_list[j]->found = 0;
			}

			char name[9] = "????????";

			*(u64*)name = group;

			if (res == GSR_SUCCESS)
			{
				ConLog.Success("Function group [%s] successfully hooked", std::string(name, 9).c_str());
			}
			else
			{
				ConLog.Error("Function group [%s] failed:%s%s", std::string(name, 9).c_str(),
					(res & GSR_MISSING ? " missing;" : ""),
					(res & GSR_EXCESS ? " excess;" : ""));
			}
		}
	}
}

void StaticExecute(u32 code)
{
	if (code < g_static_funcs_list.size())
	{
		(*g_static_funcs_list[code]->func)();
	}
	else
	{
		ConLog.Error("StaticExecute(%d): unknown function or illegal opcode", code);
	}
}

void StaticFinalize()
{
	g_static_funcs_list.clear();
}
