#pragma once

#include "types.h"

namespace discord {

class StorageManager final {
public:
    ~StorageManager() = default;

    Result Read(char const* name,
                std::uint8_t* data,
                std::uint32_t dataLength,
                std::uint32_t* read);
    void ReadAsync(char const* name,
                   std::function<void(Result, std::uint8_t*, std::uint32_t)> callback);
    void ReadAsyncPartial(char const* name,
                          std::uint64_t offset,
                          std::uint64_t length,
                          std::function<void(Result, std::uint8_t*, std::uint32_t)> callback);
    Result Write(char const* name, std::uint8_t* data, std::uint32_t dataLength);
    void WriteAsync(char const* name,
                    std::uint8_t* data,
                    std::uint32_t dataLength,
                    std::function<void(Result)> callback);
    Result Delete(char const* name);
    Result Exists(char const* name, bool* exists);
    void Count(std::int32_t* count);
    Result Stat(char const* name, FileStat* stat);
    Result StatAt(std::int32_t index, FileStat* stat);
    Result GetPath(char path[4096]);

private:
    friend class Core;

    StorageManager() = default;
    StorageManager(StorageManager const& rhs) = delete;
    StorageManager& operator=(StorageManager const& rhs) = delete;
    StorageManager(StorageManager&& rhs) = delete;
    StorageManager& operator=(StorageManager&& rhs) = delete;

    IDiscordStorageManager* internal_;
    static IDiscordStorageEvents events_;
};

} // namespace discord
