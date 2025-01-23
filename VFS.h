#pragma once

#include <vector>
#include <string>
#include <string_view>

struct lv2_fs_mount_point;
struct vfs_directory;

namespace vfs
{
	// Mount VFS device
	bool mount(std::string_view vpath, std::string_view path, bool is_dir = true);

	// Unmount VFS device
	bool unmount(std::string_view vpath);

	// Convert VFS path to fs path, optionally listing directories mounted in it
	std::string get(std::string_view vpath, std::vector<std::string>* out_dir = nullptr, std::string* out_path = nullptr);

	// Convert fs path to VFS path
	std::string retrieve(std::string_view path, const vfs_directory* node = nullptr, std::vector<std::string_view>* mount_path = nullptr);

	// Escape VFS name by replacing non-portable characters with surrogates
	std::string escape(std::string_view name, bool escape_slash = false);

	// Invert escape operation
	std::string unescape(std::string_view name);

	// Functions in this namespace operate on host filepaths, similar to fs::
	namespace host
	{
		// For internal use (don't use)
		std::string hash_path(const std::string& path, const std::string& dev_root, std::string_view prefix = {});

		// Call fs::rename with retry on access error
		bool rename(const std::string& from, const std::string& to, const lv2_fs_mount_point* mp, bool overwrite, bool lock = true);

		// Delete file without deleting its contents, emulated with MoveFileEx on Windows
		bool unlink(const std::string& path, const std::string& dev_root);

		// Delete folder contents using rename, done atomically if remove_root is true
		bool remove_all(const std::string& path, const std::string& dev_root, const lv2_fs_mount_point* mp, bool remove_root = true, bool lock = true, bool force_atomic = false);
	}
}
