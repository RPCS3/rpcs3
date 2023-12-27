#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "image_manager.h"

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

void ImageManager::Fetch(ImageHandle handle,
                         bool refresh,
                         std::function<void(Result, ImageHandle)> callback)
{
    static auto wrapper =
      [](void* callbackData, EDiscordResult result, DiscordImageHandle handleResult) -> void {
        std::unique_ptr<std::function<void(Result, ImageHandle)>> cb(
          reinterpret_cast<std::function<void(Result, ImageHandle)>*>(callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(static_cast<Result>(result), *reinterpret_cast<ImageHandle const*>(&handleResult));
    };
    std::unique_ptr<std::function<void(Result, ImageHandle)>> cb{};
    cb.reset(new std::function<void(Result, ImageHandle)>(std::move(callback)));
    internal_->fetch(internal_,
                     *reinterpret_cast<DiscordImageHandle const*>(&handle),
                     (refresh ? 1 : 0),
                     cb.release(),
                     wrapper);
}

Result ImageManager::GetDimensions(ImageHandle handle, ImageDimensions* dimensions)
{
    if (!dimensions) {
        return Result::InternalError;
    }

    auto result = internal_->get_dimensions(internal_,
                                            *reinterpret_cast<DiscordImageHandle const*>(&handle),
                                            reinterpret_cast<DiscordImageDimensions*>(dimensions));
    return static_cast<Result>(result);
}

Result ImageManager::GetData(ImageHandle handle, std::uint8_t* data, std::uint32_t dataLength)
{
    auto result = internal_->get_data(internal_,
                                      *reinterpret_cast<DiscordImageHandle const*>(&handle),
                                      reinterpret_cast<uint8_t*>(data),
                                      dataLength);
    return static_cast<Result>(result);
}

} // namespace discord
