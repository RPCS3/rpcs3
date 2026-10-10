#pragma once

#include "util/types.hpp"

#include <QWidget>

class guest_memory_dumper : QObject
{
	Q_OBJECT

public:
	guest_memory_dumper(QWidget* parent, bool delete_later = false);
	virtual ~guest_memory_dumper() {}

	void dump_guest_memory();

private:
	QWidget* m_parent = nullptr;
	bool m_delete_later = false;

	static constexpr u64 guest_address_space_size = 0x1'0000'0000ull;
	static constexpr u32 guest_page_size = 0x1000;
	static constexpr u64 dump_io_chunk_size = 4 * 1024 * 1024;

	struct guest_memory_region
	{
		u32 start = 0;
		u64 size = 0;
		u8 flags = 0;
	};

	struct spu_local_store_dump
	{
		u32 id = 0;
		u32 lv2_id = 0;
		u32 index = 0;
		u32 pc = 0;
		u32 vm_offset = 0;
		u32 type = 0;
		std::string name;
	};

	static bool emulated_processors_quiesced();
	static QString hex_u64(u64 value, int width = 8);
};
