#include "stdafx.h"
#include "BufferUtils.h"
#include "Utilities/Log.h"

#define MIN2(x, y) ((x) < (y)) ? (x) : (y)
#define MAX2(x, y) ((x) > (y)) ? (x) : (y)


inline
bool overlaps(const std::pair<size_t, size_t> &range1, const std::pair<size_t, size_t> &range2)
{
	return !(range1.second < range2.first || range2.second < range1.first);
}

std::vector<VertexBufferFormat> FormatVertexData(const rsx::data_array_format_info *vertex_array_desc, const std::vector<u8> *vertex_data, size_t *vertex_data_size, size_t base_offset)
{
	std::vector<VertexBufferFormat> Result;
	for (size_t i = 0; i < rsx::limits::vertex_count; ++i)
	{
		const rsx::data_array_format_info &vertexData = vertex_array_desc[i];
		if (!vertexData.size) continue;

		u32 addrRegVal = rsx::method_registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + i];
		u32 addr = rsx::get_address(addrRegVal & 0x7fffffff, addrRegVal >> 31);
		size_t elementCount = ((vertexData.array) ? vertex_data_size[i] : vertex_data[i].size()) / (vertexData.size * rsx::get_vertex_type_size(vertexData.type));

		// If there is a single element, stride is 0, use the size of element instead
		size_t stride = vertexData.stride;
		size_t elementSize = rsx::get_vertex_type_size(vertexData.type);
		size_t start = addr + base_offset;
		size_t end = start + elementSize * vertexData.size + (elementCount - 1) * stride - 1;
		std::pair<size_t, size_t> range = std::make_pair(start, end);
		assert(start < end);
		bool isMerged = false;

		for (VertexBufferFormat &vbf : Result)
		{
			if (overlaps(vbf.range, range) && vbf.stride == stride)
			{
				// Extend buffer if necessary
				vbf.range.first = MIN2(vbf.range.first, range.first);
				vbf.range.second = MAX2(vbf.range.second, range.second);
				vbf.elementCount = MAX2(vbf.elementCount, elementCount);

				vbf.attributeId.push_back(i);
				isMerged = true;
				break;
			}
		}
		if (isMerged)
			continue;
		VertexBufferFormat newRange = { range, std::vector<size_t>{ i }, elementCount, stride };
		Result.emplace_back(newRange);
	}
	return Result;
}

void write_vertex_array_data_to_buffer(void *buffer, u32 first, u32 count, size_t index, const rsx::data_array_format_info &vertex_array_desc)
{
	assert(vertex_array_desc.array);

	if (vertex_array_desc.frequency > 1)
		LOG_ERROR(RSX, "%s: frequency is not null (%d, index=%d)", __FUNCTION__, vertex_array_desc.frequency, index);

	u32 offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + index];
	u32 address = rsx::get_address(offset & 0x7fffffff, offset >> 31);

	u32 type_size = rsx::get_vertex_type_size(vertex_array_desc.type);
	u32 element_size = type_size * vertex_array_desc.size;

	u32 base_offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET];
	u32 base_index = rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_INDEX];

	for (u32 i = 0; i < count; ++i)
	{
		auto src = vm::ps3::_ptr<const u8>(address + base_offset + vertex_array_desc.stride * (first + i + base_index));
		u8* dst = (u8*)buffer + i * element_size;

		switch (type_size)
		{
		case 1:
			memcpy(dst, src, vertex_array_desc.size);
			break;

		case 2:
		{
			auto* c_src = (const be_t<u16>*)src;
			u16* c_dst = (u16*)dst;

			for (u32 j = 0; j < vertex_array_desc.size; ++j)
			{
				*c_dst++ = *c_src++;
			}
			break;
		}

		case 4:
		{
			auto* c_src = (const be_t<u32>*)src;
			u32* c_dst = (u32*)dst;

			for (u32 j = 0; j < vertex_array_desc.size; ++j)
			{
				*c_dst++ = *c_src++;
			}
			break;
		}
		}
	}
}

template<typename IndexType>
void uploadAsIt(char *dst, u32 address, size_t indexCount, bool is_primitive_restart_enabled, u32 &min_index, u32 &max_index)
{
	for (u32 i = 0; i < indexCount; ++i)
	{
		IndexType index = vm::ps3::_ref<IndexType>(address + i * sizeof(IndexType));
		(IndexType&)dst[i * sizeof(IndexType)] = index;
		if (is_primitive_restart_enabled && index == (IndexType)-1) // Cut
			continue;
		max_index = MAX2(max_index, index);
		min_index = MIN2(min_index, index);
	}
}

template<typename IndexType>
void expandIndexedTriangleFan(char *dst, u32 address, size_t indexCount, bool is_primitive_restart_enabled, u32 &min_index, u32 &max_index)
{
	for (unsigned i = 0; i < indexCount - 2; i++)
	{
		IndexType index0 = vm::ps3::_ref<IndexType>(address);
		(IndexType&)dst[(3 * i) * sizeof(IndexType)] = index0;
		IndexType index1 = vm::ps3::_ref<IndexType>(address + (i + 2 - 1) * sizeof(IndexType));
		(IndexType&)dst[(3 * i + 1) * sizeof(IndexType)] = index1;
		IndexType index2 = vm::ps3::_ref<IndexType>(address + (i + 2) * sizeof(IndexType));
		(IndexType&)dst[(3 * i + 2) * sizeof(IndexType)] = index2;

		if (!is_primitive_restart_enabled || index0 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index0);
			max_index = MAX2(max_index, index0);
		}
		if (!is_primitive_restart_enabled || index1 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index1);
			max_index = MAX2(max_index, index1);
		}
		if (!is_primitive_restart_enabled || index2 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index2);
			max_index = MAX2(max_index, index2);
		}
	}
}

template<typename IndexType>
void expandIndexedQuads(char *dst, u32 address, size_t indexCount, bool is_primitive_restart_enabled, u32 &min_index, u32 &max_index)
{
	for (unsigned i = 0; i < indexCount / 4; i++)
	{
		// First triangle
		IndexType index0 = vm::ps3::_ref<IndexType>(address + 4 * i * sizeof(IndexType));
		(IndexType&)dst[(6 * i) * sizeof(IndexType)] = index0;
		IndexType index1 = vm::ps3::_ref<IndexType>(address + (4 * i + 1) * sizeof(IndexType));
		(IndexType&)dst[(6 * i + 1) * sizeof(IndexType)] = index1;
		IndexType index2 = vm::ps3::_ref<IndexType>(address + (4 * i + 2) * sizeof(IndexType));
		(IndexType&)dst[(6 * i + 2) * sizeof(IndexType)] = index2;
		// Second triangle
		(IndexType&)dst[(6 * i + 3) * sizeof(IndexType)] = index2;
		IndexType index3 = vm::ps3::_ref<IndexType>(address + (4 * i + 3) * sizeof(IndexType));
		(IndexType&)dst[(6 * i + 4) * sizeof(IndexType)] = index3;
		(IndexType&)dst[(6 * i + 5) * sizeof(IndexType)] = index0;

		if (!is_primitive_restart_enabled || index0 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index0);
			max_index = MAX2(max_index, index0);
		}
		if (!is_primitive_restart_enabled || index1 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index1);
			max_index = MAX2(max_index, index1);
		}
		if (!is_primitive_restart_enabled || index2 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index2);
			max_index = MAX2(max_index, index2);
		}
		if (!is_primitive_restart_enabled || index3 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index3);
			max_index = MAX2(max_index, index3);
		}
	}
}

// Only handle quads and triangle fan now
bool isNativePrimitiveMode(unsigned m_draw_mode)
{
	switch (m_draw_mode)
	{
	default:
	case CELL_GCM_PRIMITIVE_POINTS:
	case CELL_GCM_PRIMITIVE_LINES:
	case CELL_GCM_PRIMITIVE_LINE_LOOP:
	case CELL_GCM_PRIMITIVE_LINE_STRIP:
	case CELL_GCM_PRIMITIVE_TRIANGLES:
	case CELL_GCM_PRIMITIVE_TRIANGLE_STRIP:
	case CELL_GCM_PRIMITIVE_QUAD_STRIP:
	case CELL_GCM_PRIMITIVE_POLYGON:
		return true;
	case CELL_GCM_PRIMITIVE_TRIANGLE_FAN:
	case CELL_GCM_PRIMITIVE_QUADS:
		return false;
	}
}

size_t getIndexCount(unsigned m_draw_mode, unsigned initial_index_count)
{
	// Index count
	if (isNativePrimitiveMode(m_draw_mode))
		return initial_index_count;

	switch (m_draw_mode)
	{
	case CELL_GCM_PRIMITIVE_TRIANGLE_FAN:
		return (initial_index_count - 2) * 3;
	case CELL_GCM_PRIMITIVE_QUADS:
		return (6 * initial_index_count) / 4;
	default:
		return 0;
	}
}

void write_index_array_for_non_indexed_non_native_primitive_to_buffer(char* dst, unsigned draw_mode, unsigned first, unsigned count)
{
	unsigned short *typedDst = (unsigned short *)(dst);
	switch (draw_mode)
	{
	case CELL_GCM_PRIMITIVE_TRIANGLE_FAN:
		for (unsigned i = 0; i < (count - 2); i++)
		{
			typedDst[3 * i] = first;
			typedDst[3 * i + 1] = i + 2 - 1;
			typedDst[3 * i + 2] = i + 2;
		}
		return;
	case CELL_GCM_PRIMITIVE_QUADS:
		for (unsigned i = 0; i < count / 4; i++)
		{
			// First triangle
			typedDst[6 * i] = 4 * i + first;
			typedDst[6 * i + 1] = 4 * i + 1 + first;
			typedDst[6 * i + 2] = 4 * i + 2 + first;
			// Second triangle
			typedDst[6 * i + 3] = 4 * i + 2 + first;
			typedDst[6 * i + 4] = 4 * i + 3 + first;
			typedDst[6 * i + 5] = 4 * i + first;
		}
		return;
	}
}

void write_index_array_data_to_buffer(char* dst, unsigned m_draw_mode, unsigned first, unsigned count, unsigned &min_index, unsigned &max_index)
{
	u32 address = rsx::get_address(rsx::method_registers[NV4097_SET_INDEX_ARRAY_ADDRESS], rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] & 0xf);
	u32 type = rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4;

	u32 type_size = type == CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32 ? sizeof(u32) : sizeof(u16);

	u32 base_offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET];
	u32 base_index = 0;//rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_INDEX];
	bool is_primitive_restart_enabled = !!rsx::method_registers[NV4097_SET_RESTART_INDEX_ENABLE];

	switch (m_draw_mode)
	{
	case CELL_GCM_PRIMITIVE_POINTS:
	case CELL_GCM_PRIMITIVE_LINES:
	case CELL_GCM_PRIMITIVE_LINE_LOOP:
	case CELL_GCM_PRIMITIVE_LINE_STRIP:
	case CELL_GCM_PRIMITIVE_TRIANGLES:
	case CELL_GCM_PRIMITIVE_TRIANGLE_STRIP:
	case CELL_GCM_PRIMITIVE_QUAD_STRIP:
	case CELL_GCM_PRIMITIVE_POLYGON:
		switch (type)
		{
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
			uploadAsIt<u32>(dst, address + (first + base_index) * sizeof(u32), count, is_primitive_restart_enabled, min_index, max_index);
			return;
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
			uploadAsIt<u16>(dst, address + (first + base_index) * sizeof(u16), count, is_primitive_restart_enabled, min_index, max_index);
			return;
		}
		return;
	case CELL_GCM_PRIMITIVE_TRIANGLE_FAN:
		switch (type)
		{
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
			expandIndexedTriangleFan<u32>(dst, address + (first + base_index) * sizeof(u32), count, is_primitive_restart_enabled, min_index, max_index);
			return;
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
			expandIndexedTriangleFan<u16>(dst, address + (first + base_index) * sizeof(u16), count, is_primitive_restart_enabled, min_index, max_index);
			return;
		}
	case CELL_GCM_PRIMITIVE_QUADS:
		switch (type)
		{
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
			expandIndexedQuads<u32>(dst, address + (first + base_index) * sizeof(u32), count, is_primitive_restart_enabled, min_index, max_index);
			return;
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
			expandIndexedQuads<u16>(dst, address + (first + base_index) * sizeof(u16), count, is_primitive_restart_enabled, min_index, max_index);
			return;
		}
	}
}