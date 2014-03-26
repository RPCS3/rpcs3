#include "stdafx.h"
#include "Modules.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

extern ArrayF<SFunc> g_static_funcs_list;

void StaticAnalyse(void* ptr, u32 size)
{
	u32* data = (u32*)ptr; size /= 4;

	// TODO: optimize search
	for (u32 i = 0; i < size; i++)
	{
		for (u32 j = 0; j < g_static_funcs_list.GetCount(); j++)
		{
			if ((data[i] & g_static_funcs_list[j].ops[0].mask) == g_static_funcs_list[j].ops[0].crc && (i + g_static_funcs_list[j].ops.GetCount()) < size)
			{
				bool found = true;
				for (u32 k = i + 1, x = 1; x < g_static_funcs_list[j].ops.GetCount(); k++, x++)
				{
					// skip NOP
					if (data[k] == se32(0x60000000)) 
					{
						k++;
						continue;
					}

					if ((data[k] & g_static_funcs_list[j].ops[x].mask) != g_static_funcs_list[j].ops[x].crc)
					{
						found = false;
						break;
					}
				}
				if (found)
				{
					//ConLog.Success("Function '%s' hooked", wxString(g_static_funcs_list[j].name).wx_str());
					g_static_funcs_list[j].found++;
					data[i] = re(0x39600000 | j); // li r11, j
					data[i+1] = se32(0x44000003); // sc 3
					data[i+2] = se32(0x4e800020); // blr
					i += 2; // ???
				}
			}
		}
	}

	// check function groups
	for (u32 i = 0; i < g_static_funcs_list.GetCount(); i++)
	{
		if (g_static_funcs_list[i].found) // start from some group
		{
			const u64 group = g_static_funcs_list[i].group;

			enum GroupSearchResult : u32
			{
				GSR_SUCCESS = 0, // every function from this group has been found
				GSR_MISSING = 1, // (error) not every function found
				GSR_EXCESS = 2, // (error) some function found twice or more
			};
			u32 res = GSR_SUCCESS;

			// analyse
			for (u32 j = i; j < g_static_funcs_list.GetCount(); j++) if (g_static_funcs_list[j].group == group)
			{
				u32 count = g_static_funcs_list[j].found;

				if (count == 0) // not found
				{
					// check if this function has been found with different pattern
					for (u32 k = i; k < g_static_funcs_list.GetCount(); k++) if (g_static_funcs_list[k].group == group)
					{
						if (k != j && g_static_funcs_list[k].ptr == g_static_funcs_list[j].ptr)
						{
							count += g_static_funcs_list[k].found;
						}
					}
					if (count == 0)
					{
						res |= GSR_MISSING;
						ConLog.Error("Function '%s' not found", wxString(g_static_funcs_list[j].name).wx_str());
					}
					else if (count > 1)
					{
						res |= GSR_EXCESS;
					}
				}
				else if (count == 1) // found
				{
					// ensure that this function has NOT been found with different pattern
					for (u32 k = i; k < g_static_funcs_list.GetCount(); k++) if (g_static_funcs_list[k].group == group)
					{
						if (k != j && g_static_funcs_list[k].ptr == g_static_funcs_list[j].ptr)
						{
							if (g_static_funcs_list[k].found)
							{
								res |= GSR_EXCESS;
								ConLog.Error("Function '%s' hooked twice", wxString(g_static_funcs_list[j].name).wx_str());
							}
						}
					}
				}
				else
				{
					res |= GSR_EXCESS;
					ConLog.Error("Function '%s' hooked twice", wxString(g_static_funcs_list[j].name).wx_str());
				}
			}

			// clear data
			for (u32 j = i; j < g_static_funcs_list.GetCount(); j++)
			{
				if (g_static_funcs_list[j].group == group) g_static_funcs_list[j].found = 0;
			}

			char name[9] = "????????";

			*(u64*)name = group;

			if (res == GSR_SUCCESS)
			{
				ConLog.Success("Function group [%s] successfully hooked", wxString(name, 9).wx_str());
			}
			else
			{
				ConLog.Error("Function group [%s] failed: %s%s", wxString(name, 9).wx_str(),
					wxString(res & GSR_MISSING ? "missing;" : "").wx_str(),
					wxString(res & GSR_EXCESS ? "excess;" : "").wx_str());
			}
		}
	}
}

void StaticExecute(u32 code)
{
	if (code < g_static_funcs_list.GetCount())
	{
		(*g_static_funcs_list[code].func)();
	}
	else
	{
		ConLog.Error("StaticExecute(%d): unknown function or illegal opcode", code);
	}
}

void StaticFinalize()
{
	g_static_funcs_list.Clear();
}