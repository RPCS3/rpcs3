#include "util/types.hpp"
#include "util/logs.hpp"
#include "util/asm.hpp"
#include "util/sysinfo.hpp"
#include "util/endian.hpp"
#include "Utilities/lockless.h"
#include "Utilities/File.h"
#include "Utilities/StrFmt.h"
#include "serialization_ext.hpp"

#include <zlib.h>
#include <zstd.h>

LOG_CHANNEL(sys_log, "SYS");

template <>
void fmt_class_string<utils::serial>::format(std::string& out, u64 arg)
{
	const utils::serial& ar = get_object(arg);

	be_t<u64> sample64 = 0;
	const usz read_size = std::min<usz>(ar.data.size(), sizeof(sample64));
	std::memcpy(&sample64, ar.data.data() + ar.data.size() - read_size, read_size);

	fmt::append(out, "{ %s, 0x%x/0x%x, memory=0x%x, sample64=0x%016x }", ar.is_writing() ? "writing" : "reading", ar.pos, ar.data_offset + ar.data.size(), ar.data.size(), sample64);
}

static constexpr uInt adjust_for_uint(usz size)
{
	return static_cast<uInt>(std::min<usz>(uInt{umax}, size));
}

bool uncompressed_serialization_file_handler::handle_file_op(utils::serial& ar, usz pos, usz size, const void* data)
{
	if (ar.is_writing())
	{
		if (data)
		{
			m_file->seek(pos);
			m_file->write(data, size);
			return true;
		}

		m_file->seek(ar.data_offset);
		m_file->write(ar.data);

		if (pos == umax && size == umax)
		{
			// Request to flush the file to disk
			m_file->sync();
		}

		ar.seek_end();
		ar.data_offset = ar.pos;
		ar.data.clear();
		return true;
	}

	if (!size)
	{
		return true;
	}

	if (pos == 0 && size == umax)
	{
		// Discard loaded data until pos if profitable
		const usz limit = ar.data_offset + ar.data.size();

		if (ar.pos > ar.data_offset && ar.pos < limit)
		{
			const usz may_discard_bytes = ar.pos - ar.data_offset;
			const usz moved_byte_count_on_discard = limit - ar.pos;

			// Cheeck profitability (check recycled memory and std::memmove costs)
			if (may_discard_bytes >= 0x50'0000 || (may_discard_bytes >= 0x20'0000 && moved_byte_count_on_discard / may_discard_bytes < 3))
			{
				ar.data_offset += may_discard_bytes;
				ar.data.erase(ar.data.begin(), ar.data.begin() + may_discard_bytes);

				if (ar.data.capacity() >= 0x200'0000)
				{
					// Discard memory
					ar.data.shrink_to_fit();
				}
			}

			return true;
		}

		// Discard all loaded data
		ar.data_offset = ar.pos;
		ar.data.clear();

		if (ar.data.capacity() >= 0x200'0000)
		{
			// Discard memory
			ar.data.shrink_to_fit();
		}

		return true;
	}

	if (~pos < size - 1)
	{
		// Overflow
		return false;
	}

	if (ar.data.empty())
	{
		// Relocate instead of over-fetch
		ar.data_offset = pos;
	}

	const usz read_pre_buffer = ar.data.empty() ? 0 : utils::sub_saturate<usz>(ar.data_offset, pos);

	if (read_pre_buffer)
	{
		// Read past data
		// Harsh operation on performance, luckily rare and not typically needed
		// Also this may would be disallowed when moving to compressed files
		// This may be a result of wrong usage of breathe() function
		ar.data.resize(ar.data.size() + read_pre_buffer);
		std::memmove(ar.data.data() + read_pre_buffer, ar.data.data(), ar.data.size() - read_pre_buffer);
		ensure(m_file->read_at(pos, ar.data.data(), read_pre_buffer) == read_pre_buffer);
		ar.data_offset -= read_pre_buffer;
	}

	// Adjustment to prevent overflow
	const usz subtrahend = ar.data.empty() ? 0 : 1;
	const usz read_past_buffer = utils::sub_saturate<usz>(pos + (size - subtrahend), ar.data_offset + (ar.data.size() - subtrahend));
	const usz read_limit = utils::sub_saturate<usz>(ar.m_max_data, ar.data_offset); 

	if (read_past_buffer)
	{
		// Read proceeding data
		// More lightweight operation, this is the common operation
		// Allowed to fail, if memory is truly needed an assert would take place later
		const usz old_size = ar.data.size();

		// Try to prefetch data by reading more than requested
		ar.data.resize(std::min<usz>(read_limit, std::max<usz>({ ar.data.capacity(), ar.data.size() + read_past_buffer * 3 / 2, ar.expect_little_data() ? usz{4096} : usz{0x10'0000} })));
		ar.data.resize(m_file->read_at(old_size + ar.data_offset, data ? const_cast<void*>(data) : ar.data.data() + old_size, ar.data.size() - old_size) + old_size);
	}

	return true;
}

usz uncompressed_serialization_file_handler::get_size(const utils::serial& ar, usz recommended) const
{
	if (ar.is_writing())
	{
		return *m_file ? m_file->size() : 0;
	}

	const usz memory_available = ar.data_offset + ar.data.size();

	if (memory_available >= recommended || !*m_file)
	{
		// Avoid calling size() if possible
		return memory_available;
	}

	return std::max<usz>(m_file->size(), memory_available);
}

void uncompressed_serialization_file_handler::finalize(utils::serial& ar)
{
	ar.seek_end();
	handle_file_op(ar, 0, umax, nullptr);
	ar.data = {}; // Deallocate and clear
}

enum : u64
{
	pending_compress_bytes_bound = 0x400'0000,
	pending_data_wait_bit = 1ull << 63,
};

struct compressed_stream_data
{
	z_stream m_zs{};
	lf_queue<std::vector<u8>> m_queued_data_to_process;
	lf_queue<std::vector<u8>> m_queued_data_to_write;
};

void compressed_serialization_file_handler::initialize(utils::serial& ar)
{
	if (!m_stream)
	{
		m_stream = std::make_shared<compressed_stream_data>();
	}

	if (ar.is_writing())
	{
		if (m_write_inited)
		{
			return;
		}

		z_stream& m_zs = m_stream->m_zs;
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
		if (m_read_inited)
		{
			finalize(ar);
		}

		m_zs = {};
		ensure(deflateInit2(&m_zs, ar.expect_little_data() ? 9 : 8, Z_DEFLATED, 16 + 15, 9, Z_DEFAULT_STRATEGY) == Z_OK);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
		m_write_inited = true;
		m_errored = false;

		if (!ar.expect_little_data())
		{
			m_stream_data_prepare_thread = std::make_unique<named_thread<std::function<void()>>>("CompressedPrepare Thread"sv, [this]() { this->stream_data_prepare_thread_op(); });
			m_file_writer_thread = std::make_unique<named_thread<std::function<void()>>>("CompressedWriter Thread"sv, [this]() { this->file_writer_thread_op(); });
		}
	}
	else
	{
		if (m_read_inited)
		{
			return;
		}

		if (m_write_inited)
		{
			finalize(ar);
		}

		z_stream& m_zs = m_stream->m_zs;

		m_zs.avail_in = 0;
		m_zs.avail_out = 0;
		m_zs.next_in = nullptr;
		m_zs.next_out = nullptr;
	#ifndef _MSC_VER
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wold-style-cast"
	#endif
		ensure(inflateInit2(&m_zs, 16 + 15) == Z_OK);
		m_read_inited = true;
		m_errored = false;
	}
}

bool compressed_serialization_file_handler::handle_file_op(utils::serial& ar, usz pos, usz size, const void* data)
{
	if (ar.is_writing())
	{
		initialize(ar);

		if (m_errored)
		{
			return false;
		}

		auto& manager = *m_stream;
		auto& stream_data = manager.m_queued_data_to_process;

		if (data)
		{
			ensure(false);
		}

		// Writing not at the end is forbidden
		ensure(ar.pos == ar.data_offset + ar.data.size());

		if (ar.data.empty())
		{
			if (pos == umax && size == umax && *m_file)
			{
				// Request to flush the file to disk
				m_file->sync();
			}

			return true;
		}

		ar.seek_end();

		if (!m_file_writer_thread)
		{
			// Avoid multi-threading for small files
			blocked_compressed_write(ar.data);
		}
		else
		{
			while (true)
			{
				// Avoid flooding RAM, wait if there is too much pending memory
				const usz new_value = m_pending_bytes.atomic_op([&](usz& v)
				{
					v &= ~pending_data_wait_bit;

					if (v >= pending_compress_bytes_bound)
					{
						v |= pending_data_wait_bit;
					}
					else
					{
						// Overflow detector
						ensure(~v - pending_data_wait_bit > ar.data.size());

						v += ar.data.size();
					}

					return v;
				});

				if (new_value & pending_data_wait_bit)
				{
					m_pending_bytes.wait(new_value);
				}
				else
				{
					break;
				}
			}

			stream_data.push(std::move(ar.data));
		}

		ar.data_offset = ar.pos;
		ar.data.clear();

		if (pos == umax && size == umax && *m_file)
		{
			// Request to flush the file to disk
			m_file->sync();
		}

		return true;
	}

	initialize(ar);

	if (m_errored)
	{
		return false;
	}

	if (!size)
	{
		return true;
	}

	if (pos == 0 && size == umax)
	{
		// Discard loaded data until pos if profitable
		const usz limit = ar.data_offset + ar.data.size();

		if (ar.pos > ar.data_offset && ar.pos < limit)
		{
			const usz may_discard_bytes = ar.pos - ar.data_offset;
			const usz moved_byte_count_on_discard = limit - ar.pos;

			// Cheeck profitability (check recycled memory and std::memmove costs)
			if (may_discard_bytes >= 0x50'0000 || (may_discard_bytes >= 0x20'0000 && moved_byte_count_on_discard / may_discard_bytes < 3))
			{
				ar.data_offset += may_discard_bytes;
				ar.data.erase(ar.data.begin(), ar.data.begin() + may_discard_bytes);

				if (ar.data.capacity() >= 0x200'0000)
				{
					// Discard memory
					ar.data.shrink_to_fit();
				}
			}

			return true;
		}

		// Discard all loaded data
		ar.data_offset += ar.data.size();
		ensure(ar.pos >= ar.data_offset);
		ar.data.clear();

		if (ar.data.capacity() >= 0x200'0000)
		{
			// Discard memory
			ar.data.shrink_to_fit();
		}

		return true;
	}

	if (~pos < size - 1)
	{
		// Overflow
		return false;
	}

	// TODO: Investigate if this optimization is worth an implementation for compressed stream
	// if (ar.data.empty() && pos != ar.pos)
	// {
	// 	// Relocate instead of over-fetch
	// 	ar.seek_pos(pos);
	// }

	const usz read_pre_buffer = utils::sub_saturate<usz>(ar.data_offset, pos);

	if (read_pre_buffer)
	{
		// Not allowed with compressed data for now
		// Unless someone implements mechanism for it
		ensure(false);
	}

	// Adjustment to prevent overflow
	const usz subtrahend = ar.data.empty() ? 0 : 1;
	const usz read_past_buffer = utils::sub_saturate<usz>(pos + (size - subtrahend), ar.data_offset + (ar.data.size() - subtrahend));
	const usz read_limit = utils::sub_saturate<usz>(ar.m_max_data, ar.data_offset); 

	if (read_past_buffer)
	{
		// Read proceeding data
		// More lightweight operation, this is the common operation
		// Allowed to fail, if memory is truly needed an assert would take place later
		const usz old_size = ar.data.size();

		// Try to prefetch data by reading more than requested
		const usz new_size = std::min<usz>(read_limit, std::max<usz>({ ar.data.capacity(), ar.data.size() + read_past_buffer * 3 / 2, ar.expect_little_data() ? usz{4096} : usz{0x10'0000} }));

		if (new_size < old_size)
		{
			// Read limit forbids further reads at this point
			return true;
		}

		ar.data.resize(new_size);
		ar.data.resize(this->read_at(ar, old_size + ar.data_offset, data ? const_cast<void*>(data) : ar.data.data() + old_size, ar.data.size() - old_size) + old_size);
	}

	return true;
}

usz compressed_serialization_file_handler::read_at(utils::serial& ar, usz read_pos, void* data, usz size)
{
	ensure(read_pos == ar.data.size() + ar.data_offset - size);

	if (!size || m_errored)
	{
		return 0;
	}

	initialize(ar);

	z_stream& m_zs = m_stream->m_zs;

	const usz total_to_read = size;
	usz read_size = 0;
	u8* out_data = static_cast<u8*>(data);

	for (; read_size < total_to_read;)
	{
		// Drain extracted memory stash (also before first file read)
		out_data = static_cast<u8*>(data) + read_size;
		m_zs.avail_in = adjust_for_uint(m_stream_data.size() - m_stream_data_index);
		m_zs.next_in = reinterpret_cast<const u8*>(m_stream_data.data() + m_stream_data_index);
		m_zs.next_out = out_data;
		m_zs.avail_out = adjust_for_uint(size - read_size);

		while (read_size < total_to_read && m_zs.avail_in)
		{
			const int res = inflate(&m_zs, Z_BLOCK);

			bool need_more_file_memory = false;

			switch (res)
			{
			case Z_OK:
			case Z_STREAM_END:
				break;
			case Z_BUF_ERROR:
			{
				if (m_zs.avail_in)
				{
					need_more_file_memory = true;
					break;
				}

				[[fallthrough]];
			}
			default:
				m_errored = true;
				inflateEnd(&m_zs);
				m_read_inited = false;
				sys_log.error("Failure of compressed data reading. (res=%d, read_size=0x%x, avail_in=0x%x, avail_out=0x%x, ar=%s)", res, read_size, m_zs.avail_in, m_zs.avail_out, ar);
				return read_size;
			}

			read_size = m_zs.next_out - static_cast<u8*>(data);
			m_stream_data_index = m_zs.avail_in ? m_zs.next_in - m_stream_data.data() : m_stream_data.size();

			// Adjust again in case the values simply did not fit into uInt
			m_zs.avail_out = adjust_for_uint(utils::sub_saturate<usz>(total_to_read, read_size));
			m_zs.avail_in = adjust_for_uint(m_stream_data.size() - m_stream_data_index);

			if (need_more_file_memory)
			{
				break;
			}
		}

		if (read_size >= total_to_read)
		{
			break;
		}

		const usz add_size = ar.expect_little_data() ? 0x1'0000 : 0x10'0000;
		const usz old_file_buf_size = m_stream_data.size();

		m_stream_data.resize(old_file_buf_size + add_size);
		m_stream_data.resize(old_file_buf_size + m_file->read_at(m_file_read_index, m_stream_data.data() + old_file_buf_size, add_size));

		if (m_stream_data.size() == old_file_buf_size)
		{
			// EOF
			break;
		}

		m_file_read_index += m_stream_data.size() - old_file_buf_size;
	}

	if (m_stream_data.size() - m_stream_data_index <= m_stream_data_index / 5)
	{
		// Shrink to required memory size
		m_stream_data.erase(m_stream_data.begin(), m_stream_data.begin() + m_stream_data_index);

		if (m_stream_data.capacity() >= 0x200'0000)
		{
			// Discard memory
			m_stream_data.shrink_to_fit();
		}

		m_stream_data_index = 0;
	}

	return read_size;
}

void compressed_serialization_file_handler::skip_until(utils::serial& ar)
{
	ensure(!ar.is_writing() && ar.pos >= ar.data_offset);

	if (ar.pos > ar.data_offset)
	{
		handle_file_op(ar, ar.data_offset, ar.pos - ar.data_offset, nullptr);
	}
}

void compressed_serialization_file_handler::finalize(utils::serial& ar)
{
	handle_file_op(ar, 0, umax, nullptr);

	if (!m_stream)
	{
		return;
	}

	auto& stream = *m_stream;
	z_stream& m_zs = m_stream->m_zs;

	if (m_read_inited)
	{
		ensure(inflateEnd(&m_zs) == Z_OK);
		m_read_inited = false;
		return;
	}

	stream.m_queued_data_to_process.push(std::vector<u8>());

	if (m_file_writer_thread)
	{
		// Join here to avoid log messages in the destructor
		(*m_file_writer_thread)();
	}

	m_stream_data_prepare_thread.reset();
	m_file_writer_thread.reset();

	m_zs.avail_in = 0;
	m_zs.next_in  = nullptr;

	m_stream_data.resize(0x10'0000);

	do
	{
		m_zs.avail_out = static_cast<uInt>(m_stream_data.size());
		m_zs.next_out  = m_stream_data.data();

		if (deflate(&m_zs, Z_FINISH) == Z_STREAM_ERROR)
		{
			break;
		}

		m_file->write(m_stream_data.data(), m_stream_data.size() - m_zs.avail_out);
	}
	while (m_zs.avail_out == 0);

	m_stream_data = {};
	ensure(deflateEnd(&m_zs) == Z_OK);

	m_write_inited = false;
	ar.data = {}; // Deallocate and clear

	m_file->sync();
}

void compressed_serialization_file_handler::stream_data_prepare_thread_op()
{
	compressed_stream_data& stream = *m_stream;
	z_stream& m_zs = stream.m_zs;

	while (true)
	{
		stream.m_queued_data_to_process.wait();

		for (auto&& data : stream.m_queued_data_to_process.pop_all())
		{
			if (data.empty())
			{
				// Abort is requested, flush data and exit
				stream.m_queued_data_to_write.push(std::vector<u8>());
				return;
			}

			m_zs.avail_in = adjust_for_uint(data.size());
			m_zs.next_in  = data.data();

			usz buffer_offset = 0;
			m_stream_data.resize(::compressBound(m_zs.avail_in));

			do
			{
				m_zs.avail_out = adjust_for_uint(m_stream_data.size() - buffer_offset);
				m_zs.next_out  = m_stream_data.data() + buffer_offset;

				if (deflate(&m_zs, Z_NO_FLUSH) == Z_STREAM_ERROR)
				{
					m_errored = true;
					deflateEnd(&m_zs);

					// Abort
					stream.m_queued_data_to_write.push(std::vector<u8>());
					break;
				}

				buffer_offset = m_zs.next_out - m_stream_data.data();

				m_zs.avail_in = adjust_for_uint(data.size() - (m_zs.next_in - data.data()));

				if (m_zs.avail_out == 0)
				{
					m_stream_data.resize(m_stream_data.size() + (m_zs.avail_in / 4) + (m_stream_data.size() / 16) + 1);
				}
			}
			while (m_zs.avail_out == 0 || m_zs.avail_in != 0);

			if (m_errored)
			{
				return;
			}

			// Forward for file write
			const usz queued_size = data.size();

			const usz size_diff = buffer_offset - queued_size;
			const usz new_val = m_pending_bytes.add_fetch(size_diff);
			const usz left = new_val & ~pending_data_wait_bit;

			if (new_val & pending_data_wait_bit && left < pending_compress_bytes_bound && left - size_diff >= pending_compress_bytes_bound && !m_pending_signal)
			{
				// Notification is postponed until data write and memory release
				m_pending_signal = true;
			}

			// Ensure wait bit state has not changed by the update
			ensure(~((new_val - size_diff) ^ new_val) & pending_data_wait_bit);

			if (!buffer_offset)
			{
				if (m_pending_signal)
				{
					m_pending_bytes.notify_all();
				}

				continue;
			}

			m_stream_data.resize(buffer_offset);
			stream.m_queued_data_to_write.push(std::move(m_stream_data));
		}
	}
}

void compressed_serialization_file_handler::file_writer_thread_op()
{
	compressed_stream_data& stream = *m_stream;

	while (true)
	{
		stream.m_queued_data_to_write.wait();

		for (auto&& data : stream.m_queued_data_to_write.pop_all())
		{
			if (data.empty())
			{
				return;
			}

			const usz last_size = data.size();
			m_file->write(data);
			data = {}; // Deallocate before notification

			const usz new_val = m_pending_bytes.sub_fetch(last_size);
			const usz left = new_val & ~pending_data_wait_bit;
			const bool pending_sig = m_pending_signal && m_pending_signal.exchange(false);

			if (pending_sig || (new_val & pending_data_wait_bit && left < pending_compress_bytes_bound && left + last_size >= pending_compress_bytes_bound))
			{
				m_pending_bytes.notify_all();
			}

			// Ensure wait bit state has not changed by the update
			ensure(~((new_val + last_size) ^ new_val) & pending_data_wait_bit);
		}
	}
}

void compressed_serialization_file_handler::blocked_compressed_write(const std::vector<u8>& data)
{
	z_stream& m_zs = m_stream->m_zs;

	m_zs.avail_in = adjust_for_uint(data.size());
	m_zs.next_in = data.data();

	m_stream_data.resize(::compressBound(m_zs.avail_in));

	do
	{
		m_zs.avail_out = adjust_for_uint(m_stream_data.size());
		m_zs.next_out = m_stream_data.data();

		if (deflate(&m_zs, Z_NO_FLUSH) == Z_STREAM_ERROR || m_file->write(m_stream_data.data(), m_stream_data.size() - m_zs.avail_out) != m_stream_data.size() - m_zs.avail_out)
		{
			m_errored = true;
			deflateEnd(&m_zs);
			break;
		}

		m_zs.avail_in = adjust_for_uint(data.size() - (m_zs.next_in - data.data()));
	}
	while (m_zs.avail_out == 0);
}

usz compressed_serialization_file_handler::get_size(const utils::serial& ar, usz recommended) const
{
	if (ar.is_writing())
	{
		return *m_file ? m_file->size() : 0;
	}

	const usz memory_available = ar.data_offset + ar.data.size();

	if (memory_available >= recommended || !*m_file)
	{
		// Avoid calling size() if possible
		return memory_available;
	}

	return std::max<usz>(utils::mul_saturate<usz>(m_file->size(), 6), memory_available);
}

struct compressed_zstd_stream_data
{
	ZSTD_DCtx* m_zd{};
	ZSTD_DStream* m_zs{};
	lf_queue<std::vector<u8>> m_queued_data_to_process;
	lf_queue<std::vector<u8>> m_queued_data_to_write;
};

void compressed_zstd_serialization_file_handler::initialize(utils::serial& ar)
{
	if (!m_stream)
	{
		m_stream = std::make_shared<compressed_zstd_stream_data>();
	}

	if (ar.is_writing())
	{
		if (m_write_inited)
		{
			return;
		}

		if (m_read_inited && m_stream->m_zd)
		{
			finalize(ar);
		}

		m_write_inited = true;
		m_errored = false;

		m_compression_threads.clear();
		m_file_writer_thread.reset();

		// Make sure at least one thread is free
		// Limit thread count in order to make sure memory limits are under control (TODO: scale with RAM size)
		const usz thread_count = std::min<u32>(std::max<u32>(utils::get_thread_count(), 2) - 1, 16);

		for (usz i = 0; i < thread_count; i++)
		{
			m_compression_threads.emplace_back().m_thread = std::make_unique<named_thread<std::function<void()>>>(fmt::format("CompressedPrepare Thread %d", i + 1), [this]() { this->stream_data_prepare_thread_op(); });
		}

		m_file_writer_thread = std::make_unique<named_thread<std::function<void()>>>("CompressedWriter Thread"sv, [this]() { this->file_writer_thread_op(); });
	}
	else
	{
		if (m_read_inited)
		{
			return;
		}

		if (m_write_inited)
		{
			finalize(ar);
		}

		auto& m_zd = m_stream->m_zd;
		m_zd = ZSTD_createDCtx();
		m_stream->m_zs = ZSTD_createDStream();
		m_read_inited = true;
		m_errored = false;
	}
}

bool compressed_zstd_serialization_file_handler::handle_file_op(utils::serial& ar, usz pos, usz size, const void* data)
{
	if (ar.is_writing())
	{
		initialize(ar);

		if (m_errored)
		{
			return false;
		}

		if (data)
		{
			ensure(false);
		}

		// Writing not at the end is forbidden
		ensure(ar.pos == ar.data_offset + ar.data.size());

		if (ar.data.empty())
		{
			if (pos == umax && size == umax && *m_file)
			{
				// Request to flush the file to disk
				m_file->sync();
			}

			return true;
		}

		ar.seek_end();

		const usz buffer_idx = m_input_buffer_index++ % m_compression_threads.size();
		auto& input = m_compression_threads[buffer_idx].m_input;

		while (input)
		{
			// No waiting support on non-null pointer
			thread_ctrl::wait_for(2'000);
		}

		input.store(stx::make_single_value(std::move(ar.data)));
		input.notify_all();

		ar.data_offset = ar.pos;
		ar.data.clear();

		if (pos == umax && size == umax && *m_file)
		{
			// Request to flush the file to disk
			m_file->sync();
		}

		return true;
	}

	initialize(ar);

	if (m_errored)
	{
		return false;
	}

	if (!size)
	{
		return true;
	}

	if (pos == 0 && size == umax)
	{
		// Discard loaded data until pos if profitable
		const usz limit = ar.data_offset + ar.data.size();

		if (ar.pos > ar.data_offset && ar.pos < limit)
		{
			const usz may_discard_bytes = ar.pos - ar.data_offset;
			const usz moved_byte_count_on_discard = limit - ar.pos;

			// Cheeck profitability (check recycled memory and std::memmove costs)
			if (may_discard_bytes >= 0x50'0000 || (may_discard_bytes >= 0x20'0000 && moved_byte_count_on_discard / may_discard_bytes < 3))
			{
				ar.data_offset += may_discard_bytes;
				ar.data.erase(ar.data.begin(), ar.data.begin() + may_discard_bytes);

				if (ar.data.capacity() >= 0x200'0000)
				{
					// Discard memory
					ar.data.shrink_to_fit();
				}
			}

			return true;
		}

		// Discard all loaded data
		ar.data_offset += ar.data.size();
		ensure(ar.pos >= ar.data_offset);
		ar.data.clear();

		if (ar.data.capacity() >= 0x200'0000)
		{
			// Discard memory
			ar.data.shrink_to_fit();
		}

		return true;
	}

	if (~pos < size - 1)
	{
		// Overflow
		return false;
	}

	// TODO: Investigate if this optimization is worth an implementation for compressed stream
	// if (ar.data.empty() && pos != ar.pos)
	// {
	// 	// Relocate instead of over-fetch
	// 	ar.seek_pos(pos);
	// }

	const usz read_pre_buffer = utils::sub_saturate<usz>(ar.data_offset, pos);

	if (read_pre_buffer)
	{
		// Not allowed with compressed data for now
		// Unless someone implements mechanism for it
		ensure(false);
	}

	// Adjustment to prevent overflow
	const usz subtrahend = ar.data.empty() ? 0 : 1;
	const usz read_past_buffer = utils::sub_saturate<usz>(pos + (size - subtrahend), ar.data_offset + (ar.data.size() - subtrahend));
	const usz read_limit = utils::sub_saturate<usz>(ar.m_max_data, ar.data_offset); 

	if (read_past_buffer)
	{
		// Read proceeding data
		// More lightweight operation, this is the common operation
		// Allowed to fail, if memory is truly needed an assert would take place later
		const usz old_size = ar.data.size();

		// Try to prefetch data by reading more than requested
		const usz new_size = std::min<usz>(read_limit, std::max<usz>({ ar.data.capacity(), ar.data.size() + read_past_buffer * 3 / 2, ar.expect_little_data() ? usz{4096} : usz{0x10'0000} }));

		if (new_size < old_size)
		{
			// Read limit forbids further reads at this point
			return true;
		}

		ar.data.resize(new_size);
		ar.data.resize(this->read_at(ar, old_size + ar.data_offset, data ? const_cast<void*>(data) : ar.data.data() + old_size, ar.data.size() - old_size) + old_size);
	}

	return true;
}

usz compressed_zstd_serialization_file_handler::read_at(utils::serial& ar, usz read_pos, void* data, usz size)
{
	ensure(read_pos == ar.data.size() + ar.data_offset - size);

	if (!size || m_errored)
	{
		return 0;
	}

	initialize(ar);

	auto& m_zd = m_stream->m_zd;

	const usz total_to_read = size;
	usz read_size = 0;
	u8* out_data = static_cast<u8*>(data);

	for (; read_size < total_to_read;)
	{
		// Drain extracted memory stash (also before first file read)
		out_data = static_cast<u8*>(data) + read_size;

		auto avail_in = m_stream_data.size() - m_stream_data_index;
		auto next_in = reinterpret_cast<const u8*>(m_stream_data.data() + m_stream_data_index);
		auto next_out = out_data;
		auto avail_out = size - read_size;

		for (bool is_first = true; read_size < total_to_read && avail_in; is_first = false)
		{
			bool need_more_file_memory = false;

			ZSTD_outBuffer outs{next_out, avail_out, 0};

			// Try to extract previously held data first
			ZSTD_inBuffer ins{next_in, is_first ? 0 : avail_in, 0};

			const usz res = ZSTD_decompressStream(m_zd, &outs, &ins);

			if (ZSTD_isError(res))
			{
				need_more_file_memory = true;
				// finalize(ar);
				// m_errored = true;
				// sys_log.error("Failure of compressed data reading. (res=%d, read_size=0x%x, avail_in=0x%x, avail_out=0x%x, ar=%s)", res, read_size, avail_in, avail_out, ar);
				// return read_size;
			}

			read_size += outs.pos;
			next_out += outs.pos;
			avail_out -= outs.pos;
			next_in += ins.pos;
			avail_in -= ins.pos;

			m_stream_data_index = next_in - m_stream_data.data();

			if (need_more_file_memory)
			{
				break;
			}
		}

		if (read_size >= total_to_read)
		{
			break;
		}

		const usz add_size = ar.expect_little_data() ? 0x1'0000 : 0x10'0000;
		const usz old_file_buf_size = m_stream_data.size();

		m_stream_data.resize(old_file_buf_size + add_size);
		m_stream_data.resize(old_file_buf_size + m_file->read_at(m_file_read_index, m_stream_data.data() + old_file_buf_size, add_size));

		if (m_stream_data.size() == old_file_buf_size)
		{
			// EOF
			//ensure(read_size == total_to_read);
			break;
		}

		m_file_read_index += m_stream_data.size() - old_file_buf_size;
	}

	if (m_stream_data.size() - m_stream_data_index <= m_stream_data_index / 5)
	{
		// Shrink to required memory size
		m_stream_data.erase(m_stream_data.begin(), m_stream_data.begin() + m_stream_data_index);

		if (m_stream_data.capacity() >= 0x200'0000)
		{
			// Discard memory
			m_stream_data.shrink_to_fit();
		}

		m_stream_data_index = 0;
	}

	return read_size;
}

void compressed_zstd_serialization_file_handler::skip_until(utils::serial& ar)
{
	ensure(!ar.is_writing() && ar.pos >= ar.data_offset);

	if (ar.pos > ar.data_offset)
	{
		handle_file_op(ar, ar.data_offset, ar.pos - ar.data_offset, nullptr);
	}
}

void compressed_zstd_serialization_file_handler::finalize(utils::serial& ar)
{
	handle_file_op(ar, 0, umax, nullptr);

	if (!m_stream)
	{
		return;
	}

	auto& m_zd = m_stream->m_zd;

	if (m_read_inited)
	{
		//ZSTD_decompressEnd(m_stream->m_zd);
		ensure(ZSTD_freeDCtx(m_zd));
		m_read_inited = false;
		return;
	}

	const stx::shared_ptr<std::vector<u8>> empty_data = stx::make_single<std::vector<u8>>();
	const stx::shared_ptr<std::vector<u8>> null_ptr = stx::null_ptr;

	for (auto& context : m_compression_threads)
	{
		// Try to notify all on the first iteration
		if (context.m_input.compare_and_swap_test(null_ptr, empty_data))
		{
			context.notified = true;
			context.m_input.notify_one();
		}
	}

	for (auto& context : m_compression_threads)
	{
		// Notify to abort
		while (!context.notified)
		{
			const auto data = context.m_input.compare_and_swap(null_ptr, empty_data);

			if (!data)
			{
				context.notified = true;
				context.m_input.notify_one();
				break;
			}

			// Wait until valid input is processed
			thread_ctrl::wait_for(1000);
		}
	}

	for (auto& context : m_compression_threads)
	{
		// Wait for notification to be consumed
		while (context.m_input)
		{
			thread_ctrl::wait_for(1000);
		}
	}

	for (auto& context : m_compression_threads)
	{
		// Wait for data to be writen to be read by the thread
		while (context.m_output)
		{
			thread_ctrl::wait_for(1000);
		}
	}

	for (usz idx = m_output_buffer_index;;)
	{
		auto& out_cur = m_compression_threads[idx % m_compression_threads.size()].m_output;
		auto& out_next = m_compression_threads[(idx + 1) % m_compression_threads.size()].m_output;

		out_cur.compare_and_swap_test(null_ptr, empty_data);
		out_next.compare_and_swap_test(null_ptr, empty_data);

		if (usz new_val = m_output_buffer_index; idx != new_val)
		{
			// Index was changed inbetween, retry on the next index
			idx = new_val;
			continue;
		}

		// Must be waiting on either of the two
		out_cur.notify_all();

		// Check for single thread
		if (&out_next != &out_cur)
		{
			out_next.notify_all();
		}

		break;
	}

	if (m_file_writer_thread)
	{
		// Join here to avoid log messages in the destructor
		(*m_file_writer_thread)();
	}

	m_compression_threads.clear();
	m_file_writer_thread.reset();

	m_stream_data = {};
	m_write_inited = false;
	ar.data = {}; // Deallocate and clear

	m_file->sync();
}

void compressed_zstd_serialization_file_handler::stream_data_prepare_thread_op()
{
	ZSTD_CCtx* m_zc = ZSTD_createCCtx();

	std::vector<u8> stream_data;

	const stx::shared_ptr<std::vector<u8>> null_ptr = stx::null_ptr;
	const usz thread_index = m_thread_buffer_index++;

	while (true)
	{
		auto& input = m_compression_threads[thread_index].m_input;
		auto& output = m_compression_threads[thread_index].m_output;

		while (!input)
		{
			input.wait(nullptr);
		}

		auto data = input.exchange(stx::null_ptr);
		input.notify_all();

		if (data->empty())
		{
			// Abort is requested, flush data and exit
			break;
		}

		stream_data.resize(::ZSTD_compressBound(data->size()));
		const usz out_size = ZSTD_compressCCtx(m_zc, stream_data.data(), stream_data.size(), data->data(), data->size(), ZSTD_btultra);

		ensure(!ZSTD_isError(out_size) && out_size);

		if (m_errored)
		{
			break;
		}

		stream_data.resize(out_size);

		const stx::shared_ptr<std::vector<u8>> data_ptr = make_single<std::vector<u8>>(std::move(stream_data));

		while (output || !output.compare_and_swap_test(null_ptr, data_ptr))
		{
			thread_ctrl::wait_for(1000);
		}

		//if (m_output_buffer_index % m_compression_threads.size() == thread_index)
		{
			output.notify_all();
		}
	}

	ZSTD_freeCCtx(m_zc);
}

void compressed_zstd_serialization_file_handler::file_writer_thread_op()
{
	for (m_output_buffer_index = 0;; m_output_buffer_index++)
	{
		auto& output = m_compression_threads[m_output_buffer_index % m_compression_threads.size()].m_output;

		while (!output)
		{
			output.wait(nullptr);
		}

		auto data = output.exchange(stx::null_ptr);
		output.notify_all();

		if (data->empty())
		{
			break;
		}

		m_file->write(*data);
	}
}

usz compressed_zstd_serialization_file_handler::get_size(const utils::serial& ar, usz recommended) const
{
	if (ar.is_writing())
	{
		return *m_file ? m_file->size() : 0;
	}

	const usz memory_available = ar.data_offset + ar.data.size();

	if (memory_available >= recommended || !*m_file)
	{
		// Avoid calling size() if possible
		return memory_available;
	}

	return recommended;
	//return std::max<usz>(utils::mul_saturate<usz>(ZSTD_decompressBound(m_file->size()), 2), memory_available);
}

bool null_serialization_file_handler::handle_file_op(utils::serial&, usz, usz, const void*)
{
	return true;
}

void null_serialization_file_handler::finalize(utils::serial&)
{
}
