#pragma once

#include "util/serialization.hpp"

#include <any>

namespace fs
{
	class file;
}

struct version_entry
{
	u16 type;
	u16 version;

	ENABLE_BITWISE_SERIALIZATION;
};

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

// Compressed file serialization handler
struct compressed_serialization_file_handler : utils::serialization_file_handler
{
	const std::unique_ptr<fs::file> m_file_storage;
	const std::add_pointer_t<const fs::file> m_file;
	std::vector<u8> m_stream_data;
	usz m_stream_data_index = 0;
	usz m_file_read_index = 0;
	bool m_write_inited = false;
	bool m_read_inited = false;
	std::any m_stream;

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

	// Handle file read and write requests
	bool handle_file_op(utils::serial& ar, usz pos, usz size, const void* data) override;

	// Get available memory or file size
	// Preferably memory size if is already greater/equal to recommended to avoid additional file ops
	usz get_size(const utils::serial& ar, usz recommended) const override;
	void skip_until(utils::serial& ar) override;

	void finalize(utils::serial& ar) override;

private:
	usz read_at(utils::serial& ar, usz read_pos, void* data, usz size);
	void initialize(utils::serial& ar);
};

template <typename File> requires (std::is_same_v<std::remove_cvref_t<File>, fs::file>)
inline std::unique_ptr<compressed_serialization_file_handler> make_compressed_serialization_file_handler(File&& file)
{
	ensure(file);
	return std::make_unique<compressed_serialization_file_handler>(std::forward<File>(file));
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

bool load_and_check_reserved(utils::serial& ar, usz size);
bool is_savestate_version_compatible(const std::vector<version_entry>& data, bool is_boot_check);
std::vector<version_entry> get_savestate_versioning_data(fs::file&& file, std::string_view filepath);
bool is_savestate_compatible(fs::file&& file, std::string_view filepath);
std::vector<version_entry> read_used_savestate_versions();
std::string get_savestate_file(std::string_view title_id, std::string_view boot_path, s64 abs_id, s64 rel_id);

