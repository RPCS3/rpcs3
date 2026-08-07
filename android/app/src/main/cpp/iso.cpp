
#include "stdafx.h"

#include "iso.hpp"
#include "Utilities/File.h"
#include "util/types.hpp"
#include <bit>
#include <cctype>
#include <codecvt>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <memory>
#include <span>
#include <string>

static std::string u16_ne_to_string(const char16_t *bytes, std::size_t count) {
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
  return convert.to_bytes(bytes, bytes + count);
}

static std::string u16_se_to_string(const char16_t *bytes, std::size_t count) {
  auto seBytes = (se_t<char16_t, true, alignof(char16_t)> *)bytes;

  std::vector<char16_t> neBytes(count);
  for (std::size_t i = 0; i < count; ++i) {
    neBytes[i] = seBytes[i];
  }

  return u16_ne_to_string(neBytes.data(), neBytes.size());
}

static std::string u16_be_to_string(const char16_t *bytes, std::size_t count) {
  if constexpr (std::endian::native == std::endian::big) {
    return u16_ne_to_string(bytes, count);
  } else {
    return u16_se_to_string(bytes, count);
  }
}

static std::string decodeString(std::string_view data,
                                iso::StringEncoding encoding) {
  switch (encoding) {
  case iso::StringEncoding::ascii:
    break;

  case iso::StringEncoding::utf16_be:
    return u16_be_to_string((char16_t *)data.data(), data.size() / 2);
  }

  return std::string(data);
}

bool iso_fs::initialize() {
  constexpr std::size_t primaryVolumeDescOffset = 16;
  ensure(m_dev->block_size() >= sizeof(iso::VolumeHeader));
  std::vector<std::byte> block(m_dev->block_size());

  std::optional<iso::PrimaryVolumeDescriptor> primaryVolume;
  std::optional<iso::PrimaryVolumeDescriptor> supplementaryVolume;

  for (std::size_t i = 0; i < 256; ++i) {
    if (m_dev->read(primaryVolumeDescOffset + i, block.data(), 1) != 1) {
      break;
    }

    auto header = reinterpret_cast<iso::VolumeHeader *>(block.data());

    if (header->type == 255) {
      break;
    }

    if (std::memcmp(header->standard_id, "CD001", 5) != 0) {
      continue;
    }

    if (header->type == 1) {
      primaryVolume =
          *reinterpret_cast<iso::PrimaryVolumeDescriptor *>(block.data());
      continue;
    }

    if (header->type == 2) {
      std::printf("found supplementary volume\n");
      supplementaryVolume =
          *reinterpret_cast<iso::PrimaryVolumeDescriptor *>(block.data());
      continue;
    }
  }

  if (!primaryVolume) {
    return false;
  }

  auto &pvd = supplementaryVolume ? *supplementaryVolume : *primaryVolume;
  m_encoding = supplementaryVolume ? iso::StringEncoding::utf16_be
                                   : iso::StringEncoding::ascii;
  m_root_dir = pvd.root;

  return true;
}

fs::file iso_fs::open(const std::filesystem::path &path, fs::open_mode mode) {
  if (mode != fs::open_mode::read) {
    return {};
  }

  auto optEntry = open_entry(path, false);
  if (!optEntry) {
    return {};
  }

  return read_file(*optEntry);
}

fs::dir iso_fs::open_dir(const std::filesystem::path &path) {
  auto optEntry = open_entry(path, true);
  if (!optEntry) {
    return {};
  }

  auto items = read_dir(optEntry->meta);
  std::vector<fs::dir_entry> result_items(items.first.size());

  for (std::size_t i = 0; i < result_items.size(); ++i) {
    result_items[i] = items.first[i].to_fs_entry(items.second[i]);
  }

  fs::dir result;
  result.reset(std::make_unique<fs::virtual_dir>(std::move(result_items)));
  return result;
}

std::optional<iso_entry>
iso_fs::open_entry(const std::filesystem::path &path, bool isDir) {
  auto pathString = std::filesystem::weakly_canonical(path).string();

  auto as_entry = [](const iso::DirEntry &record) {
    iso_entry result{};
    result.meta = record;
    result.extents.push_back(record);
    result.size = record.length.value();
    return result;
  };

  if (pathString == "/" || pathString == "\\" || pathString.empty()) {
    return as_entry(m_root_dir);
  }

  auto item = as_entry(m_root_dir);
  auto pathView = std::string_view(pathString);

  auto isStringEqNoCase = [](std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
      return false;
    }

    for (std::size_t i = 0; i < lhs.size(); ++i) {
      if (std::tolower(lhs[i]) != std::tolower(rhs[i])) {
        return false;
      }
    }

    return true;
  };

  while (!pathView.empty()) {
    auto sepPos = pathView.find_first_of("/\\");
    if (sepPos == 0) {
      pathView.remove_prefix(1);

      if (pathView.empty() && !isDir) {
        return {};
      }

      continue;
    }

    if ((item.meta.flags & iso::DirEntryFlags::Directory) !=
        iso::DirEntryFlags::Directory) {
      return {};
    }

    auto dirName = pathView.substr(0, sepPos);
    if (sepPos == std::string_view::npos) {
      pathView = {};
    } else {
      pathView.remove_prefix(sepPos + (isDir ? 1 : 0));
    }

    auto items = read_dir(item.meta);

    bool found = false;
    for (std::size_t i = 0; i < items.first.size(); ++i) {
      if (!isStringEqNoCase(items.second[i], dirName)) {
        continue;
      }

      item = items.first[i];
      found = true;
      break;
    }

    if (!found) {
      return {};
    }
  }

  if (isDir) {
    if ((item.meta.flags & iso::DirEntryFlags::Directory) !=
        iso::DirEntryFlags::Directory) {
      return {};
    }
  } else {
    if ((item.meta.flags & iso::DirEntryFlags::Directory) ==
        iso::DirEntryFlags::Directory) {
      return {};
    }
  }

  return item;
}

std::pair<std::vector<iso_entry>, std::vector<std::string>>
iso_fs::read_dir(const iso::DirEntry &entry) {
  if ((entry.flags & iso::DirEntryFlags::Directory) !=
      iso::DirEntryFlags::Directory) {
    return {};
  }

  auto block_size = m_dev->block_size();
  auto total_block_count = (entry.length.value() + block_size - 1) / block_size;
  auto total_buffer_block_count = std::min<std::size_t>(total_block_count, 10);

  std::vector<std::byte> buffer(total_buffer_block_count * block_size);
  auto first_block = entry.lba.value();

  std::vector<iso_entry> isoEntries;
  std::vector<std::string> names;
  bool continues_previous = false;

  for (std::size_t block = first_block, end = first_block + total_block_count;
       block < end;) {
    auto block_count =
        m_dev->read(block, buffer.data(), total_buffer_block_count);
    block += block_count;

    std::size_t buffer_offset = 0;
    std::size_t buffer_size = block_count * block_size;

    std::size_t count = 0;
    while (buffer_offset < buffer_size) {
      auto item = reinterpret_cast<const iso::DirEntry *>(buffer.data() +
                                                          buffer_offset);
      if (item->entry_length == 0) {
        buffer_offset += block_size;
        buffer_offset &= ~(block_size - 1);
        continue;
      }

      auto filename_end =
          sizeof(iso::DirEntry) +
          ((item->filename_length + 1) & ~static_cast<std::size_t>(1));

      if (item->entry_length < filename_end) {
        buffer_offset += item->entry_length;
        continue;
      }

      auto susp_area = std::span(buffer.data() + buffer_offset + filename_end,
                                 item->entry_length - filename_end);
      struct [[gnu::packed]] SuspHeader {
        char signature[2];
        u8 length;
        u8 version;
      };

      if (susp_area.size() > sizeof(SuspHeader)) {
        auto susp_header = reinterpret_cast<SuspHeader *>(susp_area.data());

        // TODO: extensions support
      }

      buffer_offset += item->entry_length;

      if (item->filename_length == 0 ||
          item->filename_length + sizeof(iso::DirEntry) > item->entry_length) {
        continue;
      }

      auto filename =
          item->filename_length > 1
              ? decodeString({reinterpret_cast<const char *>(item + 1),
                              item->filename_length},
                             m_encoding)
              : std::string{};

      if (item->filename_length == 1) {
        char c = *reinterpret_cast<const char *>(item + 1);
        // can be special name
        if (c == 0) {
          filename = ".";
        } else if (c == 1) {
          filename = "..";
        }
      }

      filename = filename.substr(0, filename.find(';'));
      if (filename.empty()) {
        continue;
      }

      const bool has_more =
          (item->flags & iso::DirEntryFlags::MultiExtent) !=
          iso::DirEntryFlags::None;

      if (continues_previous && !names.empty() && names.back() == filename) {
        isoEntries.back().extents.push_back(*item);
        isoEntries.back().size += item->length.value();
      } else {
        iso_entry merged{};
        merged.meta = *item;
        merged.extents.push_back(*item);
        merged.size = item->length.value();
        isoEntries.push_back(std::move(merged));
        names.emplace_back(filename);
      }

      continues_previous = has_more;
    }
  }

  return {std::move(isoEntries), std::move(names)};
}

fs::file iso_fs::read_file(const iso_entry &entry) {
  if ((entry.meta.flags & iso::DirEntryFlags::Directory) ==
      iso::DirEntryFlags::Directory) {
    return {};
  }

  if (entry.size == 0) {
    return fs::make_stream<std::vector<std::uint8_t>>();
  }

  const auto block_size = m_dev->block_size();

  std::vector<std::uint8_t> data;
  data.resize(entry.size);

  std::uint64_t written = 0;
  std::vector<std::uint8_t> chunk;

  for (const auto &extent : entry.extents) {
    const std::uint64_t length = extent.length.value();
    if (length == 0) {
      continue;
    }

    const auto block_count = (length + block_size - 1) / block_size;
    chunk.resize(block_count * block_size);

    if (!m_dev->read(extent.lba.value(), chunk.data(), block_count)) {
      return {};
    }

    std::memcpy(data.data() + written, chunk.data(), length);
    written += length;
  }

  data.resize(written);
  return fs::make_stream(std::move(data), entry.to_fs_stat());
}

bool iso_fs::extract(const std::filesystem::path &path,
                     const std::string &dest) {
  auto optEntry = open_entry(path, false);
  if (!optEntry) {
    return false;
  }

  const auto &entry = *optEntry;

  fs::file out(dest, fs::open_mode::create + fs::open_mode::trunc +
                         fs::open_mode::write);
  if (!out) {
    return false;
  }

  const auto block_size = m_dev->block_size();
  constexpr std::size_t max_blocks = 4096;

  std::vector<std::uint8_t> chunk(max_blocks * block_size);

  for (const auto &extent : entry.extents) {
    std::uint64_t remaining = extent.length.value();
    std::size_t lba = extent.lba.value();

    while (remaining > 0) {
      const auto want_blocks = std::min<std::uint64_t>(
          max_blocks, (remaining + block_size - 1) / block_size);
      const auto got_blocks = m_dev->read(lba, chunk.data(), want_blocks);

      if (got_blocks == 0) {
        return false;
      }

      const auto usable =
          std::min<std::uint64_t>(remaining, got_blocks * block_size);

      if (out.write(chunk.data(), usable) != usable) {
        return false;
      }

      remaining -= usable;
      lba += got_blocks;
    }
  }

  return true;
}
