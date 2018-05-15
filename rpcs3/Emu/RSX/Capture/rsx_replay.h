#pragma once

#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/lv2/sys_ppu_thread.h"

#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/unordered_set.hpp>

namespace rsx
{
	constexpr u32 FRAME_CAPTURE_MAGIC = 0x52524300; // ascii 'RRC/0'
	constexpr u32 FRAME_CAPTURE_VERSION = 0x1;
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
			u32 addr{0};
			u32 ioOffset{0xFFFFFFFF}; // rsx ioOffset, -1 signifies unused
			u32 offset{0};			  // offset into addr/ioOffset to copy state into
			u32 size{0};			  // size of block needed
			u64 data_state{0};		  // this can be 0, in which case its just needed as an alloc
			template<typename Archive>
			void serialize(Archive & ar)
			{
				ar(addr);
				ar(ioOffset);
				ar(offset);
				ar(size);
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

		// same thing as gcmtileinfo
		struct tile_info
		{
			u32 location{0};
			u32 offset{0};
			u32 size{0};
			u32 pitch{0};
			u32 comp{0};
			u32 base{0};
			u32 bank{0};
			bool binded{false};

			template<typename Archive>
			void serialize(Archive & ar)
			{
				ar(location);
				ar(offset);
				ar(size);
				ar(pitch);
				ar(comp);
				ar(base);
				ar(bank);
				ar(binded);
			}
		};

		// same thing as gcmzcullinfo
		struct zcull_info
		{
			u32 offset{0};
			u32 width{0};
			u32 height{0};
			u32 cullStart{0};
			u32 zFormat{0};
			u32 aaFormat{0};
			u32 zcullDir{0};
			u32 zcullFormat{0};
			u32 sFunc{0};
			u32 sRef{0};
			u32 sMask{0};
			bool binded{false};

			template<typename Archive>
			void serialize(Archive & ar)
			{
				ar(offset);
				ar(width);
				ar(cullStart);
				ar(zFormat);
				ar(aaFormat);
				ar(zcullDir);
				ar(zcullFormat);
				ar(sFunc);
				ar(sRef);
				ar(sMask);
				ar(binded);
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
		}

		void reset()
		{
			magic = FRAME_CAPTURE_MAGIC;
			version = FRAME_CAPTURE_VERSION;
			tile_map.clear();
			memory_map.clear();
			replay_commands.clear();
		}
	};


	class rsx_replay_thread : public ppu_thread
	{
		struct rsx_context
		{
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

		current_state cs;
		std::unique_ptr<frame_capture_data> frame;

	public:
		rsx_replay_thread(std::unique_ptr<frame_capture_data>&& frame_data)
			: ppu_thread("Rsx Capture Replay Thread"), frame(std::move(frame_data)) {};

		virtual void cpu_task() override;
	private:
		be_t<u32> allocate_context();
		std::tuple<u32, u32> get_usable_fifo_range();
		std::vector<u32> alloc_write_fifo(be_t<u32> context_id, u32 fifo_start_addr, u32 fifo_size);
		void apply_frame_state(be_t<u32> context_id, const frame_capture_data::replay_command& replay_cmd);
	};
}
