#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "achievement_manager.h"

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

class AchievementEvents final {
public:
    static void DISCORD_CALLBACK OnUserAchievementUpdate(void* callbackData,
                                                         DiscordUserAchievement* userAchievement)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->AchievementManager();
        module.OnUserAchievementUpdate(*reinterpret_cast<UserAchievement const*>(userAchievement));
    }
};

IDiscordAchievementEvents AchievementManager::events_{
  &AchievementEvents::OnUserAchievementUpdate,
};

void AchievementManager::SetUserAchievement(Snowflake achievementId,
                                            std::uint8_t percentComplete,
                                            std::function<void(Result)> callback)
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
    internal_->set_user_achievement(
      internal_, achievementId, percentComplete, cb.release(), wrapper);
}

void AchievementManager::FetchUserAchievements(std::function<void(Result)> callback)
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
    internal_->fetch_user_achievements(internal_, cb.release(), wrapper);
}

void AchievementManager::CountUserAchievements(std::int32_t* count)
{
    if (!count) {
        return;
    }

    internal_->count_user_achievements(internal_, reinterpret_cast<int32_t*>(count));
}

Result AchievementManager::GetUserAchievement(Snowflake userAchievementId,
                                              UserAchievement* userAchievement)
{
    if (!userAchievement) {
        return Result::InternalError;
    }

    auto result = internal_->get_user_achievement(
      internal_, userAchievementId, reinterpret_cast<DiscordUserAchievement*>(userAchievement));
    return static_cast<Result>(result);
}

Result AchievementManager::GetUserAchievementAt(std::int32_t index,
                                                UserAchievement* userAchievement)
{
    if (!userAchievement) {
        return Result::InternalError;
    }

    auto result = internal_->get_user_achievement_at(
      internal_, index, reinterpret_cast<DiscordUserAchievement*>(userAchievement));
    return static_cast<Result>(result);
}

} // namespace discord
