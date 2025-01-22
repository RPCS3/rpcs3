#pragma once

#include "util/serialization.hpp"
#include "util/shared_ptr.hpp"

#include "Utilities/Thread.h"

#include <deque>

namespace fs
{
	class file;
}

// Uncompressed file serialization handler
struct uncompressed_serialization_file_handler : utils::serialization_file_handler
{
	const std::unique_ptr<fs::file> m_file_storage;
	const std::add_pointer_t<const fs::file> m_file;

	explicit uncompressed_serialization_file_handler(fs::file&& file) noexcept
		: utils::serialization_file_handler()
		, m_file_storage(std::make_unique<fs::file>(std::move(file)))
		, m_file(m_file_storage.get())
	{
	}

	explicit uncompressed_serialization_file_handler(const fs::file& file) noexcept
		: utils::serialization_file_handler()
		, m_file_storage(nullptr)
		, m_file(std::addressof(file))
	{
	}

	uncompressed_serialization_file_handler(const uncompressed_serialization_file_handler&) = delete;
	uncompressed_serialization_file_handler& operator=(const uncompressed_serialization_file_handler&) = delete;

	// Handle file read and write requests
	bool handle_file_op(utils::serial& ar, usz pos, usz size, const void* data) override;

	// Get available memory or file size
	// Preferably memory size if is already greater/equal to recommended to avoid additional file ops
	usz get_size(const utils::serial& ar, usz recommended) const override;

	void finalize(utils::serial& ar) override;
};

template <typename File> requires (std::is_same_v<std::remove_cvref_t<File>, fs::file>)
inline std::unique_ptr<uncompressed_serialization_file_handler> make_uncompressed_serialization_file_handler(File&& file)
{
	ensure(file);
	return std::make_unique<uncompressed_serialization_file_handler>(std::forward<File>(file));
}

struct compressed_stream_data;

// Compressed file serialization handler
struct compressed_serialization_file_handler : utils::serialization_file_handler
{
	explicit compressed_serialization_file_handler(fs::file&& file) noexcept
		: utils::serialization_file_handler()
		, m_file_storage(std::make_unique<fs::file>(std::move(file)))
		, m_file(m_file_storage.get())
	{
	}

	explicit compressed_serialization_file_handler(const fs::file& file) noexcept
		: utils::serialization_file_handler()
		, m_file_storage(nullptr)
		, m_file(std::addressof(file))
	{
	}

	compressed_serialization_file_handler(const compressed_serialization_file_handler&) = delete;
	compressed_serialization_file_handler& operator=(const compressed_serialization_file_handler&) = delete;

	// Handle file read and write requests
	bool handle_file_op(utils::serial& ar, usz pos, usz size, const void* data) override;

	// Get available memory or file size
	// Preferably memory size if is already greater/equal to recommended to avoid additional file ops
	usz get_size(const utils::serial& ar, usz recommended) const override;
	void skip_until(utils::serial& ar) override;

	bool is_valid() const override
	{
		return !m_errored;
	}

	void finalize(utils::serial& ar) override;

private:
	const std::unique_ptr<fs::file> m_file_storage;
	const std::add_pointer_t<const fs::file> m_file;
	std::vector<u8> m_stream_data;
	usz m_stream_data_index = 0;
	usz m_file_read_index = 0;
	atomic_t<usz> m_pending_bytes = 0;
	atomic_t<bool> m_pending_signal = false;
	bool m_write_inited = false;
	bool m_read_inited = false;
	bool m_errored = false;
	std::shared_ptr<compressed_stream_data> m_stream;
	std::unique_ptr<named_thread<std::function<void()>>> m_stream_data_prepare_thread;
	std::unique_ptr<named_thread<std::function<void()>>> m_file_writer_thread;

	usz read_at(utils::serial& ar, usz read_pos, void* data, usz size);
	void initialize(utils::serial& ar);
	void stream_data_prepare_thread_op();
	void file_writer_thread_op();
	void blocked_compressed_write(const std::vector<u8>& data);
};

template <typename File> requires (std::is_same_v<std::remove_cvref_t<File>, fs::file>)
inline std::unique_ptr<compressed_serialization_file_handler> make_compressed_serialization_file_handler(File&& file)
{
	ensure(file);
	return std::make_unique<compressed_serialization_file_handler>(std::forward<File>(file));
}

struct compressed_zstd_stream_data;

// Compressed file serialization handler
struct compressed_zstd_serialization_file_handler : utils::serialization_file_handler
{
	explicit compressed_zstd_serialization_file_handler(fs::file&& file) noexcept
		: utils::serialization_file_handler()
		, m_file_storage(std::make_unique<fs::file>(std::move(file)))
		, m_file(m_file_storage.get())
	{
	}

	explicit compressed_zstd_serialization_file_handler(const fs::file& file) noexcept
		: utils::serialization_file_handler()
		, m_file_storage(nullptr)
		, m_file(std::addressof(file))
	{
	}

	compressed_zstd_serialization_file_handler(const compressed_zstd_serialization_file_handler&) = delete;
	compressed_zstd_serialization_file_handler& operator=(const compressed_zstd_serialization_file_handler&) = delete;

	// Handle file read and write requests
	bool handle_file_op(utils::serial& ar, usz pos, usz size, const void* data) override;

	// Get available memory or file size
	// Preferably memory size if is already greater/equal to recommended to avoid additional file ops
	usz get_size(const utils::serial& ar, usz recommended) const override;
	void skip_until(utils::serial& ar) override;

	bool is_valid() const override
	{
		return !m_errored;
	}

	void finalize(utils::serial& ar) override;

private:
	const std::unique_ptr<fs::file> m_file_storage;
	const std::add_pointer_t<const fs::file> m_file;
	std::vector<u8> m_stream_data;
	usz m_stream_data_index = 0;
	usz m_file_read_index = 0;
	atomic_t<usz> m_pending_bytes = 0;
	atomic_t<bool> m_pending_signal = false;
	bool m_write_inited = false;
	bool m_read_inited = false;
	atomic_t<bool> m_errored = false;

	usz m_input_buffer_index = 0;
	atomic_t<usz> m_output_buffer_index = 0;
	atomic_t<usz> m_thread_buffer_index = 0;

	struct compression_thread_context_t
	{
		atomic_ptr<std::vector<u8>> m_input;
		atomic_ptr<std::vector<u8>> m_output;
		bool notified = false;
		std::unique_ptr<named_thread<std::function<void()>>> m_thread;
	};

	std::deque<compression_thread_context_t> m_compression_threads;
	std::shared_ptr<compressed_zstd_stream_data> m_stream;
	std::unique_ptr<named_thread<std::function<void()>>> m_file_writer_thread;

	usz read_at(utils::serial& ar, usz read_pos, void* data, usz size);
	void initialize(utils::serial& ar);
	void stream_data_prepare_thread_op();
	void file_writer_thread_op();
};

template <typename File> requires (std::is_same_v<std::remove_cvref_t<File>, fs::file>)
inline std::unique_ptr<compressed_zstd_serialization_file_handler> make_compressed_zstd_serialization_file_handler(File&& file)
{
	ensure(file);
	return std::make_unique<compressed_zstd_serialization_file_handler>(std::forward<File>(file));
}

// Null file serialization handler
struct null_serialization_file_handler : utils::serialization_file_handler
{
	explicit null_serialization_file_handler() noexcept
	{
	}

	// Handle file read and write requests
	bool handle_file_op(utils::serial& ar, usz pos, usz size, const void* data) override;

	void finalize(utils::serial& ar) override;

	bool is_null() const override
	{
		return true;
	}
};

inline std::unique_ptr<null_serialization_file_handler> make_null_serialization_file_handler()
{
	return std::make_unique<null_serialization_file_handler>();
}
