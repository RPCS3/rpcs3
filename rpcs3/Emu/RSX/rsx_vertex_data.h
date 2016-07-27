#pragma once

#include "GCM.h"
#include "Utilities/types.h"

namespace rsx
{

struct data_array_format_info
{
private:
	u32& m_offset_register;
public:
	u16 frequency = 0;
	u8 stride = 0;
	u8 size = 0;
	vertex_base_type type = vertex_base_type::f;

	data_array_format_info(int id, std::array<u32, 0x10000 / 4> &registers)
		: m_offset_register(registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + id]) {}

	u32 offset() const
	{
		return m_offset_register;
	}
};

struct register_vertex_data_info
{
	u16 frequency = 0;
	u8 stride = 0;
	u8 size = 0;
	vertex_base_type type = vertex_base_type::f;

	register_vertex_data_info() {}
	std::array<u32, 4> data;

};

}
