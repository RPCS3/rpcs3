#include "util/types.hpp"
#include "util/logs.hpp"
#include "util/asm.hpp"
#include "util/simd.hpp"
#include "util/endian.hpp"

#include "Utilities/lockless.h"
#include "Utilities/File.h"
#include "Utilities/StrFmt.h"
#include "serialization_ext.hpp"

#include <zlib.h>

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
			return  true;
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
		return m_file->size();
	}

	const usz memory_available = ar.data_offset + ar.data.size();

	if (memory_available >= recommended)
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

struct compressed_stream_data
{
	z_stream m_zs{};
	lf_queue<std::vector<u8>> m_queued_data_to_process;
	lf_queue<std::vector<u8>> m_queued_data_to_write;
	atomic_t<usz> m_pending_bytes = 0;
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
		ensure(deflateInit2(&m_zs, 9, Z_DEFLATED, 16 + 15, 9, Z_DEFAULT_STRATEGY) == Z_OK);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
		m_write_inited = true;
		m_errored = false;

		if (!ar.expect_little_data())
		{
			m_stream_data_prepare_thread = std::make_unique<named_thread<std::function<void()>>>("Compressed Data Prepare Thread"sv, [this]() { this->stream_data_prepare_thread_op(); });
			m_file_writer_thread = std::make_unique<named_thread<std::function<void()>>>("Compressed File Writer Thread"sv, [this]() { this->file_writer_thread_op(); });
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
				const usz new_value = m_pending_bytes.atomic_op([&](usz v)
				{
					v &= ~(1ull << 63);

					if (v > 0x400'0000)
					{
						v |= 1ull << 63;
					}
					else
					{
						v += ar.data.size();
					}

					return v;
				});

				if (new_value & (1ull << 63))
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
		ar.data.resize(std::min<usz>(read_limit, std::max<usz>({ ar.data.capacity(), ar.data.size() + read_past_buffer * 3 / 2, ar.expect_little_data() ? usz{4096} : usz{0x10'0000} })));
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
				if (!m_stream_data.empty())
				{
					stream.m_queued_data_to_write.push(std::move(m_stream_data));
				}

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

				if (m_zs.avail_out == 0)
				{
					m_stream_data.resize(m_stream_data.size() + (m_zs.avail_in + 3ull) / 4);
				}

				m_zs.avail_in = adjust_for_uint(data.size() - (m_zs.next_in - data.data()));
			}
			while (m_zs.avail_out == 0 || m_zs.avail_in != 0);

			// Forward for file write
			const usz queued_size = data.size();
			ensure(buffer_offset);
			m_pending_bytes += buffer_offset - queued_size;
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

			if (m_pending_bytes.sub_fetch(last_size) == 1ull << 63)
			{
				m_pending_bytes.notify_all();
			}
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
		return m_file->size();
	}

	const usz memory_available = ar.data_offset + ar.data.size();

	if (memory_available >= recommended)
	{
		// Avoid calling size() if possible
		return memory_available;
	}

	return std::max<usz>(utils::mul_saturate<usz>(m_file->size(), 6), memory_available);
}

bool null_serialization_file_handler::handle_file_op(utils::serial&, usz, usz, const void*)
{
	return true;
}

void null_serialization_file_handler::finalize(utils::serial&)
{
}
