#pragma once

#include "Emu/CPU/CPUThread.h"
#include "Emu/RSX/rsx_methods.h"

#include <unordered_map>
#include <unordered_set>

namespace rsx
{
	enum : u32
	{
		c_fc_magic = "RRC"_u32,
		c_fc_version = 0x6,
	};

	struct frame_capture_data
	{
		struct memory_block_data
		{
			std::vector<u8> data{};
		};

		// simple block to hold ps3 address and data
		struct memory_block
		{
			ENABLE_BITWISE_SERIALIZATION;

			u32 offset; // Offset in rsx address space
			u32 location; // rsx memory location of the block
			u64 data_state;
		};

		struct replay_command
		{
			std::pair<u32, u32> rsx_command{};      // fifo command
			std::unordered_set<u64> memory_state{}; // index into memory_map for the various memory blocks that need applying before this command can run
			u64 tile_state{0};                      // tile state for this command
			u64 display_buffer_state{0};
		};

		struct tile_info
		{
			ENABLE_BITWISE_SERIALIZATION;

			u32 tile;
			u32 limit;
			u32 pitch;
			u32 format;
		};

		struct zcull_info
		{
			ENABLE_BITWISE_SERIALIZATION;

			u32 region;
			u32 size;
			u32 start;
			u32 offset;
			u32 status0;
			u32 status1;
		};

		// bleh, may need to break these out, might be unnecessary to do both always
		struct tile_state
		{
			ENABLE_BITWISE_SERIALIZATION;

			tile_info tiles[15]{};
			zcull_info zculls[8]{};
		};

		struct buffer_state
		{
			ENABLE_BITWISE_SERIALIZATION;

			u32 width{0};
			u32 height{0};
			u32 pitch{0};
			u32 offset{0};
		};

		struct display_buffers_state
		{
			ENABLE_BITWISE_SERIALIZATION;

			std::array<buffer_state, 8> buffers{};
			u32 count{0};
		};

		u32 magic = c_fc_magic;
		u32 version = c_fc_version;
		u32 LE_format = std::endian::little == std::endian::native;

		struct bitwise_hasher
		{
			template <typename T>
			usz operator()(const T& key) const noexcept
			{
				if constexpr (!!(requires (const T& a) { a.data[0]; }))
				{
					return std::hash<std::string_view>{}(std::string_view{ reinterpret_cast<const char*>(key.data.data()), key.data.size() * sizeof(key.data[0]) });
				}

				return std::hash<std::string_view>{}(std::string_view{ reinterpret_cast<const char*>(&key), sizeof(key) });
			}

			template <typename T>
			bool operator()(const T& keya, const T& keyb) const noexcept
			{
				if constexpr (!!(requires (const T& a) { a.data[0]; }))
				{
					if (keya.data.size() != keyb.data.size())
					{
						return false;
					}

					return std::equal(keya.data.begin(), keya.data.end(), keyb.data.begin());
				}

				return std::equal(reinterpret_cast<const char*>(&keya), reinterpret_cast<const char*>(&keya) + sizeof(T), reinterpret_cast<const char*>(&keyb));
			}
		};

		template <typename T, typename V>
		using uno_bit_map = std::unordered_map<T, V, bitwise_hasher, bitwise_hasher>;

		// Hashmap of holding various states for tile
		uno_bit_map<tile_state, u64> tile_map;

		// Hashmap of various memory 'changes' that can be applied to ps3 memory
		uno_bit_map<memory_block, u64> memory_map;

		// Hashmap of memory blocks that can be applied, this is split from above for size decrease
		uno_bit_map<memory_block_data, u64> memory_data_map;

		// Display buffer state map
		uno_bit_map<display_buffers_state, u64> display_buffers_map;

		// Actual command queue to hold everything above
		std::vector<replay_command> replay_commands;

		// Initial registers state at the beginning of the capture
		rsx::rsx_state reg_state;

		// Indexer for memory blocks
		u64 memory_indexer = 0x1234;

		void reset()
		{
			magic = c_fc_magic;
			version = c_fc_version;
			tile_map.clear();
			memory_map.clear();
			replay_commands.clear();
			reg_state = method_registers;
		}
	};


	class rsx_replay_thread : public cpu_thread
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
			frame_capture_data::display_buffers_state buffer_state{};
			frame_capture_data::tile_state tile_state{};
		};

		u32 user_mem_addr{};
		current_state cs{};
		std::unique_ptr<frame_capture_data> frame;

	public:
		rsx_replay_thread(std::unique_ptr<frame_capture_data>&& frame_data)
			: cpu_thread(0)
			, frame(std::move(frame_data))
		{
		}

		using cpu_thread::operator=;
		void cpu_task() override;
	private:
		be_t<u32> allocate_context();
		std::vector<u32> alloc_write_fifo(be_t<u32> context_id) const;
		void apply_frame_state(be_t<u32> context_id, const frame_capture_data::replay_command& replay_cmd);
	};
}
