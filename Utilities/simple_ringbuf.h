#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

// Single reader/writer simple ringbuffer.
// Counters are 32-bit.
class simple_ringbuf
{
private:

	atomic_t<u64> rw_ptr = 0;
	u32 buf_size = 0;
	std::unique_ptr<u8[]> buf{};
	atomic_t<bool> initialized = false;

public:

	simple_ringbuf() {};
	simple_ringbuf(u32 size);

	simple_ringbuf(const simple_ringbuf&) = delete;
	simple_ringbuf& operator=(const simple_ringbuf&) = delete;

	simple_ringbuf(simple_ringbuf&& other);
	simple_ringbuf& operator=(simple_ringbuf&& other);

	u32 get_free_size();
	u32 get_used_size();
	u32 get_total_size();

	// Thread unsafe functions.
	void set_buf_size(u32 size);
	void flush(); // Could be safely called from reader.

	u32 push(const void *data, u32 size);
	u32 pop(void *data, u32 size);
};
