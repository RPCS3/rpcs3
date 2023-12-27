#pragma once

#include "types.h"
#include "application_manager.h"
#include "user_manager.h"
#include "image_manager.h"
#include "activity_manager.h"
#include "relationship_manager.h"
#include "lobby_manager.h"
#include "network_manager.h"
#include "overlay_manager.h"
#include "storage_manager.h"
#include "store_manager.h"
#include "voice_manager.h"
#include "achievement_manager.h"

namespace discord {

class Core final {
public:
    static Result Create(ClientId clientId, std::uint64_t flags, Core** instance);

    ~Core();

    Result RunCallbacks();
    void SetLogHook(LogLevel minLevel, std::function<void(LogLevel, char const*)> hook);

    discord::ApplicationManager& ApplicationManager();
    discord::UserManager& UserManager();
    discord::ImageManager& ImageManager();
    discord::ActivityManager& ActivityManager();
    discord::RelationshipManager& RelationshipManager();
    discord::LobbyManager& LobbyManager();
    discord::NetworkManager& NetworkManager();
    discord::OverlayManager& OverlayManager();
    discord::StorageManager& StorageManager();
    discord::StoreManager& StoreManager();
    discord::VoiceManager& VoiceManager();
    discord::AchievementManager& AchievementManager();

private:
    Core() = default;
    Core(Core const& rhs) = delete;
    Core& operator=(Core const& rhs) = delete;
    Core(Core&& rhs) = delete;
    Core& operator=(Core&& rhs) = delete;

    IDiscordCore* internal_;
    Event<LogLevel, char const*> setLogHook_;
    discord::ApplicationManager applicationManager_;
    discord::UserManager userManager_;
    discord::ImageManager imageManager_;
    discord::ActivityManager activityManager_;
    discord::RelationshipManager relationshipManager_;
    discord::LobbyManager lobbyManager_;
    discord::NetworkManager networkManager_;
    discord::OverlayManager overlayManager_;
    discord::StorageManager storageManager_;
    discord::StoreManager storeManager_;
    discord::VoiceManager voiceManager_;
    discord::AchievementManager achievementManager_;
};

} // namespace discord
