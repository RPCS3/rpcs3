#pragma once

#include "types.h"

namespace discord {

class UserManager final {
public:
    ~UserManager() = default;

    Result GetCurrentUser(User* currentUser);
    void GetUser(UserId userId, std::function<void(Result, User const&)> callback);
    Result GetCurrentUserPremiumType(PremiumType* premiumType);
    Result CurrentUserHasFlag(UserFlag flag, bool* hasFlag);

    Event<> OnCurrentUserUpdate;

private:
    friend class Core;

    UserManager() = default;
    UserManager(UserManager const& rhs) = delete;
    UserManager& operator=(UserManager const& rhs) = delete;
    UserManager(UserManager&& rhs) = delete;
    UserManager& operator=(UserManager&& rhs) = delete;

    IDiscordUserManager* internal_;
    static IDiscordUserEvents events_;
};

} // namespace discord
