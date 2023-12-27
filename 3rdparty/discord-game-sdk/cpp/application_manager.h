#pragma once

#include "types.h"

namespace discord {

class ApplicationManager final {
public:
    ~ApplicationManager() = default;

    void ValidateOrExit(std::function<void(Result)> callback);
    void GetCurrentLocale(char locale[128]);
    void GetCurrentBranch(char branch[4096]);
    void GetOAuth2Token(std::function<void(Result, OAuth2Token const&)> callback);
    void GetTicket(std::function<void(Result, char const*)> callback);

private:
    friend class Core;

    ApplicationManager() = default;
    ApplicationManager(ApplicationManager const& rhs) = delete;
    ApplicationManager& operator=(ApplicationManager const& rhs) = delete;
    ApplicationManager(ApplicationManager&& rhs) = delete;
    ApplicationManager& operator=(ApplicationManager&& rhs) = delete;

    IDiscordApplicationManager* internal_;
    static IDiscordApplicationEvents events_;
};

} // namespace discord
