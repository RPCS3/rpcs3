#pragma once

#include <util/types.hpp>
#include <cstdint>

// Set this to 1 to force all decoding to be done on the CPU.
#define DEBUG_DMA_TILING 0

// This is a 1:1 port of the GPU code for my own sanity when debugging misplaced bits
// For a high-level explanation, read https://envytools.readthedocs.io/en/latest/hw/memory/vram.html
namespace rsx
{
	struct detiler_config
	{
		uint32_t prime;
		uint32_t factor;
		uint32_t num_tiles_per_row;
		uint32_t tile_base_address;
		uint32_t tile_size;
		uint32_t tile_address_offset;
		uint32_t tile_rw_offset;
		uint32_t tile_pitch;
		uint32_t tile_bank;
		uint32_t image_width;
		uint32_t image_height;
		uint32_t image_pitch;
		uint32_t image_bpp;
	};

#define RSX_TILE_WIDTH 256
#define RSX_TILE_HEIGHT 64
#define RSX_DMA_OP_ENCODE_TILE 0
#define RSX_DMA_OP_DECODE_TILE 1

	static inline void tiled_dma_copy(const uint32_t row, const uint32_t col, const detiler_config& conf, char* tiled_data, char* linear_data, int direction)
	{
		const uint32_t row_offset = (row * conf.tile_pitch) + conf.tile_base_address + conf.tile_address_offset;
		const uint32_t this_address = row_offset + (col * conf.image_bpp);

		// 1. Calculate row_addr
		const uint32_t texel_offset = (this_address - conf.tile_base_address) / RSX_TILE_WIDTH;
		// Calculate coordinate of the tile grid we're supposed to be in
		const uint32_t tile_x = texel_offset % conf.num_tiles_per_row;
		const uint32_t tile_y = (texel_offset / conf.num_tiles_per_row) / RSX_TILE_HEIGHT;
		// Calculate the grid offset for the tile selected and add the base offset. It's supposed to affect the bank stuff in the next step
		const uint32_t tile_id = tile_y * conf.num_tiles_per_row + tile_x;
		const uint32_t tile_selector = (tile_id + (conf.tile_base_address >> 14)) & 0x3ffff;
		// Calculate row address
		const uint32_t row_address = (tile_selector >> 2) & 0xffff;

		// 2. Calculate bank selector
		// There's a lot of weird math here, but it's just a variant of (tile_selector % 4) to pick a value between [0..3]
		uint32_t bank_selector = 0;
		const uint32_t bank_distribution_lookup[16] = { 0, 1, 2, 3, 2, 3, 0, 1, 1, 2, 3, 0, 3, 0, 1, 2 };

		if (conf.factor == 1)
		{
			bank_selector = (tile_selector & 3);
		}
		else if (conf.factor == 2)
		{
			const uint32_t idx = ((tile_selector + ((tile_y & 1) << 1)) & 3) * 4 + (tile_y & 3);
			bank_selector = bank_distribution_lookup[idx];
		}
		else if (conf.factor >= 4)
		{
			const uint32_t idx = (tile_selector & 3) * 4 + (tile_y & 3);
			bank_selector = bank_distribution_lookup[idx];
		}
		bank_selector = (bank_selector + conf.tile_bank) & 3;

		// 3. Calculate column selector
		uint32_t column_selector = 0;
		const uint32_t line_offset_in_tile = (texel_offset / conf.num_tiles_per_row) % RSX_TILE_HEIGHT;
		// Calculate column_selector by bit-twiddling line offset and the other calculated parameter bits:
		// column_selector[9:7] = line_offset_in_tile[5:3]
		// column_selector[6:4] = this_address[7:5]
		// column_selector[3:2] = line_offset_in_tile[1:0]
		// column_selector[1:0] = 0
		column_selector |= ((line_offset_in_tile >> 3) & 0x7) << 7;
		column_selector |= ((this_address >> 5) & 0x7) << 4;
		column_selector |= ((line_offset_in_tile >> 0) & 0x3) << 2;

		// 4. Calculate partition selector (0 or 1)
		const uint32_t partition_selector = (((line_offset_in_tile >> 2) & 1) + ((this_address >> 6) & 1)) & 1;

		// 5. Build tiled address
		uint32_t tile_address = 0;
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
		const uint32_t linear_image_offset = (row * conf.image_pitch) + (col * conf.image_bpp);
		const uint32_t tile_base_offset = tile_address - conf.tile_base_address;  // Distance from tile base address
		const uint32_t tile_data_offset = tile_base_offset - conf.tile_rw_offset; // Distance from data base address

		if (tile_base_offset >= conf.tile_size)
		{
			// Do not touch anything out of bounds
			return;
		}

		if (direction == RSX_DMA_OP_ENCODE_TILE)
		{
			std::memcpy(tiled_data + tile_data_offset, linear_data + linear_image_offset, conf.image_bpp);
		}
		else
		{
			std::memcpy(linear_data + linear_image_offset, tiled_data + tile_data_offset, conf.image_bpp);
		}
	}

	// Entry point. In GPU code this is handled by dispatch + main
	template <typename T, bool Decode = false>
	void tile_texel_data(void* dst, const void* src, uint32_t base_address, uint32_t base_offset, uint32_t tile_size, uint8_t bank_sense, uint16_t row_pitch_in_bytes, uint16_t image_width, uint16_t image_height)
	{
		// Some constants
		auto get_prime_factor = [](uint32_t pitch) -> std::pair<uint32_t, uint32_t>
		{
			const uint32_t base = (pitch >> 8);
			if ((pitch & (pitch - 1)) == 0)
			{
				return { 1u, base };
			}

			for (const auto prime : { 3, 5, 7, 11, 13 })
			{
				if ((base % prime) == 0)
				{
					return { prime, base / prime };
				}
			}

			// rsx_log.error("Unexpected pitch value 0x%x", pitch);
			return {};
		};

		const auto [prime, factor] = get_prime_factor(row_pitch_in_bytes);
		const uint32_t tiles_per_row = prime * factor;
		constexpr int op = Decode ? RSX_DMA_OP_DECODE_TILE : RSX_DMA_OP_ENCODE_TILE;

		auto src2 = static_cast<char*>(const_cast<void*>(src));
		auto dst2 = static_cast<char*>(dst);

		const detiler_config dconf = {
			.prime = prime,
			.factor = factor,
			.num_tiles_per_row = tiles_per_row,
			.tile_base_address = base_address,
			.tile_size = tile_size,
			.tile_address_offset = base_offset,
			.tile_rw_offset = base_offset,
			.tile_pitch = row_pitch_in_bytes,
			.tile_bank = bank_sense,
			.image_width = image_width,
			.image_height = image_height,
			.image_pitch = row_pitch_in_bytes,
			.image_bpp = sizeof(T)
		};

		for (u16 row = 0; row < image_height; ++row)
		{
			for (u16 col = 0; col < image_width; ++col)
			{
				if constexpr (op == RSX_DMA_OP_DECODE_TILE)
				{
					tiled_dma_copy(row, col, dconf, src2, dst2, op);
				}
				else
				{
					tiled_dma_copy(row, col, dconf, dst2, src2, op);
				}
			}
		}
	}

	[[maybe_unused]] static auto tile_texel_data16 = tile_texel_data<u16, false>;
	[[maybe_unused]] static auto tile_texel_data32 = tile_texel_data<u32, false>;
	[[maybe_unused]] static auto detile_texel_data16 = tile_texel_data<u16, true>;
	[[maybe_unused]] static auto detile_texel_data32 = tile_texel_data<u32, true>;

#undef RSX_TILE_WIDTH
#undef RSX_TILE_HEIGHT
#undef RSX_DMA_OP_ENCODE_TILE
#undef RSX_DMA_OP_DECODE_TILE
}
