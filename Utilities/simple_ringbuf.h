#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include <vector>

// Single reader/writer simple ringbuffer.
class simple_ringbuf
{
public:

	simple_ringbuf(u64 size = 0);
	virtual ~simple_ringbuf();

	simple_ringbuf(const simple_ringbuf& other);
	simple_ringbuf& operator=(const simple_ringbuf& other);

	// Thread unsafe functions.
	simple_ringbuf(simple_ringbuf&& other);
	simple_ringbuf& operator=(simple_ringbuf&& other);
	void set_buf_size(u64 size);

	// Helper functions
	u64 get_free_size() const;
	u64 get_used_size() const;
	u64 get_total_size() const;

	// Writer functions
	u64 push(const void* data, u64 size, bool force = false);
	void writer_flush(u64 cnt = umax);

	// Reader functions
	u64 pop(void* data, u64 size, bool force = false);
	void reader_flush(u64 cnt = umax);

private:

	struct ctr_state
	{
		alignas(sizeof(u64) * 2)
		u64 read_ptr = 0;
		u64 write_ptr = 0;

		auto operator<=>(const ctr_state& other) const = default;
	};

	static_assert(sizeof(ctr_state) == sizeof(u64) * 2);

	atomic_t<ctr_state> rw_ptr{};
	std::vector<u8> buf{};

	u64 get_free_size(ctr_state val) const;
	u64 get_used_size(ctr_state val) const;
};
