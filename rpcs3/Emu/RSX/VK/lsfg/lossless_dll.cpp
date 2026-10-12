// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later
// ReX/PS5 adaptation of Eden 5f142c7926d0c7fcbbd0ce30794d72f638a43b2a:
// bounded PE resource traversal, native-only SPIR-V, descriptor-based file I/O.
// The local DLL is never linked, loaded as code, modified or copied into a cache.

#include "lossless_dll.h"
#include "lsfg_translate.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <optional>
#include <sys/stat.h>
#include <unistd.h>

namespace rex::ui::vulkan::framegen {
namespace {
constexpr u16 kDosMagic = 0x5A4D;
constexpr u32 kPeSignature = 0x00004550;
constexpr u16 kPe32Magic = 0x010B;
constexpr u16 kPe32PlusMagic = 0x020B;
constexpr u32 kSubdirectory = 0x80000000;
constexpr u32 kRcData = 10;
constexpr size_t kMaximumDirectoryEntries = 4096;

class Reader {
 public:
  explicit Reader(std::span<const u8> bytes) : bytes_(bytes) {}
  template <typename T> bool Read(size_t offset, T& value) const {
    if (offset > bytes_.size() || sizeof(T) > bytes_.size() - offset) return false;
    std::memcpy(&value, bytes_.data() + offset, sizeof(T));
    return true;
  }
  std::optional<std::span<const u8>> Slice(size_t offset, size_t length) const {
    if (offset > bytes_.size() || length > bytes_.size() - offset) return std::nullopt;
    return bytes_.subspan(offset, length);
  }

 private:
  std::span<const u8> bytes_;
};

struct Section {
  u32 virtual_address;
  u32 raw_address;
  u32 raw_size;
};
struct ResourceEntry {
  u32 id;
  u32 offset;
  bool directory;
  bool named;
};
using ResourceSpans = std::map<u32, std::span<const u8>>;

// Resource data must be physically backed by one section in the file. An RVA
// inside a larger virtual zero-fill area cannot supply shader bytes.
std::optional<size_t> FileOffset(std::span<const Section> sections, u32 rva, u32 length) {
  for (const auto& section : sections) {
    if (rva < section.virtual_address) continue;
    const u32 relative = rva - section.virtual_address;
    if (relative <= section.raw_size && length <= section.raw_size - relative) {
      return size_t(section.raw_address) + relative;
    }
  }
  return std::nullopt;
}

bool Entries(const Reader& resource, size_t offset, std::vector<ResourceEntry>& entries) {
  u16 named{}, numbered{};
  if (!resource.Read(offset + 12, named) || !resource.Read(offset + 14, numbered)) return false;
  const size_t count = size_t(named) + numbered;
  if (count > kMaximumDirectoryEntries || !resource.Slice(offset + 16, count * 8)) return false;
  entries.clear();
  entries.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    u32 name{}, data{};
    if (!resource.Read(offset + 16 + i * 8, name) ||
        !resource.Read(offset + 20 + i * 8, data)) return false;
    entries.push_back({name & ~kSubdirectory, data & ~kSubdirectory,
                       (data & kSubdirectory) != 0, (name & kSubdirectory) != 0});
  }
  return true;
}

bool CollectResources(const Reader& file, std::span<const Section> sections,
                       std::span<const u8> resource_bytes, ResourceSpans& resources) {
  const Reader resource(resource_bytes);
  std::vector<ResourceEntry> types;
  if (!Entries(resource, 0, types)) return false;
  bool found_rcdata = false;
  for (const auto& type : types) {
    if (type.named || type.id != kRcData) continue;
    if (!type.directory || found_rcdata) return false;
    found_rcdata = true;
    std::vector<ResourceEntry> names;
    if (!Entries(resource, type.offset, names)) return false;
    for (const auto& name : names) {
      if (name.named) continue;
      if (!name.directory || resources.contains(name.id)) return false;
      std::vector<ResourceEntry> languages;
      if (!Entries(resource, name.offset, languages)) return false;
      for (const auto& language : languages) {
        if (language.named || language.directory) continue;
        u32 rva{}, size{};
        // IMAGE_RESOURCE_DATA_ENTRY occupies sixteen bytes, not just the two
        // fields we read. Directory offsets must stay inside the directory.
        if (!resource.Slice(language.offset, 16) ||
            !resource.Read(language.offset, rva) ||
            !resource.Read(size_t(language.offset) + 4, size) || !size) return false;
        const auto offset = FileOffset(sections, rva, size);
        if (!offset) return false;
        const auto bytes = file.Slice(*offset, size);
        if (!bytes) return false;
        resources.emplace(name.id, *bytes);
        break;
      }
    }
  }
  return found_rcdata;
}

bool ReadResources(std::span<const u8> image, ResourceSpans& resources) {
  const Reader file(image);
  u16 dos{};
  u32 pe{}, signature{};
  if (!file.Read(0, dos) || dos != kDosMagic || !file.Read(0x3C, pe) ||
      !file.Read(pe, signature) || signature != kPeSignature) return false;
  const size_t coff = size_t(pe) + 4;
  u16 count{}, optional_size{}, magic{};
  if (!file.Read(coff + 2, count) || !count || count > 128 ||
      !file.Read(coff + 16, optional_size)) return false;
  const size_t optional = coff + 20;
  if (!file.Slice(optional, optional_size) || !file.Read(optional, magic)) return false;
  const size_t data_offset = magic == kPe32Magic ? 96 : magic == kPe32PlusMagic ? 112 : 0;
  if (!data_offset || optional_size < data_offset + 3 * 8) return false;
  u32 directory_count{}, resource_rva{}, resource_size{};
  if (!file.Read(optional + data_offset - 4, directory_count) || directory_count < 3 ||
      !file.Read(optional + data_offset + 2 * 8, resource_rva) ||
      !file.Read(optional + data_offset + 2 * 8 + 4, resource_size) || !resource_size) return false;
  const size_t table = optional + optional_size;
  if (!file.Slice(table, size_t(count) * 40)) return false;
  std::vector<Section> sections;
  sections.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    Section section{};
    const size_t offset = table + i * 40;
    if (!file.Read(offset + 12, section.virtual_address) ||
        !file.Read(offset + 16, section.raw_size) ||
        !file.Read(offset + 20, section.raw_address) ||
        !file.Slice(section.raw_address, section.raw_size)) return false;
    sections.push_back(section);
  }
  const auto base = FileOffset(sections, resource_rva, resource_size);
  if (!base) return false;
  const auto bytes = file.Slice(*base, resource_size);
  return bytes && CollectResources(file, sections, *bytes, resources);
}

constexpr std::array<u32, 25> ShaderIds() {
  std::array<u32, 25> ids{255, 256};
  for (u32 i = 0; i < 23; ++i) ids[i + 2] = 280 + i;
  return ids;
}

bool HasVariant(const ResourceSpans& resources, u32 offset) {
  for (u32 id : ShaderIds()) {
    const auto it = resources.find(id + offset);
    if (it == resources.end() || it->second.size() > kMaximumShaderBytes ||
        !IsSpirvModule(it->second)) return false;
  }
  return true;
}
}  // namespace

ShaderLoadResult ParseShaderModules(std::span<const u8> image, ShaderModules& out_modules,
                                    bool allow_fp16, bool prefer_fp16) {
  out_modules.clear();
  ShaderLoadResult result;
  result.source_bytes = image.size();
  if (image.size() > kMaximumDllBytes) {
    result.status = LosslessStatus::TooLarge;
    result.detail = "shader container exceeds 256 MiB runtime read limit";
    return result;
  }
  ResourceSpans resources;
  if (!ReadResources(image, resources)) {
    result.status = LosslessStatus::NotPortableExecutable;
    result.detail = "missing or invalid PE RCDATA directory bounds";
    return result;
  }
  const bool fp16 = allow_fp16 && HasVariant(resources, PerformanceShader::NATIVE_FP16_OFFSET);
  const bool fp32 = HasVariant(resources, PerformanceShader::NATIVE_FP32_OFFSET);
  if (!fp32 && !fp16) {
    result.status = LosslessStatus::MissingShaders;
    result.detail = "all 25 native FP32 or supported FP16 SPIR-V resources are required";
    return result;
  }
  const bool use_fp16 = fp16 && (prefer_fp16 || !fp32);
  const u32 offset = use_fp16 ? PerformanceShader::NATIVE_FP16_OFFSET
                             : PerformanceShader::NATIVE_FP32_OFFSET;
  result.variant = use_fp16 ? ShaderVariant::NativeFp16 : ShaderVariant::NativeFp32;
  ShaderModules modules;
  for (u32 id : ShaderIds()) {
    auto words = AdoptSpirvModule(resources.at(id + offset));
    if (words.empty()) {
      result.status = LosslessStatus::InvalidShader;
      result.detail = "malformed SPIR-V instruction in resource " + std::to_string(id + offset);
      return result;
    }
    modules.emplace(id, std::move(words));
  }
  out_modules = std::move(modules);
  result.status = LosslessStatus::Ok;
  return result;
}

ShaderLoadResult LoadShaderModules(const std::string& path, ShaderModules& out_modules,
                                   bool allow_fp16, bool prefer_fp16) {
  out_modules.clear();
  if (path.empty()) return {LosslessStatus::NotInstalled, {}, 0, "local Lossless.dll absent"};
  const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW);
  if (fd < 0) {
    const int error = errno;
    return {error == ENOENT ? LosslessStatus::NotInstalled : LosslessStatus::UnreadableFile,
            {}, 0, "open errno=" + std::to_string(error)};
  }
  struct Close { int fd; ~Close() { close(fd); } } descriptor{fd};
  struct stat info {};
  if (fstat(fd, &info) != 0) {
    return {LosslessStatus::UnreadableFile, {}, 0, "fstat errno=" + std::to_string(errno)};
  }
  if (!S_ISREG(info.st_mode) || info.st_size < 0) {
    return {LosslessStatus::UnreadableFile, {}, 0, "source is not a regular file"};
  }
  if (u64(info.st_size) > kMaximumDllBytes) {
    return {LosslessStatus::TooLarge, {}, u64(info.st_size), "source exceeds runtime read limit"};
  }
  std::vector<u8> bytes(size_t(info.st_size));
  size_t offset = 0;
  while (offset < bytes.size()) {
    const ssize_t count = read(fd, bytes.data() + offset, bytes.size() - offset);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) {
      return {LosslessStatus::UnreadableFile, {}, u64(info.st_size),
              "short/failed read errno=" + std::to_string(count < 0 ? errno : 0)};
    }
    offset += size_t(count);
  }
  u8 extra{};
  ssize_t extra_count;
  do { extra_count = read(fd, &extra, 1); } while (extra_count < 0 && errno == EINTR);
  if (extra_count != 0) {
    return {LosslessStatus::UnreadableFile, {}, u64(info.st_size), "source changed during read"};
  }
  return ParseShaderModules(bytes, out_modules, allow_fp16, prefer_fp16);
}

const char* LosslessStatusName(LosslessStatus status) {
  switch (status) {
    case LosslessStatus::Ok: return "ok";
    case LosslessStatus::NotInstalled: return "not_installed";
    case LosslessStatus::UnreadableFile: return "unreadable";
    case LosslessStatus::NotPortableExecutable: return "invalid_pe_resources";
    case LosslessStatus::MissingShaders: return "missing_native_shaders";
    case LosslessStatus::InvalidShader: return "invalid_spirv";
    case LosslessStatus::TooLarge: return "runtime_size_limit";
  }
  return "unknown";
}
}  // namespace rex::ui::vulkan::framegen
