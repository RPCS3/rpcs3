#pragma once

#include "Utilities/File.h"
#include <cstdint>
#include <filesystem>

class fs_provider {
public:
  enum class handle : std::uintptr_t {
    invalid = ~static_cast<std::uintptr_t>(0)
  };

  virtual ~fs_provider() = default;
  virtual fs::file open(const std::filesystem::path &path,
                        fs::open_mode mode = fs::open_mode::read) = 0;
  virtual fs::dir open_dir(const std::filesystem::path &path) = 0;
};
