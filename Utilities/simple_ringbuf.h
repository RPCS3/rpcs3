#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

// Single reader/writer simple ringbuffer.
class simple_ringbuf
{
public:

	using ctr_t = u64;

	simple_ringbuf(ctr_t size = 0);
	~simple_ringbuf();

	simple_ringbuf(const simple_ringbuf&);
	simple_ringbuf& operator=(const simple_ringbuf&);

	// Thread unsafe functions.
	simple_ringbuf(simple_ringbuf&& other);
	simple_ringbuf& operator=(simple_ringbuf&& other);
	void set_buf_size(ctr_t size);

	// Helper functions
	ctr_t get_free_size() const;
	ctr_t get_used_size() const;
	ctr_t get_total_size() const;

	// Writer functions
	ctr_t push(const void *data, ctr_t size, bool force = false);
	void writer_flush(ctr_t cnt = umax);

	// Reader functions
	ctr_t pop(void *data, ctr_t size, bool force = false);
	void reader_flush(ctr_t cnt = umax);

private:

	struct ctr_state
	{
		alignas(sizeof(ctr_t) * 2)
		ctr_t read_ptr = 0;
		ctr_t write_ptr = 0;

		auto operator<=>(const ctr_state&) const = default;
	};

	static_assert(sizeof(ctr_state) == sizeof(ctr_t) * 2);

	atomic_t<ctr_state> rw_ptr{};
	ctr_t buf_size = 0;
	std::unique_ptr<u8[]> buf{};

	ctr_t get_free_size(ctr_state val) const;
	ctr_t get_used_size(ctr_state val) const;
};
