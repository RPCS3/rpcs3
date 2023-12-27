#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

Result Core::Create(ClientId clientId, std::uint64_t flags, Core** instance)
{
    if (!instance) {
        return Result::InternalError;
    }

    (*instance) = new Core();
    DiscordCreateParams params{};
    DiscordCreateParamsSetDefault(&params);
    params.client_id = clientId;
    params.flags = flags;
    params.events = nullptr;
    params.event_data = *instance;
    params.user_events = &UserManager::events_;
    params.activity_events = &ActivityManager::events_;
    params.relationship_events = &RelationshipManager::events_;
    params.lobby_events = &LobbyManager::events_;
    params.network_events = &NetworkManager::events_;
    params.overlay_events = &OverlayManager::events_;
    params.store_events = &StoreManager::events_;
    params.voice_events = &VoiceManager::events_;
    params.achievement_events = &AchievementManager::events_;
    auto result = DiscordCreate(DISCORD_VERSION, &params, &((*instance)->internal_));
    if (result != DiscordResult_Ok || !(*instance)->internal_) {
        delete (*instance);
        (*instance) = nullptr;
    }

    return static_cast<Result>(result);
}

Core::~Core()
{
    if (internal_) {
        internal_->destroy(internal_);
        internal_ = nullptr;
    }
}

Result Core::RunCallbacks()
{
    auto result = internal_->run_callbacks(internal_);
    return static_cast<Result>(result);
}

void Core::SetLogHook(LogLevel minLevel, std::function<void(LogLevel, char const*)> hook)
{
    setLogHook_.DisconnectAll();
    setLogHook_.Connect(std::move(hook));
    static auto wrapper =
      [](void* callbackData, EDiscordLogLevel level, char const* message) -> void {
        auto cb(reinterpret_cast<decltype(setLogHook_)*>(callbackData));
        if (!cb) {
            return;
        }
        (*cb)(static_cast<LogLevel>(level), static_cast<const char*>(message));
    };

    internal_->set_log_hook(
      internal_, static_cast<EDiscordLogLevel>(minLevel), &setLogHook_, wrapper);
}

discord::ApplicationManager& Core::ApplicationManager()
{
    if (!applicationManager_.internal_) {
        applicationManager_.internal_ = internal_->get_application_manager(internal_);
    }

    return applicationManager_;
}

discord::UserManager& Core::UserManager()
{
    if (!userManager_.internal_) {
        userManager_.internal_ = internal_->get_user_manager(internal_);
    }

    return userManager_;
}

discord::ImageManager& Core::ImageManager()
{
    if (!imageManager_.internal_) {
        imageManager_.internal_ = internal_->get_image_manager(internal_);
    }

    return imageManager_;
}

discord::ActivityManager& Core::ActivityManager()
{
    if (!activityManager_.internal_) {
        activityManager_.internal_ = internal_->get_activity_manager(internal_);
    }

    return activityManager_;
}

discord::RelationshipManager& Core::RelationshipManager()
{
    if (!relationshipManager_.internal_) {
        relationshipManager_.internal_ = internal_->get_relationship_manager(internal_);
    }

    return relationshipManager_;
}

discord::LobbyManager& Core::LobbyManager()
{
    if (!lobbyManager_.internal_) {
        lobbyManager_.internal_ = internal_->get_lobby_manager(internal_);
    }

    return lobbyManager_;
}

discord::NetworkManager& Core::NetworkManager()
{
    if (!networkManager_.internal_) {
        networkManager_.internal_ = internal_->get_network_manager(internal_);
    }

    return networkManager_;
}

discord::OverlayManager& Core::OverlayManager()
{
    if (!overlayManager_.internal_) {
        overlayManager_.internal_ = internal_->get_overlay_manager(internal_);
    }

    return overlayManager_;
}

discord::StorageManager& Core::StorageManager()
{
    if (!storageManager_.internal_) {
        storageManager_.internal_ = internal_->get_storage_manager(internal_);
    }

    return storageManager_;
}

discord::StoreManager& Core::StoreManager()
{
    if (!storeManager_.internal_) {
        storeManager_.internal_ = internal_->get_store_manager(internal_);
    }

    return storeManager_;
}

discord::VoiceManager& Core::VoiceManager()
{
    if (!voiceManager_.internal_) {
        voiceManager_.internal_ = internal_->get_voice_manager(internal_);
    }

    return voiceManager_;
}

discord::AchievementManager& Core::AchievementManager()
{
    if (!achievementManager_.internal_) {
        achievementManager_.internal_ = internal_->get_achievement_manager(internal_);
    }

    return achievementManager_;
}

} // namespace discord
