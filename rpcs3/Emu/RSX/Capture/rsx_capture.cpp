#include "stdafx.h"
#include "rsx_capture.h"
#include "Emu/RSX/Common/BufferUtils.h"
#include "Emu/RSX/Common/TextureUtils.h"
#include "Emu/RSX/GCM.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/Memory/vm.h"

namespace rsx
{
	namespace capture
	{
		void insert_mem_block_in_map(u64& indexer, std::unordered_set<u64>& mem_changes, frame_capture_data::memory_block&& block, frame_capture_data::memory_block_data&& data)
		{
			if (!data.data.empty())
			{
				const auto [it, inserted] = frame_capture.memory_data_map.try_emplace(data, 0);
				u64& data_hash = it->second;

				if (inserted)
				{
					data_hash = ++indexer;
				}

				block.data_state = data_hash;

				const auto [block_it, inserted_block] = frame_capture.memory_map.try_emplace(block, 0);
				u64& block_hash = block_it->second;

				if (inserted_block)
				{
					block_hash = ++indexer;
				}

				mem_changes.insert(block_hash);
			}
		}

		void capture_draw_memory(thread* rsx)
		{
			// the idea here is to copy any memory that is needed to make the calls work
			// todo:
			//  - tile / zcull state changing during other commands
			//  - track memory that is rendered into and ignore saving it later, this one will be tough

			if (frame_capture.replay_commands.empty())
				fmt::throw_exception("no replay commands to attach memory state to");

			// shove the mem_changes onto the last issued command
			std::unordered_set<u64>& mem_changes = frame_capture.replay_commands.back().memory_state;
			u64& mem_indexer = frame_capture.memory_indexer;

			// capture fragment shader mem
			const auto [program_offset, program_location] = method_registers.shader_program_address();

			const u32 addr          = get_address(program_offset, program_location);
			const auto program_info = program_hash_util::fragment_program_utils::analyse_fragment_program(vm::base(addr));
			const u32 program_start = program_info.program_start_offset;
			const u32 ucode_size    = program_info.program_ucode_length;

			frame_capture_data::memory_block block;
			block.offset = program_offset;
			block.location = program_location;
			frame_capture_data::memory_block_data block_data;
			block_data.data.resize(ucode_size + program_start);
			std::memcpy(block_data.data.data(), vm::base(addr), ucode_size + program_start);
			insert_mem_block_in_map(mem_indexer, mem_changes, std::move(block), std::move(block_data));

			// vertex shader is passed in registers, so it can be ignored

			// save fragment tex mem
			for (const auto& tex : method_registers.fragment_textures)
			{
				if (!tex.enabled())
					continue;

				const u32 texaddr = get_address(tex.offset(), tex.location());
				auto layout       = get_subresources_layout(tex);

				// todo: dont use this function and just get size somehow
				usz texSize = 0;
				for (const auto& l : layout)
					texSize += l.data.size();

				if (!texSize)
					continue;

				frame_capture_data::memory_block block;
				block.offset = tex.offset();
				block.location = tex.location();
				frame_capture_data::memory_block_data block_data;
				block_data.data.resize(texSize);
				std::memcpy(block_data.data.data(), vm::base(texaddr), texSize);
				insert_mem_block_in_map(mem_indexer, mem_changes, std::move(block), std::move(block_data));
			}

			// save vertex texture mem
			for (const auto& tex : method_registers.vertex_textures)
			{
				if (!tex.enabled())
					continue;

				const u32 texaddr = get_address(tex.offset(), tex.location());
				auto layout       = get_subresources_layout(tex);

				// todo: dont use this function and just get size somehow
				usz texSize = 0;
				for (const auto& l : layout)
					texSize += l.data.size();

				if (!texSize)
					continue;

				frame_capture_data::memory_block block;
				block.offset = tex.offset();
				block.location = tex.location();
				frame_capture_data::memory_block_data block_data;
				block_data.data.resize(texSize);
				std::memcpy(block_data.data.data(), vm::base(texaddr), texSize);
				insert_mem_block_in_map(mem_indexer, mem_changes, std::move(block), std::move(block_data));
			}

			// save vertex buffer memory
			if (method_registers.current_draw_clause.command == draw_command::array)
			{
				const u32 input_mask = method_registers.vertex_attrib_input_mask();
				for (u8 index = 0; index < limits::vertex_count; ++index)
				{
					const bool enabled = !!(input_mask & (1 << index));
					if (!enabled)
						continue;

					const auto& info = method_registers.vertex_arrays_info[index];
					if (!info.size())
						continue;

					// vert buffer
					const u32 base_address    = get_vertex_offset_from_base(method_registers.vertex_data_base_offset(), info.offset() & 0x7fffffff);
					const u32 memory_location = info.offset() >> 31;

					const u32 addr       = get_address(base_address, memory_location);
					const u32 vertSize   = get_vertex_type_size_on_host(info.type(), info.size());
					const u32 vertStride = info.stride();

					method_registers.current_draw_clause.begin();
					do
					{
						const auto& range = method_registers.current_draw_clause.get_range();
						const u32 vertCount = range.count;
						const usz bufferSize = (vertCount - 1) * vertStride + vertSize;

						frame_capture_data::memory_block block;
						block.offset = base_address + (range.first * vertStride);
						block.location = memory_location;
						frame_capture_data::memory_block_data block_data;
						block_data.data.resize(bufferSize);
						std::memcpy(block_data.data.data(), vm::base(addr + (range.first * vertStride)), bufferSize);
						insert_mem_block_in_map(mem_indexer, mem_changes, std::move(block), std::move(block_data));
					}
					while (method_registers.current_draw_clause.next());
				}
			}
			// save index buffer if used
			else if (method_registers.current_draw_clause.command == draw_command::indexed)
			{
				const u32 input_mask = method_registers.vertex_attrib_input_mask();

				const u32 base_address    = method_registers.index_array_address();
				const u32 memory_location = method_registers.index_array_location();

				const auto index_type = method_registers.index_type();
				const u32 type_size   = get_index_type_size(index_type);
				const u32 base_addr   = get_address(base_address, memory_location) & (0 - type_size);

				// manually parse index buffer and copy vertex buffer
				u32 min_index = 0xFFFFFFFF, max_index = 0;

				const bool is_primitive_restart_enabled = method_registers.restart_index_enabled();
				const u32 primitive_restart_index       = method_registers.restart_index();

				method_registers.current_draw_clause.begin();
				do
				{
					const auto& range = method_registers.current_draw_clause.get_range();
					const u32 idxFirst = range.first;
					const u32 idxCount = range.count;
					const u32 idxAddr  = base_addr + (idxFirst * type_size);

					const usz bufferSize = idxCount * type_size;

					frame_capture_data::memory_block block;
					block.offset = base_address + (idxFirst * type_size);
					block.location = memory_location;
					frame_capture_data::memory_block_data block_data;
					block_data.data.resize(bufferSize);
					std::memcpy(block_data.data.data(), vm::base(idxAddr), bufferSize);
					insert_mem_block_in_map(mem_indexer, mem_changes, std::move(block), std::move(block_data));

					switch (index_type)
					{
					case index_array_type::u16:
					{
						auto fifo = vm::ptr<u16>::make(idxAddr);
						for (u32 i = 0; i < idxCount; ++i)
						{
							u16 index = fifo[i];
							if (is_primitive_restart_enabled && u32{index} == primitive_restart_index)
								continue;
							index     = static_cast<u16>(get_index_from_base(index, method_registers.vertex_data_base_index()));
							min_index = std::min<u16>(index, static_cast<u16>(min_index));
							max_index = std::max<u16>(index, static_cast<u16>(max_index));
						}
						break;
					}
					case index_array_type::u32:
					{
						auto fifo = vm::ptr<u32>::make(idxAddr);
						for (u32 i = 0; i < idxCount; ++i)
						{
							u32 index = fifo[i];
							if (is_primitive_restart_enabled && index == primitive_restart_index)
								continue;
							index     = get_index_from_base(index, method_registers.vertex_data_base_index());
							min_index = std::min(index, min_index);
							max_index = std::max(index, max_index);
						}
						break;
					}
					}
				}
				while (method_registers.current_draw_clause.next());

				if (min_index <= max_index)
				{
					for (u8 index = 0; index < limits::vertex_count; ++index)
					{
						const bool enabled = !!(input_mask & (1 << index));
						if (!enabled)
							continue;

						const auto& info = method_registers.vertex_arrays_info[index];
						if (!info.size())
							continue;

						// vert buffer
						const u32 vertStride      = info.stride();
						const u32 base_address    = get_vertex_offset_from_base(method_registers.vertex_data_base_offset(), (info.offset() & 0x7fffffff));
						const u32 memory_location = info.offset() >> 31;

						const u32 addr     = get_address(base_address, memory_location);
						const u32 vertSize = get_vertex_type_size_on_host(info.type(), info.size());
						const u32 bufferSize = vertStride * (max_index - min_index + 1) + vertSize;

						frame_capture_data::memory_block block;
						block.offset = base_address + (min_index * vertStride);
						block.location = memory_location;
						frame_capture_data::memory_block_data block_data;
						block_data.data.resize(bufferSize);
						std::memcpy(block_data.data.data(), vm::base(addr + (min_index * vertStride)), bufferSize);
						insert_mem_block_in_map(mem_indexer, mem_changes, std::move(block), std::move(block_data));
					}
				}
			}

			capture_display_tile_state(rsx, frame_capture.replay_commands.back());
		}

		// i realize these are a slight copy pasta of the rsx_method implementations but its kinda unavoidable currently
		void capture_image_in(thread* rsx, frame_capture_data::replay_command& replay_command)
		{
			//const rsx::blit_engine::transfer_operation operation = method_registers.blit_engine_operation();

			const u16 clip_w = std::min(method_registers.blit_engine_output_width(), method_registers.blit_engine_clip_width());
			const u16 clip_h = std::min(method_registers.blit_engine_output_height(), method_registers.blit_engine_clip_height());

			const u16 in_w = method_registers.blit_engine_input_width();
			const u16 in_h = method_registers.blit_engine_input_height();

			//const blit_engine::transfer_origin in_origin                    = method_registers.blit_engine_input_origin();
			//const blit_engine::transfer_interpolator in_inter               = method_registers.blit_engine_input_inter();
			const rsx::blit_engine::transfer_source_format src_color_format = method_registers.blit_engine_src_color_format();

			const f32 in_x = std::floor(method_registers.blit_engine_in_x());
			const f32 in_y = std::floor(method_registers.blit_engine_in_y());

			u16 in_pitch = method_registers.blit_engine_input_pitch();

			if (in_w == 0 || in_h == 0 || clip_w == 0 || clip_h == 0)
			{
				return;
			}

			const u32 src_offset = method_registers.blit_engine_input_offset();
			const u32 src_dma    = method_registers.blit_engine_input_location();

			const u32 in_bpp              = (src_color_format == rsx::blit_engine::transfer_source_format::r5g6b5) ? 2 : 4; // bytes per pixel
			const u32 in_offset           = u32(in_x * in_bpp + in_pitch * in_y);

			frame_capture_data::memory_block block;
			block.offset = src_offset + in_offset;
			block.location = src_dma & 0xf;

			const auto src_address = rsx::get_address(block.offset, block.location);
			u8* pixels_src = vm::_ptr<u8>(src_address);

			const u32 src_size = in_pitch * (in_h - 1) + (in_w * in_bpp);
			rsx->read_barrier(src_address, src_size, true);

			frame_capture_data::memory_block_data block_data;
			block_data.data.resize(src_size);
			std::memcpy(block_data.data.data(), pixels_src, src_size);
			insert_mem_block_in_map(frame_capture.memory_indexer, replay_command.memory_state, std::move(block), std::move(block_data));

			capture_display_tile_state(rsx, replay_command);
		}

		void capture_buffer_notify(thread* rsx, frame_capture_data::replay_command& replay_command)
		{
			s32 in_pitch          = method_registers.nv0039_input_pitch();
			const u32 line_length = method_registers.nv0039_line_length();
			const u32 line_count  = method_registers.nv0039_line_count();
			//const u8 in_format    = method_registers.nv0039_input_format();

			u32 src_offset = method_registers.nv0039_input_offset();
			u32 src_dma    = method_registers.nv0039_input_location();
			u32 src_addr   = get_address(src_offset, src_dma);

			rsx->read_barrier(src_addr, in_pitch * (line_count - 1) + line_length, true);

			const u8* src = vm::_ptr<u8>(src_addr);

			frame_capture_data::memory_block block;
			block.offset = src_offset;
			block.location = src_dma;
			frame_capture_data::memory_block_data block_data;
			block_data.data.resize(in_pitch * (line_count - 1) + line_length);

			for (u32 i = 0; i < line_count; ++i)
			{
				std::memcpy(block_data.data.data() + (line_length * i), src, line_length);
				src += in_pitch;
			}

			insert_mem_block_in_map(frame_capture.memory_indexer, replay_command.memory_state, std::move(block), std::move(block_data));
			capture_display_tile_state(rsx, replay_command);
		}

		void capture_display_tile_state(thread* rsx, frame_capture_data::replay_command& replay_command)
		{
			u64& mem_indexer = frame_capture.memory_indexer;

			frame_capture_data::display_buffers_state dbstate;
			dbstate.count = rsx->display_buffers_count;

			// should this only happen on flip?
			for (u32 i = 0; i < rsx->display_buffers_count; ++i)
			{
				const auto& db            = rsx->display_buffers[i];
				dbstate.buffers[i].height = db.height;
				dbstate.buffers[i].width  = db.width;
				dbstate.buffers[i].offset = db.offset;
				dbstate.buffers[i].pitch  = db.pitch;
			}

			const auto [db_it, db_inserted] = frame_capture.display_buffers_map.try_emplace(dbstate, 0);
			u64& dbnum = db_it->second;

			if (db_inserted)
			{
				dbnum = ++mem_indexer;
			}

			// todo: hook tile call sys_rsx call or something
			frame_capture_data::tile_state tilestate;
			for (u32 i = 0; i < limits::tiles_count; ++i)
			{
				const auto tile = rsx->tiles[i].pack();
				auto& tstate = tilestate.tiles[i];
				tstate.tile = tile.tile;
				tstate.limit = tile.limit;
				tstate.pitch = rsx->tiles[i].bound ? u32{tile.pitch} : 0;
				tstate.format = rsx->tiles[i].bound ? u32{tile.format} : 0;
			}

			for (u32 i = 0; i < limits::zculls_count; ++i)
			{
				const auto zc = rsx->zculls[i].pack();
				auto& zcstate = tilestate.zculls[i];
				zcstate.region = zc.region;
				zcstate.size = zc.size;
				zcstate.start = zc.start;
				zcstate.offset = zc.offset;
				zcstate.status0 = rsx->zculls[i].bound ? u32{zc.status0} : 0;
				zcstate.status1 = rsx->zculls[i].bound ? u32{zc.status1} : 0;
			}

			const auto [ts_it, ts_inserted] = frame_capture.tile_map.try_emplace(tilestate, 0);
			u64& tsnum = ts_it->second;

			if (ts_inserted)
			{
				tsnum = ++mem_indexer;
			}

			replay_command.display_buffer_state = dbnum;
			replay_command.tile_state           = tsnum;
		}
	}
}
