#pragma once

#include "GCM.h"
#include "Utilities/types.h"

namespace rsx
{

struct data_array_format_info
{
	u16 frequency = 0;
	u8 stride = 0;
	u8 size = 0;
	vertex_base_type type = vertex_base_type::f;
	u32 m_offset;

	data_array_format_info() {}

	u32 offset() const
	{
		return m_offset;
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
