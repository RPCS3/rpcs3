#pragma once

#include "types.h"

namespace discord {

class VoiceManager final {
public:
    ~VoiceManager() = default;

    Result GetInputMode(InputMode* inputMode);
    void SetInputMode(InputMode inputMode, std::function<void(Result)> callback);
    Result IsSelfMute(bool* mute);
    Result SetSelfMute(bool mute);
    Result IsSelfDeaf(bool* deaf);
    Result SetSelfDeaf(bool deaf);
    Result IsLocalMute(Snowflake userId, bool* mute);
    Result SetLocalMute(Snowflake userId, bool mute);
    Result GetLocalVolume(Snowflake userId, std::uint8_t* volume);
    Result SetLocalVolume(Snowflake userId, std::uint8_t volume);

    Event<> OnSettingsUpdate;

private:
    friend class Core;

    VoiceManager() = default;
    VoiceManager(VoiceManager const& rhs) = delete;
    VoiceManager& operator=(VoiceManager const& rhs) = delete;
    VoiceManager(VoiceManager&& rhs) = delete;
    VoiceManager& operator=(VoiceManager&& rhs) = delete;

    IDiscordVoiceManager* internal_;
    static IDiscordVoiceEvents events_;
};

} // namespace discord
