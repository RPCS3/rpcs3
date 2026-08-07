#pragma once

#include "Utilities/File.h"
#include <cstddef>
#include <utility>

class block_dev {
  std::size_t m_block_size = 0;
  std::size_t m_block_count = 0;

public:
  virtual ~block_dev() = default;

  std::size_t block_size() const { return m_block_size; }
  std::size_t block_count() const { return m_block_count; }
  std::size_t size() const { return block_size() * block_count(); }

  virtual std::size_t read(std::size_t blockIndex, void *data,
                           std::size_t blockCount) = 0;

  virtual std::size_t write(std::size_t blockIndex, const void *data,
                            std::size_t blockCount) = 0;

protected:
  void set_block_info(std::size_t size, std::size_t count) {
    m_block_size = size;
    m_block_count = count;
  }
};

class file_block_dev final : public block_dev {
  fs::file m_file;

public:
  explicit file_block_dev(fs::file file, std::size_t blockSize = 2048)
      : m_file(std::move(file)) {
    set_block_info(blockSize, m_file.size() / blockSize);
  }

  std::size_t read(std::size_t blockIndex, void *data,
                   std::size_t blockCount) override {
    auto result = m_file.read_at(block_size() * blockIndex, data,
                                 blockCount * block_size());
    return result / block_size();
  }

  std::size_t write(std::size_t blockIndex, const void *data,
                    std::size_t blockCount) override {
    auto result = m_file.write_at(block_size() * blockIndex, data,
                                  blockCount * block_size());
    return result / block_size();
  }

  fs::file &file() { return m_file; }
  fs::file release() { return std::exchange(m_file, {}); }
};

class file_view_block_dev final : public block_dev {
  const fs::file *m_file;

public:
  explicit file_view_block_dev(const fs::file &file,
                               std::size_t blockSize = 2048)
      : m_file(&file) {
    set_block_info(blockSize, m_file->size() / blockSize);
  }

  std::size_t read(std::size_t blockIndex, void *data,
                   std::size_t blockCount) override {
    auto result = m_file->read_at(block_size() * blockIndex, data,
                                  blockCount * block_size());
    return result / block_size();
  }

  std::size_t write(std::size_t blockIndex, const void *data,
                    std::size_t blockCount) override {
    auto result = m_file->write_at(block_size() * blockIndex, data,
                                   blockCount * block_size());
    return result / block_size();
  }
};
