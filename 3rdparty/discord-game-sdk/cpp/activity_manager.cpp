#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "activity_manager.h"

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

class ActivityEvents final {
public:
    static void DISCORD_CALLBACK OnActivityJoin(void* callbackData, char const* secret)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->ActivityManager();
        module.OnActivityJoin(static_cast<const char*>(secret));
    }

    static void DISCORD_CALLBACK OnActivitySpectate(void* callbackData, char const* secret)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->ActivityManager();
        module.OnActivitySpectate(static_cast<const char*>(secret));
    }

    static void DISCORD_CALLBACK OnActivityJoinRequest(void* callbackData, DiscordUser* user)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->ActivityManager();
        module.OnActivityJoinRequest(*reinterpret_cast<User const*>(user));
    }

    static void DISCORD_CALLBACK OnActivityInvite(void* callbackData,
                                                  EDiscordActivityActionType type,
                                                  DiscordUser* user,
                                                  DiscordActivity* activity)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->ActivityManager();
        module.OnActivityInvite(static_cast<ActivityActionType>(type),
                                *reinterpret_cast<User const*>(user),
                                *reinterpret_cast<Activity const*>(activity));
    }
};

IDiscordActivityEvents ActivityManager::events_{
  &ActivityEvents::OnActivityJoin,
  &ActivityEvents::OnActivitySpectate,
  &ActivityEvents::OnActivityJoinRequest,
  &ActivityEvents::OnActivityInvite,
};

Result ActivityManager::RegisterCommand(char const* command)
{
    auto result = internal_->register_command(internal_, const_cast<char*>(command));
    return static_cast<Result>(result);
}

Result ActivityManager::RegisterSteam(std::uint32_t steamId)
{
    auto result = internal_->register_steam(internal_, steamId);
    return static_cast<Result>(result);
}

void ActivityManager::UpdateActivity(Activity const& activity, std::function<void(Result)> callback)
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
    internal_->update_activity(internal_,
                               reinterpret_cast<DiscordActivity*>(const_cast<Activity*>(&activity)),
                               cb.release(),
                               wrapper);
}

void ActivityManager::ClearActivity(std::function<void(Result)> callback)
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
    internal_->clear_activity(internal_, cb.release(), wrapper);
}

void ActivityManager::SendRequestReply(UserId userId,
                                       ActivityJoinRequestReply reply,
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
    internal_->send_request_reply(internal_,
                                  userId,
                                  static_cast<EDiscordActivityJoinRequestReply>(reply),
                                  cb.release(),
                                  wrapper);
}

void ActivityManager::SendInvite(UserId userId,
                                 ActivityActionType type,
                                 char const* content,
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
    internal_->send_invite(internal_,
                           userId,
                           static_cast<EDiscordActivityActionType>(type),
                           const_cast<char*>(content),
                           cb.release(),
                           wrapper);
}

void ActivityManager::AcceptInvite(UserId userId, std::function<void(Result)> callback)
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
    internal_->accept_invite(internal_, userId, cb.release(), wrapper);
}

} // namespace discord
