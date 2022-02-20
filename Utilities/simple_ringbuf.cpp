#include "Utilities/simple_ringbuf.h"

simple_ringbuf::simple_ringbuf(u32 size)
{
	set_buf_size(size);
}

simple_ringbuf::simple_ringbuf(simple_ringbuf&& other)
{
	rw_ptr = other.rw_ptr.load();
	buf_size = other.buf_size;
	buf = std::move(other.buf);
	initialized = other.initialized.observe();

	other.buf_size = 0;
	other.rw_ptr = 0;
	other.initialized = false;
}

simple_ringbuf& simple_ringbuf::operator=(simple_ringbuf&& other)
{
	if (this == &other) return *this;

	rw_ptr = other.rw_ptr.load();
	buf_size = other.buf_size;
	buf = std::move(other.buf);
	initialized = other.initialized.observe();

	other.buf_size = 0;
	other.rw_ptr = 0;
	other.initialized = false;

	return *this;
}

u32 simple_ringbuf::get_free_size() const
{
	const u64 _rw_ptr = rw_ptr;
	const u32 rd = static_cast<u32>(_rw_ptr);
	const u32 wr = static_cast<u32>(_rw_ptr >> 32);

	return wr >= rd ? buf_size - 1 - (wr - rd) : rd - wr - 1U;
}

u32 simple_ringbuf::get_used_size() const
{
	return buf_size - 1 - get_free_size();
}

u32 simple_ringbuf::get_total_size() const
{
	return buf_size;
}

void simple_ringbuf::set_buf_size(u32 size)
{
	ensure(size);

	this->buf_size = size + 1;
	buf = std::make_unique<u8[]>(this->buf_size);
	flush();
	initialized = true;
}

void simple_ringbuf::flush()
{
	rw_ptr.atomic_op([&](u64 &val)
	{
		val = static_cast<u32>(val >> 32) | (val & 0xFFFFFFFF'00000000);
	});
}

u32 simple_ringbuf::push(const void *data, u32 size)
{
	ensure(data != nullptr && initialized.observe());

	const u32 old     = static_cast<u32>(rw_ptr.load() >> 32);
	const u32 to_push = std::min(size, get_free_size());
	auto b_data       = static_cast<const u8*>(data);

	if (!to_push) return 0;

	if (old + to_push > buf_size)
	{
		const auto first_write_sz = buf_size - old;
		memcpy(&buf[old], b_data, first_write_sz);
		memcpy(&buf[0], b_data + first_write_sz, to_push - first_write_sz);
	}
	else
	{
		memcpy(&buf[old], b_data, to_push);
	}

	rw_ptr.atomic_op([&](u64 &val)
	{
		val = static_cast<u64>((old + to_push) % buf_size) << 32 | static_cast<u32>(val);
	});

	return to_push;
}

u32 simple_ringbuf::pop(void *data, u32 size)
{
	ensure(data != nullptr && initialized.observe());

	const u32 old    = static_cast<u32>(rw_ptr.load());
	const u32 to_pop = std::min(size, get_used_size());
	u8 *b_data       = static_cast<u8*>(data);

	if (!to_pop) return 0;

	if (old + to_pop > buf_size)
	{
		const auto first_read_sz = buf_size - old;
		memcpy(b_data, &buf[old], first_read_sz);
		memcpy(b_data + first_read_sz, &buf[0], to_pop - first_read_sz);
	}
	else
	{
		memcpy(b_data, &buf[old], to_pop);
	}

	rw_ptr.atomic_op([&](u64 &val)
	{
		val = (old + to_pop) % buf_size | (val & 0xFFFFFFFF'00000000);
	});

	return to_pop;
}
