#pragma once

#include "thread.h"
#include <common/defines.h>
#include <common/basic_types.h>

namespace rpcs3::cell::spu::raw
{
	using namespace common;

	namespace constants
	{
		enum : u32
		{
			offset = 0x00100000,
			base_address = 0xE0000000,
			ls_offset = 0x00000000,
			prob_offset = 0x00040000,
		};
	}

	force_inline static u32 get_reg_addr_by_num(int num, int offset)
	{
		return constants::offset * num + constants::base_address + constants::prob_offset + offset;
	}

	class thread final : public spu::thread
	{
	public:
		thread(const std::string& name, u32 index);
		virtual ~thread();

		bool read_reg(const u32 addr, u32& value);
		bool write_reg(const u32 addr, const u32 value);

	private:
		virtual void task() override;
	};

	std::shared_ptr<thread> get_thread(u32 id);
}