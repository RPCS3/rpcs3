#pragma once
#include <vector>
#include "Emu/Memory/vm.h"
#include "../RSXThread.h"


struct VertexBufferFormat
{
	std::pair<size_t, size_t> range;
	std::vector<size_t> attributeId;
	size_t elementCount;
	size_t stride;
};


/*
 * Detect buffer containing interleaved vertex attribute.
 * This minimizes memory upload size.
 */
std::vector<VertexBufferFormat> FormatVertexData(const rsx::data_array_format_info *vertex_array_desc, const std::vector<u8> *vertex_data, size_t *vertex_data_size, size_t base_offset);

/*
 * Write count vertex attributes from index array buffer starting at first, using vertex_array_desc
 */
void write_vertex_array_data_to_buffer(void *buffer, u32 first, u32 count, size_t index, const rsx::data_array_format_info &vertex_array_desc);

/*
 * If primitive mode is not supported and need to be emulated (using an index buffer) returns false.
 */
bool isNativePrimitiveMode(unsigned m_draw_mode);

/*
 * Returns a fixed index count for emulated primitive, otherwise returns initial_index_count
 */
size_t getIndexCount(unsigned m_draw_mode, unsigned initial_index_count);

/*
 * Write index information to bufferMap
 */
void uploadIndexData(unsigned m_draw_mode, unsigned index_type, void* indexBuffer, void* bufferMap, unsigned element_count);