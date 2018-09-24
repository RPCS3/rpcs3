#include "stdafx.h"
#include "rsx_capture.h"
#include "Emu/RSX/Common/BufferUtils.h"
#include "Emu/RSX/Common/TextureUtils.h"
#include "Emu/RSX/Common/surface_store.h"
#include "Emu/RSX/GCM.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/Memory/vm.h"

#include "xxhash.h"

namespace rsx
{
	namespace capture
	{
		u32 get_io_offset(u32 offset, u32 location)
		{
			switch (location)
			{
			case CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER:
			case CELL_GCM_LOCATION_MAIN:
			{
				if (u32 result = RSXIOMem.RealAddr(offset))
				{
					return offset;
				}
			}
			case CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_MAIN:
			{
				if (u32 result = RSXIOMem.RealAddr(0x0e000000 + offset))
				{
					return 0x0e000000 + offset;
				}
			}
			default: return 0xFFFFFFFFu;
			}
		}

		void insert_mem_block_in_map(std::unordered_set<u64>& mem_changes, frame_capture_data::memory_block&& block, frame_capture_data::memory_block_data&& data)
		{
			u64 data_hash = 0;
			if (data.data.size() > 0)
			{
				data_hash = XXH64(data.data.data(), data.data.size(), 0);
				// using 0 to signify no block in use, so this one is 'reserved'
				if (data_hash == 0)
					fmt::throw_exception("Memory block data hash equal to 0");

				block.size       = data.data.size();
				block.data_state = data_hash;

				auto it = frame_capture.memory_data_map.find(data_hash);
				if (it != frame_capture.memory_data_map.end())
				{
					if (it->second.data != data.data)
						// screw this
						fmt::throw_exception("Memory map hash collision detected...cant capture");
				}
				else
					frame_capture.memory_data_map.insert(std::make_pair(data_hash, std::move(data)));
			}

			u64 block_hash = XXH64(&block, sizeof(frame_capture_data::memory_block), 0);
			mem_changes.insert(block_hash);
			if (frame_capture.memory_map.find(block_hash) == frame_capture.memory_map.end())
				frame_capture.memory_map.insert(std::make_pair(block_hash, std::move(block)));
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

			// capture fragment shader mem
			const u32 shader_program = method_registers.shader_program_address();

			const u32 program_location = (shader_program & 0x3) - 1;
			const u32 program_offset   = (shader_program & ~0x3);

			const u32 addr          = get_address(program_offset, program_location);
			const auto program_info = program_hash_util::fragment_program_utils::analyse_fragment_program(vm::base(addr));
			const u32 program_start = program_info.program_start_offset;
			const u32 ucode_size    = program_info.program_ucode_length;

			frame_capture_data::memory_block block;
			block.ioOffset = program_offset, 
			block.location = program_location;
			frame_capture_data::memory_block_data block_data;
			block_data.data.resize(ucode_size + program_start);
			std::memcpy(block_data.data.data(), vm::base(addr), ucode_size + program_start);
			insert_mem_block_in_map(mem_changes, std::move(block), std::move(block_data));

			// vertex shader is passed in registers, so it can be ignored

			// save fragment tex mem
			for (const auto& tex : method_registers.fragment_textures)
			{
				if (!tex.enabled())
					continue;

				const u32 texaddr = get_address(tex.offset(), tex.location());
				auto layout       = get_subresources_layout(tex);

				// todo: dont use this function and just get size somehow
				size_t texSize = 0;
				for (const auto& l : layout)
					texSize += l.data.size();

				if (!texSize)
					continue;

				frame_capture_data::memory_block block;
				block.ioOffset = tex.offset();
				block.location = tex.location();

				frame_capture_data::memory_block_data block_data;
				block_data.data.resize(texSize);
				std::memcpy(block_data.data.data(), vm::base(texaddr), texSize);
				insert_mem_block_in_map(mem_changes, std::move(block), std::move(block_data));
			}

			// save vertex texture mem
			for (const auto& tex : method_registers.vertex_textures)
			{
				if (!tex.enabled())
					continue;

				const u32 texaddr = get_address(tex.offset(), tex.location());
				auto layout       = get_subresources_layout(tex);

				// todo: dont use this function and just get size somehow
				size_t texSize = 0;
				for (const auto& l : layout)
					texSize += l.data.size();

				if (!texSize)
					continue;

				frame_capture_data::memory_block block;
				block.ioOffset = tex.offset();
				block.location = tex.location();
				frame_capture_data::memory_block_data block_data;
				block_data.data.resize(texSize);
				std::memcpy(block_data.data.data(), vm::base(texaddr), texSize);
				insert_mem_block_in_map(mem_changes, std::move(block), std::move(block_data));
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

					for (const auto& range : method_registers.current_draw_clause.draw_command_ranges)
					{
						const u32 vertCount     = range.count;
						const size_t bufferSize = vertCount * vertStride + vertSize;

						frame_capture_data::memory_block block;
						block.ioOffset = base_address;
						block.location = memory_location;
						block.offset   = (range.first * vertStride);
						frame_capture_data::memory_block_data block_data;
						block_data.data.resize(bufferSize);
						std::memcpy(block_data.data.data(), vm::base(addr + block.offset), bufferSize);
						insert_mem_block_in_map(mem_changes, std::move(block), std::move(block_data));
					}
				}
			}
			// save index buffer if used
			else if (method_registers.current_draw_clause.command == draw_command::indexed)
			{
				const u32 input_mask = method_registers.vertex_attrib_input_mask();

				const u32 base_address    = method_registers.index_array_address();
				const u32 memory_location = method_registers.index_array_location();

				const u32 base_addr   = get_address(base_address, memory_location);
				const u32 type_size   = get_index_type_size(method_registers.index_type());
				const auto index_type = method_registers.index_type();

				// manually parse index buffer and copy vertex buffer
				u32 min_index = 0xFFFF, max_index = 0;
				if (index_type == index_array_type::u32)
					min_index = 0xFFFFFFFF;

				const bool is_primitive_restart_enabled = method_registers.restart_index_enabled();
				const u32 primitive_restart_index       = method_registers.restart_index();

				for (const auto& range : method_registers.current_draw_clause.draw_command_ranges)
				{
					const u32 idxFirst = range.first;
					const u32 idxCount = range.count;
					const u32 idxAddr  = base_addr + (idxFirst * type_size);

					const size_t bufferSize = idxCount * type_size;

					frame_capture_data::memory_block block;
					block.ioOffset = base_address;
					block.location = memory_location;
					block.offset   = (idxFirst * type_size);

					frame_capture_data::memory_block_data block_data;
					block_data.data.resize(bufferSize);
					std::memcpy(block_data.data.data(), vm::base(idxAddr), bufferSize);
					insert_mem_block_in_map(mem_changes, std::move(block), std::move(block_data));

					switch (index_type)
					{
					case index_array_type::u16:
					{
						auto fifo = vm::ptr<u16>::make(idxAddr);
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

				if (min_index > max_index)
				{
					// ignore?
				}

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
					const u32 base_address    = get_vertex_offset_from_base(method_registers.vertex_data_base_offset(), info.offset() & 0x7fffffff);
					const u32 memory_location = info.offset() >> 31;

					const u32 addr     = get_address(base_address, memory_location);
					const u32 vertSize = get_vertex_type_size_on_host(info.type(), info.size());

					const u32 bufferSize = vertStride * (max_index - min_index + 1) + vertSize;

					frame_capture_data::memory_block block;
					block.ioOffset = base_address;
					block.location = memory_location;
					block.offset   = (min_index * vertStride);

					frame_capture_data::memory_block_data block_data;
					block_data.data.resize(bufferSize);
					std::memcpy(block_data.data.data(), vm::base(addr + block.offset), bufferSize);
					insert_mem_block_in_map(mem_changes, std::move(block), std::move(block_data));
				}
			}

			capture_display_tile_state(rsx, frame_capture.replay_commands.back());
			capture_surface_state(rsx, frame_capture.replay_commands.back());
		}

		// i realize these are a slight copy pasta of the rsx_method implementations but its kinda unavoidable currently
		void capture_image_in(thread* rsx, frame_capture_data::replay_command& replay_command)
		{
			const rsx::blit_engine::transfer_operation operation = method_registers.blit_engine_operation();

			const u16 out_x = method_registers.blit_engine_output_x();
			const u16 out_y = method_registers.blit_engine_output_y();
			const u16 out_w = method_registers.blit_engine_output_width();
			const u16 out_h = method_registers.blit_engine_output_height();

			const u16 in_w = method_registers.blit_engine_input_width();
			const u16 in_h = method_registers.blit_engine_input_height();

			const blit_engine::transfer_origin in_origin                    = method_registers.blit_engine_input_origin();
			const blit_engine::transfer_interpolator in_inter               = method_registers.blit_engine_input_inter();
			const rsx::blit_engine::transfer_source_format src_color_format = method_registers.blit_engine_src_color_format();

			const f32 in_x = std::ceil(method_registers.blit_engine_in_x());
			const f32 in_y = std::ceil(method_registers.blit_engine_in_y());

			u16 in_pitch = method_registers.blit_engine_input_pitch();

			if (in_w == 0 || in_h == 0 || out_w == 0 || out_h == 0)
			{
				return;
			}

			const u32 src_offset = method_registers.blit_engine_input_offset();
			const u32 src_dma    = method_registers.blit_engine_input_location();

			const u32 in_bpp              = (src_color_format == rsx::blit_engine::transfer_source_format::r5g6b5) ? 2 : 4; // bytes per pixel
			const u32 in_offset           = u32(in_x * in_bpp + in_pitch * in_y);
			const tiled_region src_region = rsx->get_tiled_address(src_offset + in_offset, src_dma & 0xf);

			frame_capture_data::memory_block block;
			block.ioOffset   = src_region.tile ? src_region.base : src_offset + in_offset;
			block.location = src_dma & 0xf;

			u8* pixels_src = src_region.tile ? src_region.ptr + src_region.base : src_region.ptr;

			if (in_pitch == 0)
			{
				in_pitch = in_bpp * in_w;
			}

			rsx->read_barrier(src_region.address, in_pitch * in_h);

			frame_capture_data::memory_block_data block_data;
			block_data.data.resize(in_pitch * in_h);
			std::memcpy(block_data.data.data(), pixels_src, in_pitch * in_h);
			insert_mem_block_in_map(replay_command.memory_state, std::move(block), std::move(block_data));

			// 'capture' destination to ensure memory is alloc'd and usable in replay
			u32 dst_offset = 0;
			u32 dst_dma    = 0;
			rsx::blit_engine::transfer_destination_format dst_color_format;
			u32 out_pitch    = 0;
			u32 out_alignment = 64;

			switch (method_registers.blit_engine_context_surface())
			{
			case blit_engine::context_surface::surface2d:
				dst_dma          = method_registers.blit_engine_output_location_nv3062();
				dst_offset       = method_registers.blit_engine_output_offset_nv3062();
				dst_color_format = method_registers.blit_engine_nv3062_color_format();
				out_pitch        = method_registers.blit_engine_output_pitch_nv3062();
				out_alignment     = method_registers.blit_engine_output_alignment_nv3062();
				break;

			case blit_engine::context_surface::swizzle2d:
				dst_dma          = method_registers.blit_engine_nv309E_location();
				dst_offset       = method_registers.blit_engine_nv309E_offset();
				dst_color_format = method_registers.blit_engine_output_format_nv309E();
				break;

			default: break;
			}

			const u32 out_bpp             = (dst_color_format == rsx::blit_engine::transfer_destination_format::r5g6b5) ? 2 : 4;
			const s32 out_offset          = out_x * out_bpp + out_pitch * out_y;
			const tiled_region dst_region = rsx->get_tiled_address(dst_offset + out_offset, dst_dma & 0xf);

			frame_capture_data::memory_block blockDst;
			blockDst.ioOffset = dst_region.tile ? dst_region.base : dst_offset + out_offset;
			blockDst.location = dst_dma & 0xf;
			if (get_io_offset(blockDst.ioOffset, blockDst.location) != -1)
			{
				u32 blockSize = method_registers.blit_engine_context_surface() != blit_engine::context_surface::swizzle2d ? out_pitch * out_h : out_bpp * next_pow2(out_w) * next_pow2(out_h);

				blockDst.size = blockSize;
				frame_capture_data::memory_block_data block_data;
				insert_mem_block_in_map(replay_command.memory_state, std::move(blockDst), std::move(block_data));
			}

			capture_display_tile_state(rsx, replay_command);
		}

		void capture_buffer_notify(thread* rsx, frame_capture_data::replay_command& replay_command)
		{
			s32 in_pitch          = method_registers.nv0039_input_pitch();
			const u32 line_length = method_registers.nv0039_line_length();
			const u32 line_count  = method_registers.nv0039_line_count();
			const u8 in_format    = method_registers.nv0039_input_format();

			if (!in_pitch)
			{
				in_pitch = line_length;
			}

			u32 src_offset = method_registers.nv0039_input_offset();
			u32 src_dma    = method_registers.nv0039_input_location();
			u32 src_addr   = get_address(src_offset, src_dma);

			rsx->read_barrier(src_addr, in_pitch * line_count);

			const u8* src = (u8*)vm::base(src_addr);

			frame_capture_data::memory_block block;
			block.ioOffset = src_offset;
			block.location = src_dma;

			frame_capture_data::memory_block_data block_data;
			block_data.data.resize(in_pitch * line_count);

			for (u32 i = 0; i < line_count; ++i)
			{
				std::memcpy(block_data.data.data() + (line_length * i), src, line_length);
				src += in_pitch;
			}

			insert_mem_block_in_map(replay_command.memory_state, std::move(block), std::move(block_data));

			// we 'capture' destination mostly to ensure that the location is allocated when replaying
			u32 dst_offset = method_registers.nv0039_output_offset();
			u32 dst_dma    = method_registers.nv0039_output_location();
			u32 dst_addr   = get_address(dst_offset, dst_dma);

			s32 out_pitch = method_registers.nv0039_output_pitch();
			if (!out_pitch)
			{
				out_pitch = line_length;
			}

			frame_capture_data::memory_block blockDst;
			blockDst.ioOffset = dst_offset;
			blockDst.location = dst_dma;

			// check if allocated
			if (get_io_offset(blockDst.ioOffset, blockDst.location) != -1)
			{
				frame_capture_data::memory_block_data block_data;
				blockDst.size = out_pitch * line_count;
				insert_mem_block_in_map(replay_command.memory_state, std::move(blockDst), std::move(block_data));
			}

			capture_display_tile_state(rsx, replay_command);
		}

		void capture_display_tile_state(thread* rsx, frame_capture_data::replay_command& replay_command)
		{
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

			const u64 dbnum = XXH64(&dbstate, sizeof(frame_capture_data::display_buffers_state), 0);
			if (frame_capture.display_buffers_map.find(dbnum) == frame_capture.display_buffers_map.end())
				frame_capture.display_buffers_map.insert(std::make_pair(dbnum, std::move(dbstate)));

			// todo: hook tile call sys_rsx call or something
			frame_capture_data::tile_state tilestate;
			for (u32 i = 0; i < limits::tiles_count; ++i)
			{
				const auto& tile = rsx->tiles[i];
				auto& tstate     = tilestate.tiles[i];
				tstate.bank      = tile.bank;
				tstate.base      = tile.base;
				tstate.binded    = tile.binded;
				tstate.comp      = tile.comp;
				tstate.location  = tile.location;
				tstate.offset    = tile.offset;
				tstate.pitch     = tile.pitch;
				tstate.size      = tile.size;
			}

			for (u32 i = 0; i < limits::zculls_count; ++i)
			{
				const auto& zc      = rsx->zculls[i];
				auto& zcstate       = tilestate.zculls[i];
				zcstate.aaFormat    = zc.aaFormat;
				zcstate.binded      = zc.binded;
				zcstate.cullStart   = zc.cullStart;
				zcstate.height      = zc.height;
				zcstate.offset      = zc.offset;
				zcstate.sFunc       = zc.sFunc;
				zcstate.sMask       = zc.sMask;
				zcstate.sRef        = zc.sRef;
				zcstate.width       = zc.width;
				zcstate.zcullDir    = zc.zcullDir;
				zcstate.zcullFormat = zc.zcullFormat;
				zcstate.zFormat     = zc.zFormat;
			}

			const u64 tsnum = XXH64(&tilestate, sizeof(frame_capture_data::tile_state), 0);

			if (frame_capture.tile_map.find(tsnum) == frame_capture.tile_map.end())
				frame_capture.tile_map.insert(std::make_pair(tsnum, std::move(tilestate)));

			replay_command.display_buffer_state = dbnum;
			replay_command.tile_state           = tsnum;
		}

		// for the most part capturing this is just to make sure the iomemory is recorded/allocated and accounted for in replay
		void capture_get_report(thread* rsx, frame_capture_data::replay_command& replay_command, u32 arg)
		{
			u32 location = 0;
			u32 offset   = arg & 0xffffff;

			blit_engine::context_dma report_dma = method_registers.context_dma_report();

			switch (report_dma)
			{
			// ignore regular report location
			case blit_engine::context_dma::to_memory_get_report: location = CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_LOCAL; return;
			case blit_engine::context_dma::report_location_main: location = CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_MAIN; break;
			case blit_engine::context_dma::memory_host_buffer: location = CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER; break;
			default: return;
			}

			u32 addr = get_address(offset, location);

			frame_capture_data::memory_block block;
			block.ioOffset = offset;
			block.location = location;
			block.size     = 16;

			frame_capture_data::memory_block_data block_data;
			
			insert_mem_block_in_map(replay_command.memory_state, std::move(block), std::move(block_data));
		}

		// This just checks the color/depth addresses and makes sure they are accounted for in io allocations
		// Hopefully this works without fully having to calculate actual size
		void capture_surface_state(thread* rsx, frame_capture_data::replay_command& replay_command)
		{
			const auto target = rsx::method_registers.surface_color_target();

			u32 offset_color[] =
			{
			    rsx::method_registers.surface_a_offset(),
			    rsx::method_registers.surface_b_offset(),
			    rsx::method_registers.surface_c_offset(),
			    rsx::method_registers.surface_d_offset(),
			};

			u32 context_dma_color[] =
			{
			    rsx::method_registers.surface_a_dma(),
			    rsx::method_registers.surface_b_dma(),
			    rsx::method_registers.surface_c_dma(),
			    rsx::method_registers.surface_d_dma(),
			};

			auto check_add = [&replay_command](u32 offset, u32 dma) -> void
			{
				u32 ioOffset = get_io_offset(offset, dma);
				if (ioOffset == -1)
					return;

				frame_capture_data::memory_block block;
				block.ioOffset = offset;
				block.location = dma;
				block.size     = 64;

				frame_capture_data::memory_block_data block_data;
				insert_mem_block_in_map(replay_command.memory_state, std::move(block), std::move(block_data));
			};

			for (const auto& index : utility::get_rtt_indexes(target))
			{
				check_add(offset_color[index], context_dma_color[index]);
			}

			check_add(rsx::method_registers.surface_z_offset(), rsx::method_registers.surface_z_dma());
		}

		void capture_inline_transfer(thread* rsx, frame_capture_data::replay_command& replay_command, u32 idx, u32 arg)
		{
			const u16 x = method_registers.nv308a_x();
			const u16 y = method_registers.nv308a_y();

			const u32 pixel_offset = (method_registers.blit_engine_output_pitch_nv3062() * y) + (x << 2);
			const u32 addr_offset  = method_registers.blit_engine_output_offset_nv3062() + pixel_offset + idx * 4;

			// just need to capture dst for allocation later if in iomem

			const u32 memory_location = method_registers.blit_engine_output_location_nv3062();

			const u32 ioOffset = get_io_offset(addr_offset, memory_location);
			if (ioOffset == -1)
				return;

			frame_capture_data::memory_block block;
			block.ioOffset   = addr_offset;
			block.location = memory_location;
			block.size     = 4;

			frame_capture_data::memory_block_data block_data;
			insert_mem_block_in_map(replay_command.memory_state, std::move(block), std::move(block_data));
		}
	}
}
