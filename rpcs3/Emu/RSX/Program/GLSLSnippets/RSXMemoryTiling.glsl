R"(
#version 450
layout(local_size_x = %ws, local_size_y = 1, local_size_z = 1) in;

#define SSBO_LOCATION(x) (x + %loc)

#define MEMORY_OP %op
#define MEMORY_OP_DETILE 0
#define MEMORY_OP_TILE   1

#if (MEMORY_OP == MEMORY_OP_TILE)
  #define TILED_DATA_MODIFIER
  #define LINEAR_DATA_MODIFIER readonly
#else
  #define TILED_DATA_MODIFIER readonly
  #define LINEAR_DATA_MODIFIER
#endif

layout(%set, binding=SSBO_LOCATION(0), std430) TILED_DATA_MODIFIER restrict buffer TiledDataBlock
{
	uint tiled_data[];
};

layout(%set, binding=SSBO_LOCATION(1), std430) LINEAR_DATA_MODIFIER restrict buffer LinearDataBlock
{
	uint linear_data[];
};

#ifdef VULKAN
layout(%push_block) uniform Configuration
{
	uint prime;                       /* Prime factor derived from the number of tiles per row */
	uint factor;                      /* Counterpart to the prime value. prime * factor = tiles per row. */
	uint num_tiles_per_row;           /* Pitch / tile-width. Each "tile" is 256 bytes long */
	uint tile_base_address;           /* Base address for this tile. */
	uint tile_size;                   /* Size of the whole tile. */
	uint tile_address_offset;         /* Address offset where the texture region sits. */
	uint tile_rw_offset;              /* Access offset. If we load the entire tile then this is 0, but can be a multiple of pitch if we skip some rows for performance reasons. */
	uint tile_pitch;                  /* Row length in bytes for every line in the tile and consequently the image. */
	uint tile_bank;                   /* Bank sense offset. Acts as a memory-subsystem bias so that different FBOS can make use of different parts of the circuitry */
	uint image_width;                 /* Width of the linear 2D region we're encoding/decoding */
	uint image_height;                /* Height of the linear 2D region to encode/decode */
	uint image_pitch;                 /* Image pitch. The incoming data may be from a GPU operation with packed pixels which can have a different pitch than the tile we're writing from/to */
	uint image_bpp;                   /* Texel width of the image format. */
};
#else
	uniform uint prime;               /* Prime factor derived from the number of tiles per row */
	uniform uint factor;              /* Counterpart to the prime value. prime * factor = tiles per row. */
	uniform uint num_tiles_per_row;   /* Pitch / tile-width. Each "tile" is 256 bytes long */
	uniform uint tile_base_address;   /* Base address for this tile. */
	uniform uint tile_size;           /* Size of the whole tile. */
	uniform uint tile_address_offset; /* Address offset where the texture region sits. */
	uniform uint tile_rw_offset;      /* Access offset. If we load the entire tile then this is 0, but can be a multiple of pitch if we skip some rows for performance reasons. */
	uniform uint tile_pitch;          /* Row length in bytes for every line in the tile and consequently the image. */
	uniform uint tile_bank;           /* Bank sense offset. Acts as a memory-subsystem bias so that different FBOS can make use of different parts of the circuitry */
	uniform uint image_width;         /* Width of the linear 2D region we're encoding/decoding */
	uniform uint image_height;        /* Height of the linear 2D region to encode/decode */
	uniform uint image_pitch;         /* Image pitch. The incoming data may be from a GPU operation with packed pixels which can have a different pitch than the tile we're writing from/to */
	uniform uint image_bpp;           /* Texel width of the image format. */
#endif

// Hard constants, set by hardware
#define RSX_TILE_WIDTH  256
#define RSX_TILE_HEIGHT 64

#if (MEMORY_OP == MEMORY_OP_TILE)

uvec4 read_linear(const in uint offset)
{
	switch (image_bpp)
	{
	case 16:
	{
		return uvec4(
			linear_data[offset * 4],
			linear_data[offset * 4 + 1],
			linear_data[offset * 4 + 2],
			linear_data[offset * 4 + 3]);
	}
	case 8:
	{
		return uvec4(
			linear_data[offset * 2],
			linear_data[offset * 2 + 1],
			0,
			0);
	}
	case 4:
	{
		return uvec4(linear_data[offset], 0, 0, 0);
	}
	case 2:
	{
		const uint word = linear_data[offset >> 1];
		const int shift = int(offset & 1) << 4;
		return uvec4(bitfieldExtract(word, shift, 16), 0, 0, 0);
	}
	case 1:
	{
		const uint word = linear_data[offset >> 2];
		const int shift = int(offset & 3) << 3;
		return uvec4(bitfieldExtract(word, shift, 8), 0, 0, 0);
	}
	default:
		return uvec4(0);
	}
}

void write_tiled(const in uint offset, const in uvec4 value)
{
	switch (image_bpp)
	{
	case 16:
	{
		tiled_data[offset * 4] = value.x;
		tiled_data[offset * 4 + 1] = value.y;
		tiled_data[offset * 4 + 2] = value.z;
		tiled_data[offset * 4 + 3] = value.w;
		break;
	}
	case 8:
	{
		tiled_data[offset * 2] = value.x;
		tiled_data[offset * 2 + 1] = value.y;
		break;
	}
	case 4:
	{
		tiled_data[offset] = value.x;
		break;
	}
	case 2:
	{
		const uint word_offset = offset >> 1;
		const uint word = tiled_data[word_offset];
		const int shift = int(offset & 1) << 4;
		tiled_data[word_offset] = bitfieldInsert(word, value.x, shift, 16);
		break;
	}
	case 1:
	{
		const uint word_offset = offset >> 2;
		const uint word = tiled_data[word_offset];
		const int shift = int(offset & 3) << 3;
		tiled_data[word_offset] = bitfieldInsert(word, value.x, shift, 8);
		break;
	}
	default:
		break;
	}
}

#else

uvec4 read_tiled(const in uint offset)
{
	switch (image_bpp)
	{
	case 16:
	{
		return uvec4(
			tiled_data[offset * 4],
			tiled_data[offset * 4 + 1],
			tiled_data[offset * 4 + 2],
			tiled_data[offset * 4 + 3]);
	}
	case 8:
	{
		return uvec4(
			tiled_data[offset * 2],
			tiled_data[offset * 2 + 1],
			0,
			0);
	}
	case 4:
	{
		return uvec4(tiled_data[offset], 0, 0, 0);
	}
	case 2:
	{
		const uint word = tiled_data[offset >> 1];
		const int shift = int(offset & 1) << 4;
		return uvec4(bitfieldExtract(word, shift, 16), 0, 0, 0);
	}
	case 1:
	{
		const uint word = tiled_data[offset >> 2];
		const int shift = int(offset & 3) << 3;
		return uvec4(bitfieldExtract(word, shift, 8), 0, 0, 0);
	}
	default:
		return uvec4(0);
	}
}

void write_linear(const in uint offset, const in uvec4 value)
{
	switch (image_bpp)
	{
	case 16:
	{
		linear_data[offset * 4] = value.x;
		linear_data[offset * 4 + 1] = value.y;
		linear_data[offset * 4 + 2] = value.z;
		linear_data[offset * 4 + 3] = value.w;
		break;
	}
	case 8:
	{
		linear_data[offset * 2] = value.x;
		linear_data[offset * 2 + 1] = value.y;
		break;
	}
	case 4:
	{
		linear_data[offset] = value.x;
		break;
	}
	case 2:
	{
		const uint word_offset = offset >> 1;
		const uint word = linear_data[word_offset];
		const int shift = int(offset & 1) << 4;
		linear_data[word_offset] = bitfieldInsert(word, value.x, shift, 16);
		break;
	}
	case 1:
	{
		const uint word_offset = offset >> 2;
		const uint word = linear_data[word_offset];
		const int shift = int(offset & 3) << 3;
		linear_data[word_offset] = bitfieldInsert(word, value.x, shift, 8);
		break;
	}
	default:
		break;
	}
}

#endif

void do_memory_op(const in uint row, const in uint col)
{
	const uint row_offset = (row * tile_pitch) + tile_base_address + tile_address_offset;
	const uint this_address = row_offset + (col * image_bpp);

	// 1. Calculate row_addr
	const uint texel_offset = (this_address - tile_base_address) / RSX_TILE_WIDTH;
	// Calculate coordinate of the tile grid we're supposed to be in
	const uint tile_x = texel_offset % num_tiles_per_row;
	const uint tile_y = (texel_offset / num_tiles_per_row) / RSX_TILE_HEIGHT;
	// Calculate the grid offset for the tile selected and add the base offset. It's supposed to affect the bank stuff in the next step
	const uint tile_id = tile_y * num_tiles_per_row + tile_x;
	const uint tile_selector = (tile_id + (tile_base_address >> 14)) & 0x3ffff;
	// Calculate row address
	const uint row_address = (tile_selector >> 2) & 0xffff;

	// 2. Calculate bank selector
	// There's a lot of weird math here, but it's just a variant of (tile_selector % 4) to pick a value between [0..3]
	uint bank_selector = 0;
	const uint bank_distribution_lookup[16] = { 0, 1, 2, 3, 2, 3, 0, 1, 1, 2, 3, 0, 3, 0, 1, 2 };

	if (factor == 1)
	{
		bank_selector = (tile_selector & 3);
	}
	else if (factor == 2)
	{
		const uint idx = ((tile_selector + ((tile_y & 1) << 1)) & 3) * 4 + (tile_y & 3);
		bank_selector = bank_distribution_lookup[idx];
	}
	else if (factor >= 4)
	{
		const uint idx = (tile_selector & 3) * 4 + (tile_y & 3);
		bank_selector = bank_distribution_lookup[idx];
	}
	bank_selector = (bank_selector + tile_bank) % 4;

	// 3. Calculate column selector
	uint column_selector = 0;
	const uint line_offset_in_tile = (texel_offset / num_tiles_per_row) % RSX_TILE_HEIGHT;
	// Calculate column_selector by bit-twiddling line offset and the other calculated parameter bits:
	// column_selector[9:7] = line_offset_in_tile[5:3]
	// column_selector[6:4] = this_address[7:5]
	// column_selector[3:2] = line_offset_in_tile[1:0]
	// column_selector[1:0] = 0
	column_selector |= ((line_offset_in_tile >> 3) & 0x7) << 7;
	column_selector |= ((this_address >> 5) & 0x7) << 4;
	column_selector |= ((line_offset_in_tile >> 0) & 0x3) << 2;

	// 4. Calculate partition selector (0 or 1)
	const uint partition_selector = (((line_offset_in_tile >> 2) & 1) + ((this_address >> 6) & 1)) & 1;

	// 5. Build tiled address
	uint tile_address = 0;
	// tile_address[31:16] = row_adr[15:0]
	// tile_address[15:14] = bank_sel[1:0]
	// tile_address[13:8] = column_sel[9:4]
	// tile_address[7:7] = partition_sel[0:0]
	// tile_address[6:5] = column_sel[3:2]
	// tile_address[4:0] = this_address[4:0]
	tile_address |= ((row_address >> 0) & 0xFFFF) << 16;
	tile_address |= ((bank_selector >> 0) & 0x3) << 14;
	tile_address |= ((column_selector >> 4) & 0x3F) << 8;
	tile_address |= ((partition_selector >> 0) & 0x1) << 7;
	tile_address |= ((column_selector >> 2) & 0x3) << 5;
	tile_address |= ((this_address >> 0) & 0x1F) << 0;
	// Twiddle bits 9 and 10
	tile_address ^= (((tile_address >> 12) ^ ((bank_selector ^ tile_selector) & 1) ^ (tile_address >> 14)) & 1) << 9;
	tile_address ^= ((tile_address >> 11) & 1) << 10;

	// Calculate relative addresses and sample
	uint linear_image_offset = (row * image_pitch) + (col * image_bpp);
	uint tile_base_offset = tile_address - tile_base_address;  // Distance from tile base address
	uint tile_data_offset = tile_base_offset - tile_rw_offset; // Distance from data base address

	if (tile_base_offset >= tile_size)
	{
		// Do not touch anything out of bounds
		return;
	}

	// Convert to texel addresses for data access
	linear_image_offset /= image_bpp;
	tile_data_offset /= image_bpp;

#if (MEMORY_OP == MEMORY_OP_DETILE)
	// Write to linear from tiled
	write_linear(linear_image_offset, read_tiled(tile_data_offset));
#else
	// Opposite. Write to tile from linear
	write_tiled(tile_data_offset, read_linear(linear_image_offset));
#endif
}

void main()
{
	// The 2D coordinates are retrieved from gl_GlobalInvocationID
	const uint num_iterations = (image_bpp < 4) ? (4 / image_bpp) : 1;
	const uint row = gl_GlobalInvocationID.y;
	const uint col0 = gl_GlobalInvocationID.x;
	
	for (uint col = col0; col < (col0 + num_iterations); ++col)
	{
		if (row >= image_height || col0 >= image_width)
		{
			// Out of bounds
			return;
		}

		do_memory_op(row, col0);
	}
}
)"
