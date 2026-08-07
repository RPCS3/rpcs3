#pragma once

#include "Utilities/File.h"
#include "block_dev.hpp"
#include "fs_provider.hpp"
#include "util/endian.hpp"
#include "util/types.hpp"
#include <bit>
#include <optional>

namespace iso {
#pragma pack(push, 1)
template <typename T> struct le_be_pair {
  le_t<T> le;
  be_t<T> be;

  T value() const {
    if constexpr (std::endian::native == std::endian::little) {
      return le;
    } else {
      return be;
    }
  }
};

struct PrimaryVolumeDescriptorDateTime {
  char year[4];
  char month[2];
  char day[2];
  char hour[2];
  char minute[2];
  char second[2];
  char milliseconds[2];
  s8 gmt_offset;
};

struct VolumeHeader {
  u8 type;
  char standard_id[5];
  u8 version;
};

struct DirDateTime {
  u8 year; // + 1900
  u8 month;
  u8 day;
  u8 hour;
  u8 minute;
  u8 second;
  s8 gmt_offset;

  struct tm to_tm() const {
    struct tm time{};
    time.tm_year = year;
    time.tm_mon = month;
    time.tm_mday = day;
    time.tm_hour = hour;
    time.tm_min = minute;
    time.tm_sec = second;
    time.tm_gmtoff = gmt_offset;
    return time;
  }

  time_t to_time_t() const {
    auto tm = to_tm();
    return mktime(&tm);
  }
};

enum class DirEntryFlags : u8 {
  None = 0,
  Hidden = 1 << 0,
  Directory = 1 << 1,
  File = 1 << 2,
  ExtAttr = 1 << 3,
  Permissions = 1 << 4,
  MultiExtent = 1 << 7,
};

constexpr DirEntryFlags operator&(DirEntryFlags lhs, DirEntryFlags rhs) {
  return static_cast<DirEntryFlags>(static_cast<unsigned>(lhs) &
                                    static_cast<unsigned>(rhs));
}
constexpr DirEntryFlags operator|(DirEntryFlags lhs, DirEntryFlags rhs) {
  return static_cast<DirEntryFlags>(static_cast<unsigned>(lhs) |
                                    static_cast<unsigned>(rhs));
}

struct DirEntry {
  u8 entry_length;
  u8 ext_attr_length;
  le_be_pair<u32> lba;
  le_be_pair<u32> length;
  DirDateTime create_time;
  DirEntryFlags flags;
  u8 interleave_unit_size;
  u8 interleave_gap_size;
  le_be_pair<u16> sequence;
  u8 filename_length;

  fs::stat_t to_fs_stat() const {
    fs::stat_t result{};
    result.is_directory =
        (flags & iso::DirEntryFlags::Directory) != iso::DirEntryFlags::None;
    result.size = length.value();
    result.ctime = create_time.to_time_t();
    result.mtime = result.ctime;
    result.atime = result.ctime;
    return result;
  }
  fs::dir_entry to_fs_entry(std::string name) const {
    fs::dir_entry entry = {};
    static_cast<fs::stat_t &>(entry) = to_fs_stat();
    entry.name = std::move(name);
    return entry;
  }
};

struct PrimaryVolumeDescriptor {
  VolumeHeader header;
  uint8_t pad0;
  char system_id[32];
  char volume_id[32];
  char pad1[8];
  le_be_pair<u32> block_count;
  char pad2[32];
  le_be_pair<u16> volume_set_size;
  le_be_pair<u16> vol_seq_num;
  le_be_pair<u16> block_size;
  le_be_pair<u32> path_table_size;
  le_t<u32> path_table_block_le;
  le_t<u32> ext_path_table_block_le;
  be_t<u32> path_table_block_be;
  be_t<u32> ext_path_table_block_be;
  DirEntry root;
  u8 pad3;
  uint8_t volume_set_id[128];
  uint8_t publisher_id[128];
  uint8_t data_preparer_id[128];
  uint8_t application_id[128];
  uint8_t copyright_file_id[37];
  uint8_t abstract_file_id[37];
  uint8_t bibliographical_file_id[37];
  PrimaryVolumeDescriptorDateTime vol_creation_time;
  PrimaryVolumeDescriptorDateTime vol_modification_time;
  PrimaryVolumeDescriptorDateTime vol_expire_time;
  PrimaryVolumeDescriptorDateTime vol_effective_time;
  uint8_t version;
  uint8_t pad4;
  uint8_t app_used[512];

  u32 path_table_block() const {
    if constexpr (std::endian::native == std::endian::little) {
      return path_table_block_le;
    } else {
      return path_table_block_be;
    }
  }
};

struct PathTableEntryHeader {
    u8 name_length;
    u8 ext_attr_length;
    le_t<u32> location;
    le_t<u16> parent_id;
};

enum class StringEncoding {
  ascii,
  utf16_be,
};

#pragma pack(pop)
} // namespace iso

struct iso_entry {
  iso::DirEntry meta{};
  std::vector<iso::DirEntry> extents;
  std::uint64_t size = 0;

  fs::stat_t to_fs_stat() const {
    fs::stat_t result = meta.to_fs_stat();
    result.size = size;
    return result;
  }

  fs::dir_entry to_fs_entry(std::string name) const {
    fs::dir_entry entry = {};
    static_cast<fs::stat_t &>(entry) = to_fs_stat();
    entry.name = std::move(name);
    return entry;
  }
};

class iso_fs final : public fs_provider {
  std::unique_ptr<block_dev> m_dev;
  iso::DirEntry m_root_dir;
  iso::StringEncoding m_encoding = iso::StringEncoding::ascii;

public:
  iso_fs() = default;

  static std::optional<iso_fs> open(std::unique_ptr<block_dev> device) {
    iso_fs result;
    result.m_dev = std::move(device);

    if (!result.initialize()) {
      return {};
    }

    return result;
  }

  fs::file open(const std::filesystem::path &path,
                fs::open_mode mode = ::fs::open_mode::read) override;
  fs::dir open_dir(const std::filesystem::path &path) override;

private:
  bool initialize();

  std::optional<iso_entry> open_entry(const std::filesystem::path &path,
                                      bool isDir);

  std::pair<std::vector<iso_entry>, std::vector<std::string>>
  read_dir(const iso::DirEntry &entry);
  fs::file read_file(const iso_entry &entry);

public:
  bool extract(const std::filesystem::path &path, const std::string &dest);
};
