#include "stdafx.h"
#include "CHD.h"

#include <libchdr/chd.h>

#include <mutex>
#include <limits>
#include <new>

LOG_CHANNEL(chd_log, "CHD");

namespace
{
	class chd_image final : public fs::file_base
	{
		fs::file m_file;
		const std::string m_path;
		const u64 m_physical_size;
		u64 m_callback_pos = 0;
		std::unique_ptr<chd_file, decltype(&chd_close)> m_chd{nullptr, &chd_close};
		std::unique_ptr<u8[]> m_hunk;
		u32 m_hunk_size = 0;
		u32 m_cached_hunk = umax;
		std::mutex m_mutex;
		u64 m_size = 0;
		u64 m_pos = 0;

		static u64 callback_size(void* opaque)
		{
			return static_cast<chd_image*>(opaque)->m_physical_size;
		}

		static size_t callback_read(void* buffer, size_t size, size_t count, void* opaque)
		{
			auto& self = *static_cast<chd_image*>(opaque);
			if (!size || !count || self.m_callback_pos >= self.m_physical_size || count > SIZE_MAX / size)
			{
				return 0;
			}
			const u64 bytes = std::min<u64>(size * count, self.m_physical_size - self.m_callback_pos);
			const u64 read = self.m_file.read_at(self.m_callback_pos, buffer, bytes);
			self.m_callback_pos += read;
			return read / size;
		}

		static int callback_seek(void* opaque, s64 offset, int whence)
		{
			auto& self = *static_cast<chd_image*>(opaque);
			u64 base = 0;
			switch (whence)
			{
			case SEEK_SET:
				break;
			case SEEK_CUR:
				base = self.m_callback_pos;
				break;
			case SEEK_END:
				base = self.m_physical_size;
				break;
			default:
				return -1;
			}
			if (offset < 0)
			{
				const u64 distance = u64{0} - static_cast<u64>(offset);
				if (distance > base)
				{
					return -1;
				}
				self.m_callback_pos = base - distance;
			}
			else
			{
				if (static_cast<u64>(offset) > static_cast<u64>(std::numeric_limits<s64>::max()) - base)
				{
					return -1;
				}
				self.m_callback_pos = base + offset;
			}
			return 0;
		}

		static int callback_close(void*)
		{
			// The C++ source owns the host handle, including on a failed chd_open.
			return 0;
		}

		static constexpr core_file_callbacks s_callbacks{callback_size, callback_read, callback_close, callback_seek};

	public:
		chd_image(fs::file file, std::string path)
			: m_file(std::move(file)), m_path(std::move(path)), m_physical_size(m_file.size())
		{
		}

		bool open()
		{
			chd_header header{};
			const auto header_error = chd_read_header_core_file_callbacks(&s_callbacks, this, &header);
			if (header_error != CHDERR_NONE)
			{
				chd_log.error("Cannot read CHD header '%s': %s", m_path, chd_error_string(header_error));
				return false;
			}
			if (header.version != 5 || header.unitbytes != 2048 || !header.logicalbytes || header.logicalbytes % 2048
				|| !header.hunkbytes || header.hunkbytes % 2048 || header.hunkbytes > 16 * 1024 * 1024
				|| header.logicalbytes > 64ull * 1024 * 1024 * 1024)
			{
				chd_log.error("Unsupported CHD geometry '%s': version=%u, unit=%u, hunk=%u, bytes=%llu", m_path,
					header.version, header.unitbytes, header.hunkbytes, header.logicalbytes);
				return false;
			}
			if (std::any_of(std::begin(header.parentsha1), std::end(header.parentsha1), [](u8 byte)
			{
				return byte != 0;
			}))
			{
				chd_log.error("CHD parent images are unsupported: '%s'", m_path);
				return false;
			}
			for (u32 codec : header.compression)
			{
				if (codec != CHD_CODEC_NONE && codec != CHD_CODEC_ZLIB && codec != CHD_CODEC_LZMA
					&& codec != CHD_CODEC_HUFFMAN && codec != CHD_CODEC_FLAC && codec != CHD_CODEC_ZSTD)
				{
					chd_log.error("Unsupported CHD codec 0x%x: '%s'", codec, m_path);
					return false;
				}
			}
			// Allow at most 384 MiB for a full 12-byte/hunk map.
			// This is a map bound, not a claim about total or transient decoder RSS.
			const u64 hunks = (header.logicalbytes - 1) / header.hunkbytes + 1;
			if (hunks > (384ull * 1024 * 1024) / 12)
			{
				chd_log.error("CHD map exceeds the map memory limit: '%s'", m_path);
				return false;
			}
			chd_file* decoder = nullptr;
			const auto error = chd_open_core_file_callbacks(&s_callbacks, this, CHD_OPEN_READ, nullptr, &decoder);
			if (error != CHDERR_NONE)
			{
				chd_log.error("Cannot open CHD '%s': %s", m_path, chd_error_string(error));
				return false;
			}
			m_chd.reset(decoder);
			m_size = header.logicalbytes;
			m_hunk_size = header.hunkbytes;
			m_hunk.reset(new (std::nothrow) u8[m_hunk_size]);
			if (!m_hunk)
			{
				chd_log.error("Not enough memory for CHD hunk: '%s'", m_path);
				return false;
			}
			return true;
		}

		fs::stat_t get_stat() override
		{
			auto stat = m_file.get_stat();
			stat.size = m_size;
			stat.is_writable = false;
			return stat;
		}

		bool trunc(u64) override
		{
			fs::g_tls_error = fs::error::readonly;
			return false;
		}

		u64 write(const void*, u64) override
		{
			fs::g_tls_error = fs::error::readonly;
			return 0;
		}

		u64 size() override
		{
			return m_size;
		}

	private:
		// Caller holds m_mutex across decoding, verification and copying.
		u64 read_locked(u64 offset, void* buffer, u64 count)
		{
			if (offset >= m_size || !count)
			{
				return 0;
			}
			count = std::min(count, m_size - offset);
			u64 copied = 0;
			while (copied < count)
			{
				const u64 current = offset + copied;
				const u32 index = static_cast<u32>(current / m_hunk_size);
				if (m_cached_hunk != index)
				{
					// A failed decode may overwrite the buffer: invalidate before trying.
					m_cached_hunk = umax;
					const auto error = chd_read(m_chd.get(), index, m_hunk.get());
					if (error != CHDERR_NONE)
					{
						chd_log.error("CHD read failed '%s', hunk %u: %s", m_path, index, chd_error_string(error));
						fs::g_tls_error = fs::error::unknown;
						return 0;
					}
					m_cached_hunk = index;
				}
				const u64 within = current % m_hunk_size;
				const u64 bytes = std::min<u64>(count - copied, m_hunk_size - within);
				std::memcpy(static_cast<u8*>(buffer) + copied, m_hunk.get() + within, bytes);
				copied += bytes;
			}
			return copied;
		}

	public:
		u64 read_at(u64 offset, void* buffer, u64 count) override
		{
			std::lock_guard lock(m_mutex);
			return read_locked(offset, buffer, count);
		}

		u64 read(void* buffer, u64 count) override
		{
			std::lock_guard lock(m_mutex);
			const u64 read = read_locked(m_pos, buffer, count);
			m_pos += read;
			return read;
		}

		u64 seek(s64 offset, fs::seek_mode whence) override
		{
			std::lock_guard lock(m_mutex);
			const u64 base = whence == fs::seek_set ? 0 : (whence == fs::seek_cur ? m_pos : m_size);
			const u64 distance = u64{0} - static_cast<u64>(offset);
			if (whence > fs::seek_end || (offset < 0 ? distance > base : static_cast<u64>(offset) > static_cast<u64>(std::numeric_limits<s64>::max()) - base))
			{
				fs::g_tls_error = fs::error::inval;
				return umax;
			}
			m_pos = offset < 0 ? base - distance : base + offset;
			return m_pos;
		}
	};
}

bool is_chd_image(const fs::file& file)
{
	std::array<char, 8> magic{};
	return file && file.read_at(0, magic.data(), magic.size()) == magic.size()
		&& std::memcmp(magic.data(), "MComprHD", magic.size()) == 0;
}

std::unique_ptr<fs::file_base> open_chd_image(fs::file file, const std::string& path)
{
	if (!file)
	{
		return nullptr;
	}
	auto image = std::make_unique<chd_image>(std::move(file), path);
	if (image->open())
	{
		return image;
	}
	fs::g_tls_error = fs::error::inval;
	return nullptr;
}
