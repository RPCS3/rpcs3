#pragma once

#include "ISO.h"

#include <functional>

class ZArchiveReader;

// A disc stored in a ZArchive (.zar), holding either:
// - an image ("disc.iso"), optionally along with its decryption key ("disc.dkey" or "disc.key")
// - the files of a disc folder layout ("PS3_DISC.SFB", "PS3_GAME/PARAM.SFO" etc.)
class zar_iso_container final : public iso_container
{
public:
	static bool has_zar_extension(std::string_view path);
	static std::shared_ptr<zar_iso_container> open(const std::string& path, std::string* error = nullptr);

	bool is_image() const override { return m_image_node != umax; }
	fs::file open_image() const override;
	fs::file open_key() const override;
	bool form_hierarchy(iso_fs_node& root) const override;
	fs::file open_file(const iso_fs_node& node) const override;

	// The name of the key stored along with the image, empty if there is none
	const std::string& key_name() const { return m_key_name; }

private:
	zar_iso_container(const std::string& path, std::shared_ptr<ZArchiveReader> reader, s64 mtime);

	fs::file open_node(u32 node, std::string_view name) const;
	bool form_hierarchy(iso_fs_node& parent, const std::string& parent_path, u32 depth, u64& node_count) const;

	std::string m_path;
	std::shared_ptr<ZArchiveReader> m_reader;
	s64 m_mtime = 0;
	u32 m_image_node = umax;
	u32 m_key_node = umax;
	std::string m_key_name;
};

enum class zar_compression_status
{
	success,
	cancelled,
	invalid_source,
	invalid_key,
	output_exists,
	io_error,
	archive_error,
};

struct zar_compression_result
{
	zar_compression_status status = zar_compression_status::archive_error;
	std::string error;
	std::string embedded_key;
	u64 input_size = 0;
	u64 output_size = 0;

	explicit operator bool() const { return status == zar_compression_status::success; }
};

// Return false from the callback to cancel compression
using zar_compression_progress_cb = std::function<bool(u64 processed, u64 total, const std::string& current_file)>;

// Creates a ZArchive out of either a disc folder layout or an image. An image is stored as "disc.iso", along with its
// key as "disc.dkey" or "disc.key" when one is found for it
zar_compression_result compress_ps3_disc_to_zar(const std::string& source_path, const std::string& output_path, const zar_compression_progress_cb& progress = {});
