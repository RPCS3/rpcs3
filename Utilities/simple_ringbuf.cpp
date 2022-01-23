#include "Utilities/simple_ringbuf.h"

simple_ringbuf::simple_ringbuf(simple_ringbuf::ctr_t size)
{
	set_buf_size(size);
}

simple_ringbuf::~simple_ringbuf()
{
	rw_ptr.load();
	buf = nullptr;
}

simple_ringbuf::simple_ringbuf(const simple_ringbuf& other)
{
	ctr_state old = other.rw_ptr.load();

	for (;;)
	{
		buf_size = other.buf_size;
		memcpy(buf.get(), other.buf.get(), buf_size);
		rw_ptr = old;

		const ctr_state current = other.rw_ptr.load();
		if (old == current)
		{
			break;
		}
		old = current;
	}
}

simple_ringbuf& simple_ringbuf::operator=(const simple_ringbuf& other)
{
	if (this == &other) return *this;

	ctr_state old = other.rw_ptr.load();

	for (;;)
	{
		buf_size = other.buf_size;
		memcpy(buf.get(), other.buf.get(), buf_size);
		rw_ptr = old;

		const ctr_state current = other.rw_ptr.load();
		if (old == current)
		{
			break;
		}
		old = current;
	}

	return *this;
}

simple_ringbuf::simple_ringbuf(simple_ringbuf&& other)
{
	const ctr_state other_rw_ptr = other.rw_ptr.load();
	buf_size = other.buf_size;
	buf = std::move(other.buf);
	rw_ptr = other_rw_ptr;

	other.buf_size = 0;
	other.rw_ptr.store({});
}

simple_ringbuf& simple_ringbuf::operator=(simple_ringbuf&& other)
{
	if (this == &other) return *this;

	const ctr_state other_rw_ptr = other.rw_ptr.load();
	buf_size = other.buf_size;
	buf = std::move(other.buf);
	rw_ptr = other_rw_ptr;

	other.buf_size = 0;
	other.rw_ptr.store({});

	return *this;
}

simple_ringbuf::ctr_t simple_ringbuf::get_free_size() const
{
	return get_free_size(rw_ptr);
}

simple_ringbuf::ctr_t simple_ringbuf::get_used_size() const
{
	return get_used_size(rw_ptr);
}

simple_ringbuf::ctr_t simple_ringbuf::get_total_size() const
{
	rw_ptr.load(); // Sync
	return buf_size - 1;
}

simple_ringbuf::ctr_t simple_ringbuf::get_free_size(ctr_state val) const
{
	const simple_ringbuf::ctr_t rd = val.read_ptr % buf_size;
	const simple_ringbuf::ctr_t wr = val.write_ptr % buf_size;

	return (wr >= rd ? buf_size + rd - wr : rd - wr) - 1;
}

simple_ringbuf::ctr_t simple_ringbuf::get_used_size(ctr_state val) const
{
	const simple_ringbuf::ctr_t rd = val.read_ptr % buf_size;
	const simple_ringbuf::ctr_t wr = val.write_ptr % buf_size;

	return wr >= rd ? wr - rd : buf_size + wr - rd;
}

void simple_ringbuf::set_buf_size(simple_ringbuf::ctr_t size)
{
	ensure(size != umax);

	buf_size = size + 1;
	buf = std::make_unique<u8[]>(buf_size);
	rw_ptr.store({});
}

void simple_ringbuf::writer_flush(ctr_t cnt)
{
	rw_ptr.atomic_op([&](ctr_state &val)
	{
		const ctr_t used = get_used_size(val);
		if (used == 0) return;

		val.write_ptr += buf_size - std::min<u64>(used, cnt);
	});
}

void simple_ringbuf::reader_flush(ctr_t cnt)
{
	rw_ptr.atomic_op([&](ctr_state &val)
	{
		val.read_ptr += std::min(get_used_size(val), cnt);
	});
}

simple_ringbuf::ctr_t simple_ringbuf::push(const void *data, simple_ringbuf::ctr_t size, bool force)
{
	ensure(data != nullptr);

	return rw_ptr.atomic_op([&](ctr_state &val) -> simple_ringbuf::ctr_t
	{
		const simple_ringbuf::ctr_t old       = val.write_ptr % buf_size;
		const simple_ringbuf::ctr_t free_size = get_free_size(val);
		const simple_ringbuf::ctr_t to_push   = std::min(size, free_size);
		const auto b_data                     = static_cast<const u8*>(data);

		if (!to_push || (!force && free_size < size))
		{
			return 0;
		}

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

		val.write_ptr += to_push;

		return to_push;
	});
}

simple_ringbuf::ctr_t simple_ringbuf::pop(void *data, simple_ringbuf::ctr_t size, bool force)
{
	ensure(data != nullptr);

	return rw_ptr.atomic_op([&](ctr_state &val) -> simple_ringbuf::ctr_t
	{
		const simple_ringbuf::ctr_t old       = val.read_ptr % buf_size;
		const simple_ringbuf::ctr_t used_size = get_used_size(val);
		const simple_ringbuf::ctr_t to_pop    = std::min(size, used_size);
		const auto b_data                     = static_cast<u8*>(data);

		if (!to_pop || (!force && used_size < size))
		{
			return 0;
		}

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

		val.read_ptr += to_pop;

		return to_pop;
	});
}
