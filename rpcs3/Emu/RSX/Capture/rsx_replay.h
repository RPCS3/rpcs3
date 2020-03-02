#pragma once

#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/RSX/rsx_methods.h"

#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/unordered_set.hpp>

namespace rsx
{
	constexpr u32 FRAME_CAPTURE_MAGIC = 0x52524300; // ascii 'RRC/0'
	constexpr u32 FRAME_CAPTURE_VERSION = 0x4;
	struct frame_capture_data
	{
		struct memory_block_data
		{
			std::vector<u8> data;
			template<typename Archive>
			void serialize(Archive& ar)
			{
				ar(data);
			}
		};

		// simple block to hold ps3 address and data
		struct memory_block
		{
			u32 offset; // Offset in rsx address space
			u32 location; // rsx memory location of the block
			u64 data_state;

			template<typename Archive>
			void serialize(Archive & ar)
			{
				ar(offset);
				ar(location);
				ar(data_state);
			}
		};

		struct replay_command
		{
			std::pair<u32, u32> rsx_command;      // fifo command
			std::unordered_set<u64> memory_state; // index into memory_map for the various memory blocks that need applying before this command can run
			u64 tile_state{0};                    // tile state for this command
			u64 display_buffer_state{0};

			template<typename Archive>
			void serialize(Archive & ar)
			{
				ar(rsx_command);
				ar(memory_state);
				ar(tile_state);
				ar(display_buffer_state);
			}
		};

		struct tile_info
		{
			u32 tile;
			u32 limit;
			u32 pitch;
			u32 format;

			template<typename Archive>
			void serialize(Archive & ar)
			{
				ar(tile);
				ar(limit);
				ar(pitch);
				ar(format);
			}
		};

		struct zcull_info
		{
			u32 region;
			u32 size;
			u32 start;
			u32 offset;
			u32 status0;
			u32 status1;

			template<typename Archive>
			void serialize(Archive & ar)
			{
				ar(region);
				ar(size);
				ar(start);
				ar(offset);
				ar(status0);
				ar(status1);
			}
		};

		// bleh, may need to break these out, might be unnecessary to do both always
		struct tile_state
		{
			tile_info tiles[15];
			zcull_info zculls[8];

			template<typename Archive>
			void serialize(Archive & ar)
			{
				ar(tiles);
				ar(zculls);
			}
		};

		struct buffer_state
		{
			u32 width{0};
			u32 height{0};
			u32 pitch{0};
			u32 offset{0};

			template<typename Archive>
			void serialize(Archive & ar)
			{
				ar(width);
				ar(height);
				ar(pitch);
				ar(offset);
			}
		};

		struct display_buffers_state
		{
			std::array<buffer_state, 8> buffers;
			u32 count{0};

			template<typename Archive>
			void serialize(Archive & ar)
			{
				ar(buffers);
				ar(count);
			}
		};

		u32 magic;
		u32 version;
		// hashmap of holding various states for tile
		std::unordered_map<u64, tile_state> tile_map;
		// hashmap of various memory 'changes' that can be applied to ps3 memory
		std::unordered_map<u64, memory_block> memory_map;
		// hashmap of memory blocks that can be applied, this is split from above for size decrease
		std::unordered_map<u64, memory_block_data> memory_data_map;
		// display buffer state map
		std::unordered_map<u64, display_buffers_state> display_buffers_map;
		// actual command queue to hold everything above
		std::vector<replay_command> replay_commands;
		// Initial registers state at the beginning of the capture
		rsx::rsx_state reg_state;

		template<typename Archive>
		void serialize(Archive & ar)
		{
			ar(magic);
			ar(version);
			ar(tile_map);
			ar(memory_map);
			ar(memory_data_map);
			ar(display_buffers_map);
			ar(replay_commands);
			ar(reg_state);
		}

		void reset()
		{
			magic = FRAME_CAPTURE_MAGIC;
			version = FRAME_CAPTURE_VERSION;
			tile_map.clear();
			memory_map.clear();
			replay_commands.clear();
			reg_state = method_registers;
		}
	};


	class rsx_replay_thread
	{
		struct rsx_context
		{
			be_t<u32> user_addr;
			be_t<u64> dev_addr;
			be_t<u32> mem_handle;
			be_t<u32> context_id;
			be_t<u64> mem_addr;
			be_t<u64> dma_addr;
			be_t<u64> reports_addr;
			be_t<u64> driver_info;
		};

		struct current_state
		{
			u64 tile_hash{0};
			u64 display_buffer_hash{0};
			frame_capture_data::display_buffers_state buffer_state;
			frame_capture_data::tile_state tile_state;
		};

		u32 user_mem_addr;
		current_state cs;
		std::unique_ptr<frame_capture_data> frame;

	public:
		rsx_replay_thread(std::unique_ptr<frame_capture_data>&& frame_data)
			:frame(std::move(frame_data))
		{
		}

		void on_task();
		void operator()();
	private:
		be_t<u32> allocate_context();
		std::vector<u32> alloc_write_fifo(be_t<u32> context_id);
		void apply_frame_state(be_t<u32> context_id, const frame_capture_data::replay_command& replay_cmd);
	};
}
