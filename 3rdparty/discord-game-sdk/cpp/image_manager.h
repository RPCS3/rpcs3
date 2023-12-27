#pragma once

#include "types.h"

namespace discord {

class ImageManager final {
public:
    ~ImageManager() = default;

    void Fetch(ImageHandle handle, bool refresh, std::function<void(Result, ImageHandle)> callback);
    Result GetDimensions(ImageHandle handle, ImageDimensions* dimensions);
    Result GetData(ImageHandle handle, std::uint8_t* data, std::uint32_t dataLength);

private:
    friend class Core;

    ImageManager() = default;
    ImageManager(ImageManager const& rhs) = delete;
    ImageManager& operator=(ImageManager const& rhs) = delete;
    ImageManager(ImageManager&& rhs) = delete;
    ImageManager& operator=(ImageManager&& rhs) = delete;

    IDiscordImageManager* internal_;
    static IDiscordImageEvents events_;
};

} // namespace discord
