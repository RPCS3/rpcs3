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
					ConLog.Success("Function '%s' hooked", wxString(g_static_funcs_list[j].name).wx_str());
					data[i] = re(0x39600000 | j); // li r11, j
					data[i+1] = se32(0x44000003); // sc 3
					data[i+2] = se32(0x4e800020); // blr
					i += g_static_funcs_list[j].ops.GetCount(); // ???
				}
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