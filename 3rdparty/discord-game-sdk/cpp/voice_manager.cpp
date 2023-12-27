#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "voice_manager.h"

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

class VoiceEvents final {
public:
    static void DISCORD_CALLBACK OnSettingsUpdate(void* callbackData)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->VoiceManager();
        module.OnSettingsUpdate();
    }
};

IDiscordVoiceEvents VoiceManager::events_{
  &VoiceEvents::OnSettingsUpdate,
};

Result VoiceManager::GetInputMode(InputMode* inputMode)
{
    if (!inputMode) {
        return Result::InternalError;
    }

    auto result =
      internal_->get_input_mode(internal_, reinterpret_cast<DiscordInputMode*>(inputMode));
    return static_cast<Result>(result);
}

void VoiceManager::SetInputMode(InputMode inputMode, std::function<void(Result)> callback)
{
    static auto wrapper = [](void* callbackData, EDiscordResult result) -> void {
        std::unique_ptr<std::function<void(Result)>> cb(
          reinterpret_cast<std::function<void(Result)>*>(callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(static_cast<Result>(result));
    };
    std::unique_ptr<std::function<void(Result)>> cb{};
    cb.reset(new std::function<void(Result)>(std::move(callback)));
    internal_->set_input_mode(
      internal_, *reinterpret_cast<DiscordInputMode const*>(&inputMode), cb.release(), wrapper);
}

Result VoiceManager::IsSelfMute(bool* mute)
{
    if (!mute) {
        return Result::InternalError;
    }

    auto result = internal_->is_self_mute(internal_, reinterpret_cast<bool*>(mute));
    return static_cast<Result>(result);
}

Result VoiceManager::SetSelfMute(bool mute)
{
    auto result = internal_->set_self_mute(internal_, (mute ? 1 : 0));
    return static_cast<Result>(result);
}

Result VoiceManager::IsSelfDeaf(bool* deaf)
{
    if (!deaf) {
        return Result::InternalError;
    }

    auto result = internal_->is_self_deaf(internal_, reinterpret_cast<bool*>(deaf));
    return static_cast<Result>(result);
}

Result VoiceManager::SetSelfDeaf(bool deaf)
{
    auto result = internal_->set_self_deaf(internal_, (deaf ? 1 : 0));
    return static_cast<Result>(result);
}

Result VoiceManager::IsLocalMute(Snowflake userId, bool* mute)
{
    if (!mute) {
        return Result::InternalError;
    }

    auto result = internal_->is_local_mute(internal_, userId, reinterpret_cast<bool*>(mute));
    return static_cast<Result>(result);
}

Result VoiceManager::SetLocalMute(Snowflake userId, bool mute)
{
    auto result = internal_->set_local_mute(internal_, userId, (mute ? 1 : 0));
    return static_cast<Result>(result);
}

Result VoiceManager::GetLocalVolume(Snowflake userId, std::uint8_t* volume)
{
    if (!volume) {
        return Result::InternalError;
    }

    auto result =
      internal_->get_local_volume(internal_, userId, reinterpret_cast<uint8_t*>(volume));
    return static_cast<Result>(result);
}

Result VoiceManager::SetLocalVolume(Snowflake userId, std::uint8_t volume)
{
    auto result = internal_->set_local_volume(internal_, userId, volume);
    return static_cast<Result>(result);
}

} // namespace discord
