#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "application_manager.h"

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

void ApplicationManager::ValidateOrExit(std::function<void(Result)> callback)
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
    internal_->validate_or_exit(internal_, cb.release(), wrapper);
}

void ApplicationManager::GetCurrentLocale(char locale[128])
{
    if (!locale) {
        return;
    }

    internal_->get_current_locale(internal_, reinterpret_cast<DiscordLocale*>(locale));
}

void ApplicationManager::GetCurrentBranch(char branch[4096])
{
    if (!branch) {
        return;
    }

    internal_->get_current_branch(internal_, reinterpret_cast<DiscordBranch*>(branch));
}

void ApplicationManager::GetOAuth2Token(std::function<void(Result, OAuth2Token const&)> callback)
{
    static auto wrapper =
      [](void* callbackData, EDiscordResult result, DiscordOAuth2Token* oauth2Token) -> void {
        std::unique_ptr<std::function<void(Result, OAuth2Token const&)>> cb(
          reinterpret_cast<std::function<void(Result, OAuth2Token const&)>*>(callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(static_cast<Result>(result), *reinterpret_cast<OAuth2Token const*>(oauth2Token));
    };
    std::unique_ptr<std::function<void(Result, OAuth2Token const&)>> cb{};
    cb.reset(new std::function<void(Result, OAuth2Token const&)>(std::move(callback)));
    internal_->get_oauth2_token(internal_, cb.release(), wrapper);
}

void ApplicationManager::GetTicket(std::function<void(Result, char const*)> callback)
{
    static auto wrapper = [](void* callbackData, EDiscordResult result, char const* data) -> void {
        std::unique_ptr<std::function<void(Result, char const*)>> cb(
          reinterpret_cast<std::function<void(Result, char const*)>*>(callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(static_cast<Result>(result), static_cast<const char*>(data));
    };
    std::unique_ptr<std::function<void(Result, char const*)>> cb{};
    cb.reset(new std::function<void(Result, char const*)>(std::move(callback)));
    internal_->get_ticket(internal_, cb.release(), wrapper);
}

} // namespace discord
