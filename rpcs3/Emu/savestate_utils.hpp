#include "util/serialization.hpp"

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

	// Handle file read and write requests
	bool handle_file_op(utils::serial& ar, usz pos, usz size, const void* data) override;

	// Get available memory or file size
	// Preferably memory size if is already greater/equal to recommended to avoid additional file ops
	usz get_size(const utils::serial& ar, usz recommended) const override;
};

inline std::unique_ptr<uncompressed_serialization_file_handler> make_uncompressed_serialization_file_handler(fs::file&& file)
{
	return std::make_unique<uncompressed_serialization_file_handler>(std::move(file));
}

inline std::unique_ptr<uncompressed_serialization_file_handler> make_uncompressed_serialization_file_handler(const fs::file& file)
{
	return std::make_unique<uncompressed_serialization_file_handler>(file);
}

